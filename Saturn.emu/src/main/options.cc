/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/general.h>
#include <ss/smpc.h>
#include <ss/db.h>

namespace EmuEx
{

const char *EmuSystem::configFilename = "SaturnEmu.config";

std::span<const AspectRatioInfo> SaturnSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void SaturnSystem::onSessionOptionsLoaded(EmuApp &app)
{
	applyInputConfig(app);
}

bool SaturnSystem::resetSessionOptions(EmuApp &app)
{
	inputConfig = {};
	cartType = MDFN_IEN_SS::CART__RESERVED;
	region = {};
	sysContentRotation = Rotation::ANY;
	videoLines = {};
	showHOverscan = defaultShowHOverscan;
	deinterlaceMode = DeinterlaceMode::Bob;
	widescreenMode = WidescreenMode::Auto;
	onSessionOptionsLoaded(app);
	return true;
}

template<int maxLines>
constexpr bool linesAreValid(VideoLineRange lines)
{
	return lines.first >= 0 && lines.first < maxLines
		&& lines.last >= 0 && lines.last < maxLines
		&& lines.first <= lines.last;
}

bool SaturnSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_NA_BIOS_PATH: return readStringOptionValue(io, naBiosPath);
			case CFGKEY_JP_BIOS_PATH: return readStringOptionValue(io, jpBiosPath);
			case CFGKEY_KOF_ROM_PATH: return readStringOptionValue(io, kof95ROMPath);
			case CFGKEY_ULTRAMAN_ROM_PATH: return readStringOptionValue(io, ultramanROMPath);
			case CFGKEY_BIOS_LANG: return readOptionValue(io, biosLanguage);
			case CFGKEY_AUTO_RTC_TIME: return readOptionValue(io, autoRTCTime);
			case CFGKEY_DEFAULT_NTSC_VIDEO_LINES: return readOptionValue(io, defaultNtscLines, linesAreValid<240>);
			case CFGKEY_DEFAULT_PAL_VIDEO_LINES: return readOptionValue(io, defaultPalLines, linesAreValid<288>);
			case CFGKEY_DEFAULT_SHOW_H_OVERSCAN: return readOptionValue(io, defaultShowHOverscan);
			case CFGKEY_NO_MD5_FILENAMES: return readOptionValue(io, noMD5InFilenames);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_INPUT_PORT_CONFIG: return readOptionValue(io, inputConfig);
			case CFGKEY_CART_TYPE: return readOptionValue(io, cartType);
			case CFGKEY_REGION: return readOptionValue(io, region);
			case CFGKEY_SYSTEM_CONTENT_ROTATION: return readOptionValue(io, sysContentRotation);
			case CFGKEY_VIDEO_LINES: return readOptionValue(io, videoLines);
			case CFGKEY_SHOW_H_OVERSCAN: return readOptionValue(io, showHOverscan);
			case CFGKEY_DEINTERLACE_MODE: return readOptionValue(io, deinterlaceMode);
			case CFGKEY_WIDESCREEN_MODE: return readOptionValue(io, widescreenMode);
		}
	}
	return false;
}

void SaturnSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeStringOptionValue(io, CFGKEY_NA_BIOS_PATH, naBiosPath);
		writeStringOptionValue(io, CFGKEY_JP_BIOS_PATH, jpBiosPath);
		writeStringOptionValue(io, CFGKEY_KOF_ROM_PATH, kof95ROMPath);
		writeStringOptionValue(io, CFGKEY_ULTRAMAN_ROM_PATH, ultramanROMPath);
		writeOptionValueIfNotDefault(io, CFGKEY_BIOS_LANG, biosLanguage, MDFN_IEN_SS::SMPC_RTC_LANG_ENGLISH);
		writeOptionValueIfNotDefault(io, CFGKEY_AUTO_RTC_TIME, autoRTCTime, true);
		writeOptionValueIfNotDefault(io, CFGKEY_DEFAULT_NTSC_VIDEO_LINES, defaultNtscLines, safeNtscLines);
		writeOptionValueIfNotDefault(io, CFGKEY_DEFAULT_PAL_VIDEO_LINES, defaultPalLines, safePalLines);
		writeOptionValueIfNotDefault(io, CFGKEY_DEFAULT_SHOW_H_OVERSCAN, defaultShowHOverscan, false);
		writeOptionValueIfNotDefault(io, CFGKEY_NO_MD5_FILENAMES, noMD5InFilenames, false);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_INPUT_PORT_CONFIG, inputConfig, InputConfig{});
		writeOptionValueIfNotDefault(io, CFGKEY_CART_TYPE, cartType, MDFN_IEN_SS::CART__RESERVED);
		writeOptionValueIfNotDefault(io, CFGKEY_REGION, region, 0);
		writeOptionValueIfNotDefault(io, CFGKEY_SYSTEM_CONTENT_ROTATION, sysContentRotation, Rotation::ANY);
		writeOptionValueIfNotDefault(io, CFGKEY_VIDEO_LINES, videoLines, MDFN_IEN_SS::VDP2::PAL ? defaultPalLines : defaultNtscLines);
		writeOptionValueIfNotDefault(io, CFGKEY_SHOW_H_OVERSCAN, showHOverscan, defaultShowHOverscan);
		writeOptionValueIfNotDefault(io, CFGKEY_DEINTERLACE_MODE, deinterlaceMode, DeinterlaceMode::Bob);
		writeOptionValueIfNotDefault(io, CFGKEY_WIDESCREEN_MODE, widescreenMode, WidescreenMode::Auto);
	}
}

Rotation SaturnSystem::contentRotation() const
{
	return sysContentRotation == Rotation::ANY ? Rotation::UP : sysContentRotation;
}

}

namespace Mednafen
{

using namespace EmuEx;

uint64 MDFN_GetSettingUI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<SaturnSystem&>(gSystem());
	if("ss.scsp.resamp_quality" == name)
		return 4;
	if("ss.smpc.autortc.lang" == name)
		return sys.biosLanguage;
	if("ss.affinity.vdp2" == name)
		return 0;
	if(name.ends_with("gun_chairs"))
		return 0xFFFFFFFF;
	if(name == "ss.dbg_cem")
		return MDFN_IEN_SS::CPUCACHE_EMUMODE__COUNT;
	if(name == "ss.midi")
		return 0;
	bug_unreachable("unhandled settingUI %s", name_);
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<SaturnSystem&>(gSystem());
	if("filesys.state_comp_level" == name)
		return 6;
	if("ss.cart" == name)
		return sys.cartType;
	if("ss.cart.auto_default" == name)
		return MDFN_IEN_SS::CART_NONE;
	if("ss.region_default" == name)
		return sys.region;
	if("ss.slstart" == name || "ss.slstartp" == name)
		return sys.videoLines.first;
	if("ss.slend" == name || "ss.slendp" == name)
		return sys.videoLines.last;
	bug_unreachable("unhandled settingI %s", name_);
}

double MDFN_GetSettingF(const char *name_)
{
	std::string_view name{name_};
	if(name.ends_with(".mouse_sensitivity"))
		return 0.50;
	bug_unreachable("unhandled settingF %s", name_);
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<SaturnSystem&>(gSystem());
	if("cheats" == name)
		return false;
	if(name.ends_with(".disable_softreset"))
		return false;
	if("filesys.untrusted_fip_check" == name)
		return false;
	if("ss.bios_sanity" == name)
		return true;
	if("ss.cd_sanity" == name)
		return true;
	if("ss.correct_aspect" == name)
		return true;
	if("ss.h_overscan" == name)
		return sys.showHOverscan;
	if("ss.h_blend" == name)
		return false;
	if("ss.region_autodetect" == name)
		return !sys.region;
	if("ss.smpc.autortc" == name)
		return sys.autoRTCTime;
	if("ss.input.sport1.multitap" == name)
		return false; // multitaps are handled in onSessionOptionsLoaded()
	if("ss.input.sport2.multitap" == name)
		return false;
	bug_unreachable("unhandled settingB %s", name_);
}

std::string MDFN_GetSettingS(const char *name_)
{
	std::string_view name{name_};
	if(name == "ss.bios_jp")
		return "jp";
	if(name == "ss.bios_na_eu")
		return "na";
	if(name == "ss.cart.kof95_path")
		return "kof95";
	if(name == "ss.cart.ultraman_path")
		return "ultraman";
	bug_unreachable("unhandled settingS %s", name_);
}

uint64 MDFN_GetSettingMultiM(const char *name_)
{
	std::string_view name{name_};
	if(name == "ss.dbg_hh")
		return (unsigned)-1;
	bug_unreachable("unhandled settingMultiM %s", name_);
}

std::vector<uint64> MDFN_GetSettingMultiUI(const char *name_)
{
	bug_unreachable("unhandled settingMultiUI %s", name_);
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
			// saturn-specific
			auto &sys = static_cast<SaturnSystem&>(gSystem());
			std::string_view biosName{cd1};
			if(biosName == "kof95")
			{
				if(sys.kof95ROMPath.empty())
					throw MDFN_Error(0, _("Please set KoF '95 ROM in Options➔File Paths"));
				return std::string{sys.kof95ROMPath};
			}
			else if(biosName == "ultraman")
			{
				if(sys.ultramanROMPath.empty())
					throw MDFN_Error(0, _("Please set Ultraman ROM in Options➔File Paths"));
				return std::string{sys.ultramanROMPath};
			}
			else if(biosName == "na")
			{
				if(sys.naBiosPath.empty())
					throw MDFN_Error(0, _("Please set NA/EU BIOS in Options➔File Paths"));
				return std::string{sys.naBiosPath};
			}
			else
			{
				if(sys.jpBiosPath.empty())
					throw MDFN_Error(0, _("Please set JP BIOS in Options➔File Paths"));
				return std::string{sys.jpBiosPath};
			}
		}
		default:
			bug_unreachable("type == %d", type);
	}
}

}
