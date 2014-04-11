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

#define LOGTAG "ALSA"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>
#include <imagine/audio/Audio.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include "alsautils.h"

namespace Audio
{

PcmFormat preferredPcmFormat {::Config::MACHINE_IS_PANDORA ? 44100 : 48000, SampleFormats::s16, 2};
PcmFormat pcmFormat;
static snd_output_t *debugOutput = nullptr;
static snd_pcm_t *pcmHnd = nullptr;
static snd_pcm_uframes_t bufferSize, periodSize;
static bool useMmap;
static uint wantedLatency = 100000;

int maxRate()
{
	return ::Config::MACHINE_IS_PANDORA ? 44100 : 48000;
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

static const SampleFormat &alsaFormatToPcm(snd_pcm_format_t format)
{
	switch(format)
	{
		case SND_PCM_FORMAT_S16: return SampleFormats::s16;
		case SND_PCM_FORMAT_S8: return SampleFormats::s8;
		case SND_PCM_FORMAT_U8: return SampleFormats::u8;
		default:
			bug_branch("%d", format);
			return SampleFormats::none;
	}
}

static snd_pcm_format_t pcmFormatToAlsa(const SampleFormat &format)
{
	switch(format.toBits())
	{
		case 16 : return SND_PCM_FORMAT_S16;
		case 8 : return format.isSigned ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8;
		default:
			bug_branch("%d", format.toBits());
			return (snd_pcm_format_t)0;
	}
}

int frameDelay()
{
	if(unlikely(!isOpen()))
		return 0;
	snd_pcm_sframes_t delay;
	snd_pcm_delay(pcmHnd, &delay);
	return delay;
}

int framesFree()
{
	if(unlikely(!isOpen()))
		return 0;
	auto frames = snd_pcm_avail_update(pcmHnd);
	if(frames < 0)
	{
		logWarn("error %d getting frames free", (int)frames);
		frames = 0;
	}
	return frames;
}

void pausePcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("pausing playback");
	snd_pcm_pause(pcmHnd, 1);
}

void resumePcm()
{
	if(unlikely(!isOpen()))
		return;
	int state = snd_pcm_state(pcmHnd);
	//logMsg("pcm state: %s", alsaPcmStateToString(state));
	switch(state)
	{
		bcase SND_PCM_STATE_PREPARED:
			logMsg("starting PCM");
			snd_pcm_start(pcmHnd);
		bcase SND_PCM_STATE_PAUSED:
			logMsg("unpausing PCM");
			snd_pcm_pause(pcmHnd, 0);
		bcase SND_PCM_STATE_SUSPENDED:
			logMsg("resuming PCM");
			snd_pcm_resume(pcmHnd);
	}
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	snd_pcm_drop(pcmHnd);
	snd_pcm_prepare(pcmHnd);
}

class AlsaMmapContext : public BufferContext
{
public:
	const snd_pcm_channel_area_t *areas = nullptr;
	snd_pcm_uframes_t offset = 0;
	snd_pcm_t *pcmHnd = nullptr;

	constexpr AlsaMmapContext() {}

	CallResult begin(snd_pcm_t *pcmHnd, snd_pcm_uframes_t *frames)
	{
		var_selfs(pcmHnd);
		snd_pcm_uframes_t wantedFrames = *frames;
		if(snd_pcm_mmap_begin(pcmHnd, &areas, &offset, frames) != 0)
		{
			logErr("error in snd_pcm_mmap_begin");
			return INVALID_PARAMETER;
		}
		if(*frames == 0)
		{
			//logWarn("unexpected 0 samples returned for MMAP");
			snd_pcm_mmap_commit(pcmHnd, offset, 0);
			return INVALID_PARAMETER;
		}

		if(wantedFrames != *frames)
		{
			//logMsg("got %d frames out of %d from mmap", (int)*frames, (int)wantedFrames);
		}
		data = (char*)areas->addr + offset * (areas->step / 8);
		this->frames = *frames;

		return OK;
	}

	CallResult commitBytes(uint bytesWritten)
	{
		return commit(pcmFormat.bytesToFrames(bytesWritten));
	}

	CallResult commit(snd_pcm_uframes_t framesWritten)
	{
		auto ret = snd_pcm_mmap_commit(pcmHnd, offset, framesWritten);
		if(ret < 0 || ret != (snd_pcm_sframes_t)framesWritten)
		{
			logMsg("error in snd_pcm_mmap_commit");
			return INVALID_PARAMETER;
		}
		return OK;
	}
};

void writePcm(const void *samples, uint framesToWrite)
{
	if(unlikely(!isOpen()))
		return;

	auto framesFreeOnHW = framesFree();

	// verify PCM state
	switch((int)snd_pcm_state(pcmHnd))
	{
		bcase SND_PCM_STATE_XRUN:
			snd_pcm_recover(pcmHnd, -EPIPE, 0);
			framesFreeOnHW = framesFree();
			logMsg("recovered from xrun, %d frames free", framesFreeOnHW);
		bcase SND_PCM_STATE_PAUSED:
			logMsg("unpausing PCM");
			snd_pcm_pause(pcmHnd, 0);
		bcase SND_PCM_STATE_SUSPENDED:
			logMsg("resuming PCM");
			snd_pcm_resume(pcmHnd);
	}

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

	if(framesFreeOnHW < (int)framesToWrite)
	{
		logWarn("sending %d frames but only %d free", framesToWrite, framesFreeOnHW);
		framesToWrite = framesFreeOnHW;
	}
	snd_pcm_sframes_t written;
	if(useMmap)
	{
		written = snd_pcm_mmap_writei(pcmHnd, samples, framesToWrite);
		/*AlsaMmapContext ctx;
		//logMsg("starting mmap loop");
		auto samplePtr = (char*)samples;
		for(snd_pcm_uframes_t frames; framesToWrite; framesToWrite -= frames)
		{
			frames = framesToWrite;
			if(ctx.begin(pcmHnd, &frames) != OK)
				return;
			assert(frames <= framesToWrite);

			memcpy(ctx.data, samples, pcmFormat.framesToBytes(frames));
			samplePtr += pcmFormat.framesToBytes(frames);

			if(ctx.commit(frames) != OK)
				return;
		}*/
	}
	else
	{
		written = snd_pcm_writei(pcmHnd, samples, framesToWrite);
	}
	if(written != (snd_pcm_sframes_t)framesToWrite)
	{
		if(written < 0)
		{
			logWarn("error writing %d frames: %s", framesToWrite, alsaPcmWriteErrorToString(written));
		}
		else
			logWarn("only %ld of %d frames written", written, framesToWrite);
	}
}

static int setupPcm(const PcmFormat &format, snd_pcm_access_t access)
{
	int alsalibResample = 1;
	int err;
	if ((err = snd_pcm_set_params(pcmHnd,
		pcmFormatToAlsa(format.sample),
		access,
		format.channels,
		format.rate,
		alsalibResample,
		wantedLatency)) < 0)
	{
		logErr("Error setting pcm parameters: %s", snd_strerror(err));
		return err;
	}

	if(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
		useMmap = true;
	else
		useMmap = false;

	if((err = snd_pcm_get_params(pcmHnd, &bufferSize, &periodSize) < 0))
	{
		logErr("Error getting pcm buffer/period size parameters");
		return err;
	}
	else
	{
		logMsg("buffer size %u, period size %u, mmap %d", (uint)bufferSize, (uint)periodSize, useMmap);
		return 0;
	}
}

static CallResult openAlsaPcm(const PcmFormat &format)
{
	int ret;
	const char* name = "default";

	logMsg("Opening playback device: %s", name);
	
	int err;
	if ((err = snd_pcm_open(&pcmHnd, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0)
	{
		logErr("Playback open error: %s", snd_strerror(err));
		return INVALID_PARAMETER;
	}

	logMsg("Stream parameters: %iHz, %s, %i channels", format.rate, snd_pcm_format_name(pcmFormatToAlsa(format.sample)), format.channels);

	bool allowMmap = 1;
	bool setupPcmSuccess = 0;
	if(allowMmap)
	{
		if((err = setupPcm(format, SND_PCM_ACCESS_MMAP_INTERLEAVED)) < 0)
		{
			logErr("failed opening in MMAP mode");
		}
		else
			setupPcmSuccess = 1;
	}
	if(!setupPcmSuccess && (err = setupPcm(format, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
	{
		logErr("failed opening in normal mode");
	}
	else
		setupPcmSuccess = 1;
	if(!setupPcmSuccess)
	{
		ret = INVALID_PARAMETER; goto CLEANUP;
	}

	//snd_pcm_dump(alsaHnd, output);
	//logMsg("pcm state: %s", alsaPcmStateToString(snd_pcm_state(pcmHnd)));

	return OK;

	CLEANUP:

	if(pcmHnd)
	{
		snd_pcm_close(pcmHnd);
		pcmHnd = 0;
	}
	
	return ret;
}

static void closeAlsaPcm()
{
	if(isOpen())
	{
		logDMsg("closing pcm");
		snd_pcm_close(pcmHnd);
		pcmHnd = nullptr;
	}
}

CallResult openPcm(const PcmFormat &format)
{
	if(isOpen())
	{
		logMsg("audio already open");
		return OK;
	}
	pcmFormat = format;
	return openAlsaPcm(format);
}

void closePcm()
{
	if(!isOpen())
	{
		logMsg("audio already closed");
		return;
	}
	closeAlsaPcm();
}

bool isOpen()
{
	return pcmHnd;
}

bool isPlaying()
{
	return isOpen() && snd_pcm_state(pcmHnd) == SND_PCM_STATE_RUNNING;
}

CallResult init()
{
	/*int err = snd_output_stdio_attach(&debugOutput, stdout, 0);
	if(err < 0)
	{
		logWarn("Output failed: %s", snd_strerror(err));
		return INVALID_PARAMETER;
	}*/

	return OK;
}

}
