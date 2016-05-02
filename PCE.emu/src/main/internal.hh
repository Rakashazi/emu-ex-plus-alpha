#pragma once

#include <emuframework/Option.hh>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

extern Byte1Option optionArcadeCard;
extern FS::PathString sysCardPath;

bool hasHuCardExtension(const char *name);

namespace PCE_Fast
{
	void applyVideoFormat(MDFN_Surface &espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern bool AVPad6Enabled[5];
	extern vce_t vce;
}
