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

#include <imagine/audio/opensl/OpenSLESOutputStream.hh>
#include <imagine/audio/OutputStream.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/logger/logger.h>

namespace IG::Audio
{

constexpr SystemLogger log{"OpenSL"};

OpenSLESOutputStream::OpenSLESOutputStream(const Manager &manager)
{
	// engine object
	SLObjectItf slE;
	SLresult result = slCreateEngine(&slE, 0, nullptr, 0, nullptr, nullptr);
	if(result != SL_RESULT_SUCCESS)
	{
		log.error("error creating engine");
		return;
	}
	result = (*slE)->Realize(slE, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);

	// output mix object
	SLEngineItf slI;
	result = (*slE)->GetInterface(slE, SL_IID_ENGINE, &slI);
	assert(result == SL_RESULT_SUCCESS);
	SLObjectItf outMix;
	result = (*slI)->CreateOutputMix(slI, &outMix, 0, nullptr, nullptr);
	assert(result == SL_RESULT_SUCCESS);
	result = (*outMix)->Realize(outMix, SL_BOOLEAN_FALSE);
	if(result != SL_RESULT_SUCCESS)
	{
		log.error("error creating output mix");
		return;
	}
	this->slE = slE;
	this->outMix = outMix;
	bufferFrames = manager.nativeOutputFramesPerBuffer();
	supportsFloatFormat = manager.hasFloatFormat();
	// must create queue with 2 buffers on Android <= 4.2
	// to get low-latency path, even though we only queue 1
	outputBuffers = manager.defaultOutputBuffers();
}

OpenSLESOutputStream::~OpenSLESOutputStream()
{
	if(!outMix)
		return;
	close();
	(*outMix)->Destroy(outMix);
	(*slE)->Destroy(slE);
}

StreamError OpenSLESOutputStream::open(OutputStreamConfig config)
{
	if(player)
	{
		log.warn("stream already open");
		return {};
	}
	if(!*this) [[unlikely]]
	{
		return StreamError::NotInitialized;
	}
	auto format = config.format;
	log.info("creating stream {}Hz, {} channels, {} frames/buffer", format.rate, format.channels, bufferFrames);
	SLDataLocator_AndroidSimpleBufferQueue buffQLoc{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, outputBuffers};
	SLDataFormat_PCM slFormat
	{
		SL_DATAFORMAT_PCM, (SLuint32)format.channels, (SLuint32)format.rate * 1000, // as milliHz
		(SLuint32)format.sample.bits(), (SLuint32)format.sample.bits(),
		format.channels == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audioSrc{&buffQLoc, &slFormat};
	SLAndroidDataFormat_PCM_EX slFormatEx{};
	if(supportsFloatFormat)
	{
		slFormatEx =
		{
			SL_ANDROID_DATAFORMAT_PCM_EX, slFormat.numChannels, slFormat.samplesPerSec,
			slFormat.bitsPerSample, slFormat.containerSize, slFormat.channelMask, slFormat.endianness,
			format.sample.isFloat() ? SL_ANDROID_PCM_REPRESENTATION_FLOAT : SL_ANDROID_PCM_REPRESENTATION_SIGNED_INT
		};
		audioSrc.pFormat = &slFormatEx;
	}
	else if(format.sample.isFloat()) [[unlikely]]
	{
		log.error("floating-point samples need API level 21+");
		return StreamError::BadArgument;
	}
	SLDataLocator_OutputMix outMixLoc{SL_DATALOCATOR_OUTPUTMIX, outMix};
	SLDataSink sink{&outMixLoc, nullptr};
	const SLInterfaceID ids[]{SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME};
	const SLboolean req[std::size(ids)]{SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE};
	SLEngineItf slI;
	SLresult result = (*slE)->GetInterface(slE, SL_IID_ENGINE, &slI);
	assert(result == SL_RESULT_SUCCESS);
	result = (*slI)->CreateAudioPlayer(slI, &player, &audioSrc, &sink, std::size(ids), ids, req);
	if(result != SL_RESULT_SUCCESS) [[unlikely]]
	{
		log.error("CreateAudioPlayer returned {:#X}", result);
		player = nullptr;
		return StreamError::BadArgument;
	}
	result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_PLAY, &playerI);
	assert(result == SL_RESULT_SUCCESS);
	result = (*player)->GetInterface(player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &slBuffQI);
	assert(result == SL_RESULT_SUCCESS);
	onSamplesNeeded = config.onSamplesNeeded;
	bufferBytes = format.framesToBytes(bufferFrames);
	buffer = std::make_unique<uint8_t[]>(bufferBytes);
	result = (*slBuffQI)->RegisterCallback(slBuffQI,
		[](SLAndroidSimpleBufferQueueItf queue, void *thisPtr_)
		{
			auto thisPtr = static_cast<OpenSLESOutputStream*>(thisPtr_);
			thisPtr->doBufferCallback(queue);
		}, this);
	assert(result == SL_RESULT_SUCCESS);
	if(config.startPlaying)
		play();
	return {};
}

void OpenSLESOutputStream::play()
{
	if(!player) [[unlikely]]
		return;
	if(SLresult result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PLAYING);
		result == SL_RESULT_SUCCESS)
	{
		log.info("started playback");
		isPlaying_ = true;
		if(!bufferQueued)
		{
			doBufferCallback(slBuffQI);
			bufferQueued = true;
		}
	}
	else
	{
		log.error("SetPlayState(SL_PLAYSTATE_PLAYING) returned {:#X}", result);
	}
}

void OpenSLESOutputStream::pause()
{
	if(!player || !isPlaying_) [[unlikely]]
		return;
	log.info("pausing playback");
	if(SLresult result = (*playerI)->SetPlayState(playerI, SL_PLAYSTATE_PAUSED);
		result != SL_RESULT_SUCCESS)
	{
		log.warn("SetPlayState(SL_PLAYSTATE_PAUSED) returned {:#X}", result);
	}
	isPlaying_ = false;
}

void OpenSLESOutputStream::close()
{
	if(!player)
		return;
	log.info("closing player");
	isPlaying_ = false;
	slBuffQI = nullptr;
	(*player)->Destroy(player);
	player = nullptr;
	buffer.reset();
	bufferQueued = false;
}

void OpenSLESOutputStream::flush()
{
	if(!isOpen())
		return;
	log.info("clearing queued samples");
	pause();
	if(SLresult result = (*slBuffQI)->Clear(slBuffQI);
		result != SL_RESULT_SUCCESS)
	{
		log.warn("Clear returned {:#X}", result);
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
	onSamplesNeeded(buffer.get(), bufferFrames);
	if(SLresult result = (*queue)->Enqueue(queue, buffer.get(), bufferBytes);
			result != SL_RESULT_SUCCESS)
		{
			log.warn("Enqueue returned {:#X}", result);
		}
}

}
