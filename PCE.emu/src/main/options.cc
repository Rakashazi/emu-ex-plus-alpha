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
#include "MainSystem.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "PceEmu.config";

std::span<const AspectRatioInfo> PceSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		{"8:7", {8, 7}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void PceSystem::onSessionOptionsLoaded(EmuApp &app)
{
	set6ButtonPadEnabled(app, option6BtnPad);
}

bool PceSystem::resetSessionOptions(EmuApp &app)
{
	optionArcadeCard.reset();
	option6BtnPad.reset();
	onSessionOptionsLoaded(app);
	return true;
}

bool PceSystem::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_SYSCARD_PATH:
				return readStringOptionValue<FS::PathString>(io, readSize, [&](auto &path){sysCardPath = path;});
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_ARCADE_CARD: return optionArcadeCard.readFromIO(io, readSize);
			case CFGKEY_6_BTN_PAD: return option6BtnPad.readFromIO(io, readSize);
		}
	}
	return false;
}

void PceSystem::writeConfig(ConfigType type, IO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeStringOptionValue(io, CFGKEY_SYSCARD_PATH, sysCardPath);
	}
	else if(type == ConfigType::SESSION)
	{
		optionArcadeCard.writeWithKeyIfNotDefault(io);
		option6BtnPad.writeWithKeyIfNotDefault(io);
	}
}

}
