/*
 * platform_x86_runtime_cpu.h - x86 specific runtime discovery.
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

#ifndef PLATFORM_X86_RUNTIME_CPU_H
#define PLATFORM_X86_RUNTIME_CPU_H

/* Do not use for SkyOS */
#ifdef SKYOS
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Sortix */
#ifdef __sortix__
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for QNX 6.x */
#ifdef __QNXNTO__
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for MSVC IA64 */
#ifdef _M_IA64
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for MSVC ARM */
#ifdef _M_ARM
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Syllable */
#ifdef __SYLLABLE__
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Android */
#ifdef ANDROID_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for SCO Unix 4.x */
#ifdef SCO4UNIX_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Openserver 5.x */
#ifdef OPENSERVER5_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Openserver 6.x */
#ifdef OPENSERVER6_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Unixware 7.x */
#ifdef UNIXWARE_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Darwin */
#ifdef DARWIN_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for NextStep */
#ifdef NEXTSTEP_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for OpenStep */
#ifdef OPENSTEP_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for Rhapsody */
#ifdef RHAPSODY_COMPILE
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for MacOSX Cocoa */
#ifdef MACOSX_COCOA
#define PLATFORM_NO_X86_ASM
#endif

/* Do not use for uClibc */
#ifdef __UCLIBC__
#define PLATFORM_NO_X86_ASM
#endif

#ifdef __minix
#  ifdef __ACK__
#    define PLATFORM_NO_X86_ASM
#  else
#    ifndef __i386__
#      define __i386__
#    endif
#  endif
#endif

#if !defined(__i386__) && defined(__i386)
#  define __i386__
#endif

/* only use for supported cpu types */
#ifndef __i386__
#  ifndef __i486__
#    ifndef __i586__
#      ifndef __i686__
#        ifndef __amd64__
#          ifndef __x86_64__
#            ifndef _M_IX86
#              ifndef _M_X64
#                define PLATFORM_NO_X86_ASM
#              endif
#            endif
#          endif
#        endif
#      endif
#    endif
#  endif
#endif

#endif
