#pragma once

#include <emuframework/Option.hh>
#include <fceu/driver.h>

extern FS::PathString fdsBiosPath;
extern PathOption optionFdsBiosPath;
extern Byte1Option optionFourScore;
extern Byte1Option optionVideoSystem;
extern ESI nesInputPortDev[2];
extern uint autoDetectedVidSysPAL;

bool hasFDSBIOSExtension(const char *name);
void setupNESInputPorts();
void setupNESFourScore();
