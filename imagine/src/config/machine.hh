#pragma once

#include <config.h>
#include <config/imagineTypes.h>

namespace Config
{

enum { GENERIC, GENERIC_X86, GENERIC_ARM, GENERIC_ARMV7, OPEN_PANDORA, OUYA };
static const uint MACHINE =
#if defined(CONFIG_MACHINE_GENERIC_X86)
	GENERIC_X86
#elif defined(CONFIG_MACHINE_GENERIC_ARM)
	GENERIC_ARM
#elif defined(CONFIG_MACHINE_GENERIC_ARMV7)
	GENERIC_ARMV7
#elif defined(CONFIG_MACHINE_OUYA)
	OUYA
#elif defined(CONFIG_MACHINE_OPEN_PANDORA)
	OPEN_PANDORA
#else
	GENERIC
#endif
;

static const bool MACHINE_IS_GENERIC_X86 = MACHINE == GENERIC_X86;
static const bool MACHINE_IS_GENERIC_ARM = MACHINE == GENERIC_ARM;
static const bool MACHINE_IS_GENERIC_ARMV7 = MACHINE == GENERIC_ARMV7;
static const bool MACHINE_IS_OUYA = MACHINE == OUYA;
static const bool MACHINE_IS_OPEN_PANDORA = MACHINE == OPEN_PANDORA;

}
