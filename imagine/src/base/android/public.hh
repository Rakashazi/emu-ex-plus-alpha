#pragma once

#include "sdk.hh"

namespace Gfx
{

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	bool supportsAndroidDirectTexture();
	bool supportsAndroidDirectTextureWhitelisted();
	const char* androidDirectTextureError();
	bool useAndroidDirectTexture();
	void setUseAndroidDirectTexture(bool on);
#endif

bool supportsAndroidSurfaceTexture();
bool useAndroidSurfaceTexture();
void setUseAndroidSurfaceTexture(bool on);

}

extern fbool glSyncHackBlacklisted, glSyncHackEnabled;
extern fbool glPointerStateHack, glBrokenNpot;
