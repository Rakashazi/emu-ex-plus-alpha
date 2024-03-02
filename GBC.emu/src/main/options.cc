/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include "Palette.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "GbcEmu.config";

std::span<const AspectRatioInfo> GbcSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"10:9 (Original)", {10, 9}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void GbcSystem::onOptionsLoaded()
{
	updateColorConversionFlags();
}

bool GbcSystem::resetSessionOptions(EmuApp &)
{
	optionUseBuiltinGBPalette.reset();
	applyGBPalette();
	optionReportAsGba.reset();
	return true;
}

bool GbcSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_GB_PAL_IDX: return readOptionValue(io, optionGBPal);
			case CFGKEY_FULL_GBC_SATURATION: return readOptionValue(io, optionFullGbcSaturation);
			case CFGKEY_AUDIO_RESAMPLER: return readOptionValue(io, optionAudioResampler);
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, cheatsDir);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_USE_BUILTIN_GB_PAL: return readOptionValue(io, optionUseBuiltinGBPalette);
			case CFGKEY_REPORT_AS_GBA: return readOptionValue(io, optionReportAsGba);
		}
	}
	return false;
}

void GbcSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, optionGBPal);
		writeOptionValueIfNotDefault(io, optionFullGbcSaturation);
		writeOptionValueIfNotDefault(io, optionAudioResampler);
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, optionUseBuiltinGBPalette);
		writeOptionValueIfNotDefault(io, optionReportAsGba);
	}
}

}
