/*
 * platform_hurd_runtime_os.c - GNU Hurd runtime version discovery.
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
 * - Debian Hurd 2015
 *
 */

#include "vice.h"

#if defined(__GNU__) && !defined(NEXTSTEP_COMPILE) && !defined(OPENSTEP_COMPILE)

#include <stdio.h>
#include <sys/utsname.h>
#include <string.h>
#include <gnu/libc-version.h>

static char os[200];
static int got_os = 0;

char *platform_get_hurd_runtime_os(void)
{
    struct utsname name;

    if (!got_os) {
        uname(&name);

        sprintf(os, "GNU Hurd %s (glibc %s)", name.version, gnu_get_libc_version());

        got_os = 1;
    }
    return os;
}
#endif
