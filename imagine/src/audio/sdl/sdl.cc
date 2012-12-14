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

#define thisModuleName "audio:sdl"
#include <engine-globals.h>
#include <audio/Audio.hh>
#include <logger/interface.h>
#include <SDL.h>
#include <util/RingBuffer.hh>

namespace Audio
{

PcmFormat preferredPcmFormat = { 44100, &SampleFormats::s16, 2 };
static PcmFormat pcmFmt;
static uint bufferFrames = 800;
static uint buffers = 8;
static int startPlaybackBytes = 0;
static uchar *localBuff = nullptr;
static RingBuffer<int> rBuff;
static bool isPlaying = 0;

static void audioCallback(void *userdata, Uint8 *buf, int bytes)
{
	int read;
	if((read = rBuff.read(buf, bytes)) != bytes)
	{
		//logMsg("underrun, read %d out of %d bytes", read, bytes);
	}

	static int debugCount = 0;
	if(countToValueLooped(debugCount, 120))
	{
		//logMsg("%d bytes in buffer", rBuff.written);
	}
}

void setHintPcmFramesPerWrite(uint frames)
{
	logMsg("setting buffer frames to %d", frames);
	assert(frames < 2000);
	bufferFrames = frames;
	assert(!isOpen());
}

void setHintPcmMaxBuffers(uint maxBuffers)
{
	logMsg("setting max buffers to %d", maxBuffers);
	assert(maxBuffers < 100);
	buffers = maxBuffers;
	assert(!isOpen());
}

uint hintPcmMaxBuffers() { return buffers; }

static void startPcm()
{
	SDL_PauseAudio(0);
	isPlaying = 1;
}

/*void pausePcm()
{
	SDL_PauseAudio(1);
	isPlaying = 0;
}*/

CallResult openPcm(const PcmFormat &format)
{
	SDL_AudioSpec spec;
	mem_zero(spec);
	spec.freq = format.rate;
	spec.format = (format.sample->bits == 16) ? AUDIO_S16SYS : AUDIO_U8;
	spec.channels = format.channels;
	spec.samples = 1024;
	spec.callback = audioCallback;
	//spec.userdata = 0;
	uint bufferSize = format.framesToBytes(bufferFrames) * buffers;
	startPlaybackBytes = format.framesToBytes(bufferFrames) * buffers-1;
	localBuff = (uchar*)mem_alloc(bufferSize);
	if(!localBuff)
	{
		logMsg("error allocation audio buffer");
		return OUT_OF_MEMORY;
	}
	logMsg("allocated %d for audio buffer", bufferSize);
	rBuff.init(localBuff, bufferSize);
	if(SDL_OpenAudio(&spec, 0) < 0)
	{
		logErr("error in SDL_OpenAudio");
		return INVALID_PARAMETER;
	}
	pcmFmt = format;
	logMsg("opened audio %dHz with buffer %d samples %d size", spec.freq, spec.samples, spec.size);
	return OK;
}

void writePcm(uchar *buffer, uint framesToWrite)
{
	assert(isOpen());
	int bytes = pcmFmt.framesToBytes(framesToWrite), written;
	SDL_LockAudio();
	if((written = rBuff.write(buffer, bytes)) != bytes)
	{
		//logMsg("overrun, wrote %d out of %d bytes", written, bytes);
	}
	SDL_UnlockAudio();
	if(!isPlaying && rBuff.written >= startPlaybackBytes)
	{
		startPcm();
	}
}

void closePcm()
{
	if(isOpen()/*SDL_GetAudioStatus() != SDL_AUDIO_STOPPED*/)
	{
		isPlaying = 0;
		SDL_CloseAudio();
		rBuff.reset();
		mem_free(localBuff);
		localBuff = nullptr;
		mem_zero(pcmFmt);
	}
}

bool isOpen()
{
	return pcmFmt.rate != 0;//SDL_GetAudioStatus() == SDL_AUDIO_PLAYING;
}

int frameDelay()
{
	return 0; // TODO
}

int framesFree()
{
	return pcmFmt.bytesToFrames(rBuff.freeSpace());
}

CallResult init()
{
	#ifndef CONFIG_BASE_SDL
		// Init SDL here if not using base SDL module
		SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_AUDIO);
	#endif
	return OK;
}

}
