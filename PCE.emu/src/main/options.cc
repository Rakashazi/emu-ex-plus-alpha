/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "MainSystem.hh"
#include <mednafen/pce/pcecd.h>
#include <mednafen/pce/vce.h>
#include <mednafen/pce_fast/pcecd.h>
#include <mednafen-emuex/MDFNUtils.hh>
#include <mednafen/general.h>

namespace EmuEx
{

const char *EmuSystem::configFilename = "PceEmu.config";

std::span<const AspectRatioInfo> PceSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

static bool visibleLinesAreValid(VisibleLines lines)
{
	return lines.first <= 18 && lines.last >= 234 && lines.last <= 241;
}

void PceSystem::onSessionOptionsLoaded(EmuApp &app)
{
	set6ButtonPadEnabled(app, option6BtnPad);
}

bool PceSystem::resetSessionOptions(EmuApp &app)
{
	optionArcadeCard.reset();
	option6BtnPad.reset();
	visibleLines = defaultVisibleLines;
	core = EmuCore::Auto;
	onSessionOptionsLoaded(app);
	return true;
}

bool PceSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_SYSCARD_PATH: return readStringOptionValue(io, readSize, sysCardPath);
			case CFGKEY_DEFAULT_VISIBLE_LINES: return readOptionValue(io, readSize, defaultVisibleLines, visibleLinesAreValid);
			case CFGKEY_CORRECT_LINE_ASPECT: return readOptionValue(io, readSize, correctLineAspect);
			case CFGKEY_NO_SPRITE_LIMIT: return readOptionValue(io, readSize, noSpriteLimit);
			case CFGKEY_CD_SPEED: return readOptionValue(io, readSize, cdSpeed, [](auto val){return val <= 8;});
			case CFGKEY_CDDA_VOLUME: return readOptionValue(io, readSize, cddaVolume, [](auto val){return val <= 200;});
			case CFGKEY_ADPCM_VOLUME: return readOptionValue(io, readSize, adpcmVolume, [](auto val){return val <= 200;});
			case CFGKEY_ADPCM_FILTER: return readOptionValue(io, readSize, adpcmFilter);
			case CFGKEY_EMU_CORE: return readOptionValue(io, readSize, defaultCore, [](auto val){return val <= lastEnum<EmuCore>;});
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_ARCADE_CARD: return optionArcadeCard.readFromIO(io, readSize);
			case CFGKEY_6_BTN_PAD: return option6BtnPad.readFromIO(io, readSize);
			case CFGKEY_VISIBLE_LINES: return readOptionValue(io, readSize, visibleLines, visibleLinesAreValid);
			case CFGKEY_EMU_CORE: return readOptionValue(io, readSize, core, [](auto val){return val <= lastEnum<EmuCore>;});
		}
	}
	return false;
}

void PceSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeStringOptionValue(io, CFGKEY_SYSCARD_PATH, sysCardPath);
		if(defaultVisibleLines != VisibleLines{})
			writeOptionValue<VisibleLines>(io, CFGKEY_DEFAULT_VISIBLE_LINES, defaultVisibleLines);
		if(correctLineAspect)
			writeOptionValue(io, CFGKEY_CORRECT_LINE_ASPECT, correctLineAspect);
		if(noSpriteLimit)
			writeOptionValue(io, CFGKEY_NO_SPRITE_LIMIT, noSpriteLimit);
		if(cdSpeed != 2)
			writeOptionValue(io, CFGKEY_CD_SPEED, cdSpeed);
		if(cddaVolume != 100)
			writeOptionValue(io, CFGKEY_CDDA_VOLUME, cddaVolume);
		if(adpcmVolume != 100)
			writeOptionValue(io, CFGKEY_ADPCM_VOLUME, adpcmVolume);
		if(adpcmFilter)
			writeOptionValue(io, CFGKEY_ADPCM_FILTER, adpcmFilter);
		writeOptionValueIfNotDefault(io, CFGKEY_EMU_CORE, defaultCore, EmuCore::Auto);
	}
	else if(type == ConfigType::SESSION)
	{
		optionArcadeCard.writeWithKeyIfNotDefault(io);
		option6BtnPad.writeWithKeyIfNotDefault(io);
		if(visibleLines != defaultVisibleLines)
			writeOptionValue<VisibleLines>(io, CFGKEY_VISIBLE_LINES, visibleLines);
		writeOptionValueIfNotDefault(io, CFGKEY_EMU_CORE, core, EmuCore::Auto);
	}
}

void PceSystem::setVisibleLines(VisibleLines lines)
{
	sessionOptionSet();
	visibleLines = lines;
	if(!hasContent())
		return;
	if(isUsingAccurateCore())
	{
		MDFN_IEN_PCE::vce->slstart = lines.first;
		MDFN_IEN_PCE::vce->slend = lines.last;
	}
	else
	{
		MDFN_IEN_PCE_FAST::vce.slstart = lines.first;
		MDFN_IEN_PCE_FAST::vce.slend = lines.last;
	}
}

void PceSystem::setNoSpriteLimit(bool on)
{
	noSpriteLimit = on;
	if(!hasContent())
		return;
	if(isUsingAccurateCore())
		MDFN_IEN_PCE::vce->SetVDCUnlimitedSprites(on);
	else
		MDFN_IEN_PCE_FAST::VDC_SetSettings(on, true);
}

void PceSystem::updateCdSettings()
{
	if(!hasContent())
		return;
	if(isUsingAccurateCore())
	{
		MDFN_IEN_PCE::PCECD_Settings cdSettings
		{
			.CDDA_Volume = cddaVolume / 100.f,
			.ADPCM_Volume = adpcmVolume / 100.f,
			.ADPCM_ExtraPrecision = adpcmFilter
		};
		MDFN_IEN_PCE::PCECD_SetSettings(&cdSettings);
	}
	else
	{
		MDFN_IEN_PCE_FAST::PCECD_Settings cdSettings
		{
			.CDDA_Volume = cddaVolume / 100.f,
			.ADPCM_Volume = adpcmVolume / 100.f,
			.CD_Speed = cdSpeed,
			.ADPCM_LPF = adpcmFilter
		};
		MDFN_IEN_PCE_FAST::PCECD_SetSettings(&cdSettings);
	}
}

void PceSystem::setCdSpeed(uint8_t speed)
{
	cdSpeed = speed;
	updateCdSettings();
}

void PceSystem::setVolume(VolumeType type, uint8_t vol)
{
	volumeVar(type) = vol;
	updateCdSettings();
}

void PceSystem::setAdpcmFilter(bool on)
{
	adpcmFilter = on;
	updateCdSettings();
}

}

namespace Mednafen
{

using namespace EmuEx;

uint64 MDFN_GetSettingUI(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<PceSystem&>(gSystem());
	if("pce_fast.ocmultiplier" == name)
		return 1;
	if("pce_fast.cdspeed" == name)
		return sys.cdSpeed;
	if(name.ends_with(".cdpsgvolume"))
		return 100;
	if(name.ends_with(".cddavolume"))
		return sys.cddaVolume;
	if(name.ends_with(".adpcmvolume"))
		return sys.adpcmVolume;
	if(name.ends_with(".slstart"))
		return sys.visibleLines.first;
	if(name.ends_with(".slend"))
		return sys.visibleLines.last;
	if("pce.resamp_quality" == name)
		return 3;
	if("pce.vramsize" == name)
		return 32768;
	bug_unreachable("unhandled settingUI %s", name_);
}

int64 MDFN_GetSettingI(const char *name_)
{
	std::string_view name{name_};
	if("pce.psgrevision" == name)
		return 2; //PCE_PSG::_REVISION_COUNT
	if("filesys.state_comp_level" == name)
		return 6;
	bug_unreachable("unhandled settingI %s", name_);
}

double MDFN_GetSettingF(const char *name_)
{
	std::string_view name{name_};
	if(name.ends_with(".mouse_sensitivity"))
		return 0.50;
	if("pce.resamp_rate_error" == name)
		return 0.0000009;
	bug_unreachable("unhandled settingF %s", name_);
}

bool MDFN_GetSettingB(const char *name_)
{
	std::string_view name{name_};
	auto &sys = static_cast<PceSystem&>(gSystem());
	if("cheats" == name)
		return false;
	if(name.ends_with(".arcadecard"))
		return sys.optionArcadeCard;
	if(name.ends_with(".forcesgx"))
		return false;
	if(name.ends_with(".nospritelimit"))
		return sys.noSpriteLimit;
	if("pce_fast.forcemono" == name)
		return false;
	if(name.ends_with(".disable_softreset"))
		return false;
	if("pce_fast.adpcmlp" == name || "pce.adpcmextraprec" == name)
		return sys.adpcmFilter;
	if("pce_fast.correct_aspect" == name)
		return true;
	if("pce.input.multitap" == name)
		return true;
	if("pce.h_overscan" == name)
		return false;
	if("pce.disable_bram_cd" == name)
		return false;
	if("pce.disable_bram_hucard" == name)
		return false;
	if("filesys.untrusted_fip_check" == name)
		return 0;
	bug_unreachable("unhandled settingB %s", name_);
}

std::string MDFN_GetSettingS(const char *name_)
{
	std::string_view name{name_};
	if(name.ends_with(".cdbios"))
		return {};
	if("pce.gecdbios" == name)
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
		case MDFNMKF_FIRMWARE:
		{
			// pce-specific
			auto &sys = static_cast<PceSystem&>(gSystem());
			logMsg("system card path:%s", sys.sysCardPath.data());
			return std::string{sys.sysCardPath};
		}
		default:
			bug_unreachable("type == %d", type);
	}
}

}
