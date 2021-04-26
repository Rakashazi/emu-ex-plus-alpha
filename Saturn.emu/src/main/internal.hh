#pragma once

#include <emuframework/Option.hh>

extern "C"
{
	#include <yabause/yabause.h>
	#include <yabause/sh2core.h>
	#include <yabause/peripheral.h>
}

namespace EmuControls
{
static const unsigned gamepadKeys = 23;
}

extern Byte1Option optionSH2Core;
extern FS::PathString biosPath;
extern SH2Interface_struct *SH2CoreList[];
extern unsigned SH2Cores;
extern yabauseinit_struct yinit;
extern const int defaultSH2CoreID;
extern PerPad_struct *pad[2];

bool hasBIOSExtension(const char *name);
