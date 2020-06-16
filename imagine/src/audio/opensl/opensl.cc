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
#include <imagine/audio/opensl/OpenSLESOutputStream.hh>
#include <imagine/logger/logger.h>
#include "../../base/android/android.hh"

namespace IG::Audio
{

OpenSLESOutputStream::OpenSLESOutputStream()
{
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

std::error_code OpenSLESOutputStream::open(OutputStreamConfig config)
{
	if(player)
	{
		logWarn("stream already open");
		return {};
	}
	if(unlikely(!*this))
	{
		return {EINVAL, std::system_category()};
	}
	auto format = config.format();
	// must create queue with 2 buffers on Android <= 4.2
	// to get low-latency path, even though we only queue 1
	auto androidSDK = Base::androidSDK();
	uint32_t outputBuffers = androidSDK >= 18 ? 1 : 2;
	auto bufferFrames = AudioManager::nativeOutputFramesPerBuffer();
	logMsg("creating stream %dHz, %d channels, %u frames/buffer", format.rate, format.channels, bufferFrames);
	SLDataLocator_AndroidSimpleBufferQueue buffQLoc{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, outputBuffers};
	SLDataFormat_PCM slFormat
	{
		SL_DATAFORMAT_PCM, (SLuint32)format.channels, (SLuint32)format.rate * 1000, // as milliHz
		format.sample.bits(), format.sample.bits(),
		format.channels == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audioSrc{&buffQLoc, &slFormat};
	SLAndroidDataFormat_PCM_EX slFormatEx{};
	if(androidSDK >= 21)
	{
		slFormatEx =
		{
			SL_ANDROID_DATAFORMAT_PCM_EX, slFormat.numChannels, slFormat.samplesPerSec,
			slFormat.bitsPerSample, slFormat.containerSize, slFormat.channelMask, slFormat.endianness,
			format.sample.isFloat() ? SL_ANDROID_PCM_REPRESENTATION_FLOAT : SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT
		};
		audioSrc.pFormat = &slFormatEx;
	}
	else if(unlikely(format.sample.isFloat()))
	{
		logErr("floating-point samples need API level 21+");
		return {EINVAL, std::system_category()};
	}
	SLDataLocator_OutputMix outMixLoc{SL_DATALOCATOR_OUTPUTMIX, outMix};
	SLDataSink sink{&outMixLoc, nullptr};
	const SLInterfaceID ids[]{SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
	const SLboolean req[std::size(ids)]{SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};
	SLresult result = (*slI)->CreateAudioPlayer(slI, &player, &audioSrc, &sink, std::size(ids), ids, req);
	if(unlikely(result != SL_RESULT_SUCCESS))
	{
		logErr("CreateAudioPlayer returned 0x%X", (uint32_t)result);
		player = nullptr;
		return {EINVAL, std::system_category()};
	}
	result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_PLAY, &playerI);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &slBuffQI);
	assert(result == SL_RESULT_SUCCESS);
	pcmFormat = format;
	onSamplesNeeded = config.onSamplesNeeded();
	bufferBytes = format.framesToBytes(bufferFrames);
	buffer = std::make_unique<uint8_t[]>(bufferBytes);
	result = (*slBuffQI)->RegisterCallback(slBuffQI,
		[](SLAndroidSimpleBufferQueueItf queue, void *thisPtr_)
		{
			auto thisPtr = static_cast<OpenSLESOutputStream*>(thisPtr_);
			thisPtr->doBufferCallback(queue);
		}, this);
	assert(result == SL_RESULT_SUCCESS);
	if(config.startPlaying())
		play();
	return {};
}

void OpenSLESOutputStream::play()
{
	if(unlikely(!player))
		return;
	if(SLresult result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PLAYING);
		result == SL_RESULT_SUCCESS)
	{
		logMsg("started playback");
		isPlaying_ = 1;
		if(!bufferQueued)
		{
			doBufferCallback(slBuffQI);
			bufferQueued = true;
		}
	}
	else
	{
		logErr("SetPlayState(SL_PLAYSTATE_PLAYING) returned 0x%X", (uint32_t)result);
	}
}

void OpenSLESOutputStream::pause()
{
	if(unlikely(!player) || !isPlaying_)
		return;
	logMsg("pausing playback");
	if(SLresult result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);
		result != SL_RESULT_SUCCESS)
	{
		logWarn("SetPlayState(SL_PLAYSTATE_PAUSED) returned 0x%X", (uint32_t)result);
	}
	isPlaying_ = 0;
}

void OpenSLESOutputStream::close()
{
	if(!player)
		return
	logMsg("closing player");
	isPlaying_ = false;
	slBuffQI = nullptr;
	(*player)->Destroy(player);
	player = nullptr;
	buffer.reset();
	bufferQueued = false;
}

void OpenSLESOutputStream::flush()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pause();
	if(SLresult result = (*slBuffQI)->Clear(slBuffQI);
		result != SL_RESULT_SUCCESS)
	{
		logWarn("Clear returned 0x%X", (uint32_t)result);
	}
	bufferQueued = false;
}

bool OpenSLESOutputStream::isOpen()
{
	return player;
}

bool OpenSLESOutputStream::isPlaying()
{
	return isPlaying_;
}

OpenSLESOutputStream::operator bool() const
{
	return outMix;
}

void OpenSLESOutputStream::doBufferCallback(SLAndroidSimpleBufferQueueItf queue)
{
	onSamplesNeeded(buffer.get(), bufferBytes);
	if(SLresult result = (*queue)->Enqueue(queue, buffer.get(), bufferBytes);
			result != SL_RESULT_SUCCESS)
		{
			logWarn("Enqueue returned 0x%X", (uint32_t)result);
		}
}

}
