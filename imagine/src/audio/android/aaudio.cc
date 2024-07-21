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

#include <imagine/audio/android/AAudioOutputStream.hh>
#include <imagine/audio/OutputStream.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/base/sharedLibrary.hh>
#include <imagine/logger/logger.h>
#include <aaudio/AAudio.h>

namespace IG::Audio
{

constexpr SystemLogger log{"AAudio"};

static aaudio_result_t (*AAudio_createStreamBuilder)(AAudioStreamBuilder** builder){};
static aaudio_result_t (*AAudioStreamBuilder_openStream)(AAudioStreamBuilder* builder, AAudioStream** stream){};
static void (*AAudioStreamBuilder_setChannelCount)(AAudioStreamBuilder* builder, int32_t channelCount){};
static void (*AAudioStreamBuilder_setFormat)(AAudioStreamBuilder* builder, aaudio_format_t format){};
static void (*AAudioStreamBuilder_setPerformanceMode)(AAudioStreamBuilder* builder, aaudio_performance_mode_t mode){};
static void (*AAudioStreamBuilder_setSampleRate)(AAudioStreamBuilder* builder, int32_t sampleRate){};
static void (*AAudioStreamBuilder_setSharingMode)(AAudioStreamBuilder* builder, aaudio_sharing_mode_t sharingMode){};
static void (*AAudioStreamBuilder_setUsage)(AAudioStreamBuilder* builder, aaudio_usage_t usage){};
static void (*AAudioStreamBuilder_setDataCallback)(AAudioStreamBuilder* builder, AAudioStream_dataCallback callback, void *userData){};
static void (*AAudioStreamBuilder_setErrorCallback)(AAudioStreamBuilder* builder, AAudioStream_errorCallback callback, void *userData){};
static aaudio_result_t (*AAudioStreamBuilder_delete)(AAudioStreamBuilder* builder){};
static aaudio_result_t (*AAudioStream_close)(AAudioStream* stream){};
static aaudio_result_t (*AAudioStream_requestStart)(AAudioStream* stream){};
static aaudio_result_t (*AAudioStream_requestPause)(AAudioStream* stream){};
static aaudio_result_t (*AAudioStream_requestFlush)(AAudioStream* stream){};
static aaudio_result_t (*AAudioStream_requestStop)(AAudioStream* stream){};
static aaudio_stream_state_t (*AAudioStream_getState)(AAudioStream *stream){};
static aaudio_result_t (*AAudioStream_waitForStateChange)(AAudioStream *stream, aaudio_stream_state_t inputState,
	aaudio_stream_state_t *nextState, int64_t timeoutNanoseconds){};

static bool loadedAAudioLib()
{
	return AAudio_createStreamBuilder;
}

static void loadAAudioLib(const Manager &manager)
{
	if(loadedAAudioLib())
		return;
	log.info("loading libaaudio.so functions");
	auto lib = openSharedLibrary("libaaudio.so", {.resolveAllSymbols = true});
	if(!lib)
	{
		log.error("error opening libaaudio.so");
		return;
	}
	loadSymbol(AAudio_createStreamBuilder, lib, "AAudio_createStreamBuilder");
	loadSymbol(AAudioStreamBuilder_openStream, lib, "AAudioStreamBuilder_openStream");
	loadSymbol(AAudioStreamBuilder_setChannelCount, lib, "AAudioStreamBuilder_setChannelCount");
	loadSymbol(AAudioStreamBuilder_setFormat, lib, "AAudioStreamBuilder_setFormat");
	loadSymbol(AAudioStreamBuilder_setPerformanceMode, lib, "AAudioStreamBuilder_setPerformanceMode");
	loadSymbol(AAudioStreamBuilder_setSampleRate, lib, "AAudioStreamBuilder_setSampleRate");
	loadSymbol(AAudioStreamBuilder_setSharingMode, lib, "AAudioStreamBuilder_setSharingMode");
	loadSymbol(AAudioStreamBuilder_setDataCallback, lib, "AAudioStreamBuilder_setDataCallback");
	loadSymbol(AAudioStreamBuilder_setErrorCallback, lib, "AAudioStreamBuilder_setErrorCallback");
	loadSymbol(AAudioStreamBuilder_delete, lib, "AAudioStreamBuilder_delete");
	loadSymbol(AAudioStream_close, lib, "AAudioStream_close");
	loadSymbol(AAudioStream_requestStart, lib, "AAudioStream_requestStart");
	loadSymbol(AAudioStream_requestPause, lib, "AAudioStream_requestPause");
	loadSymbol(AAudioStream_requestFlush, lib, "AAudioStream_requestFlush");
	loadSymbol(AAudioStream_requestStop, lib, "AAudioStream_requestStop");
	loadSymbol(AAudioStream_getState, lib, "AAudioStream_getState");
	loadSymbol(AAudioStream_waitForStateChange, lib, "AAudioStream_waitForStateChange");
	if(manager.hasStreamUsage())
	{
		loadSymbol(AAudioStreamBuilder_setUsage, lib, "AAudioStreamBuilder_setUsage");
	}
}

static const char *streamResultStr(aaudio_result_t result)
{
	switch(result)
	{
		case AAUDIO_OK: return "OK";
		default: return "Unknown";
		case AAUDIO_ERROR_ILLEGAL_ARGUMENT: return "Illegal Argument";
		case AAUDIO_ERROR_INVALID_STATE: return "Invalid State";
		case AAUDIO_ERROR_INVALID_HANDLE: return "Invalid Handle";
		case AAUDIO_ERROR_UNIMPLEMENTED: return "Unimplemented";
		case AAUDIO_ERROR_UNAVAILABLE: return "Unavailable";
		case AAUDIO_ERROR_NO_MEMORY: return "No Memory";
		case AAUDIO_ERROR_NULL: return "Null Pointer";
		case AAUDIO_ERROR_TIMEOUT: return "Timed Out";
		case AAUDIO_ERROR_WOULD_BLOCK: return "Would Block";
		case AAUDIO_ERROR_INVALID_FORMAT: return "Invalid Format";
		case AAUDIO_ERROR_OUT_OF_RANGE: return "Value Out of Range";
		case AAUDIO_ERROR_NO_SERVICE: return "No Audio Service";
		case AAUDIO_ERROR_INVALID_RATE: return "Invalid Rate";
	}
}

static const char *streamStateStr(aaudio_stream_state_t state)
{
	switch(state)
	{
		case AAUDIO_STREAM_STATE_UNINITIALIZED: return "Uninitialized";
		default: return "Unknown";
		case AAUDIO_STREAM_STATE_OPEN: return "Open";
		case AAUDIO_STREAM_STATE_STARTING: return "Starting";
		case AAUDIO_STREAM_STATE_STARTED: return "Started";
		case AAUDIO_STREAM_STATE_PAUSING: return "Pausing";
		case AAUDIO_STREAM_STATE_PAUSED: return "Paused";
		case AAUDIO_STREAM_STATE_FLUSHING: return "Flushing";
		case AAUDIO_STREAM_STATE_FLUSHED: return "Flushed";
		case AAUDIO_STREAM_STATE_STOPPING: return "Stopping";
		case AAUDIO_STREAM_STATE_STOPPED: return "Stopped";
		case AAUDIO_STREAM_STATE_CLOSING: return "Closing";
		case AAUDIO_STREAM_STATE_CLOSED: return "Closed";
		case AAUDIO_STREAM_STATE_DISCONNECTED: return "Disconnected";
	}
}

AAudioOutputStream::AAudioOutputStream(const Manager &manager):
	disconnectEvent
	{
		{.debugLabel = "AAudioOutputStream::disconnectEvent", .eventLoop = EventLoop::forThread()},
		[this]
		{
			if(!stream)
				return;
			log.info("trying to re-open stream after disconnect");
			AAudioStream_close(std::exchange(stream, {}));
			if(auto res = AAudioStreamBuilder_openStream(builder, &stream);
				res != AAUDIO_OK)
			{
				log.error("error:{} creating stream", streamResultStr(res));
				return;
			}
			play();
		}
	}
{
	loadAAudioLib(manager);
	AAudio_createStreamBuilder(&builder);
}

AAudioOutputStream::~AAudioOutputStream()
{
	close();
	AAudioStreamBuilder_delete(builder);
}

StreamError AAudioOutputStream::open(OutputStreamConfig config)
{
	assert(loadedAAudioLib());
	if(stream)
	{
		log.warn("stream already open");
		return {};
	}
	bool lowLatencyMode = config.wantedLatencyHint < Microseconds{20000};
	auto format = config.format;
	if(format.sample != SampleFormats::i16 && format.sample != SampleFormats::f32)
	{
		log.error("only i16 and f32 sample formats are supported");
		return StreamError::BadArgument;
	}
	log.info("creating stream {}Hz, {} channels, low-latency:{}", format.rate, format.channels, lowLatencyMode);
	onSamplesNeeded = config.onSamplesNeeded;
	setBuilderData(builder, format, lowLatencyMode);
	if(auto res = AAudioStreamBuilder_openStream(builder, &stream);
		res != AAUDIO_OK)
	{
		log.error("error:{} creating stream", streamResultStr(res));
		return StreamError::BadArgument;
	}
	if(config.startPlaying)
		play();
	return {};
}

void AAudioOutputStream::play()
{
	if(!stream) [[unlikely]]
		return;
	isPlaying_ = true;
	AAudioStream_requestStart(stream);
}

void AAudioOutputStream::pause()
{
	if(!stream) [[unlikely]]
		return;
	isPlaying_ = false;
	AAudioStream_requestPause(stream);
}

static void waitUntilState(AAudioStream *stream, aaudio_stream_state_t wantedState)
{
	aaudio_result_t res = AAUDIO_OK;
	aaudio_stream_state_t currentState = AAudioStream_getState(stream);
	aaudio_stream_state_t inputState = currentState;
	while(res == AAUDIO_OK && currentState != wantedState)
	{
		res = AAudioStream_waitForStateChange(stream, inputState, &currentState, INT64_MAX);
		log.info("transitioned state:{} -> {}", streamStateStr(inputState), streamStateStr(currentState));
		inputState = currentState;
	}
	if(res != AAUDIO_OK)
	{
		log.error("error:{} waiting for state:{}", streamResultStr(res), streamStateStr(wantedState));
	}
}

void AAudioOutputStream::close()
{
	if(!stream) [[unlikely]]
		return;
	log.info("closing stream:{}", (void*)stream);
	isPlaying_ = false;
	// Devices like the Samsung A3 (2017) have spurious callbacks after AAudioStream_close()
	// To avoid this make sure the stream is fully stopped before closing
	// Documented in https://github.com/google/oboe/blob/master/src/aaudio/AudioStreamAAudio.cpp
	AAudioStream_requestStop(stream);
	waitUntilState(stream, AAUDIO_STREAM_STATE_STOPPED);
	if(auto res = AAudioStream_close(std::exchange(stream, {}));
		res != AAUDIO_OK)
	{
		log.error("error:{} closing stream", streamResultStr(res));
	}
}

void AAudioOutputStream::flush()
{
	if(!stream) [[unlikely]]
		return;
	isPlaying_ = false;
	AAudioStream_requestFlush(stream);
}

bool AAudioOutputStream::isOpen()
{
	return stream;
}

bool AAudioOutputStream::isPlaying()
{
	return isPlaying_;
}

AAudioOutputStream::operator bool() const
{
	return loadedAAudioLib();
}

void AAudioOutputStream::setBuilderData(AAudioStreamBuilder *builder, Format format, bool lowLatencyMode)
{
	AAudioStreamBuilder_setChannelCount(builder, format.channels);
	AAudioStreamBuilder_setFormat(builder, format.sample.isFloat() ? AAUDIO_FORMAT_PCM_FLOAT : AAUDIO_FORMAT_PCM_I16);
	AAudioStreamBuilder_setSampleRate(builder, format.rate);
	if(lowLatencyMode)
	{
		AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
		AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
	}
	if(AAudioStreamBuilder_setUsage) // present in API level 28+
		AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_GAME);
	AAudioStreamBuilder_setDataCallback(builder,
		[](AAudioStream*, void* userData, void* audioData, int32_t numFrames) -> aaudio_data_callback_result_t
		{
			auto thisPtr = (AAudioOutputStream*)userData;
			thisPtr->onSamplesNeeded(audioData, numFrames);
			return AAUDIO_CALLBACK_RESULT_CONTINUE;
		}, this);
	AAudioStreamBuilder_setErrorCallback(builder,
		[](AAudioStream*, void* userData, aaudio_result_t error)
		{
			//log.error("got error:{} callback", streamResultStr(error));
			if(error == AAUDIO_ERROR_DISCONNECTED)
			{
				// can't re-open stream on worker thread callback, notify main thread
				auto thisPtr = (AAudioOutputStream*)userData;
				thisPtr->disconnectEvent.notify();
			}
		}, this);
}

}
