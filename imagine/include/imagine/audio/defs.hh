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
#include <vector>

namespace IG::Audio
{
	namespace Config
	{
	#if (defined __linux__ && !defined CONFIG_MACHINE_PANDORA && !defined __ANDROID__)
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
};

struct ApiDesc
{
	const char *name;
	Api api;
};

std::vector<ApiDesc> audioAPIs();
Api makeValidAPI(Api api = Api::DEFAULT);

}
