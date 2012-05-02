#pragma once

#include <config.h>
#include <config/imagineTypes.h>

namespace Config
{

// TODO: have to use ANDROID_ for now since ANDROID is needed as a macro in some system headers not yet using __ANDROID__
enum { UNKNOWN, ANDROID_, IOS, MACOSX, WEBOS, LINUX, PS3 };
static const uint ENV =
#if defined(CONFIG_BASE_ANDROID)
	ANDROID_
#elif defined(CONFIG_BASE_IOS)
	IOS
#elif defined(CONFIG_BASE_MACOSX)
	MACOSX
#elif defined(CONFIG_ENV_WEBOS)
	WEBOS
#elif defined(CONFIG_ENV_LINUX)
	LINUX
#elif defined(CONFIG_BASE_PS3)
	PS3
#else
	#warning "Unknown ENV type"
	UNKNOWN
#endif
;

static const bool envIsAndroid = ENV == ANDROID_;
static const bool envIsIOS = ENV == IOS;
static const bool envIsMacOSX = ENV == MACOSX;
static const bool envIsWebOS = ENV == WEBOS;
static const bool envIsLinux = ENV == LINUX;
static const bool envIsPS3 = ENV == PS3;

static const bool envIsWebOS3 =
#if CONFIG_ENV_WEBOS_OS == 3
	1
#else
	0
#endif
;
}

#if CONFIG_ENV_ANDROID_MINSDK == 4
	#define ENV_NOTE "Android 1.6"
#elif CONFIG_ENV_ANDROID_MINSDK == 5
	#define ENV_NOTE "Android 2-2.2"
#elif CONFIG_ENV_ANDROID_MINSDK == 9
	#define ENV_NOTE "Android 2.3+"
#endif
