/*
 * platform_syllable_runtime_os.c - Syllable runtime version discovery.
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
   - Syllable 0.6.7
*/

#include "vice.h"

#ifdef __SYLLABLE__

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <string.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

static system_info psi;
static char os_string[256];
static int got_cpu = 0;
static int got_os = 0;

char *platform_get_syllable_runtime_cpu(void)
{
    if (!got_cpu) {
        get_system_info_v(&psi, SYS_INFO_VERSION);
        got_cpu = 1;
    }
    return psi.zKernelCpuArch;
}

char *platform_get_syllable_runtime_os(void)
{
    struct utsname name;

    if (!got_os) {
        uname(&name);
        get_system_info_v(&psi, SYS_INFO_VERSION);
        sprintf(os_string, "%s v%s.%s", psi.zKernelSystem, name.version, name.release);

#ifdef __GLIBC__
        sprintf(os_string, "%s (glibc %s)", os_string, gnu_get_libc_version());
#endif

        got_os = 1;
    }
    return os_string;
}
#endif
