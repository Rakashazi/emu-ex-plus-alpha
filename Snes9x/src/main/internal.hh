#pragma once

#include <emuframework/Option.hh>

extern Byte1Option optionMultitap;
#ifndef SNES9X_VERSION_1_4
extern Byte1Option optionBlockInvalidVRAMAccess;
#endif
extern int snesInputPort;

#ifndef SNES9X_VERSION_1_4
extern const int SNES_AUTO_INPUT;
extern const int SNES_JOYPAD;
extern const int SNES_MOUSE_SWAPPED;
extern const int SNES_SUPERSCOPE;
#endif

void setupSNESInput();
