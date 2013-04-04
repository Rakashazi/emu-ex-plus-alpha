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
bool supportsAndroidSurfaceTextureWhitelisted();
bool useAndroidSurfaceTexture();
void setUseAndroidSurfaceTexture(bool on);

}

namespace Base
{

void setProcessPriority(int nice);
int processPriority();
bool apkSignatureIsConsistent();
const char *androidBuildDevice();

}

extern bool glSyncHackBlacklisted, glSyncHackEnabled;
extern bool glPointerStateHack, glBrokenNpot;
