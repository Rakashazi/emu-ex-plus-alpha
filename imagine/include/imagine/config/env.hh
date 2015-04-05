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
#ifdef __ANDROID__
#include <android/api-level.h>
#endif

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
	#ifdef __ANDROID_API__
	__ANDROID_API__;
	#else
	0;
	#endif

#define CONFIG_UNICODE_CHARS
static const bool UNICODE_CHARS = true;

#ifndef NDEBUG
static constexpr bool DEBUG_BUILD = true;
#else
static constexpr bool DEBUG_BUILD = false;
#endif

#ifdef CONFIG_MACHINE_OUYA
#define ENV_NOTE "OUYA"
#elif defined __ANDROID__
#define ENV_NOTE "Android"
#endif

#ifdef CONFIG_ENV_WEBOS
	#if CONFIG_ENV_WEBOS_OS >= 3
	#define ENV_NOTE "WebOS 3.0+"
	#elif CONFIG_ENV_WEBOS_OS < 3
	#define ENV_NOTE "WebOS 1.x-2.x"
	#endif
#endif

// Platform architecture & machine

enum Machine
{
	GENERIC,
	GENERIC_X86,
	GENERIC_X86_64,
	GENERIC_ARM,
	GENERIC_ARMV5,
	GENERIC_ARMV6,
	GENERIC_ARMV7,
	GENERIC_ARMV7S,
	GENERIC_AARCH64,
	GENERIC_PPC,
	GENERIC_MIPS,
	PANDORA,
	OUYA
};

#if defined __x86_64__
static constexpr Machine MACHINE = GENERIC_X86_64;
#define CONFIG_ARCH_STR "x86_64"
#elif defined __i386__
static constexpr Machine MACHINE = GENERIC_X86;
#define CONFIG_ARCH_STR "x86"
#elif defined __powerpc__
static constexpr Machine MACHINE = GENERIC_PPC;
#define CONFIG_ARCH_STR "ppc"
#elif defined __mips__
static constexpr Machine MACHINE = GENERIC_MIPS;
#define CONFIG_ARCH_STR "mips"
#elif defined __aarch64__
static constexpr Machine MACHINE = GENERIC_AARCH64;
#define CONFIG_ARCH_STR "aarch64"
#elif __arm__
	#if defined __ARM_ARCH_7S__
	static constexpr Machine MACHINE = GENERIC_ARMV7S;
	#define CONFIG_ARCH_STR "armv7s"
	#elif defined __ARM_ARCH_5TE__
	// default Android "ARM" profile
	static constexpr Machine MACHINE = GENERIC_ARMV5;
	#define CONFIG_ARCH_STR "armv5te"
	#elif __ARM_ARCH == 7
	// default Android & iOS ARMv7 profile -> __ARM_ARCH_7A__
		#if defined CONFIG_MACHINE_OUYA
		static constexpr Machine MACHINE = OUYA;
		#elif defined CONFIG_MACHINE_PANDORA
		static constexpr Machine MACHINE = PANDORA;
		#else
		static constexpr Machine MACHINE = GENERIC_ARMV7;
		#endif
	#define CONFIG_ARCH_STR "armv7"
	#elif __ARM_ARCH == 6
	// default iOS ARMv6 profile -> __ARM_ARCH_6K__
	// default WebOS ARMv6 profile -> __ARM_ARCH_6J__
	static constexpr Machine MACHINE = GENERIC_ARMV6;
	#define CONFIG_ARCH_STR "armv6"
	#else
	static constexpr Machine MACHINE = GENERIC_ARM;
	#define CONFIG_ARCH_STR "arm"
	#endif
#else
static constexpr Machine MACHINE = GENERIC;
#warning Compiling on unknown architecture
#endif

#ifdef __ARM_ARCH
static constexpr uint ARM_ARCH = __ARM_ARCH;
#else
static constexpr uint ARM_ARCH = 0;
#endif

static constexpr bool MACHINE_IS_GENERIC_X86 = MACHINE == GENERIC_X86;
static constexpr bool MACHINE_IS_GENERIC_ARMV6 = MACHINE == GENERIC_ARMV6;
static constexpr bool MACHINE_IS_GENERIC_ARMV7 = MACHINE == GENERIC_ARMV7;
static constexpr bool MACHINE_IS_GENERIC_AARCH64 = MACHINE == GENERIC_AARCH64;
static constexpr bool MACHINE_IS_OUYA = MACHINE == OUYA;
static constexpr bool MACHINE_IS_PANDORA = MACHINE == PANDORA;

}
