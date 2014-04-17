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

#define LOGTAG "OpenSL"
#include <imagine/audio/Audio.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/number.h>
#include "../../base/android/private.hh"
#include <imagine/base/android/sdk.hh>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#if defined __ANDROID__ && defined CONFIG_MACHINE_GENERIC_ARM
// some ARMv6 kernels lack mremap mirroring features, so use a plain ring buffer
#include <imagine/util/ringbuffer/RingBuffer.hh>
using RingBufferType = StaticRingBuffer<>;
#else
#include <imagine/util/ringbuffer/LinuxRingBuffer.hh>
using RingBufferType = StaticLinuxRingBuffer<>;
#endif

namespace Audio
{
using namespace Base;

PcmFormat preferredPcmFormat { 44100, SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static SLEngineItf slI = nullptr;
static SLObjectItf outMix = nullptr, player = nullptr;
static SLPlayItf playerI = nullptr;
static SLAndroidSimpleBufferQueueItf slBuffQI = nullptr;
static uint wantedLatency = 100000;
static uint preferredOutputBufferFrames = 0;
static uint outputBufferBytes = 0; // size in bytes per buffer to enqueue
static bool isPlaying_ = false, strictUnderrunCheck = true;
static jobject audioManager = nullptr;
static JavaInstMethod<jint> jRequestAudioFocus, jAbandonAudioFocus;
static bool soloMix_ = true;
static bool reachedEndOfPlayback = false;
static RingBufferType rBuff;
static uint unqueuedBytes = 0; // number of bytes in ring buffer that haven't been enqueued to SL yet
static char *ringBuffNextQueuePos = nullptr;

int maxRate()
{
	return 48000;
}

static bool isInit()
{
	return outMix;
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

static bool commitSLBuffer(void *b, uint bytes)
{
	SLresult result = (*slBuffQI)->Enqueue(slBuffQI, b, bytes);
	if(result != SL_RESULT_SUCCESS)
	{
		logWarn("Enqueue returned 0x%X", (uint)result);
		return false;
	}
	return true;
}

// runs on internal OpenSL ES thread
static void queueCallback(SLAndroidSimpleBufferQueueItf caller, void *)
{
	rBuff.commitRead(outputBufferBytes);
}

// runs on internal OpenSL ES thread
static void playCallback(SLPlayItf caller, void *, SLuint32 event)
{
	//logMsg("play event %X", (int)event);
	assert(event == SL_PLAYEVENT_HEADSTALLED || event == SL_PLAYEVENT_HEADATEND);
	reachedEndOfPlayback = true;
	if(!::Config::MACHINE_IS_OUYA) // prevent log spam
		logMsg("got playback end event");
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

static uint bufferFramesForSampleRate(int rate)
{
	if(rate == preferredPcmFormat.rate && preferredOutputBufferFrames)
		return preferredOutputBufferFrames;
	else if(rate <= 22050)
		return 256;
	else if(rate <= 44100)
		return 512;
	else
		return 1024;
}

CallResult openPcm(const PcmFormat &format)
{
	assert(isInit());
	if(player)
	{
		logWarn("called openPcm when pcm already on");
		return OK;
	}
	pcmFormat = format;

	// setup ring buffer and related
	uint outputBufferFrames = bufferFramesForSampleRate(format.rate);
	outputBufferBytes = pcmFormat.framesToBytes(outputBufferFrames);
	uint ringBufferSize = std::max(2u, IG::divUp(pcmFormat.uSecsToBytes(wantedLatency), outputBufferBytes)) * outputBufferBytes;
	uint outputBuffers = ringBufferSize / outputBufferBytes;
	rBuff.init(ringBufferSize);
	ringBuffNextQueuePos = rBuff.writeAddr();

	logMsg("creating playback %dHz, %d channels", format.rate, format.channels);
	logMsg("using %d buffers with %d frames", outputBuffers, outputBufferFrames);
	assert(format.sample.bits == 16);
	SLDataLocator_AndroidSimpleBufferQueue buffQLoc = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, outputBuffers };
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

	result = (*slBuffQI)->RegisterCallback(slBuffQI, queueCallback, nullptr);
	assert(result == SL_RESULT_SUCCESS);
	result = (*playerI)->RegisterCallback(playerI, playCallback, nullptr);
	assert(result == SL_RESULT_SUCCESS);

	uint playStateEvMask = SL_PLAYEVENT_HEADSTALLED;
	if(strictUnderrunCheck)
		playStateEvMask = SL_PLAYEVENT_HEADATEND;
	(*playerI)->SetCallbackEventsMask(playerI, playStateEvMask);
	(*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);

	logMsg("PCM opened");
	return OK;
}

void closePcm()
{
	if(player)
	{
		logMsg("closing pcm");
		isPlaying_ = 0;
		slBuffQI = nullptr;
		(*player)->Destroy(player);
		player = nullptr;
		rBuff.deinit();
		reachedEndOfPlayback = false;
		unqueuedBytes = 0;
	}
	else
		logMsg("called closePcm when pcm already off");
}

bool isOpen()
{
	return player;
}

bool isPlaying()
{
	return isPlaying_;
}

static uint buffersQueued()
{
	SLAndroidSimpleBufferQueueState state;
	(*slBuffQI)->GetState(slBuffQI, &state);
	return state.count;
}

void pausePcm()
{
	if(unlikely(!player) || !isPlaying_)
		return;
	logMsg("pausing playback");
	auto result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);
	isPlaying_ = 0;
}

void resumePcm()
{
	if(unlikely(!player))
		return;
	auto result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PLAYING);
	if(result == SL_RESULT_SUCCESS)
	{
		logMsg("started playback with %d buffers queued", (int)rBuff.writtenSize() / outputBufferBytes);
		isPlaying_ = 1;
		reachedEndOfPlayback = false;
	}
	else
		logErr("SetPlayState returned 0x%X", (uint)result);
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	SLresult result = (*slBuffQI)->Clear(slBuffQI);
	assert(result == SL_RESULT_SUCCESS);
	rBuff.reset();
	ringBuffNextQueuePos = rBuff.writeAddr();
	unqueuedBytes = 0;
}

static bool checkXRun()
{
	if(unlikely(reachedEndOfPlayback && isPlaying_))
	{
		if(!::Config::MACHINE_IS_OUYA) // prevent log spam
			logMsg("xrun");
		pausePcm();
		return true;
	}
	return false;
}

static void updateQueue(uint written)
{
	unqueuedBytes += written;
	uint toEnqueue = unqueuedBytes / outputBufferBytes;
	//logMsg("wrote %d, to queue %d bytes (%d),", written, unqueuedBytes, toEnqueue);
	if(toEnqueue)
	{
		iterateTimes(toEnqueue, i)
		{
			if(checkXRun())
				break;
			if(!commitSLBuffer(ringBuffNextQueuePos, outputBufferBytes))
			{
				bug_exit("error in enqueue even though queue should have space");
			}
			ringBuffNextQueuePos = rBuff.advanceAddr(ringBuffNextQueuePos, outputBufferBytes);
			unqueuedBytes -= outputBufferBytes;
		}
	}
}

int contiguousFramesFree()
{
	return pcmFormat.bytesToFrames(rBuff.freeContiguousSpace());
}

BufferContext getPlayBuffer(uint wantedFrames)
{
	// use contiguousFramesFree() since we may have a plain ring buffer without mirroring
	if(unlikely(!isOpen() || !contiguousFramesFree()))
		return {};
	if((uint)contiguousFramesFree() < wantedFrames)
	{
		logDMsg("buffer has only %d/%d frames free", contiguousFramesFree(), wantedFrames);
	}
	return {rBuff.writeAddr(), std::min(wantedFrames, (uint)contiguousFramesFree())};
}

void commitPlayBuffer(BufferContext buffer, uint frames)
{
	assert(frames <= buffer.frames);
	auto written = pcmFormat.framesToBytes(frames);
	rBuff.commitWrite(written);
	updateQueue(written);
}

void writePcm(const void *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;
	uint bytes = pcmFormat.framesToBytes(framesToWrite);
	auto written = rBuff.write(samples, bytes);
	if(written != bytes)
	{
		logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
	updateQueue(written);
}

int frameDelay()
{
	return 0; // TODO
}

int framesFree()
{
	return pcmFormat.bytesToFrames(rBuff.freeSpace());
}

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
		if(newSoloMix)
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

bool hasLowLatency()
{
	// preferredOutputBufferFrames is only set if device reports low-latency audio
	return preferredOutputBufferFrames;
}

static int audioManagerIntProperty(JNIEnv* jEnv, JavaInstMethod<jobject> &jGetProperty, const char *propStr)
{
	auto propJStr = jEnv->NewStringUTF(propStr);
	auto valJStr = (jstring)jGetProperty(jEnv, audioManager, propJStr);
	jEnv->DeleteLocalRef(propJStr);
	if(!valJStr)
	{
		logWarn("%s is null", propStr);
		return 0;
	}
	auto valStr = jEnv->GetStringUTFChars(valJStr, nullptr);
	int val = atoi(valStr);
	jEnv->ReleaseStringUTFChars(valJStr, valStr);
	jEnv->DeleteLocalRef(valJStr);
	return val;
}

CallResult init()
{
	logMsg("running init");
	{
		auto jEnv = eEnv();
		JavaInstMethod<void> jSetVolumeControlStream;
		jSetVolumeControlStream.setup(jEnv, jBaseActivityCls, "setVolumeControlStream", "(I)V");
		jSetVolumeControlStream(jEnv, jBaseActivity, 3);

		// check preferred settings for low latency
		if(Base::androidSDK() >= 17)
		{
			setupAudioManagerJNI(jEnv);
			JavaInstMethod<jobject> jGetProperty;
			jclass jAudioManagerCls = jEnv->GetObjectClass(audioManager);
			jGetProperty.setup(jEnv, jAudioManagerCls, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
			preferredPcmFormat.rate = audioManagerIntProperty(jEnv, jGetProperty, "android.media.property.OUTPUT_SAMPLE_RATE");
			if(preferredPcmFormat.rate != 44100 && preferredPcmFormat.rate != 48000)
			{
				// only support 44KHz and 48KHz for now
				logMsg("ignoring preferred sample rate: %d", preferredPcmFormat.rate);
				preferredPcmFormat.rate = 44100;
			}
			else
			{
				logMsg("set preferred sample rate: %d", preferredPcmFormat.rate);
				// find the preferred buffer size for this rate if device has low-latency support
				JavaInstMethod<jboolean> jHasLowLatencyAudio;
				jHasLowLatencyAudio.setup(jEnv, jBaseActivityCls, "hasLowLatencyAudio", "()Z");
				if(jHasLowLatencyAudio(jEnv, jBaseActivity))
				{
					preferredOutputBufferFrames = audioManagerIntProperty(jEnv, jGetProperty, "android.media.property.OUTPUT_FRAMES_PER_BUFFER");
					if(!preferredOutputBufferFrames)
						logMsg("preferred buffer frames value not present");
					else
						logMsg("set preferred buffer frames: %d", preferredOutputBufferFrames);
				}
				else
					logMsg("no low-latency support");
			}
		}
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
