/*
 * platform_x86_msvc_cpuid.h - x86 msvc cpuid code.
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

#ifndef PLATFORM_X86_MSVC_CPUID_H
#define PLATFORM_X86_MSVC_CPUID_H

#define cpuid(func, a, b, c, d) \
    __asm mov eax, func \
    __asm cpuid \
    __asm mov a, eax \
    __asm mov b, ebx \
    __asm mov c, ecx \
    __asm mov d, edx

inline static int has_cpuid(void)
{
    int result;

    __asm {
            pushfd
            pop     eax
            mov     ecx,    eax
            xor     eax,    0x200000
            push    eax
            popfd
            pushfd
            pop     eax
            xor     eax,    ecx
            mov     result, eax
            push    ecx
            popfd
    };
    return (result != 0);
}
#endif
