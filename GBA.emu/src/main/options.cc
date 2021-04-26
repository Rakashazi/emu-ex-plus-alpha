/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "internal.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

enum
{
	CFGKEY_RTC_EMULATION = 256
};

const char *EmuSystem::configFilename = "GbaEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"3:2 (Original)", 3, 2},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionRtcEmulation(CFGKEY_RTC_EMULATION, RTC_EMU_AUTO, 0, optionIsValidWithMax<2>);

bool EmuSystem::resetSessionOptions(EmuApp &)
{
	optionRtcEmulation.reset();
	setRTC(optionRtcEmulation);
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_RTC_EMULATION: optionRtcEmulation.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionRtcEmulation.writeWithKeyIfNotDefault(io);
}

void setRTC(unsigned mode)
{
	if(detectedRtcGame && mode == RTC_EMU_AUTO)
	{
		logMsg("automatically enabling RTC");
		rtcEnable(true);
	}
	else
	{
		logMsg("%s RTC", mode == RTC_EMU_ON ? "enabled" : "disabled");
		rtcEnable(mode == RTC_EMU_ON);
	}
}
