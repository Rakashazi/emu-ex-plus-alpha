/*
 * platform_dragonfly_version.h - DragonFly BSD version discovery.
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

#ifndef VICE_PLATFORM_FREEBSD_VERSION_H
#define VICE_PLATFORM_FREEBSD_VERSION_H

#include <sys/param.h>

#if (__DragonFly_version==100000)
#define PLATFORM_OS "DragonFly BSD 1.0"
#endif

#if (__DragonFly_version==120000)
#define PLATFORM_OS "DragonFly BSD 1.2.0"
#endif

#if (__DragonFly_version==140000)
#define PLATFORM_OS "DragonFly BSD 1.4.x"
#endif

#if (__DragonFly_version==160000)
#define PLATFORM_OS "DragonFly BSD 1.6.0"
#endif

#if (__DragonFly_version==180000)
#  include <magic.h>
#  ifdef MAGIC_NO_CHECK_COMPRESS
#    define PLATFORM_OS "DragonFly BSD 1.8.1"
#  else
#    define PLATFORM_OS "DragonFly BSD 1.8.0"
#  endif
#endif

#if (__DragonFly_version==196000)
#define PLATFORM_OS "DragonFly BSD 1.10.x"
#endif

#if (__DragonFly_version==197500)
#define PLATFORM_OS "DragonFly BSD 1.12.x"
#endif

#if (__DragonFly_version==200000)
#  include <sys/unistd.h>
#  ifdef _SC_PAGE_SIZE
#    define PLATFORM_OS "DragonFly BSD 2.0.1"
#  else
#    define PLATFORM_OS "DragonFly BSD 2.0.0"
#  endif
#endif

#if (__DragonFly_version==200200)
#define PLATFORM_OS "DragonFly BSD 2.2.x"
#endif

#if (__DragonFly_version==200400)
#  include <sys/diskmbr.h>
#  ifdef DOSPTYP_OPENBSD
#    define PLATFORM_OS "DragonFly BSD 2.4.1"
#  else
#    define PLATFORM_OS "DragonFly BSD 2.4.0"
#  endif
#endif

#if (__DragonFly_version==200600)
#define PLATFORM_OS "DragonFly BSD 2.6.x"
#endif

#if (__DragonFly_version==200800)
#  include <arpa/inet.h>
#  ifdef _STRUCT_IN6_ADDR_DECLARED
#    undef _SYS_MOUNT_H_
#    include <sys/imgact.h>
#    ifdef _SYS_MOUNT_H_
#      define PLATFORM_OS "DragonFly BSD 2.8.2"
#    else
#      define PLATFORM_OS "DragonFly BSD 2.8.1A"
#    endif
#  else
#    define PLATFORM_OS "DragonFly BSD 2.8.1"
#  endif
#endif

#if (__DragonFly_version==201000)
#define PLATFORM_OS "DragonFly BSD 2.10.1"
#endif

#if (__DragonFly_version==300000)
#  include <machine/specialreg.h>
#  ifdef CPUID2_VMM
#    define PLATFORM_OS "DragonFly BSD 3.0.2"
#  else
#    define PLATFORM_OS "DragonFly BSD 3.0.1"
#  endif
#endif

#if (__DragonFly_version==300003)
#define PLATFORM_OS "DragonFly BSD 3.0.3"
#endif

#if (__DragonFly_version==300200)
#  include <sys/user.h>
#  ifdef LWP_MP_VNLRU
#    define PLATFORM_OS "DragonFly BSD 3.2.2"
#  else
#    define PLATFORM_OS "DragonFly BSD 3.2.1"
#  endif
#endif

#if (__DragonFly_version==300400)
#  include <net/if.h>
#  if (IFQ_MAXLEN==250)
#    define PLATFORM_OS "DragonFly BSD 3.4.3"
#  else
#    define PLATFORM_OS "DragonFly BSD 3.4.1/3.4.2"
#  endif
#endif

#if (__DragonFly_version==300600)
#  include <openssl/opensslv.h>
#  if (OPENSSL_VERSION_NUMBER==0x1000107fL)
#     define PLATFORM_OS "DragonFly BSD 3.6.2"
#  else
#    if (OPENSSL_VERSION_NUMBER==0x1000108fL)
#      define PLATFORM_OS "DragonFly BSD 3.6.3"
#    else
#      define PLATFORM_OS "DragonFly BSD 3.6.0/3.6.1"
#    endif
#  endif
#endif

#if (__DragonFly_version==300800)
#  include <openssl/opensslv.h>
#  if (OPENSSL_VERSION_NUMBER==0x1000108fL)
#    define PLATFORM_OS "DragonFly BSD 3.8.1"
#  else
#    if (OPENSSL_VERSION_NUMBER==0x1000109fL)
#      define PLATFORM_OS "DragonFly BSD 3.8.2"
#    else
#      define PLATFORM_OS "DragonFly BSD 3.8.0"
#    endif
#  endif
#endif

#if (__DragonFly_version==400000)
#  include <openssl/opensslv.h>
#  if (OPENSSL_VERSION_NUMBER==0x100010cfL)
#    define PLATFORM_OS "DragonFly BSD 4.0.3/4.0.4"
#  else
#    if (OPENSSL_VERSION_NUMBER==0x100010dfL)
#      define PLATFORM_OS "DragonFly BSD 4.0.5"
#    else
#      define PLATFORM_OS "DragonFly BSD 4.0.1/4.0.2"
#    endif
#  endif
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "DragonFly BSD"
#endif

#endif // VICE_PLATFORM_FREEBSD_VERSION_H
