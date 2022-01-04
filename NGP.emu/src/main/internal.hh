#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>

namespace EmuEx
{

extern IG::ApplicationContext appCtx;
extern Byte1Option optionNGPLanguage;
extern uint8_t inputBuff;

}

namespace MDFN_IEN_NGP
{
void applyVideoFormat(Mednafen::EmulateSpecStruct *);
void applySoundFormat(Mednafen::EmulateSpecStruct *);
}

extern Mednafen::MDFNGI EmulatedNGP;
static Mednafen::MDFNGI *emuSys = &EmulatedNGP;
