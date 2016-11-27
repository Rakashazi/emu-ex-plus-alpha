/*
 * platform_freebsd_version.h - FreeBSD version discovery.
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

#if (__FreeBSD__==1)

#include <machine/console.h>

#ifdef NUM_FKEYS
#  define PLATFORM_OS "FreeBSD 1.1.5.1"
#else
#  ifdef MAXSSAVER
#    define PLATFORM_OS "FreeBSD 1.1"
#  else
#    define PLATFORM_OS "FreeBSD 1.0"
#  endif
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 1.x"
#endif

#else

#include <sys/param.h>

#if (__FreeBSD__==2)

#include <osreldate.h>

#if (__FreeBSD_version==199411)
#define PLATFORM_OS "FreeBSD 2.0"
#endif

#if (__FreeBSD_version==199504)
#define PLATFORM_OS "FreeBSD 2.0.5"
#endif

#if (__FreeBSD_version==199501) || (__FreeBSD_version==199503) || (__FreeBSD_version==199511)
#define PLATFORM_OS "FreeBSD 2.1"
#endif

#if (__FreeBSD_version==199607)
#define PLATFORM_OS "FreeBSD 2.1.5"
#endif

#if (__FreeBSD_version==199612)
#  include <dialog.h>
#  ifdef DITEM_NO_ECHO
#    define PLATFORM_OS "FreeBSD 2.1.7"
#  else
#    define PLATFORM_OS "FreeBSD 2.1.6"
#  endif
#endif

#if (__FreeBSD_version>=220000 && __FreeBSD_version<221000)
#define PLATFORM_OS "FreeBSD 2.2"
#endif

#if (__FreeBSD_version>=221000 && __FreeBSD_version<222000)
#define PLATFORM_OS "FreeBSD 2.2.1"
#endif

#if (__FreeBSD_version>=222000 && __FreeBSD_version<223000)
#define PLATFORM_OS "FreeBSD 2.2.2"
#endif

#if (__FreeBSD_version>=225000 && __FreeBSD_version<226000)
#define PLATFORM_OS "FreeBSD 2.2.5"
#endif

#if (__FreeBSD_version>=226000 && __FreeBSD_version<227000)
#define PLATFORM_OS "FreeBSD 2.2.6"
#endif

#if (__FreeBSD_version>=227000 && __FreeBSD_version<228000)
#define PLATFORM_OS "FreeBSD 2.2.7"
#endif

#if (__FreeBSD_version>=228000 && __FreeBSD_version<229000)
#define PLATFORM_OS "FreeBSD 2.2.8"
#endif

#if (__FreeBSD_version==228001)
#undef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 2.2.9"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 2.x"
#endif

#endif

#if (__FreeBSD__==3)

#if (__FreeBSD_version>=300000 && __FreeBSD_version<310000)
#define PLATFORM_OS "FreeBSD 3.0"
#endif

#if (__FreeBSD_version>=310000 && __FreeBSD_version<320000)
#define PLATFORM_OS "FreeBSD 3.1"
#endif

#if (__FreeBSD_version>=320000 && __FreeBSD_version<330000)
#define PLATFORM_OS "FreeBSD 3.2"
#endif

#if (__FreeBSD_version>=330000 && __FreeBSD_version<340000)
#define PLATFORM_OS "FreeBSD 3.3"
#endif

#if (__FreeBSD_version>=340000 && __FreeBSD_version<350000)
#define PLATFORM_OS "FreeBSD 3.4"
#endif

#if (__FreeBSD_version>=350000 && __FreeBSD_version<=360000)
#define PLATFORM_OS "FreeBSD 3.5"
#endif

#if (__FreeBSD_version==350001)
#undef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 3.5.1"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 3.x"
#endif

#endif

#if (__FreeBSD__==4)

#if (__FreeBSD_version>=400000 && __FreeBSD_version<410000)
#define PLATFORM_OS "FreeBSD 4.0"
#endif

#if (__FreeBSD_version>=410000 && __FreeBSD_version<411000)
#define PLATFORM_OS "FreeBSD 4.1"
#endif

#if (__FreeBSD_version>=411000 && __FreeBSD_version<420000)
#define PLATFORM_OS "FreeBSD 4.1.1"
#endif

#if (__FreeBSD_version>=420000 && __FreeBSD_version<430000)
#define PLATFORM_OS "FreeBSD 4.2"
#endif

#if (__FreeBSD_version>=430000 && __FreeBSD_version<440000)
#define PLATFORM_OS "FreeBSD 4.3"
#endif

#if (__FreeBSD_version>=440000 && __FreeBSD_version<450000)
#define PLATFORM_OS "FreeBSD 4.4"
#endif

#if (__FreeBSD_version>=450000 && __FreeBSD_version<460000)
#define PLATFORM_OS "FreeBSD 4.5"
#endif

#if (__FreeBSD_version>=460000 && __FreeBSD_version<470000)
#define PLATFORM_OS "FreeBSD 4.6"
#endif

#if (__FreeBSD_version>=470000 && __FreeBSD_version<480000)
#define PLATFORM_OS "FreeBSD 4.7"
#endif

#if (__FreeBSD_version>=480000 && __FreeBSD_version<490000)
#define PLATFORM_OS "FreeBSD 4.8"
#endif

#if (__FreeBSD_version>=490000 && __FreeBSD_version<491000)
#define PLATFORM_OS "FreeBSD 4.9"
#endif

#if (__FreeBSD_version>=491000 && __FreeBSD_version<492000)
#define PLATFORM_OS "FreeBSD 4.10"
#endif

#if (__FreeBSD_version>=492000 && __FreeBSD_version<493000)
#define PLATFORM_OS "FreeBSD 4.11"
#endif

#if (__FreeBSD_version==460002)
#undef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 4.6.2"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 4.x"
#endif

#endif

#if (__FreeBSD__==5)

#if (__FreeBSD_version>=500000 && __FreeBSD_version<501000)
#define PLATFORM_OS "FreeBSD 5.0"
#endif

#if (__FreeBSD_version>=501000 && __FreeBSD_version<502000)
#define PLATFORM_OS "FreeBSD 5.1"
#endif

#if (__FreeBSD_version>=502000 && __FreeBSD_version<503000)
#define PLATFORM_OS "FreeBSD 5.2"
#endif

#if (__FreeBSD_version>=503000 && __FreeBSD_version<504000)
#define PLATFORM_OS "FreeBSD 5.3"
#endif

#if (__FreeBSD_version>=504000 && __FreeBSD_version<505000)
#define PLATFORM_OS "FreeBSD 5.4"
#endif

#if (__FreeBSD_version>=505000 && __FreeBSD_version<=506000)
#define PLATFORM_OS "FreeBSD 5.5"
#endif

#if (__FreeBSD_version==502010)
#undef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 5.2.1"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 5.x"
#endif

#endif

#if (__FreeBSD__==6)

#if (__FreeBSD_version>=600000 && __FreeBSD_version<601000)
#define PLATFORM_OS "FreeBSD 6.0"
#endif

#if (__FreeBSD_version>=601000 && __FreeBSD_version<602000)
#define PLATFORM_OS "FreeBSD 6.1"
#endif

#if (__FreeBSD_version>=602000 && __FreeBSD_version<603000)
#define PLATFORM_OS "FreeBSD 6.2"
#endif

#if (__FreeBSD_version>=603000 && __FreeBSD_version<=604000)
#define PLATFORM_OS "FreeBSD 6.3"
#endif

#if (__FreeBSD_version>=604000 && __FreeBSD_version<=605000)
#define PLATFORM_OS "FreeBSD 6.4"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 6.x"
#endif

#endif

#if (__FreeBSD__==7)

#if (__FreeBSD_version>=700000 && __FreeBSD_version<701000)
#define PLATFORM_OS "FreeBSD 7.0"
#endif

#if (__FreeBSD_version>=701000 && __FreeBSD_version<702000)
#define PLATFORM_OS "FreeBSD 7.1"
#endif

#if (__FreeBSD_version>=702000 && __FreeBSD_version<703000)
#define PLATFORM_OS "FreeBSD 7.2"
#endif

#if (__FreeBSD_version>=703000 && __FreeBSD_version<704000)
#define PLATFORM_OS "FreeBSD 7.3"
#endif

#if (__FreeBSD_version>=704000 && __FreeBSD_version<705000)
#define PLATFORM_OS "FreeBSD 7.4"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 7.x"
#endif

#endif

#if (__FreeBSD__==8)

#if (__FreeBSD_version>=800000 && __FreeBSD_version<801000)
#define PLATFORM_OS "FreeBSD 8.0"
#endif

#if (__FreeBSD_version>=801000 && __FreeBSD_version<802000)
#define PLATFORM_OS "FreeBSD 8.1"
#endif

#if (__FreeBSD_version>=802000 && __FreeBSD_version<803000)
#define PLATFORM_OS "FreeBSD 8.2"
#endif

#if (__FreeBSD_version>=803000 && __FreeBSD_version<804000)
#define PLATFORM_OS "FreeBSD 8.3"
#endif

#if (__FreeBSD_version>=804000 && __FreeBSD_version<805000)
#define PLATFORM_OS "FreeBSD 8.4"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 8.x"
#endif

#endif

#if (__FreeBSD__==9)

#if (__FreeBSD_version>=900000 && __FreeBSD_version<901000)
#define PLATFORM_OS "FreeBSD 9.0"
#endif

#if (__FreeBSD_version>=901000 && __FreeBSD_version<902000)
#define PLATFORM_OS "FreeBSD 9.1"
#endif

#if (__FreeBSD_version>=902000 && __FreeBSD_version<903000)
#define PLATFORM_OS "FreeBSD 9.2"
#endif

#if (__FreeBSD_version>=903000 && __FreeBSD_version<904000)
#define PLATFORM_OS "FreeBSD 9.3"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 9.x"
#endif

#endif

#if (__FreeBSD__==10)

#if (__FreeBSD_version>=1000000 && __FreeBSD_version<1001000)
#define PLATFORM_OS "FreeBSD 10.0"
#endif

#if (__FreeBSD_version>=1001000 && __FreeBSD_version<1002000)
#define PLATFORM_OS "FreeBSD 10.1"
#endif

#if (__FreeBSD_version>=1002000 && __FreeBSD_version<1003000)
#define PLATFORM_OS "FreeBSD 10.2"
#endif

#if (__FreeBSD_version>=1003000 && __FreeBSD_version<1004000)
#define PLATFORM_OS "FreeBSD 10.3"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 10.x"
#endif

#endif

#if (__FreeBSD__==11)

#if (__FreeBSD_version>=1100000 && __FreeBSD_version<1101000)
#define PLATFORM_OS "FreeBSD 11.0"
#endif

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD 11.x"
#endif

#endif

#endif /* __FreeBSD__==1 */

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD"
#endif

#endif // VICE_PLATFORM_FREEBSD_VERSION_H
