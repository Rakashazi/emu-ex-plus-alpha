/*
 * platform_solaris_runtime_os.c - Solaris runtime version discovery.
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

/* Tested and confirmed working on:
*/

/* cpu | version | uname.release | uname.version | uname.machine | canonial name
   -----------------------------------------------------------------------------
   x86 |     5   |     5.5.1     |   Generic     |    i86pc      | i386-pc-solaris2.5.1
 sparc |     5   |     5.5.1     |   Generic     |    sun4m      | sparc-solaris2.5.1
   x86 |     6   |     5.6       |   Generic     |    i86pc      | i386-pc-solaris2.6
 sparc |     6   |     5.6       |   Generic*    |    sun4u      | sparc-solaris2.6
   x86 |     7   |     5.7       |   Generic     |    i86pc      | i386-pc-solaris-2.7
 sparc |     7   |     5.7       |   Generic*    |    sun4u      | sparc-solaris2.7
   x86 |     8   |     5.8       |   Generic*    |    i86pc      | i386-pc-solaris2.8
 sparc |     8   |     5.8       |   Generic*    |    sun4u      | sparc-solaris2.8
   x86 |     9   |     5.9       |   Generic*    |    i86pc      | i386-pc-solaris2.9
 sparc |     9   |     5.9       |   Generic*    |    sun4u      | sparc-solaris2.9
   x86 |    10   |     5.10      |   Generic*    |    i86pc      | i386-pc-solaris2.10
 sparc |    10   |     5.10      |   Generic*    |    sun4u      | sparc-solaris2.10
   x86 |   open  |     5.11      |   *           |    i86pc      | i386-pc-solaris2.11
 sparc |   open  |
   arm |   open  |
 s390x |   open  |
   x86 |    11   |     5.11      |    11.0       |    i86pc      | i386-pc-solaris2.11
 sparc |    11   |
 */

#include "vice.h"

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))

#include <sys/utsname.h>
#include <string.h>

char *platform_get_solaris_runtime_os(void)
{
    struct utsname name;

    uname(&name);
    if (!strcasecmp(name.release, "5.3")) {
        return "Solaris 3";
    }
    if (!strcasecmp(name.release, "5.4")) {
        return "Solaris 4";
    }
    if (!strcasecmp(name.release, "5.5.1")) {
        return "Solaris 5";
    }
    if (!strcasecmp(name.release, "5.6")) {
        return "Solaris 6";
    }
    if (!strcasecmp(name.release, "5.7")) {
        return "Solaris 7";
    }
    if (!strcasecmp(name.release, "5.8")) {
        return "Solaris 8";
    }
    if (!strcasecmp(name.release, "5.9")) {
        return "Solaris 9";
    }
    if (!strcasecmp(name.release, "5.10")) {
        return "Solaris 10";
    }
    if (!strcasecmp(name.release, "5.11")) {
        if (!strcasecmp(name.version, "11.0")) {
            return "Solaris 11";
        } else {
            return "OpenSolaris";
        }
    }
    return "Unknown Solaris version";
}
#endif
