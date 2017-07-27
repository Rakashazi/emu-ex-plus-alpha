#pragma once

#include <emuframework/Option.hh>
#include <port.h>
#ifndef SNES9X_VERSION_1_4
#include <controls.h>
#endif

namespace EmuControls
{
extern const uint gamepadKeys;
}

extern Byte1Option optionMultitap;
extern Byte1Option optionVideoSystem;
#ifndef SNES9X_VERSION_1_4
extern Byte1Option optionBlockInvalidVRAMAccess;
#endif
extern int snesInputPort;
extern uint doubleClickFrames, rightClickFrames;

#ifndef SNES9X_VERSION_1_4
static const int SNES_AUTO_INPUT = 255;
static const int SNES_JOYPAD = CTL_JOYPAD;
static const int SNES_MOUSE_SWAPPED = CTL_MOUSE;
static const int SNES_SUPERSCOPE = CTL_SUPERSCOPE;
extern int snesActiveInputPort;
#else
static const int &snesActiveInputPort = snesInputPort;
#endif

void setupSNESInput();

#ifndef SNES9X_VERSION_1_4
uint16 *S9xGetJoypadBits(uint idx);
uint8 *S9xGetMouseBits(uint idx);
uint8 *S9xGetMouseDeltaBits(uint idx);
int16 *S9xGetMousePosBits(uint idx);
int16 *S9xGetSuperscopePosBits();
uint8 *S9xGetSuperscopeBits();
CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons);
#endif
