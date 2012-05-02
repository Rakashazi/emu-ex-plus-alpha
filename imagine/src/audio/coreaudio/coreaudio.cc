#define thisModuleName "audio:coreaudio"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <mach/mach_time.h>

#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <AudioToolbox/AudioQueue.h>
#undef Fixed
#undef Rect

namespace Audio
{

PcmFormat preferredPcmFormat = { 44100, &SampleFormats::s16, 2 };
PcmFormat pcmFormat= { 0 };

static uint bufferFrames = 800;
static const uint buffers = 16;
static uint maxBytesQueued;
static AudioQueueRef queue;
static AudioQueueBufferRef qBuffer[buffers];
static AudioStreamBasicDescription streamFormat;
static uint buffersQueued = 0, currQBuf = 0, bytesQueued = 0;
static bool isPlaying = 0, qIsOpen = 0;
static AudioTimeStamp lastTimestamp;
static float timestampNanosecScaler;

/*static bool queuePlaying()
{
	UInt32 running, runningSize = 4;
	AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &running, &runningSize);
	return running;
}*/

void hintPcmFramesPerWrite(uint frames)
{
	logMsg("setting queue buffer frames to %d", frames);
	bufferFrames = frames;
}

static void audioCallback(void *userdata, AudioQueueRef inQ, AudioQueueBufferRef outQB)
{
	buffersQueued--;
	assert(buffersQueued < buffers);
	bytesQueued -= outQB->mAudioDataByteSize;
	assert(bytesQueued < maxBytesQueued);
	AudioTimeStamp nowTimestamp;
	AudioQueueGetCurrentTime(inQ, 0, &nowTimestamp, 0);
	int nsLatency = float(lastTimestamp.mHostTime - nowTimestamp.mHostTime) * timestampNanosecScaler;
	//logMsg("buffer queue down to %d, last buffer with %d bytes, total %d, time %u", buffersQueued, (int)outQB->mAudioDataByteSize, bytesQueued, nsLatency);
	if(nsLatency < 50000000 || bytesQueued < pcmFormat.framesToBytes(bufferFrames * 3))
	{
		logMsg("xrun, only %u ns of buffer", nsLatency);
		AudioQueuePause(inQ);
		isPlaying = 0;
	}
}

static CallResult openQueue(AudioStreamBasicDescription &fmt)
{
	logMsg("creating queue %dHz %d channels", (int)fmt.mSampleRate, (int)fmt.mChannelsPerFrame);
	UInt32 err = AudioQueueNewOutput(&streamFormat, audioCallback, 0, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);
	if (err)
	{
		logErr("error %u in AudioQueueNewOutput", (uint)err);
		return INVALID_PARAMETER;
	}

	UInt32 bufferSize = bufferFrames * streamFormat.mBytesPerFrame;
	iterateTimes((uint)buffers, i)
	{
		err = AudioQueueAllocateBuffer(queue, bufferSize, &qBuffer[i]);
		if (err)
		{
			logErr("error %u in AudioQueueAllocateBuffer, number %d", (uint)err, i);
			return INVALID_PARAMETER;
		}
	}

	isPlaying = 0;
	qIsOpen = 1;
	buffersQueued = 0;
	currQBuf = 0;
	bytesQueued = 0;
	return OK;
}

CallResult openPcm(const PcmFormat &format)
{
	if(isOpen())
	{
		logWarn("audio queue already open");
		return OK;
	}
	streamFormat.mSampleRate = format.rate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	streamFormat.mBytesPerPacket = format.framesToBytes(1);
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerFrame = format.framesToBytes(1);
	streamFormat.mChannelsPerFrame = format.channels;
	streamFormat.mBitsPerChannel = format.sample->bits == 16 ? 16 : 8;

	pcmFormat = format;
	maxBytesQueued = format.framesToBytes(buffers * bufferFrames);
	return openQueue(streamFormat);
}

void closePcm()
{
	if(!isOpen())
	{
		logWarn("audio queue already closed");
		return;
	}
	AudioQueueDispose(queue, true);
	isPlaying = 0;
	qIsOpen = 0;
}

bool isOpen()
{
	return qIsOpen;
}

static void commitCABuffer(AudioQueueBufferRef b, uint bytes)
{
	b->mAudioDataByteSize = bytes;
	//AudioQueueEnqueueBuffer(queue, b, 0, 0);
	AudioQueueEnqueueBufferWithParameters(queue, b, 0, 0, 0, 0, 0, 0, 0, &lastTimestamp);
	//logMsg("queued buffer %d with %d bytes, %d on queue, %lld %lld", currQBuf, (int)b->mAudioDataByteSize, buffersQueued+1, lastTimestamp.mHostTime, lastTimestamp.mWordClockTime);
	IG::incWrappedSelf(currQBuf, buffers);
	buffersQueued++;
	bytesQueued += bytes;

	static int debugCount = 0;
	if(debugCount == 120)
	{
		//logMsg("%d buffers queued", buffersQueued);
		debugCount = 0;
	}
	else
		debugCount++;
}

static void startPlaybackIfNeeded()
{
	// TODO: base off total bytes and only use buffer count when max buffers reached
	if(unlikely(!isPlaying && buffersQueued >= buffers-2))
	{
		logMsg("playback starting with %d buffers", buffersQueued);
		int err = AudioQueueStart(queue, 0);
		if(err)
		{
			logErr("error %u in AudioQueueStart", (uint)err);
		}
		else
			isPlaying = 1;
	}
}

static BufferContext audioBuffLockCtx;

BufferContext *getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()))
		return 0;

	if(buffersQueued == buffers)
	{
		//logDMsg("can't write data with full buffers");
		return 0;
	}

	var_copy(b, qBuffer[currQBuf]);
	audioBuffLockCtx.data = b->mAudioData;
	audioBuffLockCtx.frames = IG::min(wantedFrames, bufferFrames);
	return &audioBuffLockCtx;
}

void commitPlayBuffer(BufferContext *buffer, uint frames)
{
	assert(frames <= buffer->frames);
	commitCABuffer(qBuffer[currQBuf], frames * streamFormat.mBytesPerFrame);
	startPlaybackIfNeeded();
}

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	if(framesToWrite > bufferFrames)
	{
		//logMsg("need more than one buffer to write all data");
	}
	while(framesToWrite)
	{
		if(buffersQueued == buffers)
		{
			logMsg("can't write data with full buffers");
			break;
		}

		uint framesToWriteInBuffer = IG::min(framesToWrite, bufferFrames);
		uint bytesToWriteInBuffer = framesToWriteInBuffer * streamFormat.mBytesPerFrame;
		var_copy(b, qBuffer[currQBuf]);
		memcpy(b->mAudioData, samples, bytesToWriteInBuffer);
		commitCABuffer(b, bytesToWriteInBuffer);
		samples += bytesToWriteInBuffer;
		framesToWrite -= framesToWriteInBuffer;
	}
	startPlaybackIfNeeded();
}

// TODO
int frameDelay() { return 0; }

int framesFree()
{
	return (buffers - buffersQueued) * bufferFrames;
}

CallResult init()
{
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	timestampNanosecScaler = (float)info.numer / info.denom;
	logMsg("timestamp nanosec scaler %f", timestampNanosecScaler);
	/*if(AudioSessionSetActive(true) != kAudioSessionNoError)
	{
		logWarn("error in AudioSessionSetActive()");
	}
	UInt32 sessionCategory = kAudioSessionCategory_SoloAmbientSound;
	AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);*/
	return OK;
}

}

#undef thisModuleName
