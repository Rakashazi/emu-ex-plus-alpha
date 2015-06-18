/*
 * platform_amigaos4_runtime_os.c - Amiga OS 4.x runtime version discovery.
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

#ifdef AMIGA_OS4

#define __USE_INLINE__

#include <proto/exec.h>
#include <exec/exectags.h>

static struct Library *WorkbenchBase;
static char *osretval = NULL;
static CONST_STRPTR runtime_cpu = NULL;

char *platform_get_amigaos4_runtime_os(void)
{
    if (!osretval) {
        if (WorkbenchBase = OpenLibrary("workbench.library", 53)) {
            osretval = "AmigaOS-4.1";
        }
        if (!osretval && (WorkbenchBase = OpenLibrary("workbench.library", 52))) {
            osretval = "AmigaOS-4.0";
        }
        if (!osretval && (WorkbenchBase = OpenLibrary("workbench.library", 51))) {
            osretval = "AmigaOS-4.0";
        }
        if (!osretval && (WorkbenchBase = OpenLibrary("workbench.library", 50))) {
            osretval = "AmigaOS-4.0";
        }
        if (osretval) {
            CloseLibrary(WorkbenchBase);
        } else {
            osretval = "Unknown AmigaOS";
        }
    }
    return osretval;
}

char *platform_get_amigaos4_runtime_cpu(void)
{
    if (!runtime_cpu) {
        GetCPUInfoTags(GCIT_ModelString, &runtime_cpu, TAG_DONE);
    }
    return (char *)runtime_cpu;
}
#endif
