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

#include <imagine/audio/Manager.hh>
#include <imagine/audio/defs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>

namespace IG::Audio
{

SampleFormat Manager::nativeSampleFormat() const
{
	return SampleFormats::f32;
}

int Manager::nativeRate() const
{
	return ::Config::MACHINE_IS_PANDORA ? 44100 : 48000;
}

Format Manager::nativeFormat() const
{
	return {nativeRate(), nativeSampleFormat(), 2};
}

void Manager::setSoloMix(std::optional<bool>) {}

bool Manager::soloMix() const { return false; }

void Manager::setMusicVolumeControlHint() {}

void Manager::startSession() {}

void Manager::endSession() {}

static constexpr ApiDesc apiDesc[]
{
	#ifdef CONFIG_PACKAGE_PULSEAUDIO
	{"PulseAudio", Api::PULSEAUDIO},
	#endif
	#ifdef CONFIG_PACKAGE_ALSA
	{"ALSA", Api::ALSA},
	#endif
};

std::vector<ApiDesc> Manager::audioAPIs() const
{
	return {apiDesc, apiDesc + std::size(apiDesc)};
}

Api Manager::makeValidAPI(Api api) const
{
	for(auto desc: apiDesc)
	{
		if(desc.api == api)
		{
			logDMsg("found requested API:%s", desc.name);
			return api;
		}
	}
	// API not found, use the default
	return apiDesc[0].api;
}

}
