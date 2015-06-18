/*
 * platform_aros_runtime_os.c - AROS runtime version discovery.
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

#include "vice.h"

/* Tested and confirmed working on:
   - i386-aros (mingw hosted)
   - i386-aros (native)
*/

#ifdef AMIGA_AROS

#include <aros/inquire.h>
#include <aros/arosbase.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/aros.h>
#include <proto/processor.h>
#include <resources/processor.h>

#include <stdio.h>

struct Library *ArosBase;
APTR ProcessorBase;

static char runtime_os[256];
static int got_runtime_os = 0;

static CONST_STRPTR modelstring = NULL;

char *platform_get_aros_runtime_os(void)
{
    int rc;
    ULONG relMajor, relMinor;
    STRPTR arch;

    if (!got_runtime_os) {
        if (!(ArosBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION))) {
            sprintf(runtime_os, "Unknown AROS version");
            got_runtime_os = 1;
        }
    }

    if (!got_runtime_os) {
        ArosInquire(AI_ArosReleaseMajor, (IPTR)&relMajor, AI_ArosReleaseMinor, (IPTR)&relMinor, AI_ArosArchitecture, (IPTR)&arch, TAG_DONE);
        sprintf(runtime_os, "AROS-%ld.%ld (%s)", relMajor, relMinor, arch);
        got_runtime_os = 1;
        CloseLibrary(ArosBase);
    }

    return runtime_os;
}

char *platform_get_aros_runtime_cpu(void)
{
    if (!modelstring) {
        ProcessorBase = OpenResource(PROCESSORNAME);
        GetCPUInfoTags(GCIT_ModelString, (IPTR)&modelstring, TAG_DONE);
    }
    return (char *)modelstring;
}
#endif
