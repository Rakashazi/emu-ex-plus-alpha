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


/** \brief  Various OS-identification macros
 *
 * The question marks indicate ports I have my doubts about they'll even run
 * VICE at all.
 *
 * <pre>
 *  ARCHDEP_OS_UNIX
 *    ARCHDEP_OS_MACOS
 *    ARCHDEP_OS_LINUX
 *    ARCHDEP_OS_BSD
 *      ARCHDEP_OS_BSD_FREE
 *      ARCHDEP_OS_BSD_NET
 *      ARCHDEP_OS_BSD_OPEN
 *      ARCHDEP_OS_BSD_DRAGON
 *  ARCHDEP_OS_WINDOWS
 *  ARCHDEP_OS_BEOS
 * </pre>
 */
#ifdef UNIX_COMPILE

/** \brief  OS is UNIX */
# define ARCHDEP_OS_UNIX

# if defined(MACOSX_SUPPORT)

/** \brief  OS is Unix and MacOS */
#  define ARCHDEP_OS_MACOS

# elif defined(__linux__)

/** \brief  OS is Linux (yay!) */
#  define ARCHDEP_OS_LINUX

# elif defined(__FreeBSD__)

/** \brief  OS is BSD */
#  define ARCHDEP_OS_BSD

/** \brief  OS is FreeBSD */
#  define ARCHDEP_OS_BSD_FREE

# elif defined(__NetBSD__)

#  define ARCHDEP_OS_BSD
/** \brief  OS is NetBSD */
#  define ARCHDEP_OS_BSD_NET

# elif defined(__OpenBSD__)
#  define ARCHDEP_OS_BSD

/** \brief  OS is OpenBSD */
#  define ARCHDEP_OS_BSD_OPEN

# elif defined(__DragonFly__)
#  define ARCHDEP_OS_BSD


/** \brief  OS is DragonFly BSD */
#  define ARCHDEP_OS_BSD_DRAGON

# endif /* ifdef UNIX_COMPILE */

#elif defined(WIN32_COMPILE)

/** \brief  OS is Windows */
# define ARCHDEP_OS_WINDOWS

#elif defined(BEOS_COMPILE)

/** \brief  OS is in the BeOS family */
# define ARCHDEP_OS_BEOS

# if defined(__BEOS__)

/** \brief  Assume OS is BeOS R5 compatible (if we still support that) */
#  define ARCHDEP_OS_BEOS_R5

# elif defined(__HAIKU__)

/** \brief  OS is Haiku */
#  define ARCHDEP_OS_HAIKU

# endif /* ifdef BEOS_COMPILE */

#endif


/** \brief  Arch-dependent directory separator used in paths
 */
#if defined(ARCHDEP_OS_WINDOWS)

/** \brief  OS-dependent directory separator
 */
# define ARCHDEP_DIR_SEPARATOR  '\\'
#else
# define ARCHDEP_DIR_SEPARATOR  '/'
#endif

/** \brief  Extension used for autostart disks
 */
#define ARCHDEP_AUTOSTART_DISK_EXTENSION    "d64"


#if defined(ARCHDEP_OS_WINDOWS)
/** \brief  Separator used for a pathlist
 */
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ";"

#else

/** \brief  Separator used for a pathlist
 */
# define ARCHDEP_FINDPATH_SEPARATOR_STRING  ":"
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
#if defined(USE_SDLUI) || defined(USE_SDLUI2)
# define ARCHDEP_USE_SDL
#endif

#if defined(ARCHDEP_OS_WINDOWS) \
    || defined(ARCHDEP_OS_BEOS)
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

/** \def    GULONG_TO_POINTER
 * \brief   Cast gulong to pointer
 *
 * GLib appears to be "missing" this macro, so we define it here.
 *
 * \param[in]   ul  gulong value
 *
 * \return  gpointer
 */

#ifndef GULONG_TO_POINTER
# define GULONG_TO_POINTER(ul) (gpointer)(uintptr_t)(ul)
#endif


/** \def    GPOINTER_TO_ULONG
 * \brief   Cast pointer to gulong
 *
 * GLib appears to be "missing" this macro, so we define it here.
 *
 * \param[in]   p   gpointer
 *
 * \return  gulong
 */

#ifndef GPOINTER_TO_ULONG
# define GPOINTER_TO_ULONG(p) (gulong)(uintptr_t)(p)
#endif

#endif
