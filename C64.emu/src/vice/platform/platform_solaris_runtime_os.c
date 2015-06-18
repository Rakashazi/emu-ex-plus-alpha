/*
 * platform_solaris_runtime_os.c - Solaris runtime version discovery.
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
 *
 * Solaris 2.3 (sparc)
 * Solaris 2.4 (intel)
 * Solaris 2.4 (sparc)
 * Solaris 2.5.1 (intel)
 * Solaris 2.5.1 (sparc)
 * Solaris 2.6 (intel)
 * Solaris 2.6 (sparc)
 * Solaris 7 (intel)
 * Solaris 7 (sparc)
 * Solaris 8 (intel)
 * Solaris 8 (sparc)
 * Solaris 9 (intel)
 * Solaris 9 (sparc)
 * Solaris 10 (intel 32&64)
 * Solaris 10 (sparc)
 * OpenSolaris (intel)
 * Solaris 11(.0) (intel 32&64)
 * Solaris 11.1 (intel 32&64)
 */

#include "vice.h"

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))

#include <sys/utsname.h>
#include <string.h>

static char *os = NULL;

char *platform_get_solaris_runtime_os(void)
{
    struct utsname name;

    if (!os) {
        uname(&name);
        if (!strcasecmp(name.release, "5.3")) {
            os = "Solaris 2.3";
        } else if (!strcasecmp(name.release, "5.4")) {
            os = "Solaris 2.4";
        } else if (!strcasecmp(name.release, "5.5")) {
            os = "Solaris 2.5";
        } else if (!strcasecmp(name.release, "5.5.1")) {
            os = "Solaris 2.5.1";
        } else if (!strcasecmp(name.release, "5.6")) {
            os = "Solaris 2.6";
        } else if (!strcasecmp(name.release, "5.7")) {
            os = "Solaris 7";
        } else if (!strcasecmp(name.release, "5.8")) {
            os = "Solaris 8";
        } else if (!strcasecmp(name.release, "5.9")) {
            os = "Solaris 9";
        } else if (!strcasecmp(name.release, "5.10")) {
            os = "Solaris 10";
        } else if (!strcasecmp(name.release, "5.11")) {
            if (!strcasecmp(name.version, "11.0")) {
                os = "Solaris 11";
            } else if (!strcasecmp(name.version, "11.1")) {
                os = "Solaris 11.1";
            else {
                os = "OpenSolaris";
            }
        } else {
            os = "Unknown Solaris version";
        }
    }
    return os;
}

#if defined(__sparc64__) || defined(sparc64) || defined(__sparc__) || defined(sparc)
#include <sys/types.h>
#include <sys/processor.h>
#include <stdio.h>

static char solaris_cpu[200];
static int got_cpu = 0;

char *platform_get_solaris_runtime_cpu(void)
{
    processor_info_t info;
    int status = 0;
    struct utsname name;
    FILE *infile = NULL;
    size_t size = 0;
    size_t size2 = 0;
    char *buffer = NULL;
    char *loc = NULL;
    char *loc2 = NULL;

    if (!got_cpu) {
        status = processor_info(0, &info);
        if (status != -1) {
            sprintf(solaris_cpu, "%s", info.pi_processor_type);
        } else {
            uname(&name);
            sprintf(solaris_cpu, "%s", name.machine);
        }
        system("dmesg >/tmp/vice.cpu.tmp");
        infile = fopen("/tmp/vice.cpu.tmp", "rb");
        if (infile) {
            fseek(infile, 0L, SEEK_END);
            size = ftell(infile);
            fseek(infile, 0L, SEEK_SET);
            buffer = (char *)malloc(size);
            size2 = fread(buffer, 1, size, infile);
            if (size == size2) {
                loc = strstr(buffer, "cpu0:");
                if (loc) {
                    loc += 6;
                    loc2 = strstr(loc, " (");
                    if (loc2) {
                        *loc2 = 0;
                        sprintf(solaris_cpu, "%s (%s)", solaris_cpu, loc);
                    }
                }
            }
            fclose(infile);
            free(buffer);
        }
        unlink("/tmp/vice.cpu.tmp");
        got_cpu = 1;
    }
    return solaris_cpu;
}
#endif

#endif
