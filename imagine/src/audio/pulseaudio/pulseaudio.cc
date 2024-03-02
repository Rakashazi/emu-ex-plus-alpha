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

#include <imagine/audio/pulseaudio/PAOutputStream.hh>
#include <imagine/audio/OutputStream.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <pulse/pulseaudio.h>
#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
#include <pulse/glib-mainloop.h>
#else
#include <pulse/thread-mainloop.h>
#endif

namespace IG::Audio
{

constexpr SystemLogger log{"PulseAudio"};

static pa_sample_format_t pcmFormatToPA(const SampleFormat &format)
{
	switch(format.bytes())
	{
		case 4 : return format.isFloat() ? PA_SAMPLE_FLOAT32NE : PA_SAMPLE_S32NE;
		case 2 : return PA_SAMPLE_S16NE;
		case 1 : return PA_SAMPLE_U8;
		default:
			bug_unreachable("bytes == %d", format.bytes());
	}
}

struct ContextStateResult
{
	constexpr ContextStateResult(PAOutputStream *thisPtr): thisPtr{thisPtr} {}
	PAOutputStream *thisPtr;
	pa_context_state_t state = PA_CONTEXT_FAILED;
};

struct StreamStateResult
{
	constexpr StreamStateResult(PAOutputStream *thisPtr): thisPtr{thisPtr} {}
	PAOutputStream *thisPtr;
	pa_stream_state state = PA_STREAM_FAILED;
};

PAOutputStream::PAOutputStream()
{
	mainloop = pa_threaded_mainloop_new();
	context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), "Test");
	auto freeMain = IG::scopeGuard([this](){ freeMainLoop(); });
	if(!context)
	{
		log.error("unable to create context");
		return;
	}
	auto unrefContext = IG::scopeGuard([this](){ pa_context_unref(context); context = {}; });
	ContextStateResult result{this};
	pa_context_set_state_callback(context,
		[](pa_context *context, void *resultPtr)
		{
			auto result = static_cast<ContextStateResult*>(resultPtr);
			auto state = pa_context_get_state(context);
			switch(state)
			{
				case PA_CONTEXT_READY:
				case PA_CONTEXT_FAILED:
				case PA_CONTEXT_TERMINATED:
					result->state = state;
					result->thisPtr->signalMainLoop();
					break;
				default: break;
			}
		}, &result);
	if(pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
	{
		log.error("unable to connect context");
		return;
	}
	lockMainLoop();
	startMainLoop();
	waitMainLoop();
	pa_context_set_state_callback(context, nullptr, nullptr);
	if(result.state != PA_CONTEXT_READY)
	{
		log.error("context connection failed");
		unlockMainLoop();
		stopMainLoop();
		return;
	}
	unlockMainLoop();
	unrefContext.cancel();
	freeMain.cancel();
}

PAOutputStream::~PAOutputStream()
{
	if(!context)
		return;
	close();
	pa_context_unref(context);
	stopMainLoop();
	freeMainLoop();
}

StreamError PAOutputStream::open(OutputStreamConfig config)
{
	if(isOpen())
	{
		log.info("audio already open");
		return {};
	}
	if(!context) [[unlikely]]
	{
		return StreamError::BadArgument;
	}
	auto format = config.format;
	pcmFormat = format;
	onSamplesNeeded = config.onSamplesNeeded;
	pa_sample_spec spec{};
	spec.format = pcmFormatToPA(format.sample);
	spec.rate = format.rate;
	spec.channels = format.channels;
	pa_proplist *props = pa_proplist_new();
	pa_proplist_sets(props, PA_PROP_MEDIA_ROLE, "game");
	lockMainLoop();
	stream = pa_stream_new_with_proplist(context, "Playback", &spec, nullptr, props);
	if(!stream)
	{
		log.error("error creating stream");
		pa_proplist_free(props);
		return StreamError::BadArgument;
	}
	pa_proplist_free(props);
	StreamStateResult result{this};
	pa_stream_set_state_callback(stream,
		[](pa_stream *stream, void *resultPtr)
		{
			auto result = static_cast<StreamStateResult*>(resultPtr);
			auto state = pa_stream_get_state(stream);
			switch(state)
			{
				case PA_STREAM_READY:
				case PA_STREAM_FAILED:
				case PA_STREAM_TERMINATED:
					result->state = state;
					result->thisPtr->signalMainLoop();
					break;
				default: break;
			}
		}, &result);
	pa_stream_set_write_callback(stream,
		[](pa_stream *stream, size_t bytes, void *thisPtr_)
		{
			auto thisPtr = static_cast<PAOutputStream*>(thisPtr_);
			void *buff;
			if(int err = pa_stream_begin_write(stream, &buff, &bytes);
				err < 0)
			{
				log.error("error:{} in pa_stream_begin_write with {} bytes", err, bytes);
				return;
			}
			assumeExpr(thisPtr->onSamplesNeeded);
			thisPtr->onSamplesNeeded(buff, thisPtr->pcmFormat.bytesToFrames(bytes));
			if(int err = pa_stream_write(stream, buff, bytes, nullptr, 0, PA_SEEK_RELATIVE);
				err < 0)
			{
				log.warn("error writing {} bytes", bytes);
			}
		}, this);
	const auto wantedLatency = config.wantedLatencyHint.count() ? config.wantedLatencyHint : IG::Microseconds{10000};
	pa_buffer_attr bufferAttr{};
	bufferAttr.maxlength = -1;
	bufferAttr.tlength = format.timeToBytes(wantedLatency);
	bufferAttr.prebuf = -1;
	bufferAttr.minreq = -1;
	if(pa_stream_connect_playback(stream, nullptr, &bufferAttr,
		pa_stream_flags_t(PA_STREAM_ADJUST_LATENCY /*| PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING*/),
		nullptr, nullptr) < 0)
	{
		log.error("error connecting playback stream");
		close();
		return StreamError::BadArgument;
	}
	waitMainLoop();
	pa_stream_set_state_callback(stream, nullptr, nullptr);
	if(result.state != PA_STREAM_READY)
	{
		log.error("error connecting playback stream async");
		close();
		return StreamError::BadArgument;
	}
	auto serverAttr = pa_stream_get_buffer_attr(stream);
	if(config.startPlaying)
	{
		// uncorked by default
		isCorked = false;
	}
	else
	{
		pa_stream_cork(stream, 1, nullptr, nullptr);
		isCorked = true;
	}
	unlockMainLoop();
	assert(serverAttr);
	log.info("opened stream with target fill bytes:{}", serverAttr->tlength);
	return {};
}

void PAOutputStream::play()
{
	if(!isOpen()) [[unlikely]]
		return;
	lockMainLoop();
	pa_stream_cork(stream, 0, nullptr, nullptr);
	unlockMainLoop();
	isCorked = false;
	iterateMainLoop();
}

void PAOutputStream::pause()
{
	if(!isOpen()) [[unlikely]]
		return;
	log.info("pausing playback");
	lockMainLoop();
	pa_stream_cork(stream, 1, nullptr, nullptr);
	unlockMainLoop();
	isCorked = true;
	iterateMainLoop();
}

void PAOutputStream::close()
{
	if(!isOpen())
	{
		return;
	}
	lockMainLoop();
	pa_stream_disconnect(stream);
	pa_stream_unref(stream);
	unlockMainLoop();
	iterateMainLoop();
	isCorked = true;
	stream = {};
}

void PAOutputStream::flush()
{
	if(!isOpen()) [[unlikely]]
		return;
	log.info("clearing queued samples");
	lockMainLoop();
	pa_stream_flush(stream, nullptr, nullptr);
	unlockMainLoop();
	iterateMainLoop();
}

bool PAOutputStream::isOpen()
{
	return stream;
}

bool PAOutputStream::isPlaying()
{
	if(!isOpen())
		return false;
	return !isCorked;
}

PAOutputStream::operator bool() const
{
	return context;
}

void PAOutputStream::lockMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_lock(mainloop);
	#endif
}

void PAOutputStream::unlockMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_unlock(mainloop);
	#endif
}

void PAOutputStream::signalMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	log.info("signaling main loop");
	assert(!mainLoopSignaled);
	mainLoopSignaled = true;
	#else
	pa_threaded_mainloop_signal(mainloop, 0);
	#endif
}

void PAOutputStream::iterateMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	int iterations = 0;
	while(g_main_context_iteration(nullptr, false) == TRUE)
	{
		iterations++;
	}
	//log.info("ran {} GLIB iterations", iterations);
	#else
	// no-op, running in separate thread
	#endif
}

void PAOutputStream::waitMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	int iterations = 0;
	while(!mainLoopSignaled)
	{
		g_main_context_iteration(nullptr, true);
		iterations++;
	}
	mainLoopSignaled = false;
	log.info("signaled after {} GLIB iterations", iterations);
	#else
	pa_threaded_mainloop_wait(mainloop);
	#endif
}

void PAOutputStream::startMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_start(mainloop);
	#endif
}

void PAOutputStream::stopMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_stop(mainloop);
	#endif
}

void PAOutputStream::freeMainLoop()
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO_GLIB
	pa_glib_mainloop_free(mainloop);
	#else
	pa_threaded_mainloop_free(mainloop);
	#endif
	mainloop = {};
}

}
