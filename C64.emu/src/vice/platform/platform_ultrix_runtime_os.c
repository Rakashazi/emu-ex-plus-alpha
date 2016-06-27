/*
 * platform_ultrix_runtime_os.c - Ultrix runtime version discovery.
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
   cpu    | Operating System
   -------------------------
   mipsel | Ultrix 4.5
   mipsel | NetBSD (emulation layer)
 */

#include "vice.h"

#if defined(ultrix) || defined(__ultrix) || defined(__ultrix__)

#include <stdio.h>
#include <sys/utsname.h>
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "platform.h"
#include "util.h"
#include "log.h"

static char ultrix_version[100];
static char ultrix_cpu[100];
static int got_ultrix_version = 0;
static int got_ultrix_cpu = 0;

char *platform_get_ultrix_runtime_cpu(void)
{
    struct utsname name;

    if (!got_ultrix_cpu) {
        uname(&name);
        if (!strcasecmp(name.machine, "RISC")) {
            sprintf(ultrix_cpu, "Mipsel");
        } else if (!strcasecmp(name.machine, "VAX")) {
            sprintf(ultrix_cpu, "VAX");
        } else {
#ifdef __mips__
            sprintf(ultrix_cpu, "Mipsel (%s)", name.machine);
#else
#if defined(__vax__) || defined(__vax)
            sprintf(ultrix_cpu, "VAX (%s)", name.machine);
#else
            sprintf(ultrix_cpu, "%s", name.machine);
#endif
#endif
        }

        got_ultrix_cpu = 1;
    }
    return ultrix_cpu;
}

char *platform_get_ultrix_runtime_os(void)
{
    struct utsname name;

    if (!got_ultrix_version) {
        uname(&name);

        sprintf(ultrix_version, "%s %s", name.sysname, name.release);

        got_ultrix_version = 1;
    }

    return ultrix_version;
}
#endif
