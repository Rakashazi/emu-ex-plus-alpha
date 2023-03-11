/** \file   archdep_defs.h
 * \brief   Defines, enums and types used by the archdep functions
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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


#ifndef VICE_ARCHDEP_DEFS_H
#define VICE_ARCHDEP_DEFS_H

#include "vice.h"
#include <inttypes.h>


/* \brief  Various OS-identification macros
 *
 * The question marks indicate ports I have my doubts about they'll even run
 * VICE at all.
 *
 * XXX:     Non of the ARCHDEP_OS_* macros should be left when we've finished
 *          the archdep cleanup!
 * TODO:    Move this comment to either src/vice.h or configure.ac.
 *
 * <pre>
 *  UNIX_COMPILE
 *    MACOS_COMPILE
 *    LINUX_COMPILE
 *    BSD_COMPILE
 *      FREEBSD_COMPILE
 *      NETBSD_COMPILE
 *      OPENBSD_COMPILE
 *      DRAGONFLYBSD_COMPILE
 *  WINDOWS_COMPILE
 *  BEOS_COMPILE
 *    HAIKU_COMPILE
 * </pre>
 */


/** \brief  Extension used for autostart disks
 */
#define ARCHDEP_AUTOSTART_DISK_EXTENSION    "d64"


#if defined(WINDOWS_COMPILE)
/** \brief  Separator used for a pathlist
 */
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ";"

#else

/** \brief  Separator used for a pathlist
 */
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ":"
#endif


/** \def ARCHDEP_PATH_MAX
 *
 * \brief   Arch-independent replacement for PATH_MAX/MAX_PATH
 *
 * The maximum size of a pathname.
 *
 * Note that there are some serious flaws with PATH_MAX and similar constants,
 * so only use this if dynamically allocating memory isn't possible. There also
 * doesn't seem to be consencus on whether PATH_MAX is enough to hold the
 * longest possible path including the terminating NUL character.
 */

#ifdef WINDOWS_COMPILE
# include <stdlib.h>
# define ARCHDEP_PATH_MAX   _MAX_PATH
#elif defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
# include <limits.h>
/* Not sure we need this fallback: on FreeBSD, NetBSD and OpenBSD using
 * `#include <limits.h>` worked even with -pedantic -ansi passed to the compiler.
 */
#if 0
# ifndef PATH_MAX
#  if defined(BSD_COMPILE) || defined(MACOS_COMPILE)
#   include <sys/syslimits.h>
#  endif
# endif
#endif
# define ARCHDEP_PATH_MAX   PATH_MAX
#else
/* Paniek! */
# define ARCHDEP_PATH_MAX   4096
#endif


/** \brief  XDG Base Directory Specifiction user cache dir
 *
 * This defines only the final element of the `XDG_CACHE_HOME` variable.
 */
#define ARCHDEP_XDG_CACHE_HOME  ".cache"


/** \brief  XDG Base Directory Specifiction user config dir
 *
 * This defines only the final element of `the XDG_CONFIG_HOME` variable.
 */
#define ARCHDEP_XDG_CONFIG_HOME ".config"


/** \def    ARCHDEP_VICERC_NAME
 * \brief   The name of the default VICE configuraton file
 */

/** \def    ARCHDEP_VICE_RTC_NAME
 * \brief   The name of the default VICE RTC status file
 */

/*
 * Determine if we compile against SDL
 */
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
# define ARCHDEP_USE_SDL
#endif

#if defined(WINDOWS_COMPILE) || defined(BEOS_COMPILE)
# ifdef ARCHDEP_USE_SDL
#  define ARCHDEP_VICERC_NAME   "sdl-vice.ini"
/* Just copying stuff, I'm backwards */
#  define ARCHDEP_VICE_RTC_NAME "sdl-vice.rtc"
# else
#  define ARCHDEP_VICERC_NAME   "vice.ini"
#  define ARCHDEP_VICE_RTC_NAME "vice.rtc"
# endif
#else
# ifdef ARCHDEP_USE_SDL
#  define ARCHDEP_VICERC_NAME   "sdl-vicerc"
#  define ARCHDEP_VICE_RTC_NAME "sdl-vice.rtc"
# else
#  define ARCHDEP_VICERC_NAME   "vicerc"
#  define ARCHDEP_VICE_RTC_NAME "vice.rtc"
# endif
#endif

/** \brief  Autostart diskimage prefix
 */
#define ARCHDEP_AUTOSTART_DISKIMAGE_PREFIX  "autostart-"

/** \brief  Autostart diskimage suffix
 */
#define ARCHDEP_AUTOSTART_DISKIMAGE_SUFFIX  ".d64"


/* Declare extra printf specifiers for Windows since Microsoft's support for
 * C99 fucking sucks.
 *
 * This declares PRI_SIZE_T and PRI_SSIZE_T for use on all platforms,
 * aliasing to 'zu'/'z' on anything not Windows, and using PRI[d|u][32|64] on
 * Windows.
 */

/** \def    PRI_SIZE_T
 * \brief   Printf type specifier alias for 'zu'
 *
 * Required to work around Microsoft's broken C99 support.
 */

/** \def    PRI_SSIZE_T
 * \brief   Printf type specifier alias for 'zd'
 *
 * Required to work around Microsoft's broken C99 support.
 */

#ifdef _WIN32
# define PRI_SIZE_T     "Iu"
# define PRI_SSIZE_T    "Id"
#else
# define PRI_SIZE_T     "zu"
# define PRI_SSIZE_T    "zd"
#endif

#endif
