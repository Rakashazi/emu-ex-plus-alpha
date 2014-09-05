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

namespace Config
{

enum { GENERIC, GENERIC_X86, GENERIC_ARMV6, GENERIC_ARMV7, GENERIC_AARCH64, PANDORA, OUYA };
static constexpr uint MACHINE =
	#if defined CONFIG_MACHINE_GENERIC_X86
	GENERIC_X86;
	#elif defined CONFIG_MACHINE_GENERIC_ARMV6
	GENERIC_ARMV6;
	#elif defined CONFIG_MACHINE_GENERIC_ARMV7
	GENERIC_ARMV7;
	#elif defined CONFIG_MACHINE_GENERIC_AARCH64
	GENERIC_AARCH64;
	#elif defined CONFIG_MACHINE_OUYA
	OUYA;
	#elif defined CONFIG_MACHINE_PANDORA
	PANDORA;
	#else
	GENERIC;
	#endif

static constexpr bool MACHINE_IS_GENERIC_X86 = MACHINE == GENERIC_X86;
static constexpr bool MACHINE_IS_GENERIC_ARMV6 = MACHINE == GENERIC_ARMV6;
static constexpr bool MACHINE_IS_GENERIC_ARMV7 = MACHINE == GENERIC_ARMV7;
static constexpr bool MACHINE_IS_GENERIC_AARCH64 = MACHINE == GENERIC_AARCH64;
static constexpr bool MACHINE_IS_OUYA = MACHINE == OUYA;
static constexpr bool MACHINE_IS_PANDORA = MACHINE == PANDORA;

}
