#pragma once

#include <emuframework/Option.hh>

extern "C"
{
	#include <gngeo/emu.h>
}

namespace EmuEx::Controls
{
static const unsigned joystickKeys = 19;
}

namespace EmuEx
{

extern Byte1Option optionListAllGames;
extern Byte1Option optionBIOSType;
extern Byte1Option optionMVSCountry;
extern Byte1Option optionTimerInt;
extern Byte1Option optionCreateAndUseCache;
extern Byte1Option optionStrictROMChecking;

void setTimerIntOption();

}

CLINK CONFIG conf;
