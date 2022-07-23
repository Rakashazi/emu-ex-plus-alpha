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
#if defined __ANDROID__
#include <imagine/audio/android/AndroidManager.hh>
#elif (defined __APPLE__ && TARGET_OS_IPHONE)
#include <imagine/audio/coreaudio/AvfManager.hh>
#else
#include <imagine/audio/BasicManager.hh>
#endif

#include <imagine/audio/Format.hh>
#include <imagine/audio/defs.hh>
#include <vector>
#include <optional>

namespace IG::Audio
{

class Manager : public ManagerImpl
{
public:
	using ManagerImpl::ManagerImpl;
	SampleFormat nativeSampleFormat() const;
	int nativeRate() const;
	Format nativeFormat() const;
	void setSoloMix(std::optional<bool>);
	bool soloMix() const;

	std::optional<bool> soloMixOption() const
	{
		if constexpr(!HAS_SOLO_MIX)
		{
			return {};
		}
		else
		{
			if(soloMix() != SOLO_MIX_DEFAULT)
				return soloMix();
			return {};
		}
	}

	void setMusicVolumeControlHint();
	void startSession();
	void endSession();
	std::vector<ApiDesc> audioAPIs() const;
	Api makeValidAPI(Api api = Api::DEFAULT) const;
	OutputStreamConfig makeNativeOutputStreamConfig() const;
};

}
