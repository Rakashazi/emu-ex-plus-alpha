#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/build.h>
#include <imagine/config/imagineTypes.h>
#if defined __APPLE__
#include <TargetConditionals.h>
#endif
#include "machine.hh"

namespace Config
{

// TODO: have to use ANDROID_ for now since ANDROID is needed as a macro in some system headers not yet using __ANDROID__
enum { UNKNOWN, ANDROID_, IOS, MACOSX, WEBOS, LINUX, WIN32_, PS3 };
static const uint ENV =
	#if defined __ANDROID__
	ANDROID_;
	#elif defined __APPLE__ && TARGET_OS_IPHONE
	IOS;
	#elif defined __APPLE__ && TARGET_OS_MAC
	MACOSX;
	#elif defined CONFIG_ENV_WEBOS
	WEBOS;
	#elif defined __linux__
	LINUX;
	#elif defined _WIN32
	WIN32_;
	#elif defined CONFIG_BASE_PS3
	PS3;
	#else
	#warning "Unknown ENV type"
	UNKNOWN;
	#endif

static const bool envIsAndroid = ENV == ANDROID_;
static const bool envIsIOS = ENV == IOS;
static const bool envIsMacOSX = ENV == MACOSX;
static const bool envIsWebOS = ENV == WEBOS;
static const bool envIsLinux = ENV == LINUX;
static const bool envIsPS3 = ENV == PS3;

static const bool envIsWebOS3 =
	#if CONFIG_ENV_WEBOS_OS >= 3
	1;
	#else
	0;
	#endif
static const bool envIsWebOS1 = envIsWebOS && !envIsWebOS3;

static const uint ENV_ANDROID_MINSDK =
	#if CONFIG_ENV_ANDROID_MINSDK
	CONFIG_ENV_ANDROID_MINSDK;
	#else
	0;
	#endif

#define CONFIG_UNICODE_CHARS
static const bool UNICODE_CHARS = true;

#if defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_IOS || defined CONFIG_ENV_WEBOS
#define CONFIG_BASE_CAN_BACKGROUND_APP
static const bool BASE_CAN_BACKGROUND_APP = 1;
#else
static const bool BASE_CAN_BACKGROUND_APP = 0;
#endif

#ifndef NDEBUG
static constexpr bool DEBUG_BUILD = true;
#else
static constexpr bool DEBUG_BUILD = false;
#endif
}

#ifdef CONFIG_MACHINE_OUYA
#define ENV_NOTE "OUYA"
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
#elif defined __aarch64__
#define CONFIG_ARCH_STR "aarch64"
#elif __arm__
	#if defined __ARM_ARCH_7S__
	#define CONFIG_ARCH_STR "armv7s"
	#elif defined __ARM_ARCH_5TE__
	// default Android "ARM" profile
	#define CONFIG_ARCH_STR "armv5te"
	#elif __ARM_ARCH == 7
	// default Android & iOS ARMv7 profile -> __ARM_ARCH_7A__
	#define CONFIG_ARCH_STR "armv7"
	#elif __ARM_ARCH == 6
	// default iOS ARMv6 profile -> __ARM_ARCH_6K__
	// default WebOS ARMv6 profile -> __ARM_ARCH_6J__
	#define CONFIG_ARCH_STR "armv6"
	#else
	#define CONFIG_ARCH_STR "arm"
	#endif
#else
#warning Compiling on unknown architecture
#endif
