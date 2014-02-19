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

#define LOGTAG "AudioSDL"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <SDL.h>
#include <util/ringbuffer/RingBuffer.hh>

namespace Audio
{

PcmFormat preferredPcmFormat = { 44100, SampleFormats::s16, 2 };
PcmFormat pcmFormat;
static bool isInit_ = false;
static uint wantedLatency = 100000;
static StaticRingBuffer<int> rBuff;

bool isPlaying()
{
	return SDL_GetAudioStatus() == SDL_AUDIO_PLAYING;
}

static bool isInit()
{
	return isInit_;
}

int maxRate()
{
	return 48000;
}

void setHintOutputLatency(uint us)
{
	wantedLatency = us;
}

uint hintOutputLatency()
{
	return wantedLatency;
}

static void audioCallback(void *userdata, Uint8 *buf, int bytes)
{
	int read;
	if((read = rBuff.read(buf, bytes)) != bytes)
	{
		logMsg("underrun, read %d out of %d bytes", read, bytes);
		uint padBytes = bytes - read;
		//logMsg("padding %d bytes", padBytes);
		mem_zero(&buf[read], padBytes);
		pausePcm();
	}

//	static int debugCount = 0;
//	if(countToValueLooped(debugCount, 120))
//	{
//		//logMsg("%d bytes in buffer", rBuff.written);
//	}
}

void resumePcm()
{
	SDL_PauseAudio(0);
}

void pausePcm()
{
	SDL_PauseAudio(1);
}

CallResult openPcm(const PcmFormat &format)
{
	SDL_AudioSpec spec{0};
	spec.freq = format.rate;
	spec.format = (format.sample.bits == 16) ? AUDIO_S16SYS : AUDIO_U8;
	spec.channels = format.channels;
	spec.samples = 1024;
	spec.callback = audioCallback;
	//spec.userdata = 0;
	rBuff.init(format.uSecsToBytes(wantedLatency));
	logMsg("allocated %d bytes for audio buffer", rBuff.freeSpace());
	if(SDL_OpenAudio(&spec, 0) < 0)
	{
		logErr("error in SDL_OpenAudio");
		return INVALID_PARAMETER;
	}
	pcmFormat = format;
	logMsg("opened audio %dHz with buffer %d samples %d size", spec.freq, spec.samples, spec.size);
	return OK;
}

void clearPcm()
{
	if(unlikely(!isOpen()))
		return;
	logMsg("clearing queued samples");
	pausePcm();
	rBuff.reset();
}

void writePcm(const void *buffer, uint framesToWrite)
{
	assert(isOpen());
	int bytes = pcmFormat.framesToBytes(framesToWrite), written;
	if((written = rBuff.write(buffer, bytes)) != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
}

void closePcm()
{
	if(!isOpen())
		return;
	SDL_CloseAudio();
	rBuff.deinit();
	pcmFormat = {};
}

bool isOpen()
{
	return pcmFormat.rate;
}

int frameDelay()
{
	return 0; // TODO
}

int framesFree()
{
	return pcmFormat.bytesToFrames(rBuff.freeSpace());
}

CallResult init()
{
	#ifndef CONFIG_BASE_SDL
	// Init SDL here if not using base SDL module
	SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_AUDIO);
	#endif
	isInit_ = true;
	return OK;
}

}
