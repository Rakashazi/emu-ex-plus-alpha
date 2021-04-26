#pragma once

#include <emuframework/Option.hh>

extern "C"
{
	#include <gngeo/emu.h>
}

namespace EmuControls
{
static const unsigned joystickKeys = 19;
}

extern Byte1Option optionListAllGames;
extern Byte1Option optionBIOSType;
extern Byte1Option optionMVSCountry;
extern Byte1Option optionTimerInt;
extern Byte1Option optionCreateAndUseCache;
extern Byte1Option optionStrictROMChecking;
CLINK CONFIG conf;

void setTimerIntOption();
