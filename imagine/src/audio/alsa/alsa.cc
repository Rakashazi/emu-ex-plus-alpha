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

#include <imagine/audio/alsa/ALSAOutputStream.hh>
#include <imagine/audio/OutputStream.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/thread/Thread.hh>
#include "alsautils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace IG::Audio
{

constexpr SystemLogger log{"ALSA"};

constexpr const SampleFormat& alsaFormatToPcm(snd_pcm_format_t format)
{
	switch(format)
	{
		case SND_PCM_FORMAT_FLOAT: return SampleFormats::f32;
		case SND_PCM_FORMAT_S32: return SampleFormats::i32;
		case SND_PCM_FORMAT_S16: return SampleFormats::i16;
		case SND_PCM_FORMAT_U8: return SampleFormats::i8;
		default:
			bug_unreachable("format == %d", format);
	}
}

constexpr snd_pcm_format_t pcmFormatToAlsa(const SampleFormat& format)
{
	switch(format.bytes())
	{
		case 4 : return format.isFloat() ? SND_PCM_FORMAT_FLOAT : SND_PCM_FORMAT_S32;
		case 2 : return SND_PCM_FORMAT_S16;
		case 1 : return SND_PCM_FORMAT_U8;
		default:
			bug_unreachable("bytes == %d", format.bytes());
	}
}

static bool recoverPCM(snd_pcm_t *handle)
{
	int state = snd_pcm_state(handle);
	//log.info("state:{}", state);
	switch(state)
	{
		case SND_PCM_STATE_XRUN:
			log.info("recovering from xrun");
			snd_pcm_recover(handle, -EPIPE, 0);
			return true;
		case SND_PCM_STATE_SUSPENDED:
			log.info("resuming PCM");
			snd_pcm_resume(handle);
			return true;
		case SND_PCM_STATE_PREPARED:
		case SND_PCM_STATE_SETUP:
			return true;
	}
	return false;
}

ALSAOutputStream::~ALSAOutputStream()
{
	close();
}

StreamError ALSAOutputStream::open(OutputStreamConfig config)
{
	if(isOpen())
	{
		log.info("already open");
		return {};
	}
	auto format = config.format;
	pcmFormat = format;
	onSamplesNeeded = config.onSamplesNeeded;
	const char* name = "default";
	log.info("Opening playback device:{}", name);
	if(int err = snd_pcm_open(&pcmHnd, name, SND_PCM_STREAM_PLAYBACK, 0);
		err < 0)
	{
		log.error("Playback open error: {}", snd_strerror(err));
		return StreamError::BadArgument;
	}
	auto closePcm = IG::scopeGuard([this](){ snd_pcm_close(pcmHnd); pcmHnd = {}; });
	log.info("Stream parameters: {}Hz, {}, {} channels", format.rate, snd_pcm_format_name(pcmFormatToAlsa(format.sample)), format.channels);
	bool allowMmap = true;
	int err = -1;
	auto wantedLatency = config.wantedLatencyHint.count() ? config.wantedLatencyHint : IG::Microseconds{10000};
	if(allowMmap)
	{
		err = setupPcm(format, SND_PCM_ACCESS_MMAP_INTERLEAVED, wantedLatency);
		if(err < 0)
		{
			log.error("failed opening in MMAP mode");
		}
	}
	if(err < 0)
	{
		err = setupPcm(format, SND_PCM_ACCESS_RW_INTERLEAVED, wantedLatency);
		if(err < 0)
		{
			log.error("failed opening in normal mode");
		}
	}
	//snd_pcm_dump(alsaHnd, output);
	//log.info("pcm state: {}", alsaPcmStateToString(snd_pcm_state(pcmHnd)));
	if(err < 0)
	{
		return StreamError::BadArgument;
	}
	closePcm.cancel();
	quitFlag = false;
	eventThread = std::thread{
		[this]()
		{
			int count = snd_pcm_poll_descriptors_count(pcmHnd);
			auto ufds = std::make_unique<struct pollfd[]>(count);
			snd_pcm_poll_descriptors(pcmHnd, ufds.get(), count);
			auto waitForEvent =
				[](snd_pcm_t *handle, struct pollfd *ufds, unsigned int count, std::atomic_bool &quitFlag)
				{
					while(true)
					{
						poll(ufds, count, -1);
						if(quitFlag)
							return -ENODEV;
						unsigned short revents = 0;
						//log.info("waiting for events");
						snd_pcm_poll_descriptors_revents(handle, ufds, count, &revents);
						if(revents & POLLERR)
						{
							log.info("got POLLERR");
							return -EIO;
						}
						if(revents & POLLOUT)
						{
							//log.info("got POLLOUT");
							return 0;
						}
						log.info("got other events:{:#X}", revents);
					}
				};
			while(true)
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
						log.error("couldn't recover PCM");
						return;
					}
					continue;
				}
				//log.info("state:{}", snd_pcm_state(pcmHnd));
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
							log.error("error in snd_pcm_mmap_begin");
							if(!recoverPCM(pcmHnd))
							{
								log.error("couldn't recover PCM");
								return;
							}
							break;
						}
						auto buff = (char*)areas->addr + offset * (areas->step / 8);
						onSamplesNeeded(buff, frames);
						if(snd_pcm_mmap_commit(pcmHnd, offset, frames) < 0)
						{
							log.error("error in snd_pcm_mmap_begin");
							if(!recoverPCM(pcmHnd))
							{
								log.error("couldn't recover PCM");
								return;
							}
							break;
						}
						//log.info("wrote {} frames with mmap", frames);
						framesToWrite -= frames;
					}
				}
				else
				{
					auto bytes = pcmFormat.framesToBytes(periodSize);
					alignas(4) char buff[bytes];
					onSamplesNeeded(&buff[0], periodSize);
					if(snd_pcm_writei(pcmHnd, buff, periodSize) < 0)
					{
						if(!recoverPCM(pcmHnd))
						{
							log.error("couldn't recover PCM");
							return;
						}
					}
					//log.info("wrote {} frames", periodSize);
				}
			}
		}};
	if(config.startPlaying)
		play();
	return {};
}

void ALSAOutputStream::play()
{
	if(!isOpen()) [[unlikely]]
		return;
	//log.info("pcm state: {}", alsaPcmStateToString(state));
	auto playFromState = [](snd_pcm_t *pcmHnd, int state)
		{
			switch(state)
			{
				case SND_PCM_STATE_PREPARED:
					log.info("starting PCM");
					return snd_pcm_start(pcmHnd);
				case SND_PCM_STATE_PAUSED:
					log.info("unpausing PCM");
					return snd_pcm_pause(pcmHnd, 0);
				case SND_PCM_STATE_SUSPENDED:
					log.info("resuming PCM");
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
	log.info("pausing playback");
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
	eventThread.join();
	pcmHnd = nullptr;
}

void ALSAOutputStream::flush()
{
	if(!isOpen()) [[unlikely]]
		return;
	log.info("clearing queued samples");
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
		log.error("Error setting pcm parameters:{}", snd_strerror(err));
		return err;
	}

	if(access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
		useMmap = true;
	else
		useMmap = false;

	if(int err = snd_pcm_get_params(pcmHnd, &bufferSize, &periodSize);
		err < 0)
	{
		log.error("Error getting pcm buffer/period size parameters");
		return err;
	}
	else
	{
		log.info("buffer size {}, period size {}, mmap {}", bufferSize, periodSize, useMmap);
		return 0;
	}
}

}
