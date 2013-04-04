/*
 * platform_qnx6_version.h - QNX 6.x version discovery.
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

#ifndef VICE_PLATFORM_QNX6_VERSION_H
#define VICE_PLATFORM_QNX6_VERSION_H

#include <sys/neutrino.h>

#if (_NTO_VERSION==600)
#define PLATFORM_OS "QNX 6.0"
#endif

#if (_NTO_VERSION==610)
#define PLATFORM_OS "QNX 6.1"
#endif

#if (_NTO_VERSION==620)
#define PLATFORM_OS "QNX 6.2"
#endif

#if (_NTO_VERSION==621)
#define PLATFORM_OS "QNX 6.2.1"
#endif

#if (_NTO_VERSION==630)
#define PLATFORM_OS "QNX 6.3"
#endif

#if (_NTO_VERSION==631)
#define PLATFORM_OS "QNX 6.3.1"
#endif

#if (_NTO_VERSION==632)
#define PLATFORM_OS "QNX 6.3.2"
#endif

#if (_NTO_VERSION==633)
#define PLATFORM_OS "QNX 6.3.3"
#endif

#if (_NTO_VERSION==640)
#define PLATFORM_OS "QNX 6.4.0"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "QNX 6.x"
#endif

#endif // VICE_PLATFORM_QNX6_VERSION_H
