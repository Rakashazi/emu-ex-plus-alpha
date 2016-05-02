#pragma once

#include <emuframework/Option.hh>
#include "genplus-config.h"

extern int8 mdInputPortDev[2];
extern t_config config;
extern bool usingMultiTap;
extern Byte1Option optionBigEndianSram;
extern Byte1Option optionSmsFM;
extern Byte1Option option6BtnPad;
extern Byte1Option optionRegion;
#ifndef NO_SCD
extern FS::PathString cdBiosUSAPath, cdBiosJpnPath, cdBiosEurPath;
extern PathOption optionCDBiosUsaPath;
extern PathOption optionCDBiosJpnPath;
extern PathOption optionCDBiosEurPath;
#endif
extern Byte1Option optionVideoSystem;

void setupMDInput();
bool hasMDExtension(const char *name);
