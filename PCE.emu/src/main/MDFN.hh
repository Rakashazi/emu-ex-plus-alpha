#pragma once

#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/md5.h>
#include <mednafen/cdrom/cdromif.h>
#include <mednafen/state-driver.h>

extern MDFNGI EmulatedPCE_Fast;
static MDFNGI *emuSys = &EmulatedPCE_Fast;

enum {
	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
};
