/*
 * platform_darwin_runtime_os.c - Darwin (and Mac OS X) runtime version discovery.
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
 * - Darwin 6.0.2 (ppc)
 * - Darwin 8.0.1 (x86)
 */

#include "vice.h"

#ifdef DARWIN_COMPILE

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <mach/mach.h>

#ifdef __ppc__
#include "archdep.h"
#include "lib.h"
#include "util.h"
#endif

static char buffer[4096];

static char os[100];
static int got_os = 0;

static char cpu[100];
static int got_cpu = 0;

char *platform_get_darwin_runtime_os(void)
{
    int isserver = 1;
    int amount = 0;
    int done = 0;
    int i = 0;
    int j = 0;
    FILE *infile = NULL;
    char *osname = NULL;
    char *osversion = NULL;
    struct utsname name;

    if (!got_os) {
        uname(&name);

        /* first try to open /System/Library/CoreServices/ServerVersion.plist */
        infile = fopen("/System/Library/CoreServices/ServerVersion.plist", "rb");

        /* if the file could not be opened, it is not a server,
           so try to open /System/Library/CoreServices/SystemVersion.plist */
        if (!infile) {
            isserver = 0;
            infile = fopen("/System/Library/CoreServices/SystemVersion.plist", "rb");
        }

        /* find out the version from the open file */
        if (infile) {
            amount = fread(buffer, 1, 4095, infile);
            if (amount) {
                osname = strstr(buffer, "<key>ProductName</key>");
                osversion = strstr(buffer, "<key>ProductVersion</key>");
                if (osname && osversion) {
                    osname = strstr(osname, "<string>");
                    osversion = strstr(osversion, "<string>");
                    if (osname && osversion) {
                        osname += 8;
                        osversion += 8;
                        while (osname[i] != 0 && osname[i] != '<') {
                            i++;
                        }
                        while (osversion[j] != 0 && osversion[j] != '<') {
                            j++;
                        }
                        if (osversion[j] == '<' && osname[i] == '<') {
                            osversion[j] = 0;
                            osname[i] = 0;
                            done = 1;
                        }
                    }
                }
            }
            fclose(infile);
        }

        if (done) {
            sprintf(os, "%s %s", osname, osversion);
        } else {
            sprintf(os, "Darwin %s", name.release);
        }
        got_os = 1;
    }
    return os;
}

char *platform_get_darwin_runtime_cpu(void)
{
    kern_return_t ret;
    struct host_basic_info hi;
    unsigned int count = HOST_BASIC_INFO_COUNT;
    char *cpu_name = NULL;
    char *cpu_subname = NULL;
#ifdef __ppc__
    FILE *infile = NULL;
    int amount = 0;
    char *tempfile = NULL;
    char *tempsystem = NULL;
#endif

    if (!got_cpu) {
        ret = host_info(host_self(), HOST_BASIC_INFO, (host_info_t)&hi, &count);
        if (ret != KERN_SUCCESS) {
            sprintf(cpu, "Unknown CPU");
        } else {
            slot_name(hi.cpu_type, hi.cpu_subtype, &cpu_name, &cpu_subname);
            sprintf(cpu, "%s (%s)", cpu_name, cpu_subname);
        }

#ifdef __ppc__
        tempfile = archdep_tmpnam();
        tempsystem = util_concat("uname -m >", tempfile, NULL);
        system(tempsystem);
        infile = fopen(tempfile, "rb");
        if (infile) {
            amount = fread(buffer, 1, 4095, infile);
            if (amount) {
                buffer[strlen(buffer)] = 0;
                if (strcmp(buffer, "Power Macintosh")) {
                    sprintf(cpu, "%s [Rosetta (%s)]", cpu, buffer);
                }
            }
            fclose(infile);
            unlink(tempfile);
        }
        lib_free(tempsystem);
        lib_free(tempfile);
#endif
        got_cpu = 1;
    }
    return cpu;
}
#endif
