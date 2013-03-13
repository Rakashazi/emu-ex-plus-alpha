#define thisModuleName "audio:coreaudio"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <util/thread/pthread.hh>
#include <util/RingBuffer.hh>

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
static MutexPThread buffersQueuedLock;
static RingBuffer<int> rBuff;
static int startPlaybackBytes = 0;
static uchar *localBuff = nullptr;

void setHintPcmFramesPerWrite(uint frames)
{
	logMsg("setting queue buffer frames to %d", frames);
	assert(frames < 2000);
	bufferFrames = frames;
	assert(!isOpen());
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

	buffersQueuedLock.lock();
	uint read = rBuff.read(buf, bytes);
	buffersQueuedLock.unlock();
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
	auto err = AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
			0, &fmt, sizeof(AudioStreamBasicDescription));
	if(err)
	{
		logErr("error %d setting stream format", (int)err);
		return INVALID_PARAMETER;
	}
	AudioUnitInitialize(outputUnit);

	uint bufferSize = bufferFrames * streamFormat.mBytesPerFrame * buffers;
	startPlaybackBytes = bufferFrames * streamFormat.mBytesPerFrame * buffers-1;
	localBuff = (uchar*)mem_alloc(bufferSize);
	if(!localBuff)
	{
		// TODO clean-up
		logMsg("error allocation audio buffer");
		return OUT_OF_MEMORY;
	}
	logMsg("allocated %d for audio buffer", bufferSize);
	rBuff.init(localBuff, bufferSize);

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
	rBuff.reset();
	mem_free(localBuff);
	localBuff = nullptr;
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

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	//checkXRun(buffersQueued);
	int bytes = framesToWrite * streamFormat.mBytesPerFrame;
	buffersQueuedLock.lock();
	auto written = rBuff.write(samples, bytes);
	buffersQueuedLock.unlock();
	if(written != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}

	startPlaybackIfNeeded();
}

// TODO
int frameDelay() { return 0; }

int framesFree()
{
	return rBuff.freeSpace() * streamFormat.mBytesPerFrame;
}

static void interruptionListenerCallback(void *inUserData, UInt32  interruptionState)
{
	if(interruptionState == kAudioSessionEndInterruption)
	{
		logMsg("re-activating audio session");
		AudioSessionSetActive(true);
	}
}

CallResult init()
{
	AudioSessionInitialize(nullptr, nullptr, interruptionListenerCallback, nullptr);
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

	buffersQueuedLock.create();
	return OK;
}

}
