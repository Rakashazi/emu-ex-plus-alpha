#pragma once

#include <emuframework/Option.hh>
#include <fceu/driver.h>

namespace EmuControls
{
extern const uint gamepadKeys;
}

extern FS::PathString fdsBiosPath;
extern PathOption optionFdsBiosPath;
extern Byte1Option optionFourScore;
extern Byte1Option optionVideoSystem;
extern ESI nesInputPortDev[2];
extern uint autoDetectedVidSysPAL;
extern uint32 zapperData[3];

bool hasFDSBIOSExtension(const char *name);
void setupNESInputPorts();
void setupNESFourScore();
void connectNESInput(int port, ESI type);
