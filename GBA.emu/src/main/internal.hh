#pragma once

#include <emuframework/Option.hh>

static const unsigned RTC_EMU_AUTO = 0, RTC_EMU_OFF = 1, RTC_EMU_ON = 2;

extern Byte1Option optionRtcEmulation;
extern bool detectedRtcGame;

void setRTC(unsigned mode);
void readCheatFile();
void writeCheatFile();
