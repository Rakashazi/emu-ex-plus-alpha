#pragma once

#include "sdk.hh"

namespace Base
{

bool hasSurfaceTexture();
void disableSurfaceTexture();

}

// Bluez dlsym functions
CallResult bluez_dl();

extern fbool glSyncHackBlacklisted, glSyncHackEnabled;
extern fbool glPointerStateHack, glBrokenNpot;
