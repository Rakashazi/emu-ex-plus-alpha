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
*/

#include "vice.h"

#ifdef __SYLLABLE__

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <string.h>

static system_info psi;
static char os_string[256];

char *platform_get_syllable_runtime_cpu(void)
{
    get_system_info_v(&psi, SYS_INFO_VERSION);
    return psi.zKernelCpuArch;
}

char *platform_get_syllable_runtime_os(void)
{
    struct utsname name;

    uname(&name);
    get_system_info_v(&psi, SYS_INFO_VERSION);
    sprintf(os_string, "%s v%s.%s", psi.zKernelSystem, name.version, name.release);

    return os_string;
}
#endif
