#pragma once

// fix name collisions with mac types
#define Fixed MacTypes_Fixed
#define Rect MacTypes_Rect
#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/md5.h>
#include <mednafen/cdrom/cdromif.h>
#include <mednafen/state-driver.h>
#undef Fixed
#undef Rect

extern MDFNGI EmulatedPCE_Fast;
static MDFNGI *emuSys = &EmulatedPCE_Fast;

enum {
	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
};
