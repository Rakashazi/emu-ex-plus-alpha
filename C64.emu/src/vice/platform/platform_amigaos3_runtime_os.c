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
 * 68000
 * 68010
 * 68020
 * 68030
 * 68040
 * 68060
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

#if defined(__VBCC__) && defined(__PPC__)
#  include <proto/powerpc.h>
#  ifdef _VBCCINLINE_POWERPC_H
#    define IS_WARPOS
#    include <powerpc/powerpc.h>
#  else
#    define IS_POWERUP
#    include <ppclib/interface.h>
#    include <ppclib/ppc.h>
#  endif
#endif

#include <proto/exec.h>
#include <exec/execbase.h>
extern struct ExecBase *SysBase;

struct Library *WorkbenchBase = NULL;
struct Library *VersionBase = NULL;

static char *wbretval = NULL;
static char *ksretval = NULL;
static char osretval[100];
static int got_cpu = 0;
static char cpu_retval[100];

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
    { "OS4x", 52, -1, -1 },
    { "OS4x", 53, -1, -1 },
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
    { "4.0", 50, -1 },
    { "4.0", 51, -1 },
    { "4.0", 52, -1 },
    { "4.1", 53, -1 },
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

#if defined(IS_POWERUP) || defined(IS_WARPOS)
static UWORD M68kGetAttn[] =
{
/*
    movem.l    d1/d2/a0-a1/a6,-(sp)    $48E760C2
    move.l     (4).w,a6                $2C780004
    move.w     (AttnFlags,a6),d0       $302E0128
    movem.l    (sp)+,d1/d2/a0-a1/a6    $4CDF4306
    rts                                $4E75
 */
    0x48E7,
    0x60C2,
    0x2C78,
    0x0004,
    0x302E,
    0x0128,
    0x4CDF,
    0x4306,
    0x4E75
};

static UWORD M68kGetWB[] = {
/*
    movem.l    d1-a6,-(sp)             $48E77FFE
    moveq      #0,d7                   $7E00
    move.l     (4).w,a6                $2C780004
    lea        (versionname,pc),a1     $43FA001E
    moveq      #0,d0                   $7000
    call       OpenLibrary             $4EAEFDD8
    tst.l      d0                      $4A80
    beq.b      exit                    $670A
    move.l     d0,a1                   $2240
    move.l     (LIB_VERSION,a1),d7     $2E290014
    call       CloseLibrary            $4EAEFE62
exit:
    move.l     d7,d0                   $2007
    movem.l    (sp)+,d1-a6             $4CDF7FFE
    rts                                $4E75
versionname:
    dc.b        'version.library',0    $76657273696F6E2E6C69627261727900
*/

    0x48E7,
    0x7FFE,
    0x7E00,
    0x2C78,
    0x0004,
    0x43FA,
    0x001E,
    0x7000,
    0x4EAE,
    0xFDD8,
    0x4A80,
    0x670A,
    0x2240,
    0x2E29,
    0x0014,
    0x4EAE,
    0xFE62,
    0x2007,
    0x4CDF,
    0x7FFE,
    0x4E75,
    0x7665,
    0x7273,
    0x696F,
    0x6E2E,
    0x6C69,
    0x6272,
    0x6172,
    0x7900
};

static UWORD M68kGetKS[] = {
/*
GetKSVer:
    movem.l    d2-a6,-(sp)             $48E73FFE
    move.l     (4).w,a6                $2C780004
    move.l     (LIB_VERSION,a6),d0     $202E0014
    moveq      #0,d1                   $7200
    move.w     (SoftVer,a6),d1         $322E0020
    movem.l    (sp)+,d2-a6             $4CDF7FFC
    rts                                $4E75
 */
    0x48E7,
    0x3FFE,
    0x2C78,
    0x0004,
    0x202E,
    0x0014,
    0x7200,
    0x322E,
    0x0020,
    0x4CDF,
    0x7FFC,
    0x4E75
};
#endif

char *platform_get_amigaos3_runtime_os(void)
{
#ifdef IS_POWERUP
    struct Caos *MyCaos = NULL;
#endif
#ifdef IS_WARPOS
    struct PPCArgs *pa = NULL;
#endif
#if defined(IS_POWERUP) || defined(IS_WARPOS)
    ULONG wb_version = 0;
    ULONG ks_version = 0;
    ULONG ks_softver = 0;
#endif

#ifdef IS_POWERUP
    if (!wbretval) {
        if (MyCaos = (struct Caos*)PPCAllocVec(sizeof(struct Caos), MEMF_PUBLIC|MEMF_CLEAR)) {
            MyCaos->caos_Un.Function = (APTR)&M68kGetWB;
            MyCaos->M68kCacheMode = IF_CACHEFLUSHNO;
            MyCaos->PPCCacheMode = IF_CACHEFLUSHNO;
            PPCCallM68k(MyCaos);
            wb_version = (ULONG)MyCaos->d0;
            PPCFreeVec(MyCaos);
            MyCaos = NULL;
        }
        wbretval = number2workbench(wb_version >> 16, wb_version & 0xffff);
    }

    if (!ksretval) {
        if (MyCaos = (struct Caos*)PPCAllocVec(sizeof(struct Caos), MEMF_PUBLIC|MEMF_CLEAR)) {
            MyCaos->caos_Un.Function = (APTR)&M68kGetKS;
            MyCaos->M68kCacheMode = IF_CACHEFLUSHNO;
            MyCaos->PPCCacheMode = IF_CACHEFLUSHNO;
            PPCCallM68k(MyCaos);
            ks_version = (ULONG)MyCaos->d0;
            ks_softver = (ULONG)MyCaos->d1;
            PPCFreeVec(MyCaos);
        }
        ksretval = number2kickstart(ks_version >> 16, ks_version & 0xffff, ks_softver);
    }
#else
#ifdef IS_WARPOS
    if (!wbretval) {
        if (pa = (struct PPCArgs*)AllocVecPPC(sizeof(struct PPCArgs), MEMF_PUBLIC|MEMF_CLEAR, 0)) {
            pa->PP_Code = (APTR)&M68kGetWB;
            Run68K(pa);
            WaitFor68K(pa);
            wb_version = (ULONG)pa->PP_Regs[PPREG_D0];
            FreeVecPPC(pa);
            pa = NULL;
        }
        wbretval = number2workbench(wb_version >> 16, wb_version & 0xffff);
    }

    if (!ksretval) {
        if (pa = (struct PPCArgs*)AllocVecPPC(sizeof(struct PPCArgs), MEMF_PUBLIC|MEMF_CLEAR, 0)) {
            pa->PP_Code = (APTR)&M68kGetKS;
            Run68K(pa);
            WaitFor68K(pa);
            ks_version = (ULONG)pa->PP_Regs[PPREG_D0];
            ks_softver = (ULONG)pa->PP_Regs[PPREG_D1];
            FreeVecPPC(pa);
        }
        ksretval = number2kickstart(ks_version >> 16, ks_version & 0xffff, ks_softver);
    }
#else
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
#endif
#endif

    if (wbretval && wbretval[0] == 'A') {
        sprintf(osretval, "AROS");
    } else if (ksretval && ksretval[0] == 'A') {
        sprintf(osretval, "WorkBench %s (AROS KickStart ROM)", wbretval ? wbretval : "Unknown");
    } else if (ksretval && ksretval[0] == 'O') {
        sprintf(osretval, "WorkBench %s (BlackBox/Petunia)", wbretval ? wbretval : "Unknown");
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

#ifndef AFF_603
#define AFF_603 (1<<8)
#endif

#ifndef AFF_604
#define AFF_604 (1<< 9)
#endif

#ifndef AFF_750
#define AFF_750 (1<<10)
#endif

#ifndef AFF_7400
#define AFF_7400 (1<<11)
#endif

#ifndef AFF_4XX
#define AFF_4XX (1<<13)
#endif

char *platform_get_amigaos3_runtime_cpu(void)
{
#ifdef IS_POWERUP
    struct Caos *MyCaos = NULL;
#endif

#ifdef IS_WARPOS
    struct PPCArgs *pa = NULL;
    struct TagItem ti_cputype[] = {{GETINFO_CPU, 0}, {TAG_END, 0}};
    int cpu_type = 0;
#endif

    UWORD attn = 0;
    char *ppc = NULL;
    char *m68k = NULL;

    if (!got_cpu) {
#ifdef IS_POWERUP
        if (MyCaos = (struct Caos*)PPCAllocVec(sizeof(struct Caos), MEMF_PUBLIC|MEMF_CLEAR)) {
            MyCaos->caos_Un.Function = (APTR)&M68kGetAttn;
            MyCaos->M68kCacheMode = IF_CACHEFLUSHNO;
            MyCaos->PPCCacheMode = IF_CACHEFLUSHNO;
            PPCCallM68k(MyCaos);
            attn = (UWORD)MyCaos->d0;
            PPCFreeVec(MyCaos);
        }

        switch (PPCGetAttr(PPCINFOTAG_CPU)) {
            case CPU_603:
                ppc = "PPC603";
                break;
            case CPU_604:
                ppc = "PPC604";
                break;
            case CPU_602:
                ppc = "PPC602";
                break;
            case CPU_603e:
                ppc = "PPC603e";
                break;
            case CPU_603p:
                ppc = "PPC603p";
                break;
            case CPU_604e:
                ppc = "PPC604e";
                break;
            default:
                ppc = "PPC";
        }
#else
#ifdef IS_WARPOS
        if (pa = (struct PPCArgs*)AllocVecPPC(sizeof(struct PPCArgs), MEMF_PUBLIC|MEMF_CLEAR, 0)) {
            pa->PP_Code = (APTR)&M68kGetAttn;
            Run68K(pa);
            WaitFor68K(pa);
            attn = (UWORD)pa->PP_Regs[PPREG_D0];
            FreeVecPPC(pa);
        }

        GetInfo(ti_cputype);
        cpu_type = ti_cputype[0].ti_Data;

        switch (cpu_type) {
            case CPUF_620:
                ppc = "PPC620";
                break;
            case CPUF_604E:
                ppc = "PPC604e";
                break;
            case CPUF_604:
                ppc = "PPC604";
                break;
            case CPUF_603E:
                ppc = "PPC603e";
                break;
            case CPUF_603:
                ppc = "PPC603";
                break;
            default:
                ppc = "PPC";
        }
#else
        attn = SysBase->AttnFlags;

        if (attn & AFF_4XX) {
            ppc = "PPC4xx";
        } else if (attn & AFF_7400) {
            ppc = "PPC7400";
        } else if (attn & AFF_750) {
            ppc = "PPC750";
        } else if (attn & AFF_604) {
            ppc = "PPC604";
        } else if (attn & AFF_603) {
            ppc = "PPC603";
        }
#endif
#endif

        if (attn & AFF_68060) {
            m68k = "68060";
        } else if (attn & AFF_68040) {
            m68k = "68040";
        } else if (attn & AFF_68030) {
            m68k = "68030";
        } else if (attn & AFF_68020) {
            m68k = "68020";
        } else if (attn & AFF_68010) {
            m68k = "68010";
        } else {
            m68k = "68000";
        }

        if (ppc) {
#if defined(IS_POWERUP) || defined(IS_WARPOS)
            sprintf(cpu_retval, "%s (m68k code on %s)", ppc, m68k);
#else
            sprintf(cpu_retval, "%s (emulated on %s)", m68k, ppc);
#endif
        } else {
            sprintf(cpu_retval, "%s", m68k);
        }

        got_cpu = 1;
    }
    return cpu_retval;
}
#endif
