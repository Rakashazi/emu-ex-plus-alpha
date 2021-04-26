#pragma once

#include <emuframework/Option.hh>
#include <fceu/driver.h>

class EmuAudio;

namespace EmuControls
{
extern const unsigned gamepadKeys;
}

extern FS::PathString fdsBiosPath;
extern Byte1Option optionFourScore;
extern SByte1Option optionInputPort1;
extern SByte1Option optionInputPort2;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionDefaultVideoSystem;
extern Byte1Option optionSpriteLimit;
extern Byte1Option optionSoundQuality;
extern Byte1Option optionCompatibleFrameskip;
extern FS::PathString defaultPalettePath;
extern ESI nesInputPortDev[2];
extern unsigned autoDetectedRegion;
extern uint32 zapperData[3];
extern bool usingZapper;

bool hasFDSBIOSExtension(const char *name);
void setupNESInputPorts();
void setupNESFourScore();
void connectNESInput(int port, ESI type);
const char *regionToStr(int region);
void emulateSound(EmuAudio *audio);
void setDefaultPalette(Base::ApplicationContext, const char *palPath);
void setRegion(int region, int defaultRegion, int detectedRegion);
