/*
 * platform_x86_watcom_cpuid.h - watcom cpuid code.
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

#ifndef PLATFORM_X86_WATCOM_CPUID_H
#define PLATFORM_X86_WATCOM_CPUID_H

#include <stdint.h>

#define EFL_CPUID           (1L << 21)      /* The CPUID bit in EFLAGS. */

static uint32_t cpu_info_stuff[4];

static void cpu_id( uint32_t cpuinfo[4], uint32_t infotype );
#pragma aux cpu_id =      \
    ".586"                \
    "cpuid"               \
    "mov  [esi+0],eax"    \
    "mov  [esi+4],ebx"    \
    "mov  [esi+8],ecx"    \
    "mov  [esi+12],edx"   \
    parm [esi] [eax] modify [ebx ecx edx];

#define cpuid(func, a, b, c, d)   \
    cpu_id(cpu_info_stuff, func); \
    a = cpu_info_stuff[0];        \
    b = cpu_info_stuff[1];        \
    c = cpu_info_stuff[2];        \
    d = cpu_info_stuff[3];

/* Read the EFLAGS register. */
static uint32_t eflags_read(void);
#pragma aux eflags_read = \
    "pushfd"              \
    "pop  eax"            \
    value [eax] modify [eax];

/* Write the EFLAGS register. */
static uint32_t eflags_write(uint32_t eflg);
#pragma aux eflags_write = \
    "push eax"             \
    "popfd"                \
    parm [eax] modify [];

inline static int has_cpuid(void)
{
    uint32_t old_eflg;
    uint32_t new_eflg;

    old_eflg = eflags_read();
    new_eflg = old_eflg ^ EFL_CPUID;
    eflags_write(new_eflg);
    new_eflg = eflags_read();
    return (new_eflg != old_eflg);
}
#endif
