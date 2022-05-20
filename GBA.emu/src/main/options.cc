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
#include "MainSystem.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

namespace EmuEx
{

const char *EmuSystem::configFilename = "GbaEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"3:2 (Original)", 3, 2},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

bool GbaSystem::resetSessionOptions(EmuApp &)
{
	optionRtcEmulation.reset();
	setRTC((RtcMode)optionRtcEmulation.val);
	return true;
}

bool GbaSystem::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_RTC_EMULATION: return optionRtcEmulation.readFromIO(io, readSize);
		}
	}
	return false;
}

void GbaSystem::writeConfig(ConfigType type, IO &io)
{
	if(type == ConfigType::SESSION)
	{
		optionRtcEmulation.writeWithKeyIfNotDefault(io);
	}
}

void GbaSystem::setRTC(RtcMode mode)
{
	if(detectedRtcGame && mode == RtcMode::AUTO)
	{
		logMsg("automatically enabling RTC");
		rtcEnable(true);
	}
	else
	{
		logMsg("%s RTC", mode == RtcMode::ON ? "enabled" : "disabled");
		rtcEnable(mode == RtcMode::ON);
	}
}

}
