#define thisModuleName "audio:coreaudio"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <util/thread/pthread.hh>
#include <util/MachRingBuffer.hh>

#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#undef Fixed
#undef Rect

namespace Audio
{

PcmFormat preferredPcmFormat { 44100, &SampleFormats::s16, 2 };
PcmFormat pcmFormat;

static uint bufferFrames = 800;
static uint buffers = 8;
static AudioComponentInstance outputUnit = nullptr;
static AudioStreamBasicDescription streamFormat;
static bool isPlaying = 0, isOpen_ = 0;
static AudioTimeStamp lastTimestamp;
static MachRingBuffer<> rBuff;
static uint startPlaybackBytes = 0;
static BufferContext audioBuffLockCtx;
static bool sessionInit = false;
static bool soloMix_ = true;

void setHintPcmFramesPerWrite(uint frames)
{
	if(frames != bufferFrames)
	{
		closePcm();
		logMsg("setting queue buffer frames to %d", frames);
		assert(frames < 2000);
		bufferFrames = frames;
	}
}

void setHintPcmMaxBuffers(uint maxBuffers)
{
	logMsg("setting max buffers to %d", maxBuffers);
	assert(maxBuffers < 100);
	buffers = maxBuffers;
	assert(!isOpen());
}

uint hintPcmMaxBuffers() { return buffers; }

static OSStatus outputCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	auto *buf = (uchar*)ioData->mBuffers[0].mData;
	uint bytes = inNumberFrames * streamFormat.mBytesPerFrame;

	if(unlikely(!isPlaying))
	{
		*ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
		mem_zero(buf, bytes);
		return 0;
	}

	uint read = rBuff.read(buf, bytes);
	if(unlikely(read != bytes))
	{
		//logMsg("underrun, read %d out of %d bytes", read, bytes);
		isPlaying = 0;
		uint padBytes = bytes - read;
		//logMsg("padding %d bytes", padBytes);
		mem_zero(&buf[read], padBytes);
	}
	return 0;
}

static CallResult openUnit(AudioStreamBasicDescription &fmt)
{
	logMsg("creating unit %dHz %d channels", (int)fmt.mSampleRate, (int)fmt.mChannelsPerFrame);

	uint bufferSize = bufferFrames * streamFormat.mBytesPerFrame * buffers;
	startPlaybackBytes = bufferFrames * streamFormat.mBytesPerFrame * (buffers-1);
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
	logMsg("allocated %d for audio buffer", bufferSize);

	isPlaying = 0;
	isOpen_ = 1;

	return OK;
}

CallResult openPcm(const PcmFormat &format)
{
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
	streamFormat.mBitsPerChannel = format.sample->bits == 16 ? 16 : 8;

	pcmFormat = format;
	return openUnit(streamFormat);
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
	isPlaying = 0;
	isOpen_ = 0;
	logMsg("closed audio unit");
}

bool isOpen()
{
	return isOpen_;
}

static void startPlaybackIfNeeded()
{
	if(unlikely(!isPlaying && rBuff.written >= startPlaybackBytes))
	{
		logMsg("playback starting with %u frames", (uint)(rBuff.written * streamFormat.mBytesPerFrame));
		auto err = AudioOutputUnitStart(outputUnit);
		if(err)
		{
			logErr("error %d in AudioOutputUnitStart", (int)err);
		}
		else
			isPlaying = 1;
	}
}

void pausePcm()
{
	if(unlikely(!isOpen()))
		return;
	AudioOutputUnitStop(outputUnit);
	isPlaying = 0;
}

void resumePcm()
{
	if(unlikely(!isOpen()))
		return;
	startPlaybackIfNeeded();
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	rBuff.reset();
}

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	uint bytes = framesToWrite * streamFormat.mBytesPerFrame;
	auto written = rBuff.write(samples, bytes);
	if(written != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
	startPlaybackIfNeeded();
}

BufferContext *getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()))
		return nullptr;
	if((uint)framesFree() < bufferFrames)
	{
		logDMsg("can't get buffer with only %d frames free", framesFree());
		return nullptr;
	}
	audioBuffLockCtx.data = rBuff.writePos();
	audioBuffLockCtx.frames = std::min(wantedFrames, (uint)framesFree());
	return &audioBuffLockCtx;
}

void commitPlayBuffer(BufferContext *buffer, uint frames)
{
	assert(frames <= buffer->frames);
	auto bytes = frames * streamFormat.mBytesPerFrame;
	rBuff.advanceWritePos(bytes);
	startPlaybackIfNeeded();
}

// TODO
int frameDelay() { return 0; }

int framesFree()
{
	return rBuff.freeSpace() / streamFormat.mBytesPerFrame;
}

void setSoloMix(bool newSoloMix)
{
	if(soloMix_ != newSoloMix)
	{
		logMsg("setting solo mix: %d", newSoloMix);
		UInt32 sessionCategory = newSoloMix ? kAudioSessionCategory_SoloAmbientSound
			: kAudioSessionCategory_AmbientSound;
		AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
		soloMix_ = newSoloMix;
	}
}

bool soloMix()
{
	return soloMix_;
}

static void interruptionListenerCallback(void *inUserData, UInt32  interruptionState)
{
	if(interruptionState == kAudioSessionEndInterruption)
	{
		logMsg("re-activating audio session");
		AudioSessionSetActive(true);
	}
}

void initSession()
{
	AudioSessionInitialize(nullptr, nullptr, interruptionListenerCallback, nullptr);
	sessionInit = true;
}

CallResult init()
{
	assert(sessionInit);
	logMsg("setting up playback audio unit");
	AudioComponentDescription defaultOutputDescription =
	{
		kAudioUnitType_Output,
		kAudioUnitSubType_RemoteIO,
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

	if(AudioSessionSetActive(true) != kAudioSessionNoError)
	{
		logWarn("error in AudioSessionSetActive()");
	}
	return OK;
}

}
