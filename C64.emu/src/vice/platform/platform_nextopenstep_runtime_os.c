/*
 * platform_nextopenstep_runtime_os.c - NextStep/OpenStep runtime version discovery.
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
 * - NextStep 3.3
 * - OpenStep 4.2
 */

#include "vice.h"

#if defined(NEXTSTEP_COMPILE) || defined(OPENSTEP_COMPILE)

#include <stdio.h>
#include <mach/mach.h>

static char os[100];
static int got_os = 0;

static char cpu[100];
static int got_cpu = 0;

char *platform_get_nextopenstep_runtime_os(void)
{
    kern_return_t ret;
    kernel_version_t string;
    char *version = NULL;
    int i = 0;

    if (!got_os) {
        ret = host_kernel_version(host_self(), string);
        if (ret != KERN_SUCCESS) {
            sprintf(os, "Unknown NextStep or OpenStep version\n");
        } else {
            version = string;
            while (*version != ' ') {
                version ++;
            }
            version++;
            while (*version != ' ') {
                version ++;
            }
            version++;
            while (version[i] != ':') {
                i++;
            }
            version[i] = 0;
            if (version[0] == '4') {
                sprintf(os, "OpenStep %s", version);
            } else {
                sprintf(os, "NextStep %s", version);
            }
        }
        got_os = 1;
    }
    return os;
}

char *platform_get_nextopenstep_runtime_cpu(void)
{
    kern_return_t ret;
    struct host_basic_info hi;
    unsigned int count = HOST_BASIC_INFO_COUNT;
    char *cpu_name = NULL;
    char *cpu_subname = NULL;

    if (!got_cpu) {
        ret = host_info(host_self(), HOST_BASIC_INFO, (host_info_t)&hi, &count);
        if (ret != KERN_SUCCESS) {
            sprintf(cpu, "Unknown CPU");
        } else {
            slot_name(hi.cpu_type, hi.cpu_subtype, &cpu_name, &cpu_subname);
            sprintf(cpu, "%s (%s)", cpu_name, cpu_subname);
        }
        got_cpu = 1;
    }
    return cpu;
}
#endif
