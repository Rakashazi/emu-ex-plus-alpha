#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

class EmuApp;

namespace EmuControls
{
extern const unsigned gamepadKeys;
}

extern Base::ApplicationContext appCtx;
extern Byte1Option optionArcadeCard;
extern Byte1Option option6BtnPad;
extern FS::PathString sysCardPath;
extern std::array<uint16, 5> inputBuff;

void set6ButtonPadEnabled(EmuApp &, bool);
bool hasHuCardExtension(std::string_view name);

namespace MDFN_IEN_PCE_FAST
{
void applyVideoFormat(Mednafen::EmulateSpecStruct *);
void applySoundFormat(Mednafen::EmulateSpecStruct *);
extern vce_t vce;
}

extern Mednafen::MDFNGI EmulatedPCE_Fast;
static Mednafen::MDFNGI *emuSys = &EmulatedPCE_Fast;
