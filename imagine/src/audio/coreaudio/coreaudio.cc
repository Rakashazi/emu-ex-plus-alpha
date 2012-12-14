#define thisModuleName "audio:coreaudio"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <util/thread/pthread.hh>
//#include <mach/mach_time.h>

#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <AudioToolbox/AudioQueue.h>
#undef Fixed
#undef Rect

namespace Audio
{

PcmFormat preferredPcmFormat { 44100, &SampleFormats::s16, 2 };
PcmFormat pcmFormat;

static uint bufferFrames = 800;
static uint buffers = 8;
static uint maxBytesQueued;
static AudioQueueRef queue;
static AudioQueueBufferRef *qBuffer = nullptr;
static AudioStreamBasicDescription streamFormat;
static uint buffersQueued = 0, currQBuf = 0; //, bytesQueued = 0;
static bool isPlaying = 0, qIsOpen = 0;
//static AudioTimeStamp lastTimestamp;
//static float timestampNanosecScaler;
static MutexPThread buffersQueuedLock;
static BufferContext audioBuffLockCtx;

/*static bool queuePlaying()
{
	UInt32 running, runningSize = 4;
	AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &running, &runningSize);
	return running;
}*/

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

static void audioCallback(void *userdata, AudioQueueRef inQ, AudioQueueBufferRef outQB)
{
	buffersQueuedLock.lock();
	buffersQueued--;
	assert(buffersQueued < buffers);
	/*bytesQueued -= outQB->mAudioDataByteSize;
	assert(bytesQueued < maxBytesQueued);
	AudioTimeStamp nowTimestamp;
	AudioQueueGetCurrentTime(inQ, 0, &nowTimestamp, 0);
	int nsLatency = float(lastTimestamp.mHostTime - nowTimestamp.mHostTime) * timestampNanosecScaler;
	static int debugCount = 0;
	if(countToValueLooped(debugCount, 30))
	{
		//logMsg("buffer queue down to %d, last buffer with %d bytes, total %d, time %u", buffersQueued, (int)outQB->mAudioDataByteSize, bytesQueued, nsLatency);
		logMsg("buffer queue down to %d, last buffer with %d bytes, total %d, time %u", buffersQueued, (int)outQB->mAudioDataByteSize, bytesQueued);
	}*/
	/*if(nsLatency < 40000000 || bytesQueued <= pcmFormat.framesToBytes(bufferFrames))
	{
		logMsg("xrun, only %u ns of buffer, %d bytes queued", nsLatency, bytesQueued);
		AudioQueuePause(inQ);
		isPlaying = 0;
	}*/
	buffersQueuedLock.unlock();
}

static void checkXRun(uint queued)
{
	if(unlikely(isPlaying && queued == 0))
	{
		logMsg("xrun, no buffers queued");
		AudioQueuePause(queue);
		isPlaying = 0;
	}
}

static CallResult openQueue(AudioStreamBasicDescription &fmt)
{
	logMsg("creating queue %dHz %d channels", (int)fmt.mSampleRate, (int)fmt.mChannelsPerFrame);
	UInt32 err = AudioQueueNewOutput(&streamFormat, audioCallback, 0, 0 /*CFRunLoopGetCurrent()*/, kCFRunLoopCommonModes, 0, &queue);
	if (err)
	{
		logErr("error %u in AudioQueueNewOutput", (uint)err);
		return INVALID_PARAMETER;
	}

	UInt32 bufferSize = bufferFrames * streamFormat.mBytesPerFrame;
	qBuffer = (AudioQueueBufferRef*)mem_alloc(sizeof(AudioQueueBufferRef) * buffers);
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
	//bytesQueued = 0;
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
	mem_free(qBuffer);
	qBuffer = nullptr;
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
	AudioQueueEnqueueBuffer(queue, b, 0, 0);
	//AudioQueueEnqueueBufferWithParameters(queue, b, 0, 0, 0, 0, 0, 0, 0, &lastTimestamp);
	//logMsg("queued buffer %d with %d bytes, %d on queue, %lld %lld", currQBuf, (int)b->mAudioDataByteSize, buffersQueued+1, lastTimestamp.mHostTime, lastTimestamp.mWordClockTime);
	IG::incWrappedSelf(currQBuf, buffers);
	buffersQueuedLock.lock();
	buffersQueued++;
	//bytesQueued += bytes;
	buffersQueuedLock.unlock();

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
	if(unlikely(!isPlaying && buffersQueued >= buffers-1))
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

BufferContext *getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!isOpen()))
		return nullptr;

	if(buffersQueued == buffers)
	{
		//logDMsg("can't write data with full buffers");
		return nullptr;
	}

	auto b = qBuffer[currQBuf];
	audioBuffLockCtx.data = b->mAudioData;
	audioBuffLockCtx.frames = IG::min(wantedFrames, bufferFrames);
	return &audioBuffLockCtx;
}

void commitPlayBuffer(BufferContext *buffer, uint frames)
{
	assert(frames <= buffer->frames);
	checkXRun(buffersQueued);
	commitCABuffer(qBuffer[currQBuf], frames * streamFormat.mBytesPerFrame);
	startPlaybackIfNeeded();
}

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	checkXRun(buffersQueued);
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
		auto b = qBuffer[currQBuf];
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
	/*mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	timestampNanosecScaler = (float)info.numer / info.denom;
	logMsg("timestamp nanosec scaler %f", timestampNanosecScaler);*/
	/*if(AudioSessionSetActive(true) != kAudioSessionNoError)
	{
		logWarn("error in AudioSessionSetActive()");
	}
	UInt32 sessionCategory = kAudioSessionCategory_SoloAmbientSound;
	AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);*/
	buffersQueuedLock.create();
	return OK;
}

}
