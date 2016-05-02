#pragma once

#include <emuframework/Option.hh>

extern "C"
{
	#include <yabause/yabause.h>
	#include <yabause/sh2core.h>
}

extern Byte1Option optionSH2Core;
extern FS::PathString biosPath;
extern SH2Interface_struct *SH2CoreList[];
extern uint SH2Cores;
extern yabauseinit_struct yinit;

bool hasBIOSExtension(const char *name);
