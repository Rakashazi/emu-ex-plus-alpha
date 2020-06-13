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

#define LOGTAG "AAudio"
#include "../../base/android/android.hh"
#include <imagine/audio/android/AAudioOutputStream.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/logger/logger.h>
#include <aaudio/AAudio.h>
#include <dlfcn.h>

namespace IG::Audio
{

static void *libHandle{};

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

template<class T>
static void dlsymFunc(T &funcPtr, void *lib, const char *funcName)
{
	funcPtr = (T)dlsym(lib, funcName);
}

static void loadAAudioLib()
{
	if(libHandle)
		return;
	logMsg("loading libaaudio.so functions");
	void *lib = dlopen("libaaudio.so", RTLD_NOW);
	if(!lib)
	{
		logErr("error opening libaaudio.so");
		return;
	}
	dlsymFunc(AAudio_createStreamBuilder, lib, "AAudio_createStreamBuilder");
	dlsymFunc(AAudioStreamBuilder_openStream, lib, "AAudioStreamBuilder_openStream");
	dlsymFunc(AAudioStreamBuilder_setChannelCount, lib, "AAudioStreamBuilder_setChannelCount");
	dlsymFunc(AAudioStreamBuilder_setFormat, lib, "AAudioStreamBuilder_setFormat");
	dlsymFunc(AAudioStreamBuilder_setPerformanceMode, lib, "AAudioStreamBuilder_setPerformanceMode");
	dlsymFunc(AAudioStreamBuilder_setSampleRate, lib, "AAudioStreamBuilder_setSampleRate");
	dlsymFunc(AAudioStreamBuilder_setSharingMode, lib, "AAudioStreamBuilder_setSharingMode");
	dlsymFunc(AAudioStreamBuilder_setDataCallback, lib, "AAudioStreamBuilder_setDataCallback");
	dlsymFunc(AAudioStreamBuilder_setErrorCallback, lib, "AAudioStreamBuilder_setErrorCallback");
	dlsymFunc(AAudioStreamBuilder_delete, lib, "AAudioStreamBuilder_delete");
	dlsymFunc(AAudioStream_close, lib, "AAudioStream_close");
	dlsymFunc(AAudioStream_requestStart, lib, "AAudioStream_requestStart");
	dlsymFunc(AAudioStream_requestPause, lib, "AAudioStream_requestPause");
	dlsymFunc(AAudioStream_requestFlush, lib, "AAudioStream_requestFlush");
	dlsymFunc(AAudioStream_requestStop, lib, "AAudioStream_requestStop");
	if(Base::androidSDK() >= 28)
	{
		dlsymFunc(AAudioStreamBuilder_setUsage, lib, "AAudioStreamBuilder_setUsage");
	}
	libHandle = lib;
}

AAudioOutputStream::AAudioOutputStream()
{
	loadAAudioLib();
	disconnectEvent.attach(
		[this]()
		{
			if(!stream)
				return;
			logMsg("trying to re-open stream after disconnect");
			close();
			if(!openStream(pcmFormat, lowLatencyMode))
				return;
			play();
		});
}

std::error_code AAudioOutputStream::open(OutputStreamConfig config)
{
	assert(libHandle);
	if(stream)
	{
		logWarn("stream already open");
		return {};
	}
	bool lowLatencyMode = config.wantedLatencyHint() < IG::Microseconds{20000};
	auto format = config.format();
	if(!openStream(format, lowLatencyMode))
	{
		return {EINVAL, std::system_category()};
	}
	pcmFormat = format;
	onSamplesNeeded = config.onSamplesNeeded();
	this->lowLatencyMode = lowLatencyMode;
	if(config.startPlaying())
		play();
	return {};
}

bool AAudioOutputStream::openStream(PcmFormat format, bool lowLatencyMode)
{
	assert(!stream);
	logMsg("creating stream %dHz, %d channels, low-latency:%s", format.rate, format.channels,
		lowLatencyMode ? "y" : "n");
	AAudioStreamBuilder *builder;
	AAudio_createStreamBuilder(&builder);
	auto deleteStreamBuilder = IG::scopeGuard([&](){ AAudioStreamBuilder_delete(builder); });
	AAudioStreamBuilder_setChannelCount(builder, format.channels);
	assert(format.sample.bits == 16);
	AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
	AAudioStreamBuilder_setSampleRate(builder, format.rate);
	if(lowLatencyMode)
	{
		AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
		AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
	}
	if(AAudioStreamBuilder_setUsage) // present in API level 28+
		AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_GAME);
	AAudioStreamBuilder_setDataCallback(builder,
		[](AAudioStream *stream, void *userData, void *audioData, int32_t numFrames) -> aaudio_data_callback_result_t
		{
			auto thisPtr = (AAudioOutputStream*)userData;
			thisPtr->onSamplesNeeded(audioData, thisPtr->pcmFormat.framesToBytes(numFrames));
			return AAUDIO_CALLBACK_RESULT_CONTINUE;
		}, this);
	AAudioStreamBuilder_setErrorCallback(builder,
		[](AAudioStream *stream, void *userData, aaudio_result_t error)
		{
			//logErr("got error:%d callback", error);
			if(error == AAUDIO_ERROR_DISCONNECTED)
			{
				// can't re-open stream on worker thread callback, notify main thread
				auto thisPtr = (AAudioOutputStream*)userData;
				thisPtr->disconnectEvent.notify();
			}
		}, this);
	AAudioStream *stream;
	if(auto res = AAudioStreamBuilder_openStream(builder, &stream);
		res != AAUDIO_OK)
	{
		logErr("error:%d creating stream", res);
		return false;
	}
	this->stream = stream;
	return true;
}

void AAudioOutputStream::play()
{
	if(unlikely(!stream))
		return;
	AAudioStream_requestStart(stream);
	isPlaying_ = true;
}

void AAudioOutputStream::pause()
{
	if(unlikely(!stream))
		return;
	AAudioStream_requestPause(stream);
	isPlaying_ = false;
}

void AAudioOutputStream::close()
{
	if(unlikely(!stream))
		return;
	logMsg("closing stream");
	AAudioStream_close(stream);
	isPlaying_ = false;
	stream = nullptr;
}

void AAudioOutputStream::flush()
{
	if(unlikely(!stream))
		return;
	AAudioStream_requestFlush(stream);
	isPlaying_ = false;
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
	return libHandle;
}

}
