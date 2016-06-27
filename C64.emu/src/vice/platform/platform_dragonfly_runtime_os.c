/*
 * platform_dragonfly_runtime_os.c - DragonFly BSD runtime version discovery.
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
   cpu   | Operating System
   ------------------------
   i386  | DragonFly BSD 1.0A
   i386  | DragonFly BSD 1.2.0
   i386  | DragonFly BSD 1.4.0
   i386  | DragonFly BSD 1.4.4
   i386  | DragonFly BSD 1.6.0
   i386  | DragonFly BSD 1.8.0
   i386  | DragonFly BSD 1.8.1
   i386  | DragonFly BSD 1.10.0
   i386  | DragonFly BSD 1.10.1
   i386  | DragonFly BSD 1.12.0
   i386  | DragonFly BSD 1.12.1
   i386  | DragonFly BSD 1.12.2
   i386  | DragonFly BSD 2.0.0
   i386  | DragonFly BSD 2.0.1
   i386  | DragonFly BSD 2.2.0
   i386  | DragonFly BSD 2.2.1
   amd64 | DragonFly BSD 2.4.0
   i386  | DragonFly BSD 2.4.0
   amd64 | DragonFly BSD 2.4.1
   i386  | DragonFly BSD 2.4.1
   amd64 | DragonFly BSD 2.6.1
   i386  | DragonFly BSD 2.6.1
   i386  | DragonFly BSD 2.6.2
   amd64 | DragonFly BSD 2.6.3
   i386  | DragonFly BSD 2.6.3
   amd64 | DragonFly BSD 2.8.1
   i386  | DragonFly BSD 2.8.1
   amd64 | DragonFly BSD 2.8.1A
   i386  | DragonFly BSD 2.8.1A
   amd64 | DragonFly BSD 2.8.2
   i386  | DragonFly BSD 2.8.2
   amd64 | DragonFly BSD 2.10.1
   i386  | DragonFly BSD 2.10.1
   amd64 | DragonFly BSD 3.0.1
   i386  | DragonFly BSD 3.0.1
   amd64 | DragonFly BSD 3.0.2
   i386  | DragonFly BSD 3.0.2
   amd64 | DragonFly BSD 3.0.3
   i386  | DragonFly BSD 3.0.3
   amd64 | DragonFly BSD 3.2.1
   i386  | DragonFly BSD 3.2.1
   amd64 | DragonFly BSD 3.2.2
   i386  | DragonFly BSD 3.2.2
   amd64 | DragonFly BSD 3.4.1
   i386  | DragonFly BSD 3.4.1
   amd64 | DragonFly BSD 3.4.2
   i386  | DragonFly BSD 3.4.2
   amd64 | DragonFly BSD 3.4.3
   i386  | DragonFly BSD 3.4.3
   amd64 | DragonFly BSD 3.6.0
   i386  | DragonFly BSD 3.6.0
   amd64 | DragonFly BSD 3.6.1
   i386  | DragonFly BSD 3.6.1
   amd64 | DragonFly BSD 3.6.2
   i386  | DragonFly BSD 3.6.2
   amd64 | DragonFly BSD 3.6.3
   i386  | DragonFly BSD 3.6.3
   amd64 | DragonFly BSD 3.8.0
   i386  | DragonFly BSD 3.8.0
   amd64 | DragonFly BSD 3.8.1
   i386  | DragonFly BSD 3.8.1
   amd64 | DragonFly BSD 3.8.2
   i386  | DragonFly BSD 3.8.2
   amd64 | DragonFly BSD 4.0.1
   amd64 | DragonFly BSD 4.0.2
   amd64 | DragonFly BSD 4.0.3
   amd64 | DragonFly BSD 4.0.4
   amd64 | DragonFly BSD 4.0.5
 */

#include "vice.h"

#ifdef __DragonFly__

#include <stdio.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "archdep.h"
#include "lib.h"
#include "platform.h"
#include "util.h"
#include "log.h"

static char dragonfly_version[100];
static char dragonfly_cpu[100];
static int got_dragonfly_version = 0;
static int got_dragonfly_cpu = 0;

char *platform_get_dragonfly_runtime_cpu(void)
{
    char *model = NULL;
    size_t len = 0;

    if (!got_dragonfly_cpu) {
        sprintf(dragonfly_cpu, "Unknown CPU");

        sysctlbyname("hw.model", NULL, &len, NULL, 0);
        model = lib_malloc(len);
        sysctlbyname("hw.model", model, &len, NULL, 0);

        sprintf(dragonfly_cpu, "%s", model);

        if (strstr(dragonfly_cpu, "Unknown") || !strlen(dragonfly_cpu)) {
            sprintf(dragonfly_cpu, "%s", platform_get_x86_runtime_cpu());
        }

        if (model) {
            lib_free(model);
        }
        got_dragonfly_cpu = 1;
    }
    return dragonfly_cpu;
}

char *platform_get_dragonfly_runtime_os(void)
{
    struct utsname name;

    if (!got_dragonfly_version) {
        uname(&name);

        sprintf(dragonfly_version, "%s %s", name.sysname, name.release);

        got_dragonfly_version = 1;
    }

    return dragonfly_version;
}
#endif
