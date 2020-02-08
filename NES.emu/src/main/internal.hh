#pragma once

#include <emuframework/Option.hh>
#include <fceu/driver.h>

namespace EmuControls
{
extern const uint gamepadKeys;
}

extern FS::PathString fdsBiosPath;
extern Byte1Option optionFourScore;
extern SByte1Option optionInputPort1;
extern SByte1Option optionInputPort2;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionDefaultVideoSystem;
extern Byte1Option optionSpriteLimit;
extern Byte1Option optionSoundQuality;
extern FS::PathString defaultPalettePath;
extern ESI nesInputPortDev[2];
extern uint autoDetectedRegion;
extern uint32 zapperData[3];
extern bool usingZapper;

bool hasFDSBIOSExtension(const char *name);
void setupNESInputPorts();
void setupNESFourScore();
void connectNESInput(int port, ESI type);
const char *regionToStr(int region);
void emulateSound(bool renderAudio);
void setDefaultPalette(const char *palPath);
void setRegion(int region, int defaultRegion, int detectedRegion);
