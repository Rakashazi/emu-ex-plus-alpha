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
#include <memory>
#include <imagine/audio/alsa/ALSAOutputStream.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/thread/Thread.hh>
#include "alsautils.h"

namespace IG::Audio
{

static const SampleFormat &alsaFormatToPcm(snd_pcm_format_t format)
{
	switch(format)
	{
		case SND_PCM_FORMAT_FLOAT: return SampleFormats::f32;
		case SND_PCM_FORMAT_S32: return SampleFormats::i32;
		case SND_PCM_FORMAT_S16: return SampleFormats::i16;
		case SND_PCM_FORMAT_U8: return SampleFormats::i8;
		default:
			bug_unreachable("format == %d", format);
			return SampleFormats::none;
	}
}

static snd_pcm_format_t pcmFormatToAlsa(const SampleFormat &format)
{
	switch(format.bytes())
	{
		case 4 : return format.isFloat() ? SND_PCM_FORMAT_FLOAT : SND_PCM_FORMAT_S32;
		case 2 : return SND_PCM_FORMAT_S16;
		case 1 : return SND_PCM_FORMAT_U8;
		default:
			bug_unreachable("bytes == %d", format.bytes());
			return (snd_pcm_format_t)0;
	}
}

static bool recoverPCM(snd_pcm_t *handle)
{
	int state = snd_pcm_state(handle);
	//logMsg("state:%d", state);
	switch(state)
	{
		case SND_PCM_STATE_XRUN:
			logMsg("recovering from xrun");
			snd_pcm_recover(handle, -EPIPE, 0);
			return true;
		case SND_PCM_STATE_SUSPENDED:
			logMsg("resuming PCM");
			snd_pcm_resume(handle);
			return true;
		case SND_PCM_STATE_PREPARED:
			return true;
		case SND_PCM_STATE_SETUP:
			return true;
	}
	return false;
}


ALSAOutputStream::ALSAOutputStream() {}

ALSAOutputStream::~ALSAOutputStream()
{
	close();
}

IG::ErrorCode ALSAOutputStream::open(OutputStreamConfig config)
{
	if(isOpen())
	{
		logMsg("already open");
		return {};
	}
	auto format = config.format();;
	pcmFormat = format;
	onSamplesNeeded = config.onSamplesNeeded();
	const char* name = "default";
	logMsg("Opening playback device: %s", name);
	if(int err = snd_pcm_open(&pcmHnd, name, SND_PCM_STREAM_PLAYBACK, 0);
		err < 0)
	{
		logErr("Playback open error: %s", snd_strerror(err));
		return {EINVAL};
	}
	auto closePcm = IG::scopeGuard([this](){ snd_pcm_close(pcmHnd); pcmHnd = 0; });
	logMsg("Stream parameters: %iHz, %s, %i channels", format.rate, snd_pcm_format_name(pcmFormatToAlsa(format.sample)), format.channels);
	bool allowMmap = 1;
	int err = -1;
	auto wantedLatency = config.wantedLatencyHint().count() ? config.wantedLatencyHint() : IG::Microseconds{10000};
	if(allowMmap)
	{
		err = setupPcm(format, SND_PCM_ACCESS_MMAP_INTERLEAVED, wantedLatency);
		if(err < 0)
		{
			logErr("failed opening in MMAP mode");
		}
	}
	if(err < 0)
	{
		err = setupPcm(format, SND_PCM_ACCESS_RW_INTERLEAVED, wantedLatency);
		if(err < 0)
		{
			logErr("failed opening in normal mode");
		}
	}
	//snd_pcm_dump(alsaHnd, output);
	//logMsg("pcm state: %s", alsaPcmStateToString(snd_pcm_state(pcmHnd)));
	if(err < 0)
	{
		return {EINVAL};
	}
	closePcm.cancel();
	quitFlag = false;
	IG::makeDetachedThread(
		[this]()
		{
			int count = snd_pcm_poll_descriptors_count(pcmHnd);
			auto ufds = std::make_unique<struct pollfd[]>(count);
			snd_pcm_poll_descriptors(pcmHnd, ufds.get(), count);
			auto waitForEvent =
				[](snd_pcm_t *handle, struct pollfd *ufds, unsigned int count, std::atomic_bool &quitFlag)
				{
					while(1)
					{
						poll(ufds, count, -1);
						if(quitFlag)
							return -ENODEV;
						unsigned short revents = 0;
						//logMsg("waiting for events");
						snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);
						if(revents & POLLERR)
						{
							logMsg("got POLLERR");
							return -EIO;
						}
						if(revents & POLLOUT)
						{
							//logMsg("got POLLOUT");
							return 0;
						}
						logMsg("got other events:0x%X", revents);
					}
				};
			while(1)
			{
				if(int err = waitForEvent(pcmHnd, ufds.get(), count, quitFlag);
					err < 0)
				{
					if(err == -ENODEV)
					{
						return;
					}
					if(!recoverPCM(pcmHnd))
					{
						logErr("couldn't recover PCM");
						return;
					}
					continue;
				}
				//logMsg("state:%d", snd_pcm_state(pcmHnd));
				if(useMmap)
				{
					snd_pcm_avail_update(pcmHnd);
					auto framesToWrite = periodSize;
					while(framesToWrite)
					{
						const snd_pcm_channel_area_t *areas{};
						snd_pcm_uframes_t offset = 0;
						snd_pcm_uframes_t frames = framesToWrite;
						if(snd_pcm_mmap_begin(pcmHnd, &areas, &offset, &frames) < 0)
						{
							logErr("error in snd_pcm_mmap_begin");
							if(!recoverPCM(pcmHnd))
							{
								logErr("couldn't recover PCM");
								return;
							}
							break;
						}
						auto buff = (char*)areas->addr + offset * (areas->step / 8);
						onSamplesNeeded(buff, frames);
						if(snd_pcm_mmap_commit(pcmHnd, offset, frames) < 0)
						{
							logErr("error in snd_pcm_mmap_begin");
							if(!recoverPCM(pcmHnd))
							{
								logErr("couldn't recover PCM");
								return;
							}
							break;
						}
						//logMsg("wrote %d frames with mmap", (int)frames);
						framesToWrite -= frames;
					}
				}
				else
				{
					auto bytes = pcmFormat.framesToBytes(periodSize);
					alignas(4) char buff[bytes];
					onSamplesNeeded(buff, periodSize);
					if(snd_pcm_writei(pcmHnd, buff, periodSize) < 0)
					{
						if(!recoverPCM(pcmHnd))
						{
							logErr("couldn't recover PCM");
							return;
						}
					}
					//logMsg("wrote %d frames", (int)periodSize);
				}
			}
		});
	if(config.startPlaying())
		play();
	return {};
}

void ALSAOutputStream::play()
{
	if(!isOpen()) [[unlikely]]
		return;
	//logMsg("pcm state: %s", alsaPcmStateToString(state));
	auto playFromState = [](snd_pcm_t *pcmHnd, int state)
		{
			switch(state)
			{
				case SND_PCM_STATE_PREPARED:
					logMsg("starting PCM");
					return snd_pcm_start(pcmHnd);
				case SND_PCM_STATE_PAUSED:
					logMsg("unpausing PCM");
					return snd_pcm_pause(pcmHnd, 0);
				case SND_PCM_STATE_SUSPENDED:
					logMsg("resuming PCM");
					return snd_pcm_resume(pcmHnd);
			}
			return 0;
		};
	playFromState(pcmHnd, snd_pcm_state(pcmHnd));
}

void ALSAOutputStream::pause()
{
	if(!isOpen()) [[unlikely]]
		return;
	logMsg("pausing playback");
	snd_pcm_pause(pcmHnd, 1);
}

void ALSAOutputStream::close()
{
	if(!isOpen()) [[unlikely]]
		return;
	logDMsg("closing pcm");
	quitFlag = true;
	snd_pcm_drop(pcmHnd);
	snd_pcm_close(pcmHnd);
	pcmHnd = nullptr;
}

void ALSAOutputStream::flush()
{
	if(!isOpen()) [[unlikely]]
		return;
	logMsg("clearing queued samples");
	snd_pcm_drop(pcmHnd);
	snd_pcm_prepare(pcmHnd);
	snd_pcm_start(pcmHnd);
	snd_pcm_pause(pcmHnd, 1);
}

bool ALSAOutputStream::isOpen()
{
	return pcmHnd;
}

bool ALSAOutputStream::isPlaying()
{
	return isOpen() && snd_pcm_state(pcmHnd) == SND_PCM_STATE_RUNNING;
}

ALSAOutputStream::operator bool() const
{
	return true;
}

int ALSAOutputStream::setupPcm(Format format, snd_pcm_access_t access, IG::Microseconds wantedLatency)
{
	int alsalibResample = 1;
	if(int err = snd_pcm_set_params(pcmHnd,
		pcmFormatToAlsa(format.sample),
		access,
		format.channels,
		format.rate,
		alsalibResample,
		wantedLatency.count());
		err < 0)
	{
		logErr("Error setting pcm parameters: %s", snd_strerror(err));
		return err;
	}

	if(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
		useMmap = true;
	else
		useMmap = false;

	if(int err = snd_pcm_get_params(pcmHnd, &bufferSize, &periodSize);
		err < 0)
	{
		logErr("Error getting pcm buffer/period size parameters");
		return err;
	}
	else
	{
		logMsg("buffer size %u, period size %u, mmap %d", (uint32_t)bufferSize, (uint32_t)periodSize, useMmap);
		return 0;
	}
}

}
