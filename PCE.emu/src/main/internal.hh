#pragma once

#include <emuframework/Option.hh>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

namespace EmuControls
{
extern const uint gamepadKeys;
}

extern Byte1Option optionArcadeCard;
extern FS::PathString sysCardPath;
extern std::array<uint16, 5> inputBuff;
extern bool useSixButtonPad;

bool hasHuCardExtension(const char *name);

namespace PCE_Fast
{
	void applyVideoFormat(EmulateSpecStruct *espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern vce_t vce;
}
