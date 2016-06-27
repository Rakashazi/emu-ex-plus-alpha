/*
 * platform_netbsd_version.h - NetBSD version discovery.
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


#ifndef VICE_PLATFORM_NETBSD_VERSION_H
#define VICE_PLATFORM_NETBSD_VERSION_H

#include <sys/param.h>

#ifdef NetBSD0_8
#define PLATFORM_OS "NetBSD 0.8"
#endif

#ifdef NetBSD0_9
#define PLATFORM_OS "NetBSD 0.9"
#endif


#ifdef NetBSD1_0

#if (NetBSD1_0==1)
#define PLATFORM_OS "NetBSD 1.0"
#endif

#if (NetBSD1_0==2)
#define PLATFORM_OS "NetBSD 1.0A"
#endif

#endif /* NetBSD1_0 */

#if !defined(PLATFORM_OS)

#ifdef NetBSD1_1
#define PLATFORM_OS "NetBSD 1.1"
#endif

#ifdef NetBSD1_2
#define PLATFORM_OS "NetBSD 1.2.x"
#endif

#if (__NetBSD_Version__==103000300)
#define PLATFORM_OS "NetBSD 1.3.3"
#endif

#if defined(NetBSD1_3) && !defined(PLATFORM_OS) && (NetBSD==199712)
#define PLATFORM_OS "NetBSD 1.3"
#endif

#if defined(NetBSD1_3) && !defined(PLATFORM_OS)
#define PLATFORM_OS "NetBSD 1.3.x"
#endif

#if (__NetBSD_Version__==104000000)
#define PLATFORM_OS "NetBSD 1.4"
#endif

#if (__NetBSD_Version__==104000100)
#define PLATFORM_OS "NetBSD 1.4.1"
#endif

#if (__NetBSD_Version__==104000200)
#define PLATFORM_OS "NetBSD 1.4.2"
#endif

#if (__NetBSD_Version__==104000300)
#define PLATFORM_OS "NetBSD 1.4.3"
#endif

#if (__NetBSD_Version__==105000000)
#define PLATFORM_OS "NetBSD 1.5"
#endif

#if (__NetBSD_Version__==105000100)
#define PLATFORM_OS "NetBSD 1.5.1"
#endif

#if (__NetBSD_Version__==105000200)
#define PLATFORM_OS "NetBSD 1.5.2"
#endif

#if (__NetBSD_Version__==105000300)
#define PLATFORM_OS "NetBSD 1.5.3"
#endif

#if (__NetBSD_Version__==106000000)
#define PLATFORM_OS "NetBSD 1.6"
#endif

#if (__NetBSD_Version__==106000100)
#define PLATFORM_OS "NetBSD 1.6.1"
#endif

#if (__NetBSD_Version__==106000200)
#define PLATFORM_OS "NetBSD 1.6.2"
#endif

#if (__NetBSD_Version__==200000000)
#define PLATFORM_OS "NetBSD 2.0"
#endif

#if (__NetBSD_Version__==200000200)
#define PLATFORM_OS "NetBSD 2.0.2"
#endif

#if (__NetBSD_Version__==201000000)
#define PLATFORM_OS "NetBSD 2.1"
#endif

#if (__NetBSD_Version__==300000000)
#define PLATFORM_OS "NetBSD 3.0"
#endif

#if (__NetBSD_Version__==300001000)
#define PLATFORM_OS "NetBSD 3.0.1"
#endif

#if (__NetBSD_Version__==300002000)
#define PLATFORM_OS "NetBSD 3.0.2"
#endif

#if (__NetBSD_Version__==301003000) || (__NetBSD_Version__==300003000)
#define PLATFORM_OS "NetBSD 3.0.3"
#endif

#if (__NetBSD_Version__==301000000)
#define PLATFORM_OS "NetBSD 3.1"
#endif

#if (__NetBSD_Version__==301001000)
#define PLATFORM_OS "NetBSD 3.1.1"
#endif

/* NetBSD 4.0.1 seems to be 'mis-versioned' */
#if (__NetBSD_Version__==400010000) || (__NetBSD_Version__==400000003)
#define PLATFORM_OS "NetBSD 4.0.x"
#endif

#if (__NetBSD_Version__==500000000)
#define PLATFORM_OS "NetBSD 5.0"
#endif

/* NetBSD 5.0.x seems to be 'mis-versioned' */
#if (__NetBSD_Version__==500000100) || (__NetBSD_Version__==500000200) || (__NetBSD_Version__==500000003) || (__NetBSD_Version__==500000002)
#define PLATFORM_OS "NetBSD 5.0.x"
#endif

#if (__NetBSD_Version__==501000000)
#define PLATFORM_OS "NetBSD 5.1"
#endif

#if (__NetBSD_Version__==501000100)
#define PLATFORM_OS "NetBSD 5.1.1"
#endif

#if (__NetBSD_Version__==501000200)
#define PLATFORM_OS "NetBSD 5.1.2"
#endif

#if (__NetBSD_Version__==501000300)
#define PLATFORM_OS "NetBSD 5.1.3"
#endif

#if (__NetBSD_Version__==501000400)
#define PLATFORM_OS "NetBSD 5.1.4"
#endif

#if (__NetBSD_Version__==501000500)
#define PLATFORM_OS "NetBSD 5.1.5"
#endif

#if (__NetBSD_Version__==502000000)
#define PLATFORM_OS "NetBSD 5.2"
#endif

#if (__NetBSD_Version__==502000100)
#define PLATFORM_OS "NetBSD 5.2.1"
#endif

#if (__NetBSD_Version__==502000200)
#define PLATFORM_OS "NetBSD 5.2.2"
#endif

#if (__NetBSD_Version__==502000300)
#define PLATFORM_OS "NetBSD 5.2.3"
#endif

#if (__NetBSD_Version__==600000000)
#define PLATFORM_OS "NetBSD 6.0"
#endif

/* NetBSD 6.0.1 seems to be 'mis-versioned' */
#if (__NetBSD_Version__==600000100) || (__NetBSD_Version__==600000001)
#define PLATFORM_OS "NetBSD 6.0.1"
#endif

/* NetBSD 6.0.2 seems to be 'mis-versioned' */
#if (__NetBSD_Version__==600000200) || (__NetBSD_Version__==600000002)
#define PLATFORM_OS "NetBSD 6.0.2"
#endif

#if (__NetBSD_Version__==600000300)
#define PLATFORM_OS "NetBSD 6.0.3"
#endif

#if (__NetBSD_Version__==600000400)
#define PLATFORM_OS "NetBSD 6.0.4"
#endif

#if (__NetBSD_Version__==600000500)
#define PLATFORM_OS "NetBSD 6.0.5"
#endif

#if (__NetBSD_Version__==600000600)
#define PLATFORM_OS "NetBSD 6.0.6"
#endif

#if (__NetBSD_Version__==601000000)
#define PLATFORM_OS "NetBSD 6.1"
#endif

#if (__NetBSD_Version__==601000100)
#define PLATFORM_OS "NetBSD 6.1.1"
#endif

#if (__NetBSD_Version__==601000200)
#define PLATFORM_OS "NetBSD 6.1.2"
#endif

#if (__NetBSD_Version__==601000300)
#define PLATFORM_OS "NetBSD 6.1.3"
#endif

#if (__NetBSD_Version__==601000400)
#define PLATFORM_OS "NetBSD 6.1.4"
#endif

#if (__NetBSD_Version__==601000500)
#define PLATFORM_OS "NetBSD 6.1.5"
#endif

#if (__NetBSD_Version__==700000001)
#define PLATFORM_OS "NetBSD 7.0"
#endif

#endif /* !PLATFORM_OS */

#ifndef PLATFORM_OS
#define PLATFORM_OS "NetBSD"
#endif

#endif // VICE_PLATFORM_NETBSD_VERSION_H
