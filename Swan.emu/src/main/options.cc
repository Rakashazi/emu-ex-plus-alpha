/*  This file is part of Swan.emu.

	Swan.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Swan.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Swan.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/general.h>

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

bool WsSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_USER_NAME: return readStringOptionValue(io, userName);
			case CFGKEY_USER_PROFILE: return readOptionValue<uint32_t>(io, [&](auto v)
				{
					userProfile = WsUserProfile::unpack(v);
				});
			case CFGKEY_NO_MD5_FILENAMES: return readOptionValue(io, noMD5InFilenames);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_SHOW_VGAMEPAD_Y_HORIZ: return readOptionValue(io, showVGamepadYWhenHorizonal);
			case CFGKEY_SHOW_VGAMEPAD_AB_VERT: return readOptionValue(io, showVGamepadABWhenVertical);
			case CFGKEY_WS_ROTATION: return readOptionValue(io, rotation, [](auto val)
				{
					return val <= lastEnum<WsRotation>;
				});
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
		writeOptionValueIfNotDefault(io, CFGKEY_NO_MD5_FILENAMES, noMD5InFilenames, false);
	}
	else if(type == ConfigType::SESSION)
	{
		if(!showVGamepadYWhenHorizonal)
			writeOptionValue(io, CFGKEY_SHOW_VGAMEPAD_Y_HORIZ, showVGamepadYWhenHorizonal);
		if(showVGamepadABWhenVertical)
			writeOptionValue(io, CFGKEY_SHOW_VGAMEPAD_AB_VERT, showVGamepadABWhenVertical);
		if(rotation != WsRotation::Auto)
			writeOptionValue(io, CFGKEY_WS_ROTATION, rotation);
	}
}

bool WsSystem::resetSessionOptions(EmuApp &app)
{
	showVGamepadYWhenHorizonal = true;
	showVGamepadABWhenVertical = false;
	rotation = WsRotation::Auto;
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

void WsSystem::setRotation(WsRotation r)
{
	if(r == rotation || !hasContent())
		return;
	rotation = r;
	sessionOptionSet();
	auto &app = EmuApp::get(appContext());
	setupInput(app);
	app.updateContentRotation();
}

}

namespace Mednafen
{

#define EMU_MODULE "wswan"

using namespace EmuEx;

uint64 MDFN_GetSettingUI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<WsSystem&>(gSystem());
	if(EMU_MODULE".bday" == name)
		return sys.userProfile.birthDay;
	if(EMU_MODULE".bmonth" == name)
		return sys.userProfile.birthMonth;
	if(EMU_MODULE".byear" == name)
		return sys.userProfile.birthYear;
	bug_unreachable("unhandled settingUI %s", name_);
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<WsSystem&>(gSystem());
	if("filesys.state_comp_level" == name)
		return 6;
	if(EMU_MODULE".sex" == name)
		return sys.userProfile.sex;
	if(EMU_MODULE".blood" == name)
		return sys.userProfile.bloodType;
	bug_unreachable("unhandled settingI %s", name_);
}

double MDFN_GetSettingF(const char *name)
{
	bug_unreachable("unhandled settingF %s", name);
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<WsSystem&>(gSystem());
	if("cheats" == name)
		return 0;
	if(EMU_MODULE".language" == name)
		return sys.userProfile.languageIsEnglish;
	if(EMU_MODULE".excomm" == name)
		return 0;
	if("filesys.untrusted_fip_check" == name)
		return 0;
	bug_unreachable("unhandled settingB %s", name_);
}

std::string MDFN_GetSettingS(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<WsSystem&>(gSystem());
	if(EMU_MODULE".name" == name)
		return std::string{sys.userName};
	if(EMU_MODULE".excomm.path" == name)
		return {};
	bug_unreachable("unhandled settingS %s", name_);
}

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	switch(type)
	{
		case MDFNMKF_STATE:
		case MDFNMKF_SAV:
		case MDFNMKF_SAVBACK:
			return savePathMDFN(id1, cd1);
		default:
			bug_unreachable("type == %d", type);
	}
}

}
