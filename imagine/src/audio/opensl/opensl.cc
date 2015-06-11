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
#include "../../base/android/android.hh"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <imagine/util/ringbuffer/LinuxRingBuffer.hh>
using RingBufferType = StaticLinuxRingBuffer<>;

namespace Audio
{
PcmFormat pcmFormat{};
static SLEngineItf slI{};
static SLObjectItf outMix{}, player{};
static SLPlayItf playerI{};
static SLAndroidSimpleBufferQueueItf slBuffQI{};
static uint wantedLatency = 100000;
static uint outputBufferBytes = 0; // size in bytes per buffer to enqueue
static bool isPlaying_ = false, strictUnderrunCheck = true;
static bool reachedEndOfPlayback = false;
static RingBufferType rBuff{};
static uint unqueuedBytes = 0; // number of bytes in ring buffer that haven't been enqueued to SL yet
static char *ringBuffNextQueuePos{};

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

static uint bufferFramesForSampleRate(int rate)
{
	if(rate == AudioManager::nativeFormat().rate && AudioManager::nativeOutputFramesPerBuffer())
		return AudioManager::nativeOutputFramesPerBuffer();
	else if(rate <= 22050)
		return 256;
	else if(rate <= 44100)
		return 512;
	else
		return 1024;
}

static void init()
{
	logMsg("running init");

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
}

CallResult openPcm(const PcmFormat &format)
{
	if(player)
	{
		logWarn("called openPcm when pcm already on");
		return OK;
	}
	if(unlikely(!isInit()))
		init();
	pcmFormat = format;

	// setup ring buffer and related
	uint outputBufferFrames = bufferFramesForSampleRate(format.rate);
	outputBufferBytes = pcmFormat.framesToBytes(outputBufferFrames);
	uint ringBufferSize = std::max(2u, IG::divRoundUp(pcmFormat.uSecsToBytes(wantedLatency), outputBufferBytes)) * outputBufferBytes;
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

}
