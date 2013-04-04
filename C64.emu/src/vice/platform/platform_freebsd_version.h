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
#define PLATFORM_OS "FreeBSD 1.x"
#else

#include <sys/param.h>

#if (__FreeBSD_version==119411)
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
#define PLATFORM_OS "FreeBSD 2.1.6"
#endif

#if (__FreeBSD_version==199508) || (__FreeBSD_version==199512) || (__FreeBSD_version==199608) || (__FreeBSD_version>=220000 && __FreeBSD_version<=221999)
#define PLATFORM_OS "FreeBSD 2.2"
#endif

#if (__FreeBSD_version>=222000 && __FreeBSD_version<=224999)
#define PLATFORM_OS "FreeBSD 2.2.2"
#endif

#if (__FreeBSD_version>=225000 && __FreeBSD_version<=225999)
#define PLATFORM_OS "FreeBSD 2.2.5"
#endif

#if (__FreeBSD_version>=226000 && __FreeBSD_version<=226999)
#define PLATFORM_OS "FreeBSD 2.2.6"
#endif

#if (__FreeBSD_version>=227000 && __FreeBSD_version<=227999)
#define PLATFORM_OS "FreeBSD 2.2.7"
#endif

#if (__FreeBSD_version>=228000 && __FreeBSD_version<=299999)
#define PLATFORM_OS "FreeBSD 2.2.8"
#endif

#if (__FreeBSD_version>=300000 && __FreeBSD_version<=309999)
#define PLATFORM_OS "FreeBSD 3.0"
#endif

#if (__FreeBSD_version>=310000 && __FreeBSD_version<=319999)
#define PLATFORM_OS "FreeBSD 3.1"
#endif

#if (__FreeBSD_version>=320000 && __FreeBSD_version<=329999)
#define PLATFORM_OS "FreeBSD 3.2"
#endif

#if (__FreeBSD_version>=330000 && __FreeBSD_version<=339999)
#define PLATFORM_OS "FreeBSD 3.3"
#endif

#if (__FreeBSD_version>=340000 && __FreeBSD_version<=349999)
#define PLATFORM_OS "FreeBSD 3.4"
#endif

#if (__FreeBSD_version>=350000 && __FreeBSD_version<=399999)
#define PLATFORM_OS "FreeBSD 3.5"
#endif

#if (__FreeBSD_version>=400000 && __FreeBSD_version<=409999)
#define PLATFORM_OS "FreeBSD 4.0"
#endif

#if (__FreeBSD_version>=410000 && __FreeBSD_version<=410999)
#define PLATFORM_OS "FreeBSD 4.1"
#endif

#if (__FreeBSD_version>=411000 && __FreeBSD_version<=419999)
#define PLATFORM_OS "FreeBSD 4.1.1"
#endif

#if (__FreeBSD_version>=420000 && __FreeBSD_version<=429999)
#define PLATFORM_OS "FreeBSD 4.2"
#endif

#if (__FreeBSD_version>=430000 && __FreeBSD_version<=439999)
#define PLATFORM_OS "FreeBSD 4.3"
#endif

#if (__FreeBSD_version>=440000 && __FreeBSD_version<=449999)
#define PLATFORM_OS "FreeBSD 4.4"
#endif

#if (__FreeBSD_version>=450000 && __FreeBSD_version<=459999)
#define PLATFORM_OS "FreeBSD 4.5"
#endif

#if (__FreeBSD_version>=460000 && __FreeBSD_version<=469999)
#define PLATFORM_OS "FreeBSD 4.6"
#endif

#if (__FreeBSD_version>=470000 && __FreeBSD_version<=479999)
#define PLATFORM_OS "FreeBSD 4.7"
#endif

#if (__FreeBSD_version>=480000 && __FreeBSD_version<=489999)
#define PLATFORM_OS "FreeBSD 4.8"
#endif

#if (__FreeBSD_version>=490000 && __FreeBSD_version<=490999)
#define PLATFORM_OS "FreeBSD 4.9"
#endif

#if (__FreeBSD_version>=491000 && __FreeBSD_version<=491999)
#define PLATFORM_OS "FreeBSD 4.10"
#endif

#if (__FreeBSD_version>=492000 && __FreeBSD_version<=499999)
#define PLATFORM_OS "FreeBSD 4.11"
#endif

#if (__FreeBSD_version>=500000 && __FreeBSD_version<=500999)
#define PLATFORM_OS "FreeBSD 5.0"
#endif

#if (__FreeBSD_version>=501000 && __FreeBSD_version<=501999)
#define PLATFORM_OS "FreeBSD 5.1"
#endif

#if (__FreeBSD_version>=502000 && __FreeBSD_version<=502999)
#define PLATFORM_OS "FreeBSD 5.2"
#endif

#if (__FreeBSD_version>=503000 && __FreeBSD_version<=503999)
#define PLATFORM_OS "FreeBSD 5.3"
#endif

#if (__FreeBSD_version>=504000 && __FreeBSD_version<=504999)
#define PLATFORM_OS "FreeBSD 5.4"
#endif

#if (__FreeBSD_version>=505000 && __FreeBSD_version<=599999)
#define PLATFORM_OS "FreeBSD 5.5"
#endif

#if (__FreeBSD_version>=600000 && __FreeBSD_version<=600999)
#define PLATFORM_OS "FreeBSD 6.0"
#endif

#if (__FreeBSD_version>=601000 && __FreeBSD_version<=601999)
#define PLATFORM_OS "FreeBSD 6.1"
#endif

#if (__FreeBSD_version>=602000 && __FreeBSD_version<=602999)
#define PLATFORM_OS "FreeBSD 6.2"
#endif

#if (__FreeBSD_version>=603000 && __FreeBSD_version<=603999)
#define PLATFORM_OS "FreeBSD 6.3"
#endif

#if (__FreeBSD_version>=604000 && __FreeBSD_version<=699999)
#define PLATFORM_OS "FreeBSD 6.4"
#endif

#if (__FreeBSD_version>=700000 && __FreeBSD_version<=700999)
#define PLATFORM_OS "FreeBSD 7.0"
#endif

#if (__FreeBSD_version>=701000 && __FreeBSD_version<=701999)
#define PLATFORM_OS "FreeBSD 7.1"
#endif

#if (__FreeBSD_version>=702000 && __FreeBSD_version<=702999)
#define PLATFORM_OS "FreeBSD 7.2"
#endif

#if (__FreeBSD_version>=703000 && __FreeBSD_version<=799999)
#define PLATFORM_OS "FreeBSD 7.3"
#endif

#if (__FreeBSD_version>=800000 && __FreeBSD_version<=800999)
#define PLATFORM_OS "FreeBSD 8.0"
#endif

#if (__FreeBSD_version>=801000 && __FreeBSD_version<=899999)
#define PLATFORM_OS "FreeBSD 8.1"
#endif

#if (__FreeBSD_version>=900000 && __FreeBSD_version<=999999)
#define PLATFORM_OS "FreeBSD 9.0"
#endif

#endif /* __FreeBSD__==1 */

#ifndef PLATFORM_OS
#define PLATFORM_OS "FreeBSD"
#endif

#endif // VICE_PLATFORM_FREEBSD_VERSION_H
