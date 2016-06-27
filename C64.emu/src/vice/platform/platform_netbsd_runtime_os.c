/*
 * platform_netbsd_runtime_os.c - NetBSD runtime version discovery.
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
   cpu      | netbsd version
   -------------------------
   i386     | 0.9
   i386     | 1.0
   i386     | 1.1
   i386     | 1.2
   i386     | 1.2.1
   i386     | 1.3
   i386     | 1.3.1
   i386     | 1.3.2
   i386     | 1.3.3
   i386     | 1.4
   i386     | 1.4.1
   i386     | 1.4.2
   i386     | 1.4.3
   i386     | 1.5
   i386     | 1.5.1
   i386     | 1.5.2
   i386     | 1.5.3
   i386     | 1.6
   i386     | 1.6.1
   i386     | 1.6.2
   i386     | 2.0
   i386     | 2.0.2
   i386     | 2.1
   i386     | 3.0
   i386     | 3.0.1
   i386     | 3.0.2
   i386     | 3.0.3
   i386     | 3.1
   i386     | 3.1.1
   i386     | 4.0
   i386     | 4.0.1
   i386     | 5.0
   i386     | 5.0.1
   i386     | 5.0.2
   i386     | 5.1
   i386     | 5.1.1
   i386     | 5.1.2
   i386     | 5.1.3
   i386     | 5.1.4
   i386     | 5.1.5
   i386     | 5.2
   i386     | 5.2.1
   i386     | 5.2.2
   i386     | 5.2.3
   i386     | 6.0
   i386     | 6.0.1
   i386     | 6.0.2
   i386     | 6.0.3
   i386     | 6.0.4
   i386     | 6.0.5
   i386     | 6.0.6
   i386     | 6.1
   i386     | 6.1.1
   i386     | 6.1.2
   i386     | 6.1.3
   i386     | 6.1.4
   amd64    | 6.1.5
   i386     | 6.1.5
   m68k     | 6.1.5
   mipsel   | 6.1.5
   mipsel64 | 6.1.5
   sparc    | 6.1.5
   sparc64  | 6.1.5
   vax      | 6.1.5
   amd64    | 7.0
   i386     | 7.0
 */

#include "vice.h"

#ifdef __NetBSD__

#include <stdio.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "archdep.h"
#include "lib.h"
#include "platform.h"
#include "util.h"
#include "log.h"

static char netbsd_version[100];
static char netbsd_cpu[100];
static int got_netbsd_version = 0;
static int got_netbsd_cpu = 0;

char *platform_get_netbsd_runtime_cpu(void)
{
    FILE *cpuinfo = NULL;
    char *buffer = NULL;
    char *loc1 = NULL;
    char *loc2 = NULL;
    char *loc3 = NULL;
    char *tempfile = NULL;
    char tempsystem[512];
    size_t size1 = 0;
    size_t size2 = 0;
    struct utsname name;
#if !defined(__i386__) && !defined(__amd64__)
    char *cpu = NULL;
    char *machine = NULL;
    char *model = NULL;
    size_t len = 0;
#endif

    if (!got_netbsd_cpu) {
        sprintf(netbsd_cpu, "Unknown CPU");
#if !defined(__i386__) && !defined(__amd64__)
        if (sysctlbyname("hw.machine_arch", NULL, &len, NULL, 0) == 0) {
            cpu = lib_malloc(len);
            sysctlbyname("hw.machine_arch", cpu, &len, NULL, 0);

            sysctlbyname("hw.model", NULL, &len, NULL, 0);
            model = lib_malloc(len);
            sysctlbyname("hw.model", model, &len, NULL, 0);

            sysctlbyname("hw.machine", NULL, &len, NULL, 0);
            machine = lib_malloc(len);
            sysctlbyname("hw.machine", machine, &len, NULL, 0);

            sprintf(netbsd_cpu, "%s", cpu);

            /* Get specific CPU/MMU/FPU for m68k */
            if (!strcasecmp(cpu, "m68k")) {
                loc1 = strstr(model, "(");
                if (loc1) {
                    loc1++;
                    loc2 = strstr(loc1, ")");
                    if (loc2) {
                        loc2[0] = 0;
                        sprintf(netbsd_cpu, "%s", loc1);
                    }
                }
            }

            /* Get specific CPU for hpcmips */
            if (!strcasecmp(cpu, "mipsel")) {
                sprintf(netbsd_cpu, "%s", cpu);
                if (!strcasecmp(machine, "hpcmips")) {
                    loc1 = strstr(model, "(");
                    if (loc1) {
                        loc1++;
                        loc2 = strstr(loc1, ")");
                        if (loc2) {
                            loc2[0] = 0;
                            sprintf(netbsd_cpu, "%s (%s)", cpu, loc1);
                        }
                    }
                }
            }

            /* Get specific CPU for sparc or sparc64 */
            if (!strcasecmp(cpu, "sparc") || !strcasecmp(cpu, "sparc64")) {
                loc1 = strstr(model, "(");
                if (loc1) {
                    loc1++;
                    loc2 = strstr(loc1, ")");
                    if (loc2) {
                        loc2[0] = 0;
                        sprintf(netbsd_cpu, "%s (%s)", cpu, loc1);
                    }
                }
            }

            if (cpu) {
                lib_free(cpu);
            }
            if (machine) {
                lib_free(machine);
            }
            if (model) {
                lib_free(model);
            }
            got_netbsd_cpu = 1;
        } else
#endif
        {
            cpuinfo = fopen("/proc/cpuinfo", "rb");
            if (cpuinfo) {
                fclose(cpuinfo);
                cpuinfo = NULL;
                tempfile = archdep_tmpnam();
                sprintf(tempsystem, "cat /proc/cpuinfo >%s", tempfile);
                if (system(tempsystem) < 0) {
                    log_warning(LOG_ERR, "`%s' failed.", tempsystem);
                }
                cpuinfo = fopen(tempfile, "rb");
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
                        loc2 = strstr(loc1, ":");
                        if (loc2) {
                            loc2 += 2;
                            while (isspace((int)*loc2)) {
                                loc2++;
                            }
                            loc3 = strstr(loc2, "\n");
                            if (loc3) {
                                *loc3 = 0;
                                sprintf(netbsd_cpu, "%s", loc2);
                                got_netbsd_cpu = 1;
                            }
                        }
                    }
                }
                fclose(cpuinfo);
                unlink(tempfile);
                lib_free(tempfile);
                if (buffer) {
                    free(buffer);
                }
            }
        }
#ifndef PLATFORM_NO_X86_ASM
        if (!got_netbsd_cpu) {
            sprintf(netbsd_cpu, "%s", platform_get_x86_runtime_cpu());
            got_netbsd_cpu = 1;
        }
#endif
        if (!got_netbsd_cpu) {
            uname(&name);
            sprintf(netbsd_cpu, "%s", name.machine);
            got_netbsd_cpu = 1;
        }
    }
    return netbsd_cpu;
}

char *platform_get_netbsd_runtime_os(void)
{
    struct utsname name;

    if (!got_netbsd_version) {
        uname(&name);

        sprintf(netbsd_version, "%s %s", name.sysname, name.release);

        got_netbsd_version = 1;
    }

    return netbsd_version;
}
#endif
