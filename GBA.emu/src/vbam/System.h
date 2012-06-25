#ifndef SYSTEM_H
#define SYSTEM_H

#include "common/Types.h"

#include <zlib.h>

class SoundDriver;

struct EmulatedSystem {
  // main emulation function
  void (*emuMain)(bool renderGfx, bool processGfx, bool renderAudio);
  // reset emulator
  void (*emuReset)();
  // clean up memory
  void (*emuCleanUp)();
  // load battery file
  bool (*emuReadBattery)(const char *);
  // write battery file
  bool (*emuWriteBattery)(const char *);
  // load state
  bool (*emuReadState)(const char *);
  // save state
  bool (*emuWriteState)(const char *);
  // load memory state (rewind)
  bool (*emuReadMemState)(char *, int);
  // write memory state (rewind)
  bool (*emuWriteMemState)(char *, int);
  // write PNG file
  //bool (*emuWritePNG)(const char *);
  // write BMP file
  //bool (*emuWriteBMP)(const char *);
  // emulator update CPSR (ARM only)
  void (*emuUpdateCPSR)();
  // emulator has debugger
  bool emuHasDebugger;
  // clock ticks to emulate
  //int emuCount;
};

extern void log(const char *,...);

extern bool systemPauseOnFrame();
extern void systemGbPrint(u8 *,int,int,int,int,int);
extern void systemScreenCapture(int);
extern void systemDrawScreen();
// updates the joystick data
extern bool systemReadJoypads();
// return information about the given joystick, -1 for default joystick
extern u32 systemReadJoypad(int);
extern u32 systemGetClock();
#ifndef NDEBUG
extern void systemMessage(int, const char *, ...);
#else
#define systemMessage(i, s, ...) ({ })
#endif
extern void systemSetTitle(const char *);
extern SoundDriver * systemSoundInit();
extern void systemOnWriteDataToSoundBuffer(const u16 * finalWave, int length);
extern void systemOnSoundShutdown();
extern void systemScreenMessage(const char *);
extern void systemUpdateMotionSensor();
extern int  systemGetSensorX();
extern int  systemGetSensorY();
extern bool systemCanChangeSoundQuality();
extern void systemShowSpeed(int);
extern void system10Frames(int);
extern void systemFrame();
extern void systemGbBorderOn();

extern void Sm60FPS_Init();
extern bool Sm60FPS_CanSkipFrame();
extern void Sm60FPS_Sleep();
extern void DbgMsg(const char *msg, ...);
#ifdef SDL
#define winlog log
#else
extern void winlog(const char *,...);
#endif

extern void (*dbgOutput)(const char *s, u32 addr);
extern void (*dbgSignal)(int sig,int number);

#define SUPPORT_PIX_16BIT
union SystemColorMap
{
#ifdef SUPPORT_PIX_32BIT
	u32 map32[0x10000];
#endif
#ifdef SUPPORT_PIX_16BIT
	u16 map16[0x10000];
#endif
};
extern SystemColorMap systemColorMap;
extern u16 systemGbPalette[24];
extern int systemRedShift;
extern int systemGreenShift;
extern int systemBlueShift;
extern int systemColorDepth;
extern int systemDebug;
static const int systemVerbose = 0;
extern int systemSaveUpdateCounter;
extern int systemSpeed;

#define SYSTEM_SAVE_UPDATED 30
#define SYSTEM_SAVE_NOT_UPDATED 0

#endif // SYSTEM_H
