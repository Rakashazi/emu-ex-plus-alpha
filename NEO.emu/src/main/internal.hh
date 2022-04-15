#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

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

void setTimerIntOption(EmuSystem &);

}

constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags SRAM_DIRTY_BIT = IG::bit(0);
constexpr EmuEx::EmuSystem::BackupMemoryDirtyFlags MEMCARD_DIRTY_BIT = IG::bit(1);

CLINK CONFIG conf;
