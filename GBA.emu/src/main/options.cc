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
#include "MainSystem.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>
#include <vbam/gba/Sound.h>

namespace EmuEx
{

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
	setRTC((RtcMode)optionRtcEmulation.val);
	optionSaveTypeOverride.reset();
	sensorType = GbaSensorType::Auto;
	return true;
}

bool GbaSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_PCM_VOLUME: return readOptionValue<uint8_t>(io, readSize, [](auto v){soundSetVolume(gGba, v / 100.f, false);});
			case CFGKEY_GB_APU_VOLUME: return readOptionValue<uint8_t>(io, readSize, [](auto v){soundSetVolume(gGba, v / 100.f, true);});
			case CFGKEY_SOUND_FILTERING: return readOptionValue<uint8_t>(io, readSize, [](auto v){soundSetFiltering(gGba, v / 100.f);});
			case CFGKEY_SOUND_INTERPOLATION: return readOptionValue<bool>(io, readSize, [](auto on){soundSetInterpolation(gGba, on);});
			case CFGKEY_LIGHT_SENSOR_SCALE: return readOptionValue<uint16_t>(io, readSize, [&](auto val){lightSensorScaleLux = val;});
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_RTC_EMULATION: return optionRtcEmulation.readFromIO(io, readSize);
			case CFGKEY_SAVE_TYPE_OVERRIDE: return optionSaveTypeOverride.readFromIO(io, readSize);
			case CFGKEY_SENSOR_TYPE:
				return readOptionValue(io, readSize, sensorType, [&](auto v){return v <= IG::lastEnum<GbaSensorType>;});
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
	}
	else if(type == ConfigType::SESSION)
	{
		optionRtcEmulation.writeWithKeyIfNotDefault(io);
		optionSaveTypeOverride.writeWithKeyIfNotDefault(io);
		if(sensorType != GbaSensorType::Auto)
			writeOptionValue(io, CFGKEY_SENSOR_TYPE, (uint8_t)sensorType);
	}
}

void GbaSystem::setRTC(RtcMode mode)
{
	if(detectedRtcGame && mode == RtcMode::AUTO)
	{
		logMsg("automatically enabling RTC");
		rtcEnable(true);
	}
	else
	{
		logMsg("%s RTC", mode == RtcMode::ON ? "enabled" : "disabled");
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
