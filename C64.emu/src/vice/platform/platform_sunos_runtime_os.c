/*
 * platform_sunos_runtime_os.c - SunOS runtime version discovery.
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
 * SunOS 4.1.1 (sun2)
 * NetBSD (m68k)
 * SunOS 4.1.4 (sun4m)
 * Solaris 2.3 (sparc)
 * Solaris 2.4 (sparc)
 * Solaris 2.5.1 (sparc)
 * Solaris 2.6 (sparc)
 * Solaris 7 (sparc)
 * Solaris 8 (sparc)
 * Solaris 9 (sparc)
 * Solaris 10 (sparc)
 * NetBSD (sparc)
 */

#include "vice.h"

#if (defined(sun) || defined(__sun)) && !(defined(__SVR4) || defined(__svr4__))

#include <sys/utsname.h>
#include <string.h>
#include <stdio.h>

static char *cpu = NULL;

char *platform_get_sunos_runtime_cpu(void)
{
    struct utsname name;

    if (!cpu) {
        uname(&name);
        if (!strcmp(name.machine, "sun") || !strcmp(name.machine, "sun1")) {
            cpu = "68000";
        } else if (!strcmp(name.machine, "sun2")) {
            cpu = "68010";
        } else if (!strcmp(name.machine, "sun3")) {
            cpu = "68020";
        } else if (!strcmp(name.machine, "sun3x")) {
            cpu = "68030";
        } else if (!strcmp(name.machine, "sun386") || !strcmp(name.machine, "sun386i")) {
            cpu = "80386";
        } else if (!strcmp(name.machine, "sun4") || !strcmp(name.machine, "sun4c") || !strcmp(name.machine, "sun4m") || !strcmp(name.machine, "sparc")) {
            cpu = "Sparc";
        } else if (!strcmp(name.machine, "sun4u")) {
            cpu = "Sparc64";
        } else {
            cpu = "Unknown CPU";
        }
    }
    return cpu;
}

static char osname[100];
static int got_os = 0;

char *platform_get_sunos_runtime_os(void)
{
    struct utsname name;
    FILE *bsd_emul_test;
    int ret;
    int is_bsd = 0;

    if (!got_os) {
        unlink("emultest.sh");
        bsd_emul_test = fopen("emultest.sh", "wb");
        if (bsd_emul_test) {
            unlink("emultest.result");
            fprintf(bsd_emul_test, "#!/bin/sh\n");
            fprintf(bsd_emul_test, "if test -f /proc/self/emul; then\n");
            fprintf(bsd_emul_test, "  echo emulation >emultest.result\n");
            fprintf(bsd_emul_test, "fi\n");
            fclose(bsd_emul_test);
            ret = system("sh ./emultest.sh");
            if (!ret) {
                unlink("emultest.sh");
            }
            bsd_emul_test = fopen("emultest.result", "rb");
            if (bsd_emul_test) {
                sprintf(osname, "NetBSD");
                fclose(bsd_emul_test);
                unlink("emultest.result");
                is_bsd = 1;
            }
        }

        if (!is_bsd) {
            uname(&name);
            if (name.release[0] == '5') {
                if (name.release[2] <= '6') {
                    sprintf(osname, "Solaris 2.%s", name.release + 2);
                } else {
                    sprintf(osname, "Solaris %s", name.release + 2);
                }
            } else {
                sprintf(osname, "%s %s", name.sysname, name.release);
            }
        }
        got_os = 1;
    }
    return osname;    
}
#endif
