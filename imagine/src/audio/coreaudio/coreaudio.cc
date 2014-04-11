/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "CoreAudio"
#include <imagine/audio/Audio.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/number.h>
#include <imagine/util/thread/pthread.hh>
#include <imagine/util/ringbuffer/MachRingBuffer.hh>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <TargetConditionals.h>

namespace Audio
{

PcmFormat preferredPcmFormat { 44100, SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static uint wantedLatency = 100000;
static AudioComponentInstance outputUnit = nullptr;
static AudioStreamBasicDescription streamFormat;
static bool isPlaying_ = false, isOpen_ = false, hadUnderrun = false;
static StaticMachRingBuffer<> rBuff;
#if TARGET_OS_IPHONE
static bool sessionInit = false;
static bool soloMix_ = true;
#endif

int maxRate()
{
	return 48000;
}

static bool isInit()
{
	return outputUnit;
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

static OSStatus outputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	auto *buf = (char*)ioData->mBuffers[0].mData;
	uint bytes = inNumberFrames * streamFormat.mBytesPerFrame;

	if(unlikely(hadUnderrun))
	{
		*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
		mem_zero(buf, bytes);
		return 0;
	}

	uint read = rBuff.read(buf, bytes);
	if(unlikely(read != bytes))
	{
		//logMsg("underrun, read %d out of %d bytes", read, bytes);
		hadUnderrun = true;
		uint padBytes = bytes - read;
		//logMsg("padding %d bytes", padBytes);
		mem_zero(&buf[read], padBytes);
	}
	return 0;
}

static CallResult openUnit(AudioStreamBasicDescription &fmt, uint bufferSize)
{
	logMsg("creating unit %dHz %d channels", (int)fmt.mSampleRate, (int)fmt.mChannelsPerFrame);

	if(!rBuff.init(bufferSize))
	{
		return OUT_OF_MEMORY;
	}

	auto err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
			0, &fmt, sizeof(AudioStreamBasicDescription));
	if(err)
	{
		logErr("error %d setting stream format", (int)err);
		rBuff.deinit();
		return INVALID_PARAMETER;
	}
	AudioUnitInitialize(outputUnit);
	isOpen_ = true;

	return OK;
}

CallResult openPcm(const PcmFormat &format)
{
	assert(isInit());
	if(isOpen())
	{
		logWarn("audio unit already open");
		return OK;
	}
	streamFormat.mSampleRate = format.rate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
	streamFormat.mBytesPerPacket = format.framesToBytes(1);
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerFrame = format.framesToBytes(1);
	streamFormat.mChannelsPerFrame = format.channels;
	streamFormat.mBitsPerChannel = format.sample.bits == 16 ? 16 : 8;
	pcmFormat = format;
	uint bufferSize = format.uSecsToBytes(wantedLatency);
	return openUnit(streamFormat, bufferSize);
}

void closePcm()
{
	if(!isOpen())
	{
		logWarn("audio unit already closed");
		return;
	}
	AudioOutputUnitStop(outputUnit);
	AudioUnitUninitialize(outputUnit);
	rBuff.deinit();
	isPlaying_ = false;
	isOpen_ = false;
	hadUnderrun = false;
	logMsg("closed audio unit");
}

bool isOpen()
{
	return isOpen_;
}

bool isPlaying()
{
	return isPlaying_ && !hadUnderrun;
}

void pausePcm()
{
	if(unlikely(!isOpen()))
		return;
	AudioOutputUnitStop(outputUnit);
	isPlaying_ = false;
	hadUnderrun = false;
}

void resumePcm()
{
	if(unlikely(!isOpen()))
		return;
	if(!isPlaying_ || hadUnderrun)
	{
		logMsg("playback starting with %u frames", (uint)(rBuff.writtenSize() / streamFormat.mBytesPerFrame));
		hadUnderrun = false;
		if(!isPlaying_)
		{
			auto err = AudioOutputUnitStart(outputUnit);
			if(err)
			{
				logErr("error %d in AudioOutputUnitStart", (int)err);
			}
			else
				isPlaying_ = true;
		}
	}
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	rBuff.reset();
}

void writePcm(const void *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	uint bytes = framesToWrite * streamFormat.mBytesPerFrame;
	auto written = rBuff.write(samples, bytes);
	if(written != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
}

BufferContext getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()) || !framesFree())
		return {};
	if((uint)framesFree() < wantedFrames)
	{
		logDMsg("buffer has only %d/%d frames free", framesFree(), wantedFrames);
	}
	// will always have a contiguous block from mirrored pages
	return {rBuff.writeAddr(), std::min(wantedFrames, (uint)framesFree())};
}

void commitPlayBuffer(BufferContext buffer, uint frames)
{
	assert(frames <= buffer.frames);
	auto bytes = frames * streamFormat.mBytesPerFrame;
	rBuff.commitWrite(bytes);
}

// TODO
int frameDelay() { return 0; }

int framesFree()
{
	return rBuff.freeSpace() / streamFormat.mBytesPerFrame;
}

#if TARGET_OS_IPHONE
static void setAudioCategory(UInt32 category)
{
	AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(category), &category);
}

void setSoloMix(bool newSoloMix)
{
	if(soloMix_ != newSoloMix)
	{
		logMsg("setting solo mix: %d", newSoloMix);
		soloMix_ = newSoloMix;
		if(!isInit())
			return; // audio init() will take care of initial focus setting
		setAudioCategory(newSoloMix ? kAudioSessionCategory_SoloAmbientSound : kAudioSessionCategory_AmbientSound);
	}
}

bool soloMix()
{
	return soloMix_;
}

void initSession()
{
	AudioSessionInitialize(nullptr, nullptr,
		[](void *inUserData, UInt32 interruptionState)
		{
			if(interruptionState == kAudioSessionEndInterruption)
			{
				logMsg("re-activating audio session");
				AudioSessionSetActive(true);
			}
		}, nullptr);
	sessionInit = true;
}
#endif

CallResult init()
{
	#if TARGET_OS_IPHONE
	assert(sessionInit);
	if(!soloMix_)
		setAudioCategory(kAudioSessionCategory_AmbientSound); // kAudioSessionCategory_SoloAmbientSound is default
	#endif
	logMsg("setting up playback audio unit");
	AudioComponentDescription defaultOutputDescription =
	{
		kAudioUnitType_Output,
		#if TARGET_OS_IPHONE
		kAudioUnitSubType_RemoteIO,
		#else
		kAudioUnitSubType_DefaultOutput,
		#endif
		kAudioUnitManufacturer_Apple
	};
	AudioComponent defaultOutput = AudioComponentFindNext(nullptr, &defaultOutputDescription);
	assert(defaultOutput);
	auto err = AudioComponentInstanceNew(defaultOutput, &outputUnit);
	if(!outputUnit)
	{
		bug_exit("error creating output unit: %d", (int)err);
	}
	AURenderCallbackStruct renderCallbackProp =
	{
		outputCallback,
		//nullptr
	};
	err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
	    0, &renderCallbackProp, sizeof(renderCallbackProp));
	if(err)
	{
		bug_exit("error setting callback: %d", (int)err);
	}

	#if TARGET_OS_IPHONE
	if(AudioSessionSetActive(true) != kAudioSessionNoError)
	{
		logWarn("error in AudioSessionSetActive()");
	}
	#endif
	return OK;
}

}
