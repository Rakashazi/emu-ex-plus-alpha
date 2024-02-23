/*! \file vice.h
 *
 *  \brief Main header file for VICE.
 *
 *  \author Ettore Perazzoli <ettore@comm2000.it>
 *  \author Jouko Valta <jopi@stekt.oulu.fi>
 *  \author Andreas Boose <viceteam@t-online.de>
 */

/*
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

#ifndef VICE_VICE_H
#define VICE_VICE_H

#include <vice-config.h>

/* ------------------------------------------------------------------------- */

/* Portability... */

/* currently tested/testing for the following cpu types:
 *
 * (please let's get rid of this, I personally enjoy making stuff work on OS's
 * I don't expect to support it, but VICE should just support OS's people
 * actually use. (Windows, Linux, MacOS, BSD)
 *
 * cpu        4*u_char fetch   1*u_int32 fetch   define(s)
 * -----      --------------   ---------------   ---------
 * alpha          faster           slower        __alpha__
 * arm (gp2x)     slower (*)       faster (*)    GP2X
 * ppc            slower           faster        __powerpc__ || __ppc__
 * x86            slower           faster        __i386__
 * m68020+        slower           faster        __m680[2346]0__
 * x86_64         slower           faster        __x86_64__ || __amd64__
 *
 * arm           untested         untested       __arm__ && !GP2X
 * bfin          untested         untested       BFIN
 * hppa          untested         untested       ???
 * ia64          untested         untested       __ia64__
 * m88k          untested         untested       ???
 * mips          untested         untested       __mips__
 * s390          untested         untested       __s390__
 * s390x         untested         untested       __s390x__
 * sparc         untested         untested       sparc
 * sparc64       untested         untested       ???
 * vax           untested         untested       __vax__
 */

/* Allow unaligned access for i386+ based platforms */
#ifdef __i386__
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for amd64/x86_64 based platforms */
#if defined(__x86_64__) || defined(__amd64__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for m68020+ based platforms */
#if defined(__m68020__) || defined(__m68030__) || defined(__m68040__) || defined(__m68060__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for PPC based platforms */
#if defined(__powerpc__) || defined(__ppc__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for ARMv6 and newer
#if __ARM_ARCH >= 6
#define ALLOW_UNALIGNED_ACCESS
#endif

/* Allow unaligned access for AArch64 */
#if defined(__aarch64__)
#define ALLOW_UNALIGNED_ACCESS
#endif

/* ------------------------------------------------------------------------- */

#if (defined(BEOS_COMPILE) && defined(WORDS_BIGENDIAN))
#ifndef __cplusplus
#undef inline
#define inline
#endif
#endif

#ifdef USE_GCC
#define int64_t_C(c) (c ## ll)
#define uint64_t_C(c) (c ## ull)
#endif

/* Avoid windows.h including too much garbage
 */
#ifdef WINDOWS_COMPILE
# define WIN32_LEAN_AND_MEAN
#endif

/* Provide define for checking 32/64-bit Windows
 *
 * No need for WIN32_COMPILE, just use !WIN64_COMPILE. Avoid confusion with the
 * old WIN23_COMPILE define.
 */
#ifdef WINDOWS_COMPILE
# ifdef _WIN64
#  define WIN64_COMPILE
# endif
#endif

/* some attribute defines that are useful mostly for static analysis */
/* see https://clang.llvm.org/docs/AttributeReference.html */
#ifdef __clang__
#define VICE_ATTR_NORETURN  __attribute__((analyzer_noreturn))
#elif defined(__GNUC__)
#define VICE_ATTR_NORETURN  __attribute__((noreturn))
#else
#define VICE_ATTR_NORETURN
#endif

/* format checking attributes for printf style functions */
#if defined(__GNUC__)
/* like regular printf, func(format, ...) */
#define VICE_ATTR_PRINTF    __attribute__((format(printf, 1, 2)))
/* one extra param on the left, func(param, format, ...) */
#define VICE_ATTR_PRINTF2   __attribute__((format(printf, 2, 3)))
/* two extra param on the left, func(param, param, format, ...) */
#define VICE_ATTR_PRINTF3   __attribute__((format(printf, 3, 4)))
/* one extra param after the format, func(format, param, ...) (used for resource sprintf) */
#define VICE_ATTR_RESPRINTF __attribute__((format(printf, 1, 3)))
#else
#define VICE_ATTR_PRINTF
#define VICE_ATTR_PRINTF2
#define VICE_ATTR_PRINTF3
#define VICE_ATTR_RESPRINTF
#endif

/* M_PI is non-standard, so in order for -std=c99 to work we define it here */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
