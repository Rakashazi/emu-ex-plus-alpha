/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include "GBASys.hh"
#include <core/gba/gba.h>
#include <core/gba/gbaRtc.h>
#include <core/gba/gbaSound.h>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"GBA.emu"};
const char *EmuSystem::configFilename = "GbaEmu.config";

std::span<const AspectRatioInfo> GbaSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"3:2 (Original)", {3, 2}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool GbaSystem::resetSessionOptions(EmuApp &)
{
	optionRtcEmulation.reset();
	setRTC(optionRtcEmulation);
	optionSaveTypeOverride.reset();
	sensorType = GbaSensorType::Auto;
	useBios.reset();
	return true;
}

bool GbaSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_PCM_VOLUME: return readOptionValue<uint8_t>(io, [](auto v){soundSetVolume(gGba, v / 100.f, false);});
			case CFGKEY_GB_APU_VOLUME: return readOptionValue<uint8_t>(io, [](auto v){soundSetVolume(gGba, v / 100.f, true);});
			case CFGKEY_SOUND_FILTERING: return readOptionValue<uint8_t>(io, [](auto v){soundSetFiltering(gGba, v / 100.f);});
			case CFGKEY_SOUND_INTERPOLATION: return readOptionValue<bool>(io, [](auto on){soundSetInterpolation(gGba, on);});
			case CFGKEY_LIGHT_SENSOR_SCALE: return readOptionValue<uint16_t>(io, [&](auto val){lightSensorScaleLux = val;});
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, cheatsDir);
			case CFGKEY_PATCHES_PATH: return readStringOptionValue(io, patchesDir);
			case CFGKEY_BIOS_PATH: return readStringOptionValue(io, biosPath);
			case CFGKEY_DEFAULT_USE_BIOS: return readOptionValue(io, defaultUseBios);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_RTC_EMULATION: return readOptionValue(io, optionRtcEmulation);
			case CFGKEY_SAVE_TYPE_OVERRIDE: return readOptionValue(io, optionSaveTypeOverride);
			case CFGKEY_SENSOR_TYPE:
				return readOptionValue(io, sensorType, [&](auto v){return v <= IG::lastEnum<GbaSensorType>;});
			case CFGKEY_USE_BIOS: return readOptionValue(io, useBios);
		}
	}
	return false;
}

void GbaSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_PCM_VOLUME, (uint8_t)soundVolumeAsInt(gGba, false), 100);
		writeOptionValueIfNotDefault(io, CFGKEY_GB_APU_VOLUME, (uint8_t)soundVolumeAsInt(gGba, true), 100);
		writeOptionValueIfNotDefault(io, CFGKEY_SOUND_FILTERING, (uint8_t)soundFilteringAsInt(gGba), 50);
		writeOptionValueIfNotDefault(io, CFGKEY_SOUND_INTERPOLATION, soundGetInterpolation(gGba), true);
		writeOptionValueIfNotDefault(io, CFGKEY_LIGHT_SENSOR_SCALE, (uint16_t)lightSensorScaleLux, (uint16_t)lightSensorScaleLuxDefault);
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
		writeStringOptionValue(io, CFGKEY_PATCHES_PATH, patchesDir);
		writeStringOptionValue(io, CFGKEY_BIOS_PATH, biosPath);
		writeOptionValueIfNotDefault(io, defaultUseBios);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, optionRtcEmulation);
		writeOptionValueIfNotDefault(io, optionSaveTypeOverride);
		if(sensorType != GbaSensorType::Auto)
			writeOptionValue(io, CFGKEY_SENSOR_TYPE, sensorType);
		writeOptionValueIfNotDefault(io, useBios);
	}
}

void GbaSystem::setRTC(RtcMode mode)
{
	if(detectedRtcGame && mode == RtcMode::AUTO)
	{
		log.info("automatically enabling RTC");
		rtcEnable(true);
	}
	else
	{
		log.info("{} RTC", mode == RtcMode::ON ? "enabled" : "disabled");
		rtcEnable(mode == RtcMode::ON);
	}
}

void GbaSystem::setSensorType(GbaSensorType type)
{
	sensorType = type;
	sessionOptionSet();
}

int soundVolumeAsInt(GBASys &, bool gbVol)
{
	return std::round(100.f * soundGetVolume(gGba, gbVol));
}

int soundFilteringAsInt(GBASys &)
{
	return std::round(100.f * soundGetFiltering(gGba));
}

}
