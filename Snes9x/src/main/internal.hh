#pragma once

#include <emuframework/Option.hh>
#include <port.h>
#ifndef SNES9X_VERSION_1_4
#include <controls.h>
#endif

namespace EmuControls
{
extern const unsigned gamepadKeys;
}

#ifdef SNES9X_VERSION_1_4
static constexpr bool IS_SNES9X_VERSION_1_4 = true;
#else
static constexpr bool IS_SNES9X_VERSION_1_4 = false;
#endif

extern Byte1Option optionMultitap;
extern SByte1Option optionInputPort;
extern Byte1Option optionVideoSystem;
#ifndef SNES9X_VERSION_1_4
extern Byte1Option optionBlockInvalidVRAMAccess;
extern Byte1Option optionSeparateEchoBuffer;
extern Byte1Option optionSuperFXClockMultiplier;
extern Byte1Option optionAudioDSPInterpolation;
#endif
extern int snesInputPort;
extern unsigned doubleClickFrames, rightClickFrames;

#ifndef SNES9X_VERSION_1_4
static const int SNES_AUTO_INPUT = -1;
static const int SNES_JOYPAD = CTL_JOYPAD;
static const int SNES_MOUSE_SWAPPED = CTL_MOUSE;
static const int SNES_SUPERSCOPE = CTL_SUPERSCOPE;
extern int snesActiveInputPort;
#else
static const int &snesActiveInputPort = snesInputPort;
#endif

void setupSNESInput();
void setSuperFXSpeedMultiplier(unsigned val);

#ifndef SNES9X_VERSION_1_4
uint16 *S9xGetJoypadBits(unsigned idx);
uint8 *S9xGetMouseBits(unsigned idx);
uint8 *S9xGetMouseDeltaBits(unsigned idx);
int16 *S9xGetMousePosBits(unsigned idx);
int16 *S9xGetSuperscopePosBits();
uint8 *S9xGetSuperscopeBits();
CLINK bool8 S9xReadMousePosition(int which, int &x, int &y, uint32 &buttons);
#endif

void checkAndEnableGlobalCheats();
uint32_t numCheats();
