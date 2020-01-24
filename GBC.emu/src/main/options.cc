/*  This file is part of GBA.emu.

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
#include "internal.hh"

enum
{
	CFGKEY_GB_PAL_IDX = 270, CFGKEY_REPORT_AS_GBA = 271,
	CFGKEY_FULL_GBC_SATURATION = 272, CFGKEY_AUDIO_RESAMPLER = 273,
	CFGKEY_USE_BUILTIN_GB_PAL = 274
};

const char *EmuSystem::configFilename = "GbcEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"10:9 (Original)", 10, 9},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionGBPal{CFGKEY_GB_PAL_IDX, 0, 0, optionIsValidWithMax<std::size(gbPal)-1>};
Byte1Option optionUseBuiltinGBPalette{CFGKEY_USE_BUILTIN_GB_PAL, 1};
Byte1Option optionReportAsGba{CFGKEY_REPORT_AS_GBA, 0};
Byte1Option optionAudioResampler{CFGKEY_AUDIO_RESAMPLER, 1};
Option<OptionMethodRef<bool, gambatte::useFullColorSaturation>, uint8_t>
optionFullGbcSaturation{CFGKEY_FULL_GBC_SATURATION, 0};

bool EmuSystem::resetSessionOptions()
{
	optionUseBuiltinGBPalette.reset();
	applyGBPalette();
	optionReportAsGba.reset();
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_USE_BUILTIN_GB_PAL: optionUseBuiltinGBPalette.readFromIO(io, readSize);
		bcase CFGKEY_REPORT_AS_GBA: optionReportAsGba.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionUseBuiltinGBPalette.writeWithKeyIfNotDefault(io);
	optionReportAsGba.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_GB_PAL_IDX: optionGBPal.readFromIO(io, readSize);
		bcase CFGKEY_FULL_GBC_SATURATION: optionFullGbcSaturation.readFromIO(io, readSize);
		bcase CFGKEY_AUDIO_RESAMPLER: optionAudioResampler.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionGBPal.writeWithKeyIfNotDefault(io);
	optionFullGbcSaturation.writeWithKeyIfNotDefault(io);
	optionAudioResampler.writeWithKeyIfNotDefault(io);
}
