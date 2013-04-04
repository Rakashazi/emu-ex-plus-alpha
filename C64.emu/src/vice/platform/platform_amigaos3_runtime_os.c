/*
 * platform_amigaos3_runtime_os.c - Amiga OS 3.x runtime version discovery.
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
   - AmigaOS 3.0
   - AmigaOS 3.1
   - AmigaOS 3.5
   - AmigaOS 3.9
   - AROS
*/

#include "vice.h"

#ifdef AMIGA_M68K

#define __USE_INLINE__

#include <proto/exec.h>
#include <exec/execbase.h>
extern struct ExecBase *SysBase;

struct Library *WorkbenchBase;

char *platform_get_amigaos3_runtime_os(void)
{
    char *retval = NULL;

    /* arosc.library only opens if the aros kernel is used */
    if (WorkbenchBase = OpenLibrary("arosc.library", 0)) {
        retval = "AROS";
    }

    /* Check for regular workbench */
    if (!retval && (WorkbenchBase = OpenLibrary("workbench.library", 45))) {
        retval = "AmigaOS-3.9";
    }
    if (!retval && (WorkbenchBase = OpenLibrary("workbench.library", 44))) {
        retval = "AmigaOS-3.5";
    }
    if (!retval && (WorkbenchBase = OpenLibrary("workbench.library", 40))) {
        retval = "AmigaOS-3.1";
    }
    if (!retval && (WorkbenchBase = OpenLibrary("workbench.library", 39))) {
        retval = "AmigaOS-3.0";
    }
    if (retval) {
        CloseLibrary(WorkbenchBase);
    } else {
        retval = "Unknown AmigaOS";
    }

    return retval;
}

char *platform_get_amigaos3_runtime_cpu(void)
{
    UWORD attnflags = SysBase->AttnFlags;

    if (attnflags & 0x80) {
        return "68060";
    }
    if (attnflags & AFF_68040) {
        return "68040";
    }
    if (attnflags & AFF_68030) {
        return "68030";
    }
    if (attnflags & AFF_68020) {
        return "68020";
    }
    if (attnflags & AFF_68010) {
        return "68010";
    }
    return "68000";
}
#endif
