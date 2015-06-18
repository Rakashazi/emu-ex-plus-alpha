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

/* Tested and confirmed working on the following CPU types:
 */

/* Tested and confirmed working on the following WorkBench versions:
 * 1.0
 * 1.1
 * 1.2 (33.56)
 * 1.3 (34.20)
 * 1.3.2 (34.28)
 * 1.3.3 (34.34)
 * 1.4 (36.8)
 * 2.0 (36.68)
 * 2.04 (37.67)
 * 2.05 (37.71)
 * 2.1 (38.35)
 * 2.1.1 (38.36)
 * 3.0 (39.29)
 * 3.1 (40.42)
 * 3.5 (44.2)
 * 3.5-BB2 (44.5)
 * 3.9 (45.1)
 * 3.9-BB1 (45.2)
 * 3.9-BB2 (45.3)
 */

/* Tested and confirmed working on the following KickStart versions:
 * 1.1 (31.34)
 * 1.1 (32.34)
 * 1.2 (33.166)
 * 1.2 (33.180)
 * 1.3 (34.5)
 * 2.0 (36.67)
 * 2.0 (36.143)
 * 2.04 (37.175)
 * 2.05 (37.210)
 * 2.05 (37.299)
 * 2.05 (37.300)
 * 2.05 (37.350)
 * 3.0 (39.106)
 * 3.1 (40.55)
 * 3.1 (40.60)
 * 3.1 (40.62)
 * 3.1 (40.68)
 * 3.1 (40.70)
 * 3.2 (43.1)
 * AROS KickStart ROM (51.51)
 */

#include "vice.h"

#ifdef AMIGA_M68K

#define __USE_INLINE__

#include <proto/exec.h>
#include <exec/execbase.h>
extern struct ExecBase *SysBase;

struct Library *WorkbenchBase = NULL;
struct Library *VersionBase = NULL;

static char *wbretval = NULL;
static char *ksretval = NULL;
static char osretval[100];
static char *cpuretval = NULL;

typedef struct ksver_s {
    char *name;
    int major;
    int minor;
    int softver;
} ksver_t;

static ksver_t ks_versions[] = {
    { "1.0", 30, -1, -1 },
    { "1.1", 31, -1, -1 },
    { "1.1", 32, -1, -1 },
    { "1.2", 33, -1, -1 },
    { "1.3", 34, -1, -1 },
    { "1.3", 37, 201, 30 },
    { "1.4", 35, -1, -1 },
    { "1.4", 36, -1, 16 },
    { "2.0", 36, -1, -1 },
    { "2.04", 37, -1, 175 },
    { "2.05", 37, -1, 210 },
    { "2.05", 37, -1, 299 },
    { "2.05", 37, -1, 300 },
    { "2.05", 37, -1, 350 },
    { "2.1", 38, -1, -1 },
    { "3.0", 39, -1, -1 },
    { "3.1", 40, -1, -1 },
    { "3.2", 43, -1, -1 },
    { "3.5", 45, -1, -1 },
    { "AROS", 51, -1, -1 },
    { NULL, 0, 0, 0 }
};

static char *number2kickstart(int major, int minor, int softver)
{
    int i;

    for (i = 0; ks_versions[i].name; i++) {
        if (ks_versions[i].major == major) {
            if (ks_versions[i].minor == -1 || ks_versions[i].minor == minor) {
                if (ks_versions[i].softver == -1 || ks_versions[i].softver == softver) {
                    return ks_versions[i].name;
                }
            }
        }
    }
    return NULL;
}

typedef struct wbver_s {
    char *name;
    int major;
    int minor;
} wbver_t;

static wbver_t wb_versions[] = {
    { "1.0", 30, -1 },
    { "1.1", 31, -1 },
    { "1.1", 32, -1 },
    { "1.1", 1, 1 },
    { "1.2", 33, -1 },
    { "1.3.1", 34, 25 },
    { "1.3.2", 34, 28 },
    { "1.3.3", 34, 34 },
    { "1.3", 34, -1 },
    { "1.4", 36, 1123 },
    { "1.4", 36, 1228 },
    { "1.4", 36, 16 },
    { "1.4", 36, 8 },
    { "1.4", 36, 993 },
    { "2.01", 36, 69 },
    { "2.02", 36, 70 },
    { "2.03", 36, 102 },
    { "2.04", 36, 67 },
    { "2.04", 37, 67 },
    { "2.0", 36, -1 },
    { "2.05", 37, -1 },
    { "2.1.1", 38, 36 },
    { "2.1", 38, -1 },
    { "3.0", 39, -1 },
    { "3.1", 40, -1 },
    { "3.2", 43, -1 },
    { "3.5", 44, 2 },
    { "3.5-BB1", 44, 4 },
    { "3.5-BB2", 44, 5 },
    { "3.9", 45, 1 },
    { "3.9-BB1", 45, 2 },
    { "3.9-BB2", 45, 3 },
    { NULL, 0, 0 }
};

static char *number2workbench(int major, int minor)
{
    int i;

    for (i = 0; wb_versions[i].name; i++) {
        if (wb_versions[i].major == major) {
            if (wb_versions[i].minor == -1 || wb_versions[i].minor == minor) {
                return wb_versions[i].name;
            }
        }
    }
    return NULL;
}

char *platform_get_amigaos3_runtime_os(void)
{
    if (!wbretval) {
        /* arosc.library only opens if the aros kernel is used */
        if (WorkbenchBase = OpenLibrary("arosc.library", 0)) {
            wbretval = "AROS";
        } else {
            if (VersionBase = OpenLibrary("version.library", 0)) {
                wbretval = number2workbench(VersionBase->lib_Version, VersionBase->lib_Revision);
                if (!wbretval) {
                    printf("WB major: %d, minor: %d\n", VersionBase->lib_Version, VersionBase->lib_Revision);
                }
            } else {
                wbretval = "1.0";
            }
        }

        if (VersionBase) {
            CloseLibrary(VersionBase);
        }
    }

    if (!ksretval) {
        ksretval = number2kickstart(SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision, SysBase->SoftVer);
    }

    if (wbretval && wbretval[0] == 'A') {
        sprintf(osretval, "AROS");
    } else if (ksretval && ksretval[0] == 'A') {
        sprintf(osretval, "WorkBench %s (AROS KickStart ROM)", wbretval ? wbretval : "Unknown");
    } else {
        sprintf(osretval, "WorkBench %s (KickStart %s)", wbretval ? wbretval : "Unknown", ksretval ? ksretval : "Unknown");
    }

    return osretval;
}

#ifndef AFB_68060
#define AFB_68060 7
#endif

#ifndef AFF_68060
#define AFF_68060 (1<<AFB_68060)
#endif

char *platform_get_amigaos3_runtime_cpu(void)
{
    UWORD attn = SysBase->AttnFlags;

    if (attn & AFF_68060) {
        return "68060";
    }
    if (attn & AFF_68040) {
        return "68040";
    }
    if (attn & AFF_68030) {
        return "68030";
    }
    if (attn & AFF_68020) {
        return "68020";
    }
    if (attn & AFF_68010) {
        return "68010";
    }
    return "68000";
}
#endif
