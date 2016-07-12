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

#include <imagine/config/defs.hh>
#include <imagine/audio/AudioManager.hh>
#include <imagine/util/audio/PcmFormat.hh>
#include <system_error>

#if defined CONFIG_AUDIO_ALSA
#include <imagine/audio/alsa/config.hh>
#else
#include <imagine/audio/config.hh>
#endif

namespace Audio
{
	namespace Config
	{
	#define CONFIG_AUDIO_LATENCY_HINT
	}

struct BufferContext
{
	void *data{};
	uframes frames = 0;

	constexpr BufferContext() {}
	constexpr BufferContext(void *data, uframes frames): data{data}, frames{frames} {}

	explicit operator bool() const
	{
		return data;
	}
};

extern PcmFormat pcmFormat; // the currently playing format

std::error_code openPcm(const PcmFormat &format);
void closePcm();
void pausePcm();
void resumePcm();
void clearPcm();
bool isOpen();
bool isPlaying();
void writePcm(const void *samples, uint framesToWrite);
BufferContext getPlayBuffer(uint wantedFrames);
void commitPlayBuffer(BufferContext buffer, uint frames);
int frameDelay();
int framesFree();
void setHintOutputLatency(uint us);
uint hintOutputLatency();
void setHintStrictUnderrunCheck(bool on);
bool hintStrictUnderrunCheck();
int maxRate();
}
