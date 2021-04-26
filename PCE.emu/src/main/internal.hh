#pragma once

#include <emuframework/Option.hh>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

namespace EmuControls
{
extern const unsigned gamepadKeys;
}

extern Byte1Option optionArcadeCard;
extern Byte1Option option6BtnPad;
extern FS::PathString sysCardPath;
extern std::array<uint16, 5> inputBuff;

bool hasHuCardExtension(const char *name);

namespace PCE_Fast
{
	void applyVideoFormat(EmulateSpecStruct *espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern vce_t vce;
}
