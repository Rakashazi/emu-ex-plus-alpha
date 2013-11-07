/*
 * platform_openbsd_version.h - OpenBSD version discovery.
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

#ifndef VICE_PLATFORM_OPENBSD_VERSION_H
#define VICE_PLATFORM_OPENBSD_VERSION_H

#include <sys/param.h>

#ifdef OpenBSD2_0
#define PLATFORM_OS "OpenBSD 2.0"
#endif

#ifdef OpenBSD2_1
#define PLATFORM_OS "OpenBSD 2.1"
#endif

#ifdef OpenBSD2_2
#define PLATFORM_OS "OpenBSD 2.2"
#endif

#ifdef OpenBSD2_3
#define PLATFORM_OS "OpenBSD 2.3"
#endif

#ifdef OpenBSD2_4
#define PLATFORM_OS "OpenBSD 2.4"
#endif

#ifdef OpenBSD2_5
#define PLATFORM_OS "OpenBSD 2.5"
#endif

#ifdef OpenBSD2_6
#define PLATFORM_OS "OpenBSD 2.6"
#endif

#ifdef OpenBSD2_7
#define PLATFORM_OS "OpenBSD 2.7"
#endif

#ifdef OpenBSD2_8
#define PLATFORM_OS "OpenBSD 2.8"
#endif

#ifdef OpenBSD2_9
#define PLATFORM_OS "OpenBSD 2.9"
#endif

#ifdef OpenBSD3_0
#define PLATFORM_OS "OpenBSD 3.0"
#endif

#ifdef OpenBSD3_1
#define PLATFORM_OS "OpenBSD 3.1"
#endif

#ifdef OpenBSD3_2
#define PLATFORM_OS "OpenBSD 3.2"
#endif

#ifdef OpenBSD3_3
#define PLATFORM_OS "OpenBSD 3.3"
#endif

#ifdef OpenBSD3_4
#define PLATFORM_OS "OpenBSD 3.4"
#endif

#ifdef OpenBSD3_5
#define PLATFORM_OS "OpenBSD 3.5"
#endif

#ifdef OpenBSD3_6
#define PLATFORM_OS "OpenBSD 3.6"
#endif

#ifdef OpenBSD3_7
#define PLATFORM_OS "OpenBSD 3.7"
#endif

#ifdef OpenBSD3_8
#define PLATFORM_OS "OpenBSD 3.8"
#endif

#ifdef OpenBSD3_9
#define PLATFORM_OS "OpenBSD 3.9"
#endif

#ifdef OpenBSD4_0
#define PLATFORM_OS "OpenBSD 4.0"
#endif

#ifdef OpenBSD4_1
#define PLATFORM_OS "OpenBSD 4.1"
#endif

#ifdef OpenBSD4_2
#define PLATFORM_OS "OpenBSD 4.2"
#endif

#ifdef OpenBSD4_3
#define PLATFORM_OS "OpenBSD 4.3"
#endif

#ifdef OpenBSD4_4
#define PLATFORM_OS "OpenBSD 4.4"
#endif

#ifdef OpenBSD4_5
#define PLATFORM_OS "OpenBSD 4.5"
#endif

#ifdef OpenBSD4_6
#define PLATFORM_OS "OpenBSD 4.6"
#endif

#ifdef OpenBSD4_7
#define PLATFORM_OS "OpenBSD 4.7"
#endif

#ifdef OpenBSD4_8
#define PLATFORM_OS "OpenBSD 4.8"
#endif

#ifdef OpenBSD4_9
#define PLATFORM_OS "OpenBSD 4.9"
#endif

#ifdef OpenBSD5_0
#define PLATFORM_OS "OpenBSD 5.0"
#endif

#ifdef OpenBSD5_1
#define PLATFORM_OS "OpenBSD 5.1"
#endif

#ifdef OpenBSD5_2
#define PLATFORM_OS "OpenBSD 5.2"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "OpenBSD"
#endif

#endif // VICE_PLATFORM_OPENBSD_VERSION_H
