#pragma once

#include <emuframework/Option.hh>
#include "genplus-config.h"

extern t_config config;

namespace EmuEx::Controls
{
extern const unsigned gamepadKeys;
}

namespace EmuEx
{

class EmuApp;

extern unsigned playerIdxMap[4];
extern int8 mdInputPortDev[2];
extern Byte1Option optionBigEndianSram;
extern Byte1Option optionSmsFM;
extern Byte1Option option6BtnPad;
extern Byte1Option optionMultiTap;
extern SByte1Option optionInputPort1;
extern SByte1Option optionInputPort2;
extern Byte1Option optionRegion;
#ifndef NO_SCD
extern FS::PathString cdBiosUSAPath, cdBiosJpnPath, cdBiosEurPath;
#endif
extern Byte1Option optionVideoSystem;

void setupMDInput(EmuApp &);
bool hasMDExtension(std::string_view name);

}
