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
#include "MainSystem.hh"
#include "Palette.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "GbcEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"10:9 (Original)", 10, 9},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

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

bool GbcSystem::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_GB_PAL_IDX: return optionGBPal.readFromIO(io, readSize);
			case CFGKEY_FULL_GBC_SATURATION: return optionFullGbcSaturation.readFromIO(io, readSize);
			case CFGKEY_AUDIO_RESAMPLER: return optionAudioResampler.readFromIO(io, readSize);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_USE_BUILTIN_GB_PAL: return optionUseBuiltinGBPalette.readFromIO(io, readSize);
			case CFGKEY_REPORT_AS_GBA: return optionReportAsGba.readFromIO(io, readSize);
		}
	}
	return false;
}

void GbcSystem::writeConfig(ConfigType type, IO &io)
{
	if(type == ConfigType::MAIN)
	{
		optionGBPal.writeWithKeyIfNotDefault(io);
		optionFullGbcSaturation.writeWithKeyIfNotDefault(io);
		optionAudioResampler.writeWithKeyIfNotDefault(io);
	}
	else if(type == ConfigType::SESSION)
	{
		optionUseBuiltinGBPalette.writeWithKeyIfNotDefault(io);
		optionReportAsGba.writeWithKeyIfNotDefault(io);
	}
}

}
