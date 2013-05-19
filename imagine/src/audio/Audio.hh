#pragma once

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

#include <engine-globals.h>
#include <util/audio/PcmFormat.hh>

#if defined(CONFIG_AUDIO_ALSA)
	#include <audio/alsa/config.hh>
#else
	#include <audio/config.hh>
#endif

namespace Audio
{

	namespace Config
	{
	#if defined CONFIG_AUDIO_OPENSL_ES || defined CONFIG_AUDIO_COREAUDIO || \
		defined CONFIG_AUDIO_SDL || defined CONFIG_AUDIO_ALSA
		#define CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	#endif

	#if defined CONFIG_AUDIO_OPENSL_ES || defined CONFIG_AUDIO_COREAUDIO
		#define CONFIG_AUDIO_SOLO_MIX
	#endif
	}

struct BufferContext
{
	constexpr BufferContext() { }
	void *data = nullptr;
	uframes frames = 0;
};

static const PcmFormat maxFormat { maxRate, &SampleFormats::s16, 2 };

#if !defined(CONFIG_AUDIO)

static PcmFormat preferredPcmFormat = maxFormat;
static PcmFormat pcmFormat = maxFormat;

static CallResult init() { return OK; }
static CallResult openPcm(const PcmFormat &format) { return UNSUPPORTED_OPERATION; }
static void closePcm() {}
static bool isOpen(){ return 0; }
static void writePcm(uchar *samples, uint framesToWrite) {}
static BufferContext *getPlayBuffer(uint wantedFrames) { return 0; }
static void commitPlayBuffer(BufferContext *buffer, uint frames) {}
static int frameDelay() { return 0; }
static int framesFree() { return 0; }
static void setHintPcmFramesPerWrite(uint frames) { }
static void setHintPcmMaxBuffers(uint buffers) { }
static uint hintPcmMaxBuffers() { return 0; }

#else

extern PcmFormat preferredPcmFormat;
extern PcmFormat pcmFormat; // the currently playing format

CallResult init() ATTRS(cold);
CallResult openPcm(const PcmFormat &format);
void closePcm();
void pausePcm();
void resumePcm();
void clearPcm();
bool isOpen();
void writePcm(uchar *samples, uint framesToWrite);
BufferContext *getPlayBuffer(uint wantedFrames);
void commitPlayBuffer(BufferContext *buffer, uint frames);
int frameDelay();
int framesFree();
void setHintPcmFramesPerWrite(uint frames);
void setHintPcmMaxBuffers(uint buffers);
uint hintPcmMaxBuffers();
void setHintStrictUnderrunCheck(bool on);
bool hintStrictUnderrunCheck();

#ifdef CONFIG_AUDIO_SOLO_MIX
void setSoloMix(bool newSoloMix);
bool soloMix();
#else
static void setSoloMix(bool newSoloMix) { }
static bool soloMix() { return 0; }
#endif

#endif

// shortcuts
static PcmFormat &pPCM = preferredPcmFormat;

static CallResult openPcm(int rate) { return openPcm({ rate, pPCM.sample, pPCM.channels }); }
static CallResult openPcm(int rate, int channels) { return openPcm({ rate, pPCM.sample, channels }); }
static CallResult openPcm() { return openPcm(pPCM); }
static bool supportsRateNative(int rate) { return rate <= pPCM.rate; }
}
