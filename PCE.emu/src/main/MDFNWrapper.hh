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
	CFGKEY_PCEKEY_UP = 256, CFGKEY_PCEKEY_RIGHT = 257,
	CFGKEY_PCEKEY_DOWN = 258, CFGKEY_PCEKEY_LEFT = 259,
	CFGKEY_PCEKEY_SELECT = 260, CFGKEY_PCEKEY_RUN = 261,
	CFGKEY_PCEKEY_I = 262, CFGKEY_PCEKEY_II = 263,
	CFGKEY_PCEKEY_III = 264, CFGKEY_PCEKEY_IV = 265,
	CFGKEY_PCEKEY_V = 266, CFGKEY_PCEKEY_VI = 267,
	CFGKEY_PCEKEY_MODE_SELECT = 268,
	CFGKEY_PCEKEY_LEFT_UP = 269, CFGKEY_PCEKEY_RIGHT_UP = 270,
	CFGKEY_PCEKEY_RIGHT_DOWN = 271, CFGKEY_PCEKEY_LEFT_DOWN = 272,
	CFGKEY_PCEKEY_I_TURBO = 273, CFGKEY_PCEKEY_II_TURBO = 274,

	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
};
