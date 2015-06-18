/*
 * platform_dos_runtime_os.c - DOS runtime version discovery.
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

#include "vice.h"

#ifdef __MSDOS__

#include <stdio.h>
#include <dos.h>

static char archdep_os_version[128];
static int got_os = 0;

char *platform_get_dos_runtime_os(void)
{
    unsigned short real_version;

    if (!got_os) {
        real_version = _get_dos_version(1);
        sprintf(archdep_os_version, "%s v%d.%d", _os_flavor, real_version >> 8, real_version & 0xff);
        got_os = 1;
    }
    return archdep_os_version;
}
#endif
