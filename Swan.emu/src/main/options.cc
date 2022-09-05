/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "SwanEmu.config";

std::span<const AspectRatioInfo> WsSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"14:9 (Original)", {14, 9}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool WsSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_USER_NAME: return readStringOptionValue(io, readSize, userName);
			case CFGKEY_USER_PROFILE: return readOptionValue<uint32_t>(io, readSize, [&](auto v)
				{
					userProfile = WsUserProfile::unpack(v);
				});
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_SHOW_VGAMEPAD_Y_HORIZ: return readOptionValue(io, readSize, showVGamepadYWhenHorizonal);
			case CFGKEY_SHOW_VGAMEPAD_AB_VERT: return readOptionValue(io, readSize, showVGamepadABWhenVertical);
		}
	}
	return false;
}

void WsSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		if(userName.size())
			writeStringOptionValue(io, CFGKEY_USER_NAME, userName);
		if(userProfile != defaultUserProfile)
			writeOptionValue(io, CFGKEY_USER_PROFILE, WsUserProfile::pack(userProfile));
	}
	else if(type == ConfigType::SESSION)
	{
		if(!showVGamepadYWhenHorizonal)
			writeOptionValue(io, CFGKEY_SHOW_VGAMEPAD_Y_HORIZ, showVGamepadYWhenHorizonal);
		if(showVGamepadABWhenVertical)
			writeOptionValue(io, CFGKEY_SHOW_VGAMEPAD_AB_VERT, showVGamepadABWhenVertical);
	}
}

bool WsSystem::resetSessionOptions(EmuApp &app)
{
	showVGamepadYWhenHorizonal = true;
	showVGamepadABWhenVertical = false;
	return true;
}

void WsSystem::setShowVGamepadYWhenHorizonal(bool on)
{
	sessionOptionSet();
	showVGamepadYWhenHorizonal = on;
	setupInput(EmuApp::get(appContext()));
}

void WsSystem::setShowVGamepadABWhenVertical(bool on)
{
	sessionOptionSet();
	showVGamepadABWhenVertical = on;
	setupInput(EmuApp::get(appContext()));
}

}
