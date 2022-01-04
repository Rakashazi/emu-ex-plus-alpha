#pragma once

#include <emuframework/Option.hh>

extern "C"
{
	#include <yabause/yabause.h>
	#include <yabause/sh2core.h>
	#include <yabause/peripheral.h>
}

namespace EmuEx::Controls
{
static const unsigned gamepadKeys = 23;
}

extern const int defaultSH2CoreID;
extern SH2Interface_struct *SH2CoreList[];

namespace EmuEx
{

extern Byte1Option optionSH2Core;
extern FS::PathString biosPath;
extern unsigned SH2Cores;
extern yabauseinit_struct yinit;
extern PerPad_struct *pad[2];

bool hasBIOSExtension(std::string_view name);

}
