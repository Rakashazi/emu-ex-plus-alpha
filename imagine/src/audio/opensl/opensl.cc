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

#define thisModuleName "audio:opensl"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <util/number.h>
#include <util/thread/pthread.hh>
#include <base/android/private.hh>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

namespace Audio
{
using namespace Base;

PcmFormat preferredPcmFormat { 44100, &SampleFormats::s16, 2 };
PcmFormat pcmFormat;

static SLEngineItf slI = nullptr;
static SLObjectItf outMix = nullptr, player = nullptr;
static SLPlayItf playerI = nullptr;
static SLAndroidSimpleBufferQueueItf slBuffQI = nullptr;
static uint bufferFrames = 800, currQBuf = 0;
static uint buffers = 10;
static uchar **qBuffer = nullptr;
static bool isPlaying = 0, reachedEndOfPlayback = 0, strictUnderrunCheck = 1;
static BufferContext audioBuffLockCtx;
static jobject audioManager = nullptr;
static JavaInstMethod<jint> jRequestAudioFocus, jAbandonAudioFocus;
static bool soloMix_ = true;

static bool isInit()
{
	return outMix;
}

// runs on internal OpenSL ES thread
/*static void queueCallback(SLAndroidSimpleBufferQueueItf caller, void *)
{
}*/

// runs on internal OpenSL ES thread
static void playCallback(SLPlayItf caller, void *, SLuint32 event)
{
	//logMsg("play event %X", (int)event);
	assert(event == SL_PLAYEVENT_HEADSTALLED || event == SL_PLAYEVENT_HEADATEND);
	logMsg("got playback end event");
	reachedEndOfPlayback = 1;
}

static void setupAudioManagerJNI(JNIEnv* jEnv)
{
	if(!audioManager)
	{
		JavaInstMethod<jobject> jAudioManager;
		jAudioManager.setup(jEnv, jBaseActivityCls, "audioManager", "()Landroid/media/AudioManager;");
		audioManager = jAudioManager(jEnv, jBaseActivity);
		assert(audioManager);
		audioManager = jEnv->NewGlobalRef(audioManager);
		jclass jAudioManagerCls = jEnv->GetObjectClass(audioManager);
		jRequestAudioFocus.setup(jEnv, jAudioManagerCls, "requestAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;II)I");
		jAbandonAudioFocus.setup(jEnv, jAudioManagerCls, "abandonAudioFocus", "(Landroid/media/AudioManager$OnAudioFocusChangeListener;)I");
	}
}

static void requestAudioFocus(JNIEnv* jEnv)
{
	setupAudioManagerJNI(jEnv);
	auto res = jRequestAudioFocus(jEnv, audioManager, jBaseActivity, 3, 1);
	//logMsg("%d from requestAudioFocus()", (int)res);
}

static void abandonAudioFocus(JNIEnv* jEnv)
{
	setupAudioManagerJNI(jEnv);
	jAbandonAudioFocus(jEnv, audioManager, jBaseActivity);
}

CallResult openPcm(const PcmFormat &format)
{
	if(player)
	{
		logWarn("called openPcm when pcm already on");
		return OK;
	}
	pcmFormat = format;
	logMsg("creating playback %dHz, %d channels, %d buffers", format.rate, format.channels, buffers);
	assert(format.sample->bits == 16);
	SLDataLocator_AndroidSimpleBufferQueue buffQLoc = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, buffers };
	SLDataFormat_PCM slFormat =
	{
		SL_DATAFORMAT_PCM, (SLuint32)format.channels, (SLuint32)format.rate * 1000, // as milliHz
		SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
		format.channels == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audioSrc = { &buffQLoc, &slFormat };
	SLDataLocator_OutputMix outMixLoc = { SL_DATALOCATOR_OUTPUTMIX, outMix };
	SLDataSink sink = { &outMixLoc, nullptr };
	const SLInterfaceID ids[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
	const SLboolean req[sizeofArray(ids)] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE };
	SLresult result = (*slI)->CreateAudioPlayer(slI, &player, &audioSrc, &sink, sizeofArray(ids), ids, req);
	if(result != SL_RESULT_SUCCESS)
	{
		logErr("CreateAudioPlayer returned 0x%X", (uint)result);
		player = nullptr;
		return INVALID_PARAMETER;
	}

	result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_PLAY, &playerI);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &slBuffQI);
	assert(result == SL_RESULT_SUCCESS);

	/*result = (*slBuffQI)->RegisterCallback(slBuffQI, queueCallback, nullptr);
	assert(result == SL_RESULT_SUCCESS);*/
	result = (*playerI)->RegisterCallback(playerI, playCallback, nullptr);
	assert(result == SL_RESULT_SUCCESS);

	uint playStateEvMask = SL_PLAYEVENT_HEADSTALLED;
	if(strictUnderrunCheck)
		playStateEvMask = SL_PLAYEVENT_HEADATEND;
	(*playerI)->SetCallbackEventsMask(playerI, playStateEvMask);
	(*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);

	auto bufferBytes = pcmFormat.framesToBytes(bufferFrames);
	logMsg("allocating %d bytes per buffer", bufferBytes);
	uint bufferPointerTableSize = sizeof(uchar*) * buffers;
	// combined allocation of pointer table and actual buffer data
	void *bufferStorage = mem_alloc(bufferPointerTableSize + (bufferBytes * buffers));
	assert(bufferStorage);
	uchar **bufferBlock = (uchar**)bufferStorage;
	uchar *startOfBufferData = (uchar*)bufferStorage + bufferPointerTableSize;
	bufferBlock[0] = startOfBufferData;
	iterateTimes(buffers-1, i)
	{
		bufferBlock[i+1] = bufferBlock[i] + bufferBytes;
	}
	qBuffer = bufferBlock;

	return OK;
}

void closePcm()
{
	if(player)
	{
		logMsg("closing pcm");
		currQBuf = 0;
		isPlaying = 0;
		(*player)->Destroy(player);
		mem_free(qBuffer);
		player = nullptr;
		reachedEndOfPlayback = 0;
	}
	else
		logMsg("called closePcm when pcm already off");
}

bool isOpen()
{
	return player;
}

static void commitSLBuffer(void *b, uint bytes)
{
	SLresult result = (*slBuffQI)->Enqueue(slBuffQI, b, bytes);
	if(result != SL_RESULT_SUCCESS)
	{
		logWarn("Enqueue returned 0x%X", (uint)result);
		return;
	}
	//logMsg("queued buffer %d with %d bytes, %d on queue, %lld %lld", currQBuf, (int)b->mAudioDataByteSize, buffersQueued+1, lastTimestamp.mHostTime, lastTimestamp.mWordClockTime);
	IG::incWrappedSelf(currQBuf, buffers);
}

static uint buffersQueued()
{
	SLAndroidSimpleBufferQueueState state;
	(*slBuffQI)->GetState(slBuffQI, &state);
	/*static int debugCount = 0;
	if(countToValueLooped(debugCount, 30))
	{
		//logMsg("queue state: %u count, %u index", (uint)state.count, (uint)state.index);
	}*/
	return state.count;
}

static void checkXRun(uint queued)
{
	/*SLmillisecond pos;
	(*playerI)->GetPosition(playerI, &pos);
	logMsg("pos: %u/, %d", pcmFormat.mSecsToFrames(pos));*/

	if(unlikely(isPlaying && reachedEndOfPlayback))
	{
		logMsg("xrun");
		pausePcm();
	}
}

static void startPlaybackIfNeeded(uint queued)
{
	if(unlikely(!isPlaying && queued >= buffers-1))
	{
		reachedEndOfPlayback = 0;
		SLresult result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PLAYING);
		if(result == SL_RESULT_SUCCESS)
		{
			logMsg("started playback with %d buffers", buffers);
			isPlaying = 1;
		}
		else
			logErr("SetPlayState returned 0x%X", (uint)result);
	}
}

void pausePcm()
{
	if(unlikely(!player) || !isPlaying)
		return;
	logMsg("pausing playback");
	auto result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);
	isPlaying = 0;
}

void resumePcm()
{
	if(unlikely(!player))
		return;
	startPlaybackIfNeeded(buffersQueued());
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	SLresult result = (*slBuffQI)->Clear(slBuffQI);
	assert(result == SL_RESULT_SUCCESS);
}

BufferContext *getPlayBuffer(uint wantedFrames)
{
	if(unlikely(!player))
		return nullptr;

	auto queued = buffersQueued();
	if(queued == buffers)
	{
		//logDMsg("can't write data with full buffers");
		return nullptr;
	}
	auto b = qBuffer[currQBuf];
	audioBuffLockCtx.data = b;
	audioBuffLockCtx.frames = std::min(wantedFrames, bufferFrames);
	return &audioBuffLockCtx;
}

void commitPlayBuffer(BufferContext *buffer, uint frames)
{
	assert(frames <= buffer->frames);
	auto queued = buffersQueued();
	checkXRun(queued);
	commitSLBuffer(qBuffer[currQBuf], pcmFormat.framesToBytes(frames));
	startPlaybackIfNeeded(queued+1);
}

void writePcm(uchar *samples, uint framesToWrite)
{
	if(unlikely(!player))
		return;

	auto queued = buffersQueued();

	checkXRun(queued);
	if(framesToWrite > bufferFrames)
	{
		logMsg("need more than one buffer to write %d frames", framesToWrite);
	}
	while(framesToWrite)
	{
		if(queued == buffers)
		{
			logMsg("can't write data with full buffers");
			break;
		}

		uint framesToWriteInBuffer = std::min(framesToWrite, bufferFrames);
		uint bytesToWriteInBuffer = pcmFormat.framesToBytes(framesToWriteInBuffer);
		auto b = qBuffer[currQBuf];
		memcpy(b, samples, bytesToWriteInBuffer);
		commitSLBuffer(b, bytesToWriteInBuffer);
		samples += bytesToWriteInBuffer;
		framesToWrite -= framesToWriteInBuffer;
		queued++;
	}
	startPlaybackIfNeeded(queued);
}

int frameDelay()
{
	return 0; // TODO
}

int framesFree()
{
	return 0; // TODO
}

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

void setHintStrictUnderrunCheck(bool on)
{
	strictUnderrunCheck = on;
}

bool hintStrictUnderrunCheck()
{
	return strictUnderrunCheck;
}

void setSoloMix(bool newSoloMix)
{
	if(soloMix_ != newSoloMix)
	{
		logMsg("setting solo mix: %d", newSoloMix);
		soloMix_ = newSoloMix;
		if(!isInit())
			return; // audio init() will take care of initial focus setting
		if(soloMix_)
		{
			requestAudioFocus(eEnv());
		}
		else
		{
			abandonAudioFocus(eEnv());
		}
	}
}

bool soloMix()
{
	return soloMix_;
}

void updateFocusOnResume()
{
	if(soloMix())
	{
		requestAudioFocus(eEnv());
	}
}

void updateFocusOnPause()
{
	if(soloMix())
	{
		abandonAudioFocus(eEnv());
	}
}

CallResult init()
{
	logMsg("doing init");
	{
		auto jEnv = eEnv();
		JavaInstMethod<void> jSetVolumeControlStream;
		jSetVolumeControlStream.setup(jEnv, jBaseActivityCls, "setVolumeControlStream", "(I)V");
		jSetVolumeControlStream(jEnv, jBaseActivity, 3);
	}

	updateFocusOnResume();

	// engine object
	SLObjectItf slE;
	SLresult result = slCreateEngine(&slE, 0, nullptr, 0, nullptr, nullptr);
	assert(result == SL_RESULT_SUCCESS);
	result = (*slE)->Realize(slE, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	result = (*slE)->GetInterface(slE, SL_IID_ENGINE, &slI);
	assert(result == SL_RESULT_SUCCESS);

	// output mix object
	result = (*slI)->CreateOutputMix(slI, &outMix, 0, nullptr, nullptr);
	assert(result == SL_RESULT_SUCCESS);
	result = (*outMix)->Realize(outMix, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);

	return OK;
}

}
