/*
 * platform_linux_libc_version.h - Linux libc version discovery.
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

#ifndef VICE_PLATFORM_LINUX_LIBC_VERSION_H
#define VICE_PLATFORM_LINUX_LIBC_VERSION_H

#define QUOTE(x) XQUOTE(x)
#define XQUOTE(x) #x

/* Linux old libc / glibc version discovery */
#if !defined(PLATFORM_OS) && defined(__GNU_LIBRARY__)
#  if (__GNU_LIBRARY__ < 6)
#    define PLATFORM_OS "Linux libc " QUOTE(__GNU_LIBRARY__) "." QUOTE(__GNU_LIBRARY_MINOR__)
#  else
#    define PLATORM_OS "Linux glibc " QUOTE(__GLIBC__) "." QUOTE(__GLIBC__MINOR__)
#  endif
#endif

/* Linux uClibc version discovery */
#if !defined(PLATFORM_OS) && defined(__UCLIBC__)
#define PLATFORM_OS "Linux uClibc " QUOTE(__UCLIBC_MAJOR__) "." QUOTE(__UCLIBC_MINOR__) "." QUOTE(__UCLIBC_SUBLEVEL__)
#endif

/* Linux dietlibc discovery */
#if !defined(PLATFORM_OS) && defined(__dietlibc__)
#define PLATFORM_OS "Linux dietlibc"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "Linux"
#endif

#endif
