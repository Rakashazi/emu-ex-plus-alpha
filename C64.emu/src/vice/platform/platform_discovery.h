/*
 * platform_discovery.h - port/platform specific discovery macros.
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

#ifndef VICE_PLATFORM_DISCOVERY_H
#define VICE_PLATFORM_DISCOVERY_H

#include "vice.h"

#include "uiapi.h"

#if !defined(WIN32_COMPILE) && (defined(__CYGWIN32__) || defined(__CYGWIN__))
#include <cygwin/version.h>
#endif

#undef XQUOTE
#undef QUOTE
#define QUOTE(x) XQUOTE(x)
#define XQUOTE(x) #x


/* win32/64 discovery */
#ifdef WIN32_COMPILE
#  ifdef _WIN64
#    ifdef _M_IA64
#      ifndef PLATFORM_CPU
#        define PLATFORM_CPU "IA64"
#      endif
#    else
#      ifdef _M_ARM
#        ifndef PLATFORM_CPU
#          define PLATFORM_CPU "ARM"
#          define PLATFORM_OS "WINRT"
#        endif
#      else
#        ifndef PLATFORM_CPU
#          define PLATFORM_CPU "X64"
#        endif
#      endif
#    endif
#    ifndef PLATFORM_OS
#      define PLATFORM_OS "WIN64"
#    endif
#  else
#    ifdef WINMIPS
#      define PLATFORM_CPU "MIPS"
#      define PLATFORM_OS "WIN32"
#      ifndef PLATFORM_COMPILER
#        define PLATFORM_COMPILER "MSVC"
#      endif
#    else
#      ifndef __GNUC__
#        ifdef MSVC_RC
#          ifdef WATCOM_COMPILE
#            ifndef PLATFORM_COMPILER
#              define PLATFORM_COMPILER "WATCOM"
#            endif
#          else
#            ifndef PLATFORM_COMPILER
#              define PLATFORM_COMPILER "MSVC"
#            endif
#          endif
#        endif
#      endif
#      ifndef PLATFORM_OS
#        define PLATFORM_OS "WIN32"
#      endif
#      define FIND_X86_CPU
#    endif
#  endif
#endif


/* Cygwin discovery */
#if !defined(WIN32_COMPILE) && (defined(__CYGWIN32__) || defined(__CYGWIN__))
#  define PLATFORM_OS "Cygwin API " QUOTE(CYGWIN_VERSION_API_MAJOR) "." QUOTE(CYGWIN_VERSION_API_MINOR)
#  define FIND_X86_CPU
#endif


/* Interix/SFU/SUA discovery */
#ifdef __INTERIX
#  include <interix/registry.h>
#  include <interix/interix.h>
#  ifndef ROOT_KEY_SYSWOW
#    ifndef PTHREAD_CHUNK_SIZE
#      define PLATFORM_OS "Interix 3.0"
#    else
#      define PLATFORM_OS "Interix 3.5"
#    endif
#  else
#    ifndef REGVAL_ENABLE_SU_TO_ROOT
#      define PLATFORM_OS "Interix 5.2"
#    else
#      include <net/if.h>
#      if (IF_NAMESIZE==127)
#        define PLATFORM_OS "Interix 6.0"
#      else
#        define PLATFORM_OS "Interix 6.1/6.2"
#      endif
#    endif
#  endif
#  define FIND_X86_CPU
#endif


/* Syllable discovery */
#ifdef __SYLLABLE__
#  ifdef __GLIBC__
#    define PLATFORM_OS "Syllable glibc " QUOTE(__GLIBC__) "." QUOTE(__GLIBC_MINOR__)
#  else
#    define PLATFORM_OS "Syllable"
#  endif
#endif


/* MacOS X discovery */
#if defined(__APPLE__) && !defined(RHAPSODY_COMPILE) && !defined(DARWIN_COMPILE)
#  include "platform_macosx.h"
#endif


/* Darwin discovery */
#ifdef DARWIN_COMPILE
#  define PLATFORM_OS "Darwin"
#endif


/* NextStep discovery */
#ifdef NEXTSTEP_COMPILE
#  define PLATFORM_OS "NextStep"
#endif


/* OpenStep discovery */
#ifdef OPENSTEP_COMPILE
#  define PLATFORM_OS "OpenStep"
#endif


/* Rhapsody discovery */
#ifdef RHAPSODY_COMPILE
#  define PLATFORM_OS "Rhapsody"
#endif


/* Sortix discovery */
#ifdef __sortix__
#  define PLATFORM_OS "Sortix"
#endif


/* AIX discovery */
#ifdef _AIX
#  include "platform_aix_version.h"
#endif


/* AmigaOS 3.x discovery */
#ifdef AMIGA_M68K
#  if defined(__VBCC__) && defined(__PPC__)
#    include <proto/powerpc.h>
#    ifdef _VBCCINLINE_POWERPC_H
#      define PLATFORM_OS "WarpOS (AmigaOS 3.x)"
#    else
#      define PLATFORM_OS "PowerUP (AmigaOS 3.x)"
#    endif
#  else
#    define PLATFORM_OS "AmigaOS 3.x"
#  endif
#endif


/* AmigaOS 4.x discovery */
#ifdef AMIGA_OS4
#  define PLATFORM_OS "AmigaOS 4.x"
#  define PLATFORM_CPU "PPC"
#endif


/* AROS discovery */
#ifdef AMIGA_AROS
#  define PLATFORM_OS "AROS"
#endif


/* MorphOS discovery */
#ifdef AMIGA_MORPHOS
#  define PLATFORM_OS "MorphOS"
#endif


/* Android discovery */
#ifdef __ANDROID__
#  define PLATFORM_OS "Android"
#endif


/* BeOS discovery */
#ifdef __BEOS__
#  ifdef __MWERKS__
#    define PLATFORM_CPU "PPC"
#    define PLATFORM_COMPILER "MetroWerks"
#  else
#    define FIND_X86_CPU
#  endif
#  ifdef __ZETA__
#    define PLATFORM_OS "Zeta"
#  else
#    define PLATFORM_OS "BeOS"
#  endif
#endif /* __BEOS__ */


/* Haiku discovery */
#ifdef __HAIKU__
#  define PLATFORM_OS "Haiku"
#  define FIND_X86_CPU
#endif


/* BSDI discovery */
#ifdef __bsdi__
#  define PLATFORM_OS "BSDi"
#  define FIND_X86_CPU
#endif


/* DragonFly BSD discovery */
#ifdef __DragonFly__
#  define FIND_X86_CPU
#  include "platform_dragonfly_version.h"
#endif


/* FreeBSD discovery */
#if defined(__FreeBSD__) && !defined(__DragonFly__)
#  include "platform_freebsd_version.h"
#endif


/* NetBSD discovery */
#ifdef __NetBSD__
#  include "platform_netbsd_version.h"
#endif


/* OpenBSD discovery */
#ifdef __OpenBSD__
#  include "platform_openbsd_version.h"
#endif


/* QNX 4.x discovery */
#if defined(__QNX__) && !defined(__QNXNTO__)
#  define PLATFORM_OS "QNX 4.x"
#  define PLATFORM_COMPILER "Watcom"
#  define FIND_X86_CPU
#endif


/* QNX 6.x discovery */
#ifdef __QNXNTO__
#  include "platform_qnx6_version.h"
#  ifdef __arm__
#    define PLATFORM_CPU "ARMLE"
#  endif
#  ifdef __mips__
#    define PLATFORM_CPU "MIPSLE"
#  endif
#  ifdef __sh__
#    define PLATFORM_CPU "SHLE"
#  endif
#  if defined(__powerpc__) || defined(__ppc__)
#    define PLATFORM_CPU "PPCBE"
#  endif
#  ifndef PLATFORM_CPU
#    define FIND_X86_CPU
#  endif
#endif


/* HPUX discovery */
#if defined(__hpux) || defined(_hpux)
#  define PLATFORM_OS "HPUX"
#endif


/* IRIX discovery */
#ifdef __sgi
#  define PLATFORM_OS "IRIX"
#endif


/* SCO Unix 4.x discovery */
#ifdef SCO4UNIX_COMPILE
#  ifdef __GNU_LIBRARY__
#    define PLATFORM_OS "SCO Unix 4.x (glibc 1.x)"
#  else 
#    define PLATFORM_OS "SCO Unix 4.x"
#  endif
#  define FIND_X86_CPU
#endif


/* OpenServer 5.x discovery */
#ifdef OPENSERVER5_COMPILE
#  define PLATFORM_OS "OpenServer 5.x"
#  define FIND_X86_CPU
#endif


/* OpenServer 6.x discovery */
#ifdef OPENSERVER6_COMPILE
#  define PLATFORM_OS "OpenServer 6.x"
#  define FIND_X86_CPU
#endif


/* UnixWare 7.x discovery */
#ifdef UNIXWARE_COMPILE
#  define PLATFORM_OS "UnixWare 7.x"
#  define FIND_X86_CPU
#endif


/* SunOS and Solaris discovery */
#if defined(sun) || defined(__sun)
#  if defined(__SVR4) || defined(__svr4__)
#    include "platform_solaris_version.h"
#  else
#    define PLATFORM_OS "SunOS"
#  endif
#endif


/* AMIX discovery */
#ifdef __AMIX__
#  define PLATFORM_OS "AMIX"
#endif

/* Linux discovery */
#if defined(__linux) && !defined(__ANDROID__) && !defined(AMIGA_AROS)
#  include "platform_linux_libc_version.h"
#endif


/* DYNIX discovery */
#ifdef _SEQUENT_
#  define PLATFORM_OS "DYNIX"
#endif


/* GNU Hurd discovery */
#if defined(__GNU__) && !defined(NEXTSTEP_COMPILE) && !defined(OPENSTEP_COMPILE)
#  include <mach/version.h>
#  include <features.h>
#  define PLATFORM_OS "GNU Hurd " QUOTE(KERNEL_MAJOR_VERSION) "." QUOTE(KERNEL_MINOR_VERSION) " (glibc " QUOTE(__GLIBC__) "." QUOTE(__GLIBC_MINOR__) ")"
#endif


/* LynxOS discovery */
#ifdef __Lynx__
#  define PLATFORM_OS "LynxOS"
#endif


/* SkyOS discovery */
#ifdef __SKYOS__
#  define PLATFORM_OS "SkyOS"
#endif


/* Minix discovery */
#ifdef __minix
#  ifdef __minix_vmd
#    define PLATFORM_OS "Minix-vmd"
#  else
#    include <minix/config.h>
#    if defined(OS_RELEASE) && defined(OS_VERSION)
#      define PLATFORM_OS "Minix " OS_RELEASE "." OS_VERSION
#    else
#      define PLATFORM_OS "Minix"
#    endif
#  endif
#endif


/* DOS discovery */
#ifdef __MSDOS__
#  define PLATFORM_OS "DOS"
#endif


/* OS/2 discovery */
#ifdef __OS2__
#  define PLATFORM_OS "OS/2"
#endif


/* Sinux (Reliant UNIX) discovery */
#ifdef sinux
#  define PLATFORM_OS "Sinux"
#endif


/* Symbian OS discovery */
#ifdef __SYMBIAN32__
#  define PLATFORM_OS "Symbian OS"
#endif


/* Tru64 discovery */
#if defined(__osf__) || defined(__osf)
#  define PLATFORM_OS "Tru64"
#endif


/* Ultrix discovery */
#if defined(ultrix) || defined(__ultrix) || defined(__ultrix__)
#  define PLATFORM_OS "Ultrix"
#endif


/* VMS discovery */
#if defined(VMS) || defined(__VMS)
#  define PLATFORM_OS "VMS"
#endif


/* XBOX(360) discovery */
#ifdef _XBOX
#  ifdef _XBOX_VER
#    if (_XBOX_VER>199)
#      define PLATFORM_OS "XBOX360"
#    else
#      define PLATFORM_OS "XBOX"
#    endif
#  endif
#endif


/* vxworks discovery */
#ifdef __vxworks__
#  define PLATFORM_OS "VXWORKS"
#endif


/* System V Release 4 discovery */
#if !defined(PLATFORM_OS) && defined(__svr4__)
#define PLATFORM_OS "Unix System V Release 4"
#endif


/* Generic cpu discovery */
#include "platform_cpu_type.h"

/* Generic compiler discovery */
#include "platform_compiler.h"

/* Fallbacks for unidentified systems */
#ifndef PLATFORM_CPU
#  define PLATFORM_CPU "unknown CPU"
#endif

#ifndef PLATFORM_OS
#  define PLATFORM_OS "unknown OS"
#endif

#ifndef PLATFORM_COMPILER
#  define PLATFORM_COMPILER "unknown compiler"
#endif

#ifndef PLATFORM
#  define PLATFORM PLATFORM_OS " " PLATFORM_CPU " " PLATFORM_COMPILER
#endif

#endif
