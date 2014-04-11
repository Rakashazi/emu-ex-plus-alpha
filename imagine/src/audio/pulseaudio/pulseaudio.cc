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

#define LOGTAG "PulseAudio"
#include <imagine/audio/Audio.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <pulse/pulseaudio.h>
#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
#include <pulse/glib-mainloop.h>
#else
#include <pulse/thread-mainloop.h>
#endif

namespace Audio
{

PcmFormat preferredPcmFormat { 48000, SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static uint wantedLatency = 100000;
static pa_context* context = nullptr;
static pa_stream* stream = nullptr;
static bool isCorked = true;

#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
static pa_glib_mainloop* mainloop = nullptr;
static bool mainLoopSignaled = false;
#else
static pa_threaded_mainloop* mainloop = nullptr;
#endif

int maxRate()
{
	return 48000;
}

static void lockMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_lock(mainloop);
	#endif
}

static void unlockMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_unlock(mainloop);
	#endif
}

static void signalMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	logMsg("signaling main loop");
	assert(!mainLoopSignaled);
	mainLoopSignaled = true;
	#else
	pa_threaded_mainloop_signal(mainloop, 0);
	#endif
}

static void iterateMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	int iterations = 0;
	while(g_main_context_iteration(nullptr, false) == TRUE)
	{
		iterations++;
	}
	//logMsg("ran %d GLIB iterations", iterations);
	#else
	// no-op, running in separate thread
	#endif
}

static void waitMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	int iterations = 0;
	while(!mainLoopSignaled)
	{
		g_main_context_iteration(nullptr, true);
		iterations++;
	}
	mainLoopSignaled = false;
	logMsg("signaled after %d GLIB iterations", iterations);
	#else
	pa_threaded_mainloop_wait(mainloop);
	#endif
}

static void startMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_start(mainloop);
	#endif
}

static void stopMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	// no-op
	#else
	pa_threaded_mainloop_stop(mainloop);
	#endif
}

static void freeMainLoop()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	pa_glib_mainloop_free(mainloop);
	#else
	pa_threaded_mainloop_free(mainloop);
	#endif
}

static pa_sample_format_t pcmFormatToPA(const SampleFormat &format)
{
	switch(format.toBits())
	{
		case 16 : return PA_SAMPLE_S16LE;
		case 8 : return PA_SAMPLE_U8;
		default:
			bug_branch("%d", format.toBits());
			return (pa_sample_format_t)0;
	}
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

int frameDelay()
{
	if(unlikely(!isOpen()))
		return 0;
	pa_usec_t delay;
	iterateMainLoop();
	lockMainLoop();
	int err = pa_stream_get_latency(stream, &delay, nullptr);
	unlockMainLoop();
	if(err)
	{
		// TODO: check exact error like PA_ERR_NODATA
		logErr("error getting stream latency");
		return 0;
	}
	return pcmFormat.uSecsToFrames(delay);
}

int framesFree()
{
	if(unlikely(!isOpen()))
		return 0;
	iterateMainLoop();
	lockMainLoop();
	auto bytes = pa_stream_writable_size(stream);
	unlockMainLoop();
	return pcmFormat.bytesToFrames(bytes);
}

void pausePcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("pausing playback");
	lockMainLoop();
	pa_stream_cork(stream, 1, nullptr, nullptr);
	unlockMainLoop();
	isCorked = true;
	iterateMainLoop();
}

void resumePcm()
{
	if(unlikely(!isOpen()))
		return;
	lockMainLoop();
	pa_stream_cork(stream, 0, nullptr, nullptr);
	unlockMainLoop();
	isCorked = false;
	iterateMainLoop();
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	lockMainLoop();
	pa_stream_flush(stream, nullptr, nullptr);
	unlockMainLoop();
	iterateMainLoop();
}

void writePcm(const void *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	iterateMainLoop();
	lockMainLoop(); // TODO: locking here is sub-optimal, writes should be async using a ring-buffer
	auto bytesFreeOnHW = pa_stream_writable_size(stream);
	auto framesFreeOnHW = pcmFormat.bytesToFrames(bytesFreeOnHW);
	auto bytesToWrite = pcmFormat.framesToBytes(framesToWrite);
	//logMsg("writing %d frames, %d free", framesToWrite, framesFreeOnHW);
	/*{
		static uint framesToWriteAvg = 0, writeCount = 0;
		framesToWriteAvg += framesToWrite;
		writeCount++;
		if(writeCount == 120)
		{
			logMsg("avg frames: %f, %d free", framesToWriteAvg / (double)120., framesFree());
			framesToWriteAvg = 0;
			writeCount = 0;
		}
	}*/
	if(framesFreeOnHW < framesToWrite)
	{
		logWarn("sending %d frames but only %d free", framesToWrite, framesFreeOnHW);
		bytesToWrite = bytesFreeOnHW;
	}
	int err = pa_stream_write(stream, samples, bytesToWrite, nullptr, 0, PA_SEEK_RELATIVE);
	unlockMainLoop();
	if(err < 0)
	{
		logWarn("error writing %d bytes", bytesToWrite);
	}
	iterateMainLoop();
}

CallResult openPcm(const PcmFormat &format)
{
	if(isOpen())
	{
		logMsg("audio already open");
		return OK;
	}
	pcmFormat = format;
	pa_sample_spec spec {(pa_sample_format_t)0};
	spec.format = pcmFormatToPA(format.sample);
	spec.rate = format.rate;
	spec.channels = format.channels;
	pa_proplist *props = pa_proplist_new();
	pa_proplist_sets(props, PA_PROP_MEDIA_ROLE, "game");
	lockMainLoop();
	stream = pa_stream_new_with_proplist(context, "Playback", &spec, nullptr, props);
	if(!stream)
	{
		logErr("error creating stream");
		pa_proplist_free(props);
		return INVALID_PARAMETER;
	}
	pa_proplist_free(props);
	pa_stream_state_t finalState = PA_STREAM_FAILED;
	pa_stream_set_state_callback(stream,
		[](pa_stream *stream, void *finalStateOut)
		{
			auto state = pa_stream_get_state(stream);
			switch(state)
			{
				case PA_STREAM_READY:
				case PA_STREAM_FAILED:
				case PA_STREAM_TERMINATED:
					*((pa_stream_state_t*)finalStateOut) = state;
					signalMainLoop();
				bdefault:
				break;
			}
		}, &finalState);
	pa_buffer_attr bufferAttr {0};
	bufferAttr.maxlength = -1;
	bufferAttr.tlength = format.uSecsToBytes(wantedLatency);
	bufferAttr.prebuf = -1;
	bufferAttr.minreq = -1;
	if(pa_stream_connect_playback(stream, nullptr, &bufferAttr,
		pa_stream_flags_t(PA_STREAM_ADJUST_LATENCY /*| PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_INTERPOLATE_TIMING*/),
		nullptr, nullptr) < 0)
	{
		logErr("error connecting playback stream");
		closePcm();
		return INVALID_PARAMETER;
	}
	waitMainLoop();
	pa_stream_set_state_callback(stream, nullptr, nullptr);
	if(finalState != PA_STREAM_READY)
	{
		logErr("error connecting playback stream async");
		closePcm();
		return INVALID_PARAMETER;
	}
	auto serverAttr = pa_stream_get_buffer_attr(stream);
	unlockMainLoop();
	assert(serverAttr);
	isCorked = false;
	logMsg("opened stream with target fill bytes: %d", serverAttr->tlength);
	return OK;
}

void closePcm()
{
	if(!isOpen())
	{
		logMsg("audio already closed");
		return;
	}
	lockMainLoop();
	pa_stream_disconnect(stream);
	pa_stream_unref(stream);
	unlockMainLoop();
	iterateMainLoop();
	isCorked = true;
	stream = nullptr;
}

bool isOpen()
{
	return stream;
}

bool isPlaying()
{
	if(!isOpen())
		return false;
	return !isCorked;
//	lockMainLoop();
//	bool isCorked = pa_stream_is_corked(stream);
//	unlockMainLoop();
//	return !isCorked;
}

CallResult init()
{
	#ifdef CONFIG_AUDIO_PULSEAUDIO_GLIB
	mainloop = pa_glib_mainloop_new(nullptr);
	context = pa_context_new(pa_glib_mainloop_get_api(mainloop), "Test");
	logMsg("init with GLIB main loop");
	#else
	mainloop = pa_threaded_mainloop_new();
	context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), "Test");
	logMsg("init with threaded main loop");
	#endif
	if(!context)
	{
		logErr("unable to create context");
		freeMainLoop();
		return IO_ERROR;
	}
	pa_context_state_t finalState = PA_CONTEXT_FAILED;
	pa_context_set_state_callback(context,
		[](pa_context *context, void *finalStateOut)
		{
			auto state = pa_context_get_state(context);
			switch(state)
			{
				case PA_CONTEXT_READY:
				case PA_CONTEXT_FAILED:
				case PA_CONTEXT_TERMINATED:
					*((pa_context_state_t*)finalStateOut) = state;
					signalMainLoop();
				bdefault:
				break;
			}
		}, &finalState);
	if(pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
	{
		logErr("unable to connect context");
		pa_context_unref(context);
		freeMainLoop();
		return IO_ERROR;
	}
	lockMainLoop();
	startMainLoop();
	waitMainLoop();
	pa_context_set_state_callback(context, nullptr, nullptr);
	if(finalState != PA_CONTEXT_READY)
	{
		logErr("context connection failed");
		pa_context_unref(context);
		unlockMainLoop();
		stopMainLoop();
		freeMainLoop();
		return IO_ERROR;
	}
	unlockMainLoop();
	return OK;
}

}
