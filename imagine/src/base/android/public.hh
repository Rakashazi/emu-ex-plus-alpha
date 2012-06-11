#pragma once

#include "sdk.hh"

namespace Base
{

bool hasSurfaceTexture();
void disableSurfaceTexture();

}

// Bluez dlsym functions
extern bool bluez_loaded;
CallResult bluez_dl();

extern fbool glSyncHackBlacklisted, glSyncHackEnabled;
extern fbool glPointerStateHack, glBrokenNpot;
