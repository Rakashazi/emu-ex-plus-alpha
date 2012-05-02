#pragma once

#include <util/ansiTypes.h>

#ifndef CONFIG_ENV_ANDROID_MINSDK
	#warning "CONFIG_ENV_ANDROID_MINSDK not defined, using 9"
	#define CONFIG_ENV_ANDROID_MINSDK 9
#endif

namespace Base
{

const uint aMinSDK = CONFIG_ENV_ANDROID_MINSDK;
uint androidSDK();

}
