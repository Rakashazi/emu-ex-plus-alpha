#pragma once

#include <emuframework/Option.hh>
#include <fceu/driver.h>

namespace EmuEx::Controls
{
extern const unsigned gamepadKeys;
}

namespace EmuEx
{

class EmuAudio;
class EmuSystem;

extern FS::PathString fdsBiosPath;
extern Byte1Option optionFourScore;
extern SByte1Option optionInputPort1;
extern SByte1Option optionInputPort2;
extern Byte1Option optionVideoSystem;
extern Byte1Option optionDefaultVideoSystem;
extern Byte1Option optionSpriteLimit;
extern Byte1Option optionSoundQuality;
extern Byte1Option optionCompatibleFrameskip;
extern Byte1Option optionStartVideoLine;
extern Byte1Option optionVisibleVideoLines;
extern Byte1Option optionHorizontalVideoCrop;
extern FS::PathString defaultPalettePath;
extern ESI nesInputPortDev[2];
extern unsigned autoDetectedRegion;
extern uint32 zapperData[3];
extern bool usingZapper;

bool hasFDSBIOSExtension(std::string_view name);
void setupNESInputPorts();
void setupNESFourScore();
void connectNESInput(int port, ESI type);
const char *regionToStr(int region);
void emulateSound(EmuAudio *audio);
void setDefaultPalette(IG::ApplicationContext, IG::CStringView palPath);
void setRegion(int region, int defaultRegion, int detectedRegion);
void updateVideoPixmap(EmuVideo &, bool horizontalCrop, int lines);

}
