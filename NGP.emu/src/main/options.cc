/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "NgpEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"20:19 (Original)", 20, 19},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

bool NgpSystem::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_NGPKEY_LANGUAGE: return optionNGPLanguage.readFromIO(io, readSize);
		}
	}
	return false;
}

void NgpSystem::writeConfig(ConfigType type, IO &io)
{
	if(type == ConfigType::MAIN)
	{
		optionNGPLanguage.writeWithKeyIfNotDefault(io);
	}
}

}
