/*
 * platform_aix_version.h - AIX version discovery.
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

#ifndef VICE_PLATFORM_AIX_VERSION_H
#define VICE_PLATFORM_AIX_VERSION_H

/* AIX discovery */

/* find out what version of AIX is being used */
#ifdef _AIX61
#  define PLATFORM_OS "AIX 6.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX53)
#  define PLATFORM_OS "AIX 5.3"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX52)
#  define PLATFORM_OS "AIX 5.2"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX51)
#  define PLATFORM_OS "AIX 5.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX433)
#  define PLATFORM_OS "AIX 4.3.3"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX432)
#  define PLATFORM_OS "AIX 4.3.2"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX431)
#  define PLATFORM_OS "AIX 4.3.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX43)
#  define PLATFORM_OS "AIX 4.3"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX421)
#  define PLATFORM_OS "AIX 4.2.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX42)
#  define PLATFORM_OS "AIX 4.2"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX415)
#  define PLATFORM_OS "AIX 4.1.5"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX414)
#  define PLATFORM_OS "AIX 4.1.4"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX413)
#  define PLATFORM_OS "AIX 4.1.3"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX411)
#  define PLATFORM_OS "AIX 4.1.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX41)
#  define PLATFORM_OS "AIX 4.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX4)
#  define PLATFORM_OS "AIX 4.0"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX32)
#  define PLATFORM_OS "AIX 3.2"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX31)
#  define PLATFORM_OS "AIX 3.1"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX3)
#  define PLATFORM_OS "AIX 3.0"
#endif

#ifdef PLATFORM_OS
#  define PLATFORM_CPU "RS6000"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX13)
#  define PLATFORM_OS "AIX 1.3"
#endif

#if !defined(PLATFORM_OS) && defined(_AIX11)
#  define PLATFORM_OS "AIX 1.1"
#endif

#ifndef PLATFORM_OS
#  define PLATFORM_OS "AIX"
#endif

/* define FIND_X86_CPU for later generic x86 cpu discovery */
#ifndef PLATFORM_CPU
#  define FIND_X86_CPU
#endif

#endif // VICE_PLATFORM_AIX_VERSION_H
