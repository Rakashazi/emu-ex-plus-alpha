#pragma once

#include <emuframework/Option.hh>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

class EmuApp;

namespace EmuControls
{
extern const unsigned gamepadKeys;
}

extern Byte1Option optionArcadeCard;
extern Byte1Option option6BtnPad;
extern FS::PathString sysCardPath;
extern std::array<uint16, 5> inputBuff;

void set6ButtonPadEnabled(EmuApp &, bool);
bool hasHuCardExtension(IG::CStringView name);

namespace MDFN_IEN_PCE_FAST
{
	void applyVideoFormat(EmulateSpecStruct *espec);
	void applySoundFormat(EmulateSpecStruct *espec);
	extern vce_t vce;
}
