/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "internal.hh"

extern "C"
{
	#include <yabause/sh2int.h>
}

enum
{
	CFGKEY_BIOS_PATH = 279, CFGKEY_SH2_CORE = 280
};

SH2Interface_struct *SH2CoreList[]
{
	#ifdef SH2_DYNAREC
	&SH2Dynarec,
	#endif
	&SH2Interpreter,
	//&SH2DebugInterpreter,
	nullptr
};

static bool OptionSH2CoreIsValid(uint8_t val)
{
	for(const auto &coreI : SH2CoreList)
	{
		if(coreI && coreI->id == val)
		{
			logMsg("SH2 core option valid");
			return true;
		}
	}
	logMsg("SH2 core option not valid");
	return false;
}

const char *EmuSystem::configFilename = "SaturnEmu.config";
static PathOption optionBiosPath{CFGKEY_BIOS_PATH, biosPath, ""};
Byte1Option optionSH2Core{CFGKEY_SH2_CORE, (uint8_t)defaultSH2CoreID, false, OptionSH2CoreIsValid};
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
unsigned SH2Cores = std::size(SH2CoreList) - 1;
bool EmuApp::hasIcon = false;
bool EmuApp::autoSaveStateDefault = false;
bool EmuSystem::hasSound = !(Config::envIsAndroid || Config::envIsIOS);
int EmuSystem::forcedSoundRate = 44100;
bool EmuSystem::constFrameRate = true;

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonSpacing(100);
	EmuApp::setDefaultVControlsButtonStagger(3);
}

EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext)
{
	yinit.sh2coretype = optionSH2Core;
	return {};
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_BIOS_PATH: optionBiosPath.readFromIO(io, readSize);
		bcase CFGKEY_SH2_CORE: optionSH2Core.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionBiosPath.writeToIO(io);
	optionSH2Core.writeWithKeyIfNotDefault(io);
}
