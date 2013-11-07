/*
 * platform_linux_runtime_os.c - Linux runtime version discovery.
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

/* Tested and confirmed working on:
*/

/* cpu | libc
   -----------
   x86 | libc4
   x86 | libc5
   x86 | glibc1.09
   x86 | glibc2.0
   x86 | glibc2.0.2
   x86 | glibc2.0.3
   x86 | glibc2.0.4
   x86 | glibc2.0.5
   x86 | glibc2.1.1
   x86 | glibc2.1.2
   x86 | glibc2.1.3
   x86 | glibc2.1.92
   x86 | glibc2.2
   x86 | glibc2.2.1
   x86 | glibc2.2.2
   x86 | glibc2.2.3
   x86 | glibc2.2.4
   x86 | glibc2.2.5
   x86 | dietlibc
   x86 | newlib
   x86 | musl
 */

#include "vice.h"

#ifdef __linux

#include <stdio.h>
#include <ctype.h>
#include <sys/utsname.h>

#if defined(__GLIBC__) && (__GLIBC__==2) && (__GLIBC__MINOR__>0)
#  include <gnu/libc-version.h>
#endif

static char linux_version[100];

char *platform_get_linux_runtime_os(void)
{
    struct utsname name;

    uname(&name);

    sprintf(linux_version, "%s %s", name.sysname, name.release);

#ifdef __dietlibc__
#define CLIB_HANDLED
    sprintf(linux_version, "%s (dietlibc)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && defined(_NEWLIB_VERSION)
#define CLIB_HANDLED
    sprintf(linux_version, "%s (newlib %s)", linux_version, _NEWLIB_VERSION);
#endif

#if !defined(CLIB_HANDLED) && defined(__GLIBC__)
#  define CLIB_HANDLED
#  if (__GLIBC__==2)
#    if (__GLIBC__MINOR__>0)
    sprintf(linux_version, "%s (glibc %s)", linux_version, gnu_get_libc_version());
#    else
    sprintf(linux_version, "%s (glibc 2.x)", linux_version);
#    endif
#  else
    sprintf(linux_version, "%s (glibc 1.x)", linux_version);
#  endif
#endif

#if !defined(CLIB_HANDLED) && defined(_LINUX_C_LIB_VERSION)
#  define CLIB_HANDLED
    sprintf(linux_version, "%s (libc %s)", linux_version, _LINUX_C_LIB_VERSION);
#endif

#if !defined(CLIB_HANDLED) && (VICE_LINUX_CLIB_VERSION_MAJOR==1)
#  define CLIB_HANDLED
    sprintf(linux_version, "%s (glibc 1.x)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && (VICE_LINUX_CLIB_VERSION_MAJOR==6)
#  define CLIB_HANDLED
    sprintf(linux_version, "%s (glibc 2.x)", linux_version);
#endif

#ifndef CLIB_HANDLED
#  include <sys/ucontext.h>
#  ifdef _UCONTEXT_H
#    define CLIB_HANDLED
    sprintf(linux_version, "%s (musl)", linux_version);
#  endif
#endif

#ifndef CLIB_HANDLED
    sprintf(linux_version, "%s (unknown libc)", linux_version);
#endif

    return linux_version;
}
#endif
