#pragma once

#include <mednafen/mednafen.h>
#include <mednafen/general.h>
#include <mednafen/hash/md5.h>
#include <mednafen/cdrom/CDInterface.h>
#include <mednafen/state-driver.h>

extern Mednafen::MDFNGI EmulatedPCE_Fast;
static Mednafen::MDFNGI *emuSys = &EmulatedPCE_Fast;
