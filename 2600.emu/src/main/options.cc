/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

// TODO: Some Stella types collide with MacTypes.h
#define BytePtr BytePtrMac
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#undef BytePtr
#undef Debugger
#ifdef Success
#undef Success // conflict with macro in X11 headers
#endif
#include "internal.hh"

enum
{
	CFGKEY_2600_TV_PHOSPHOR = 270, CFGKEY_VIDEO_SYSTEM = 271,
	CFGKEY_2600_TV_PHOSPHOR_BLEND = 272
};

const char *EmuSystem::configFilename = "2600emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);
Byte1Option optionTVPhosphor{CFGKEY_2600_TV_PHOSPHOR, TV_PHOSPHOR_AUTO, false, optionIsValidWithMax<2>};
Byte1Option optionTVPhosphorBlend{CFGKEY_2600_TV_PHOSPHOR_BLEND, 80, false, optionIsValidWithMax<100>};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<6>};

bool EmuSystem::resetSessionOptions()
{
	optionTVPhosphor.reset();
	setRuntimeTVPhosphor(optionTVPhosphor, optionTVPhosphorBlend);
	optionVideoSystem.reset();
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_2600_TV_PHOSPHOR: optionTVPhosphor.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionTVPhosphor.writeWithKeyIfNotDefault(io);
	optionTVPhosphorBlend.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_2600_TV_PHOSPHOR_BLEND: optionTVPhosphorBlend.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionTVPhosphorBlend.writeWithKeyIfNotDefault(io);
}

const char *optionVideoSystemToStr()
{
	switch((int)optionVideoSystem)
	{
		case 1: return "NTSC";
		case 2: return "PAL";
		case 3: return "SECAM";
		case 4: return "NTSC50";
		case 5: return "PAL60";
		case 6: return "SECAM60";
		default: return "AUTO";
	}
}

void setRuntimeTVPhosphor(int val, int blend)
{
	if(!EmuSystem::gameIsRunning() || !osystem->hasConsole())
	{
		return;
	}
	// change runtime phosphor value
	bool usePhosphor = false;
	if(val == TV_PHOSPHOR_AUTO)
	{
		usePhosphor = defaultGameProps.get(Display_Phosphor) == "YES";
	}
	else
	{
		usePhosphor = val;
	}
	logMsg("Phosphor effect %s", usePhosphor ? "on" : "off");
	auto props = osystem->console().properties();
	if(usePhosphor)
	{
		props.set(Display_Phosphor, "Yes");
	}
	else
	{
		props.set(Display_Phosphor, "No");
	}
	osystem->console().setProperties(props);
	osystem->frameBuffer().tiaSurface().enablePhosphor(usePhosphor, blend);
}
