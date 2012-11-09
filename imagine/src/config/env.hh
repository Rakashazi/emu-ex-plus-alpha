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
#if CONFIG_ENV_WEBOS_OS >= 3
	1
#else
	0
#endif
;

static const uint ENV_ANDROID_MINSDK =
#if CONFIG_ENV_ANDROID_MINSDK
		CONFIG_ENV_ANDROID_MINSDK
#else
		0
#endif
;

#if (defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK < 9)
	static const bool UNICODE_CHARS = 0;
#else
	#define CONFIG_UNICODE_CHARS
	static const bool UNICODE_CHARS = 1;
#endif
}

#if CONFIG_ENV_ANDROID_MINSDK == 4
	#define ENV_NOTE "Android 1.6"
#elif CONFIG_ENV_ANDROID_MINSDK == 5
	#define ENV_NOTE "Android 2-2.2"
#elif CONFIG_ENV_ANDROID_MINSDK == 9
	#define ENV_NOTE "Android 2.3+"
#endif

#ifdef CONFIG_ENV_WEBOS
	#if CONFIG_ENV_WEBOS_OS >= 3
	 #define ENV_NOTE "WebOS 3.0+"
	#elif CONFIG_ENV_WEBOS_OS < 3
	 #define ENV_NOTE "WebOS 1.x-2.x"
	#endif
#endif

// Platform architecture

#if defined __x86_64__
	#define CONFIG_ARCH_STR "x86_64"
#elif defined __i386__
	#define CONFIG_ARCH_STR "x86"
#elif defined __powerpc__
	#define CONFIG_ARCH_STR "ppc"
#elif defined __mips__
	#define CONFIG_ARCH_STR "mips"
#elif __arm__
	#if defined __ARM_ARCH_7A__
		// default Android & iOS ARMv7 profile
		#define CONFIG_ARCH_STR "armv7"
	#elif defined __ARM_ARCH_7S__
		#define CONFIG_ARCH_STR "armv7s"
	#elif defined __ARM_ARCH_6__ || defined __ARM_ARCH_6K__ || defined __ARM_ARCH_6J__
		// default iOS ARMv6 profile -> __ARM_ARCH_6K__
		// default WebOS ARMv6 profile -> __ARM_ARCH_6J__
		#define CONFIG_ARCH_STR "armv6"
	#elif defined __ARM_ARCH_5TE__
		// default Android "ARMv6" profile
		#define CONFIG_ARCH_STR "armv5"
	#else
		#define CONFIG_ARCH_STR "arm"
	#endif
#else
	#warning Compiling on unknown architecture
#endif
