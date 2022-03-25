/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <imagine/util/string.h>
#include "internal.hh"

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/emu.h>
}

namespace EmuEx
{

enum
{
	CFGKEY_LIST_ALL_GAMES = 275, CFGKEY_BIOS_TYPE = 276,
	CFGKEY_MVS_COUNTRY = 277, CFGKEY_TIMER_INT = 278,
	CFGKEY_CREATE_USE_CACHE = 279,
	CFGKEY_NEOGEOKEY_TEST_SWITCH = 280, CFGKEY_STRICT_ROM_CHECKING = 281
};

static bool systemEnumIsValid(uint8_t val)
{
	return val < SYS_MAX;
}

static bool countryEnumIsValid(uint8_t val)
{
	return val < CTY_MAX;
}

const char *EmuSystem::configFilename = "NeoEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
bool EmuApp::autoSaveStateDefault = false;
Byte1Option optionListAllGames{CFGKEY_LIST_ALL_GAMES, 0};
Byte1Option optionBIOSType{CFGKEY_BIOS_TYPE, SYS_UNIBIOS, 0, systemEnumIsValid};
Byte1Option optionMVSCountry{CFGKEY_MVS_COUNTRY, CTY_USA, 0, countryEnumIsValid};
Byte1Option optionTimerInt{CFGKEY_TIMER_INT, 2};
Byte1Option optionCreateAndUseCache{CFGKEY_CREATE_USE_CACHE, 0};
Byte1Option optionStrictROMChecking{CFGKEY_STRICT_ROM_CHECKING, 0};

void setTimerIntOption(EmuSystem &sys)
{
	switch(optionTimerInt)
	{
		bcase 0: conf.raster = 0;
		bcase 1: conf.raster = 1;
		bcase 2:
			bool needsTimer = sys.hasContent() && IG::stringContainsAny(sys.contentDisplayName(),
				"Sidekicks 2", "Sidekicks 3", "Ultimate 11", "Neo-Geo Cup", "Spin Master", "Neo Turf Masters");
			if(needsTimer) logMsg("auto enabled timer interrupt");
			conf.raster = needsTimer;
	}
}

void EmuSystem::initOptions(EmuApp &app)
{
	app.setDefaultVControlsButtonSpacing(100);
	app.setDefaultVControlsButtonStagger(5);
}

void EmuSystem::onOptionsLoaded()
{
	conf.system = (SYSTEM)optionBIOSType.val;
	conf.country = (COUNTRY)optionMVSCountry.val;
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	optionTimerInt.reset();
	setTimerIntOption(app.system());
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_TIMER_INT: optionTimerInt.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionTimerInt.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_LIST_ALL_GAMES: optionListAllGames.readFromIO(io, readSize);
		bcase CFGKEY_BIOS_TYPE: optionBIOSType.readFromIO(io, readSize);
		bcase CFGKEY_MVS_COUNTRY: optionMVSCountry.readFromIO(io, readSize);
		bcase CFGKEY_CREATE_USE_CACHE: optionCreateAndUseCache.readFromIO(io, readSize);
		bcase CFGKEY_STRICT_ROM_CHECKING: optionStrictROMChecking.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionListAllGames.writeWithKeyIfNotDefault(io);
	optionBIOSType.writeWithKeyIfNotDefault(io);
	optionMVSCountry.writeWithKeyIfNotDefault(io);
	optionCreateAndUseCache.writeWithKeyIfNotDefault(io);
	optionStrictROMChecking.writeWithKeyIfNotDefault(io);
}

}
