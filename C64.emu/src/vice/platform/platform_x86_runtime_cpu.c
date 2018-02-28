/*
 * platform_x86_runtime_cpu.c - x86 specific runtime discovery.
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

#include "platform_x86_runtime_cpu.h"

#ifndef PLATFORM_NO_X86_ASM
#include "types.h"
#include <string.h>

/* cpuid code for ow */
#ifdef WATCOM_COMPILE
#  define CPUID_DEFINED
#  include "platform_x86_watcom_cpuid.h"
#endif

/* cpuid code for amd64 msvc */
#if !defined(CPUID_DEFINED) && defined(_MSC_VER) && defined(_WIN64)
#  define CPUID_DEFINED
#  include "platform_amd64_msvc_cpuid.h"
#endif

/* cpuid code for x86 msvc / vac++ */
#if !defined(CPUID_DEFINED) && (defined(_MSC_VER) || defined(__IBMC__))
#  define CPUID_DEFINED
#  include "platform_x86_msvc_cpuid.h"
#endif

/* cpuid code for BeOS */
#if !defined(CPUID_DEFINED) && defined(BEOS_COMPILE)
#  define CPUID_DEFINED
#  include "platform_x86_beos_cpuid.h"
#endif

#ifndef CPUID_DEFINED
#  include "platform_x86_gcc_cpuid.h"
#endif

#define CPU_VENDOR_UNKNOWN     0
#define CPU_VENDOR_INTEL       1
#define CPU_VENDOR_UMC         2
#define CPU_VENDOR_AMD         3
#define CPU_VENDOR_CYRIX       4
#define CPU_VENDOR_NEXGEN      5
#define CPU_VENDOR_CENTAUR     6
#define CPU_VENDOR_RISE        7
#define CPU_VENDOR_SIS         8
#define CPU_VENDOR_TRANSMETA   9
#define CPU_VENDOR_NSC         10
#define CPU_VENDOR_VIA         11
#define CPU_VENDOR_IDT         12

typedef struct x86_cpu_vendor_s {
    char *string;
    int id;
    int (*identify)(void);
} x86_cpu_vendor_t;

static int is_idt_cpu(void)
{
    DWORD regax, regbx, regcx, regdx;

    cpuid(0xC0000000, regax, regbx, regcx, regdx);
    if (regax == 0xC0000000) {
        return 1;
    }
    return 0;
}

static x86_cpu_vendor_t x86_cpu_vendors[] = {
    { "GenuineIntel", CPU_VENDOR_INTEL, NULL },
    { "AuthenticAMD", CPU_VENDOR_AMD, NULL },
    { "AMDisbetter!", CPU_VENDOR_AMD, NULL },
    { "AMD ISBETTER", CPU_VENDOR_AMD, NULL },
    { "Geode by NSC", CPU_VENDOR_NSC, NULL },
    { "CyrixInstead", CPU_VENDOR_CYRIX, NULL },
    { "UMC UMC UMC ", CPU_VENDOR_UMC, NULL },
    { "NexGenDriven", CPU_VENDOR_NEXGEN, NULL },
    { "CentaurHauls", CPU_VENDOR_CENTAUR, NULL },
    { "RiseRiseRise", CPU_VENDOR_RISE, NULL },
    { "GenuineTMx86", CPU_VENDOR_TRANSMETA, NULL },
    { "TransmetaCPU", CPU_VENDOR_TRANSMETA, NULL },
    { "SiS SiS SiS ", CPU_VENDOR_SIS, NULL },
    { "VIA VIA VIA ", CPU_VENDOR_VIA, NULL },
    { NULL, CPU_VENDOR_IDT, is_idt_cpu },
    { NULL, CPU_VENDOR_UNKNOWN, NULL }
};

typedef struct x86_cpu_name_s {
    int id;
    DWORD fms;
    DWORD mask;
    char *name;
} x86_cpu_name_t;

static x86_cpu_name_t x86_cpu_names[] = {
    { CPU_VENDOR_INTEL, 0x00300, 0x00f00, "Intel 80386" },
    { CPU_VENDOR_INTEL, 0x00400, 0x00f00, "Intel 80486" },
    { CPU_VENDOR_INTEL, 0x00500, 0x00f00, "Intel Pentium" },
    { CPU_VENDOR_INTEL, 0x00600, 0x00f00, "Intel Pentium Pro/II/III/Celeron/Core/Core 2/Atom" },
    { CPU_VENDOR_INTEL, 0x00700, 0x00f00, "Intel Itanium" },
    { CPU_VENDOR_INTEL, 0x00f00, 0xf0f00, "Intel Pentium 4/Pentium D/Pentium Extreme Edition/Celeron/Xeon/Xeon MP" },
    { CPU_VENDOR_INTEL, 0x10f00, 0xf0f00, "Intel Itanium 2" },
    { CPU_VENDOR_INTEL, 0x20f00, 0xf0f00, "Intel Itanium 2 dual core" },
    { CPU_VENDOR_INTEL, 0x00000, 0x00000, "Unknown Intel CPU" },

    { CPU_VENDOR_AMD, 0x00300, 0x00f00, "AMD Am386" },
    { CPU_VENDOR_AMD, 0x00400, 0x00f00, "AMD Am486" },
    { CPU_VENDOR_AMD, 0x00500, 0x00f00, "AMD K5/K6" },
    { CPU_VENDOR_AMD, 0x00600, 0x00f00, "AMD Athlon/Duron" },
    { CPU_VENDOR_AMD, 0x00700, 0x00f00, "AMD Athlon64/Opteron/Sempron/Turion" },
    { CPU_VENDOR_AMD, 0x00f00, 0xf0f00, "AMD K8" },
    { CPU_VENDOR_AMD, 0x10f00, 0xf0f00, "AMD K8L" },
    { CPU_VENDOR_AMD, 0x00000, 0x00000, "Unknown AMD CPU" },

    { CPU_VENDOR_NSC, 0x00500, 0x00f00, "NSC Geode GX1" },
    { CPU_VENDOR_NSC, 0x00000, 0x00000, "Unknown NSC CPU" },

    { CPU_VENDOR_CYRIX, 0x00300, 0x00f00, "Cyrix C&T 3860xDX/SX" },
    { CPU_VENDOR_CYRIX, 0x00400, 0x00f00, "Cyrix Cx486" },
    { CPU_VENDOR_CYRIX, 0x00500, 0x00f00, "Cyrix M1" },
    { CPU_VENDOR_CYRIX, 0x00600, 0x00f00, "Cyrix M2" },
    { CPU_VENDOR_CYRIX, 0x00000, 0x00000, "Unknown Cyrix CPU" },

    { CPU_VENDOR_UMC, 0x00400, 0x00f00, "UMC 486 U5" },
    { CPU_VENDOR_UMC, 0x00000, 0x00000, "Unknown UMC CPU" },

    { CPU_VENDOR_NEXGEN, 0x00500, 0x00f00, "NexGen Nx586" },
    { CPU_VENDOR_NEXGEN, 0x00000, 0x00000, "Unknown NexGen CPU" },

    { CPU_VENDOR_CENTAUR, 0x00500, 0x00f00, "Centaur C6" },
    { CPU_VENDOR_CENTAUR, 0x00000, 0x00000, "Unknown Centaur CPU" },

    { CPU_VENDOR_RISE, 0x00500, 0x00f00, "Rise mP6" },
    { CPU_VENDOR_RISE, 0x00000, 0x00000, "Unknown Rise CPU" },

    { CPU_VENDOR_TRANSMETA, 0x00500, 0x00f00, "Transmeta Crusoe" },
    { CPU_VENDOR_TRANSMETA, 0x00000, 0x00000, "Unknown Transmeta CPU" },

    { CPU_VENDOR_SIS, 0x00500, 0x00f00, "SiS 55x" },
    { CPU_VENDOR_SIS, 0x00000, 0x00000, "Unknown SiS CPU" },

    { CPU_VENDOR_VIA, 0x00600, 0x00f00, "VIA C3" },
    { CPU_VENDOR_VIA, 0x00000, 0x00000, "Unknown VIA CPU" },

    { CPU_VENDOR_IDT, 0x00500, 0x00f00, "IDT WinChip" },
    { CPU_VENDOR_IDT, 0x00000, 0x00000, "Unknown IDT CPU" },

    { CPU_VENDOR_UNKNOWN, 0x00300, 0x00f00, "Unknown 80386 compatible CPU" },
    { CPU_VENDOR_UNKNOWN, 0x00400, 0x00f00, "Unknown 80486 compatible CPU" },
    { CPU_VENDOR_UNKNOWN, 0x00500, 0x00f00, "Unknown Pentium compatible CPU" },
    { CPU_VENDOR_UNKNOWN, 0x00600, 0x00f00, "Unknown Pentium Pro compatible CPU" },
    { CPU_VENDOR_UNKNOWN, 0x00000, 0x00000, "Unknown CPU" },

    { 0, 0, 0, NULL }
};

/* return cpu name if present */
static char *get_x86_runtime_cpu_name(void)
{
/* makes certain binaries crash */
#if 0
    DWORD regax, regbx, regcx, regdx;
    static char name_buf[49];
    int i;

    cpuid(0x80000000, regax, regbx, regcx, regdx);
    if (regax >= 0x80000004) {
        for (i = 0; i < 3; i++) {
            cpuid(0x80000002 + i, regax, regbx, regcx, regdx);
            memcpy(name_buf + (i * 16) + 0, &regax, 4);
            memcpy(name_buf + (i * 16) + 4, &regbx, 4);
            memcpy(name_buf + (i * 16) + 8, &regcx, 4);
            memcpy(name_buf + (i * 16) + 12, &regdx, 4);
        }
        name_buf[48] = 0;
        return name_buf;
    }
#endif
    return NULL;
}

/* runtime cpu detection */
char *platform_get_x86_runtime_cpu(void)
{
    DWORD regax, regbx, regcx, regdx;
    char *cpu_name;
    char type_buf[13];
    int hasCPUID;
    int found = 0;
    int id = CPU_VENDOR_UNKNOWN;
    int i;

    hasCPUID = has_cpuid();
    if (hasCPUID && (cpu_name = get_x86_runtime_cpu_name()) != NULL) {
        return cpu_name;
    } else if (hasCPUID) {
        cpuid(0, regax, regbx, regcx, regdx);
        memcpy(type_buf + 0, &regbx, 4);
        memcpy(type_buf + 4, &regdx, 4);
        memcpy(type_buf + 8, &regcx, 4);
        type_buf[12] = 0;
        for (i = 0; (x86_cpu_vendors[i].id != CPU_VENDOR_UNKNOWN) && (found == 0); i++) {
            if (x86_cpu_vendors[i].identify != NULL) {
                if (x86_cpu_vendors[i].identify() == 1) {
                    id = x86_cpu_vendors[i].id;
                    found = 1;
                }
            } else {
                if (!strcmp(type_buf, x86_cpu_vendors[i].string)) {
                    id = x86_cpu_vendors[i].id;
                    found = 1;
                }
            }
        }
        cpuid(1, regax, regbx, regcx, regdx);
        for (i = 0; x86_cpu_names[i].name != NULL; i++) {
            if ((regax & x86_cpu_names[i].mask) == x86_cpu_names[i].fms && x86_cpu_names[i].id == id) {
                return x86_cpu_names[i].name;
            }
        }
        return "Unknown CPU";
    } else {
        return "No cpuid instruction present, output not implemented yet.";
    }
}
#endif
