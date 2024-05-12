#ifndef VBAM_CORE_BASE_SYSTEM_H_
#define VBAM_CORE_BASE_SYSTEM_H_

#include <cstdint>
#include <memory>

#include "core/base/sound_driver.h"

namespace EmuEx
{
class EmuVideo;
class EmuAudio;
class EmuSystemTaskContext;
}

enum IMAGE_TYPE {
    IMAGE_UNKNOWN = -1,
    IMAGE_GBA = 0,
    IMAGE_GB = 1
};

struct EmulatedSystem {
  // main emulation function
	void (*emuMain)(int);
  // reset emulator
  void (*emuReset)();
  // clean up memory
  void (*emuCleanUp)();
  // load battery file
  bool (*emuReadBattery)(const char*);
  // write battery file
  bool (*emuWriteBattery)(const char*);
#ifdef __LIBRETRO__
  // load state
  bool (*emuReadState)(const uint8_t*);
  // load state
  unsigned (*emuWriteState)(uint8_t*);
#else
  // load state
  bool (*emuReadState)(const char*);
  // save state
  bool (*emuWriteState)(const char*);
#endif
  // load memory state (rewind)
  bool (*emuReadMemState)(char*, int);
  // write memory state (rewind)
  bool (*emuWriteMemState)(char*, int, long&);
  // write PNG file
  bool (*emuWritePNG)(const char*);
  // write BMP file
  bool (*emuWriteBMP)(const char*);
  // emulator update CPSR (ARM only)
  void (*emuUpdateCPSR)();
  // emulator has debugger
  bool emuHasDebugger;
  // clock ticks to emulate
  int emuCount;
};

// The `coreOptions` object must be instantiated by the embedder.
extern struct CoreOptions {
    bool cpuIsMultiBoot = false;
    static constexpr bool mirroringEnable = true;
    static constexpr bool skipBios = false;
    static constexpr bool parseDebug = true;
    static constexpr bool speedHack = false;
    static constexpr bool speedup = false;
    static constexpr bool speedup_throttle_frame_skip = false;
    static constexpr int cheatsEnabled = 0;
    static constexpr int cpuDisableSfx = 0;
    int cpuSaveType = 0;
    static constexpr int layerSettings = 0xff00;
    static constexpr int rtcEnabled = 1;
    int saveType = 0;
    static constexpr int skipSaveGameBattery = 0;
    static constexpr int skipSaveGameCheats = 0;
    int useBios = 0;
    static constexpr int winGbPrinterEnabled = 1;
    static constexpr uint32_t speedup_throttle = 100;
    static constexpr uint32_t speedup_frame_skip = 9;
    static constexpr uint32_t throttle = 100;
    static constexpr const char *loadDotCodeFile = nullptr;
    static constexpr const char *saveDotCodeFile = nullptr;
} coreOptions;

// The following functions must be implemented by the emulator.
extern void log(const char*, ...);
extern bool systemPauseOnFrame();
extern void systemGbPrint(uint8_t*, int, int, int, int, int);
extern void systemScreenCapture(int);
extern void systemSendScreen();
extern void systemDrawScreen(EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo &);
// updates the joystick data
extern bool systemReadJoypads();
// return information about the given joystick, -1 for default joystick
extern uint32_t systemReadJoypad(int);
extern uint32_t systemGetClock();
extern void systemSetTitle(const char*);
extern SoundDriver* systemSoundInit();
extern void systemOnWriteDataToSoundBuffer(EmuEx::EmuAudio* audio, const uint16_t* finalWave, int length);
extern void systemOnSoundShutdown();
extern void systemScreenMessage(const char*);
extern void systemUpdateMotionSensor();
extern int  systemGetSensorX();
extern int  systemGetSensorY();
extern int systemGetSensorZ();
extern uint8_t systemGetSensorDarkness();
extern void systemCartridgeRumble(bool);
extern void systemPossibleCartridgeRumble(bool);
extern void updateRumbleFrame();
extern bool systemCanChangeSoundQuality();
extern void systemShowSpeed(int);
extern void system10Frames();
extern void systemFrame();
extern void systemGbBorderOn();
extern void (*dbgOutput)(const char* s, uint32_t addr);
extern void (*dbgSignal)(int sig, int number);

union SystemColorMap
{
	uint32_t map32[0x10000];
	uint16_t map16[0x10000];
};
extern SystemColorMap systemColorMap;
extern uint16_t systemGbPalette[24];
extern int systemFrameSkip;
constexpr int systemVerbose = 0;
extern int systemSaveUpdateCounter;
extern int systemSpeed;
#define MAX_CHEATS 16384
#define SYSTEM_SAVE_UPDATED 30
#define SYSTEM_SAVE_NOT_UPDATED 0
#endif // VBAM_CORE_BASE_SYSTEM_H_
