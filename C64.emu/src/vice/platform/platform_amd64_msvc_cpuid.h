/*
 * platform_amd64_msvc_cpuid.h - amd64 msvc cpuid code.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef PLATFORM_AMD64_MSVC_CPUID_H
#define PLATFORM_AMD64_MSVC_CPUID_H

#ifndef __INTERIX
#  include <intrin.h>
#endif

static int cpu_info_stuff[4];

void __cpuid(
   int CPUInfo[4],
   int InfoType
);

#define cpuid(func, a, b, c, d)    \
    __cpuid(cpu_info_stuff, func); \
    a = cpu_info_stuff[0];         \
    b = cpu_info_stuff[1];         \
    c = cpu_info_stuff[2];         \
    d = cpu_info_stuff[3];

inline static int has_cpuid(void)
{
    return 1;
}
#endif
