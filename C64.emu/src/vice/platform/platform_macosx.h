/*
 * platform_macosx.h - Mac OS X Platform detection
 *
 * Written by
 *  Christian Vogelgsang <chris@vogelgsang.org>
 *
 * Based on Code by
 *  http://www.cocoadev.com/index.pl?DeterminingOSVersion
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

#ifndef VICE_PLATFORM_MACOSX_H
#define VICE_PLATFORM_MACOSX_H

#include "AvailabilityMacros.h"

/* determine compile-time OS */
#if !defined(PLATFORM_OS) && defined MAC_OS_X_VERSION_10_12
#define PLATFORM_OS "MacOS Sierra (10.12)"
#endif

#if !defined(PLATFORM_OS) && defined MAC_OS_X_VERSION_10_11
#define PLATFORM_OS "Mac OS X 10.11 (El Capitan)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_10)
#define PLATFORM_OS "Mac OS X 10.10 (Yosemite)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_9)
#define PLATFORM_OS "Mac OS X 10.9 (Mavericks)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_8)
#define PLATFORM_OS "Mac OS X 10.8 (Mountain Lion)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_7)
#define PLATFORM_OS "Mac OS X 10.7 (Lion)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_6)
#define PLATFORM_OS "Mac OS X 10.6 (Snow Leopard)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_5)
#define PLATFORM_OS "Mac OS X 10.5 (Leopard)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_4)
#define PLATFORM_OS "Mac OS X 10.4 (Tiger)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_3)
#define PLATFORM_OS "Mac OS X 10.3 (Panther)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_2)
#define PLATFORM_OS "Mac OS X 10.2 (Jaguar)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_1)
#define PLATFORM_OS "Mac OS X 10.1 (Puma)"
#endif

#if !defined(PLATFORM_OS) && defined(MAC_OS_X_VERSION_10_0)
#define PLATFORM_OS "Mac OS X 10.0 (Cheetah)"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "Unknown Mac OS X"
#endif

/* detrmine compile-time CPU */
#ifdef __POWERPC__
#   define PLATFORM_CPU "ppc"
#else
#   ifdef __x86_64
#       define PLATFORM_CPU "x86_64"
#   else
#       define PLATFORM_CPU "i386"
#   endif
#endif

#endif /* VICE_PLATFORM_MACOSX_H */
