/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "internal.hh"

enum
{
	CFGKEY_DEFAULT_MACHINE_NAME = 256, CFGKEY_SKIP_FDC_ACCESS = 257,
	CFGKEY_MACHINE_FILE_PATH = 258, CFGKEY_SESSION_MACHINE_NAME = 259
};

const char *EmuSystem::configFilename = "MsxEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
int EmuSystem::forcedSoundRate = 44100;
#define optionMachineNameDefault "MSX2"
static char optionDefaultMachineNameStr[128] = optionMachineNameDefault;
static char optionSessionMachineNameStr[128]{};
PathOption optionDefaultMachineName{CFGKEY_DEFAULT_MACHINE_NAME, optionDefaultMachineNameStr, optionMachineNameDefault};
PathOption optionMachineName{CFGKEY_SESSION_MACHINE_NAME, optionSessionMachineNameStr, ""};
Byte1Option optionSkipFdcAccess{CFGKEY_SKIP_FDC_ACCESS, 1};
PathOption optionFirmwarePath{CFGKEY_MACHINE_FILE_PATH, machineCustomPath, ""};
uint activeBoardType = BOARD_MSX;

bool EmuSystem::resetSessionOptions()
{
	string_copy(optionSessionMachineNameStr, "");
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_SESSION_MACHINE_NAME: optionMachineName.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionMachineName.writeToIO(io);
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_DEFAULT_MACHINE_NAME: optionDefaultMachineName.readFromIO(io, readSize);
		bcase CFGKEY_SKIP_FDC_ACCESS: optionSkipFdcAccess.readFromIO(io, readSize);
		bcase CFGKEY_MACHINE_FILE_PATH: optionFirmwarePath.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	if(!optionDefaultMachineName.isDefault())
	{
		optionDefaultMachineName.writeToIO(io);
	}
	optionSkipFdcAccess.writeWithKeyIfNotDefault(io);
	optionFirmwarePath.writeToIO(io);
}

EmuSystem::Error EmuSystem::onOptionsLoaded()
{
	machineBasePath = makeMachineBasePath(machineCustomPath);
	return {};
}

bool setDefaultMachineName(const char *name)
{
	if(string_equal(name, optionDefaultMachineNameStr))
		return false;
	string_copy(optionDefaultMachineNameStr, name);
	return true;
}
