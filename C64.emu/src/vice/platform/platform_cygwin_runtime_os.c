/*
 * platform_cygwin_runtime_os.c - Cygwin runtime version discovery.
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

#if !defined(WIN32_COMPILE) && defined(__CYGWIN32__)

#include <sys/utsname.h>
#include <stdio.h>

#include "platform.h"

static char api_version[200];

char *platform_get_cygwin_runtime_os(void)
{
    int i = 0;
    struct utsname name;
    char temp[21];

    uname(&name);
    sprintf(temp, "%s", name.release);
    while (temp[i] != '(') {
        i++;
    }
    temp[i] = 0;
    sprintf(api_version, "Cygwin DLL %s API %s", temp, temp + i + 1);
    i = 0;
    while (api_version[i] != '/') {
        i++;
    }
    api_version[i] = 0;

    sprintf(api_version, "%s [%s]", api_version, platform_get_windows_runtime_os());

    return api_version;
}
#endif
