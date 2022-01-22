#pragma once

#include <emuframework/Option.hh>

namespace IG
{
class ApplicationContext;
}

namespace EmuEx
{

class EmuVideo;
class EmuAudio;

static const unsigned RTC_EMU_AUTO = 0, RTC_EMU_OFF = 1, RTC_EMU_ON = 2;

extern Byte1Option optionRtcEmulation;
extern bool detectedRtcGame;

void setRTC(unsigned mode);
void readCheatFile(IG::ApplicationContext);
void writeCheatFile(IG::ApplicationContext);

}

struct GBASys;

void setGameSpecificSettings(GBASys &gba, int romSize);
void CPULoop(GBASys &, EmuEx::EmuSystemTaskContext, EmuEx::EmuVideo *, EmuEx::EmuAudio *);
void CPUCleanUp();
bool CPUReadBatteryFile(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteBatteryFile(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUReadState(IG::ApplicationContext, GBASys &gba, const char *);
bool CPUWriteState(IG::ApplicationContext, GBASys &gba, const char *);
