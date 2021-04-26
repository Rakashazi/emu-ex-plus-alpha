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
#include "MDFN.hh"
#include "internal.hh"

enum
{
	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
	CFGKEY_6_BTN_PAD = 277
};

const char *EmuSystem::configFilename = "PceEmu.config";
Byte1Option optionArcadeCard{CFGKEY_ARCADE_CARD, 1};
Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
static PathOption optionSysCardPath{CFGKEY_SYSCARD_PATH, sysCardPath, ""};

const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

void EmuSystem::onSessionOptionsLoaded(EmuApp &app)
{
	app.setActiveFaceButtons(option6BtnPad ? 6 : 2);
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	optionArcadeCard.reset();
	option6BtnPad.reset();
	onSessionOptionsLoaded(app);
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_ARCADE_CARD: optionArcadeCard.readFromIO(io, readSize);
		bcase CFGKEY_6_BTN_PAD: option6BtnPad.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionArcadeCard.writeWithKeyIfNotDefault(io);
	option6BtnPad.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_SYSCARD_PATH: optionSysCardPath.readFromIO(io, readSize);
		logMsg("syscard path %s", sysCardPath.data());
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionSysCardPath.writeToIO(io);
}
