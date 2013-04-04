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
#ifdef MAC_OS_X_VERSION_10_7
#define PLATFORM_OS "Mac OS X 10.7"
#else
#ifdef MAC_OS_X_VERSION_10_6
#define PLATFORM_OS "Mac OS X 10.6"
#else
#ifdef MAC_OS_X_VERSION_10_5
#define PLATFORM_OS "Mac OS X 10.5"
#else
#ifdef MAC_OS_X_VERSION_10_4
#define PLATFORM_OS "Mac OS X 10.4"
#else
#ifdef MAC_OS_X_VERSION_10_3
#define PLATFORM_OS "Mac OS X 10.3"
#else
#ifdef MAC_OS_X_VERSION_10_2
#define PLATFORM_OS "Mac OS X 10.2"
#else
#ifdef MAC_OS_X_VERSION_10_1
#define PLATFORM_OS "Mac OS X 10.1"
#else
#ifdef MAC_OS_X_VERSION_10_0
#define PLATFORM_OS "Mac OS X 10.0"
#else
#define PLATFORM_OS "Mac OS X"
#endif /* 10.0 */
#endif /* 10.1 */
#endif /* 10.2 */
#endif /* 10.3 */
#endif /* 10.4 */
#endif /* 10.5 */
#endif /* 10.6 */
#endif /* 10.7 */

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
