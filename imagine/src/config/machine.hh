#pragma once

#include <config.h>
#include <config/imagineTypes.h>

namespace Config
{

enum { GENERIC, GENERIC_X86, GENERIC_ARM, GENERIC_ARMV7, PANDORA, OUYA };
static constexpr uint MACHINE =
	#if defined(CONFIG_MACHINE_GENERIC_X86)
	GENERIC_X86;
	#elif defined(CONFIG_MACHINE_GENERIC_ARM)
	GENERIC_ARM;
	#elif defined(CONFIG_MACHINE_GENERIC_ARMV7)
	GENERIC_ARMV7;
	#elif defined(CONFIG_MACHINE_OUYA)
	OUYA;
	#elif defined(CONFIG_MACHINE_PANDORA)
	PANDORA;
	#else
	GENERIC;
	#endif

static constexpr bool MACHINE_IS_GENERIC_X86 = MACHINE == GENERIC_X86;
static constexpr bool MACHINE_IS_GENERIC_ARM = MACHINE == GENERIC_ARM;
static constexpr bool MACHINE_IS_GENERIC_ARMV7 = MACHINE == GENERIC_ARMV7;
static constexpr bool MACHINE_IS_OUYA = MACHINE == OUYA;
static constexpr bool MACHINE_IS_PANDORA = MACHINE == PANDORA;

}
