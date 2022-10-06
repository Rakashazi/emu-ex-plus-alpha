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

#include <imagine/util/string.h>
#include "MainApp.hh"

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/emu.h>
}

namespace EmuEx
{

const char *EmuSystem::configFilename = "NeoEmu.config";

std::span<const AspectRatioInfo> NeoSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void NeoSystem::setTimerIntOption()
{
	switch(optionTimerInt)
	{
		bcase 0: conf.raster = 0;
		bcase 1: conf.raster = 1;
		bcase 2:
			bool needsTimer = hasContent() && IG::stringContainsAny(contentDisplayName(),
				"Sidekicks 2", "Sidekicks 3", "Ultimate 11", "Neo-Geo Cup", "Spin Master", "Neo Turf Masters");
			if(needsTimer) logMsg("auto enabled timer interrupt");
			conf.raster = needsTimer;
	}
}

void NeoSystem::onOptionsLoaded()
{
	conf.system = (SYSTEM)optionBIOSType.val;
	conf.country = (COUNTRY)optionMVSCountry.val;
}

bool NeoSystem::resetSessionOptions(EmuApp &app)
{
	optionTimerInt.reset();
	setTimerIntOption();
	return true;
}

bool NeoSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_LIST_ALL_GAMES: return optionListAllGames.readFromIO(io, readSize);
			case CFGKEY_BIOS_TYPE: return optionBIOSType.readFromIO(io, readSize);
			case CFGKEY_MVS_COUNTRY: return optionMVSCountry.readFromIO(io, readSize);
			case CFGKEY_CREATE_USE_CACHE: return optionCreateAndUseCache.readFromIO(io, readSize);
			case CFGKEY_STRICT_ROM_CHECKING: return optionStrictROMChecking.readFromIO(io, readSize);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_TIMER_INT: return optionTimerInt.readFromIO(io, readSize);
		}
	}
	return false;
}

void NeoSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		optionListAllGames.writeWithKeyIfNotDefault(io);
		optionBIOSType.writeWithKeyIfNotDefault(io);
		optionMVSCountry.writeWithKeyIfNotDefault(io);
		optionCreateAndUseCache.writeWithKeyIfNotDefault(io);
		optionStrictROMChecking.writeWithKeyIfNotDefault(io);
	}
	else if(type == ConfigType::SESSION)
	{
		optionTimerInt.writeWithKeyIfNotDefault(io);
	}
}

}
