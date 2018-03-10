#pragma once

#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/hash/md5.h>
#include <mednafen/cdrom/cdromif.h>
#include <mednafen/state-driver.h>

extern MDFNGI EmulatedPCE_Fast;
static MDFNGI *emuSys = &EmulatedPCE_Fast;
