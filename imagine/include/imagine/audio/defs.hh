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
#include <imagine/util/DelegateFunc.hh>

namespace IG::Audio
{
	namespace Config
	{
	#if (defined __linux__ && !defined CONFIG_MACHINE_PANDORA)
	#define CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	static constexpr bool MULTIPLE_SYSTEM_APIS = true;
	#else
	static constexpr bool MULTIPLE_SYSTEM_APIS = false;
	#endif
	}

enum class Api: uint8_t
{
	DEFAULT,
	ALSA,
	PULSEAUDIO,
	COREAUDIO,
	OPENSL_ES,
	AAUDIO,
};

#if defined __ANDROID__
constexpr std::array systemApis{Api::AAUDIO, Api::OPENSL_ES};
#elif defined __APPLE__
constexpr std::array systemApis{Api::COREAUDIO};
#else
	constexpr std::array systemApis{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO
	Api::PULSEAUDIO,
	#endif
	#ifdef CONFIG_PACKAGE_ALSA
	Api::ALSA,
	#endif
	};
#endif

struct ApiDesc
{
	const char *name = "";
	Api api{Api::DEFAULT};

	constexpr bool operator ==(Api api_) const { return api == api_; }
};

using OnSamplesNeededDelegate = DelegateFunc<bool(void *buff, size_t frames)>;

enum class StreamError
{
	Ok, BadArgument, NotInitialized
};

struct OutputStreamConfig;
class Manager;

}
