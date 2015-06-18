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
   - Cygwin 1.5.25 (x86)
   - Cygwin 1.7.16 (x86)
   - Cygwin 1.7.25 (x86)
   - Cygwin 1.7.25 (x86_64)
*/

#include "vice.h"

#if !defined(WIN32_COMPILE) && (defined(__CYGWIN32__) || defined(__CYGWIN__))

#include <sys/utsname.h>
#include <stdio.h>

#include "platform.h"

static char api_version[200];
static int got_api_version = 0;
static char cygwin_cpu[100];
static int got_cygwin_cpu = 0;

char *platform_get_cygwin_runtime_os(void)
{
    int i = 0;
    struct utsname name;
    char temp[21];

    if (!got_api_version) {
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
        got_api_version = 1;
    }

    return api_version;
}

char *platform_get_cygwin_runtime_cpu(void)
{
    FILE *cpuinfo = NULL;
    char *buffer = NULL;
    char *loc1 = NULL;
    char *loc2 = NULL;
    char *loc3 = NULL;
    size_t size1 = 0;
    size_t size2 = 0;

    if (!got_cygwin_cpu) {
        sprintf(cygwin_cpu, "Unknown CPU");
        cpuinfo = fopen("/proc/cpuinfo", "rb");
        if (cpuinfo) {
            fclose(cpuinfo);
            cpuinfo = NULL;
            system("cp /proc/cpuinfo cpuinfo.tmp");
            cpuinfo = fopen("cpuinfo.tmp", "rb");
        }
        if (cpuinfo) {
            fseek(cpuinfo, 0L, SEEK_END);
            size1 = ftell(cpuinfo);
            fseek(cpuinfo, 0L, SEEK_SET);
            buffer = (char *)malloc(size1);
            size2 = fread(buffer, 1, size1, cpuinfo);
            if (size1 == size2) {
                loc1 = strstr(buffer, "model name");
                if (loc1) {
                    loc2 = strstr(loc1, ": ");
                    if (loc2) {
                        loc2 += 2;
                        loc3 = strstr(loc2, "\n");
                        if (loc3) {
                            *loc3 = 0;
                            sprintf(cygwin_cpu, "%s", loc2);
                            got_cygwin_cpu = 1;
                        }
                    }
                }
            }
            fclose(cpuinfo);
            unlink("cpuinfo.tmp");
            if (buffer) {
                free(buffer);
            }
        }
#ifndef PLATFORM_NO_X86_ASM
        if (!got_cygwin_cpu) {
            sprintf(cygwin_cpu, "%s", platform_get_x86_runtime_cpu());
            got_cygwin_cpu = 1;
        }
#endif
    }
    return cygwin_cpu;
}
#endif
