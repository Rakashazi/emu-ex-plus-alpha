/*  This file is part of Lynx.emu.

	Lynx.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Lynx.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Lynx.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/general.h>

void Lynx_SetLowpassFilter(bool);

namespace EmuEx
{

const char *EmuSystem::configFilename = "LynxEmu.config";

std::span<const AspectRatioInfo> LynxSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"80:51 (Original)", {80, 51}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool LynxSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_BIOS: return readStringOptionValue(io, biosPath);
			case CFGKEY_LOWPASS_FILTER: return readOptionValue(io, lowpassFilter);
			case CFGKEY_NO_MD5_FILENAMES: return readOptionValue(io, noMD5InFilenames);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_LYNX_ROTATION: return readOptionValue(io, rotation, [](auto val)
				{
					return val <= lastEnum<LynxRotation>;
				});
		}
	}
	return false;
}

void LynxSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeStringOptionValue(io, CFGKEY_BIOS, biosPath);
		writeOptionValueIfNotDefault(io, CFGKEY_LOWPASS_FILTER, lowpassFilter, false);
		writeOptionValueIfNotDefault(io, CFGKEY_NO_MD5_FILENAMES, noMD5InFilenames, false);
	}
	else if(type == ConfigType::SESSION)
	{
		if(rotation != LynxRotation::Auto)
			writeOptionValue(io, CFGKEY_LYNX_ROTATION, rotation);
	}
}

bool LynxSystem::resetSessionOptions(EmuApp &app)
{
	rotation = LynxRotation::Auto;
	return true;
}


void LynxSystem::setRotation(LynxRotation r)
{
	if(r == rotation || !hasContent())
		return;
	rotation = r;
	sessionOptionSet();
	auto &app = EmuApp::get(appContext());
	app.updateContentRotation();
}

void LynxSystem::setLowpassFilter(bool on)
{
	lowpassFilter = on;
	if(hasContent())
	{
		Lynx_SetLowpassFilter(on);
	}
}

}

namespace Mednafen
{

#define EMU_MODULE "lynx"

using namespace EmuEx;

uint64 MDFN_GetSettingUI(const char *name_)
{
	bug_unreachable("unhandled settingUI %s", name_);
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	if("filesys.state_comp_level" == name)
		return 6;
	bug_unreachable("unhandled settingI %s", name_);
}

double MDFN_GetSettingF(const char *name)
{
	bug_unreachable("unhandled settingF %s", name);
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<LynxSystem&>(gSystem());
	if("cheats" == name)
		return false;
	if(EMU_MODULE".lowpass" == name)
		return sys.lowpassFilter;
	if("filesys.untrusted_fip_check" == name)
		return false;
	bug_unreachable("unhandled settingB %s", name_);
}

std::string MDFN_GetSettingS(const char *name_)
{
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
		case MDFNMKF_FIRMWARE:
		{
			// lynx-specific
			auto &sys = static_cast<LynxSystem&>(gSystem());
			return std::string{sys.biosPath};
		}
		default:
			bug_unreachable("type == %d", type);
	}
}

}
