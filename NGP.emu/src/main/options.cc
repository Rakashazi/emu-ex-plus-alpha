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

#include <neopop.h>
#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>

enum
{
	CFGKEY_NGPKEY_LANGUAGE = 269,
};

const char *EmuSystem::configFilename = "NgpEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"20:19 (Original)", 20, 19},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

static Option<OptionMethodRef<bool, language_english>, uint8> optionNGPLanguage{CFGKEY_NGPKEY_LANGUAGE, 1};

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_NGPKEY_LANGUAGE: optionNGPLanguage.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionNGPLanguage.writeWithKeyIfNotDefault(io);
}
