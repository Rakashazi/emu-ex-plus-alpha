/*
 * platform_minix_runtime_os.c - Minix runtime version discovery.
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
 *
 * Minix-vmd 1.7
 * Minix 3.1.2a
 * Minix 3.1.3a
 * Minix 3.1.5
 * Minix 3.1.6
 * Minix 3.1.7
 * Minix 3.1.8
 * Minix 3.2.0
 * Minix 3.2.1
 */

#include "vice.h"

#ifdef __minix

#include <sys/types.h>
#include <sys/utsname.h>
#include <string.h>

static char os[200];
static int got_os = 0;

char *platform_get_minix_runtime_os(void)
{
    struct utsname name;

    if (!got_os) {
        uname(&name);

        if (!strcasecmp(name.sysname, "Minix-vmd")) {
            sprintf(os, "%s %s", name.sysname, name.release);
        } else {
            sprintf(os, "%s %s.%s", name.sysname, name.release, name.version);
        }
        got_os = 1;
    }
    return os;
}

#ifdef __ACK__
static char cpu[100];
static int got_cpu = 0;

char *platform_get_minix_runtime_cpu(void)
{
    struct utsname name;

    if (!got_cpu) {
        uname(&name);
        sprintf(cpu, "%s", name.arch);
        got_cpu = 1;
    }
    return cpu;
}
#endif

#endif
