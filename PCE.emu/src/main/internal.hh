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
extern uint16 inputBuff[5];

bool hasHuCardExtension(const char *name);

namespace PCE_Fast
{
	void applyVideoFormat(MDFN_Surface &espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern bool AVPad6Enabled[5];
	extern vce_t vce;
}
