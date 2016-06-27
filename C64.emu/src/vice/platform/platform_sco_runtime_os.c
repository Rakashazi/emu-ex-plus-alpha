/*
 * platform_sco_runtime_os.c - SCO unix/openserver/unixware runtime version discovery.
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
   - SCO Unix 4.x
   - SCO Openserver 5.0.7
   - SCO Openserver 6.0.0
   - SCO Unixware 7.1.3
   - NetBSD (Emulation layer)
   - FreeBSD (Emulation layer)
*/

#include "vice.h"

#if defined(SCO4UNIX_COMPILE) || defined(OPENSERVER5_COMPILE) || defined(OPENSERVER6_COMPILE) || defined(UNIXWARE_COMPILE)

#include <stdio.h>
#include <sys/utsname.h>

#include "archdep.h"
#include "lib.h"
#include "util.h"

static char buffer[80];

static char platform_os[80];
static int got_os = 0;

char *platform_get_sco_runtime_os(void)
{
    int retval = 0;
    struct utsname name;
    FILE *infile;
    FILE *bsd_emul_test;
    int amount = 0;
    int i = 0;
    int is_bsd = 0;
    int ret;
    char *tempfile = NULL;
    char *tempsystem = NULL;

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
                sprintf(platform_os, "NetBSD");
                fclose(bsd_emul_test);
                unlink("emultest.result");
                is_bsd = 1;
            }
        }

        if (!is_bsd) {
            buffer[0] = 0;
            retval = uname(&name);
            if (!retval) {
                sprintf(buffer, "%s", name.version);
            } else {
                tempfile = archdep_tmpnam();
                tempsystem = util_concat("uname -v >", tempfile, NULL);
                system(tempsystem);
                infile = fopen(tempfile, "rb");
                if (infile) {
                    amount = fread(buffer, 1, 80, infile);
                    if (amount) {
                        while (buffer[i] != '\n') {
                            i++;
                        }
                        buffer[i] = 0;
                    }
                    fclose(infile);
                }
                unlink(tempfile);
                lib_free(tempfile);
                lib_free(tempsystem);
            }
            if (!retval && !strcasecmp(name.sysname, "FreeBSD")) {
                sprintf(platform_os, "FreeBSD");
            } else if (buffer[0] == '2') {
                sprintf(platform_os, "SCO Unix v4.x");
            } else if (buffer[0] == '5' || buffer[0] == '6') {
                sprintf(platform_os, "SCO Openserver %s", buffer);
            } else if (buffer[0] == '7') {
                sprintf(platform_os, "SCO Unixware %s", buffer);
            } else {
                sprintf(platform_os, "SCO Unix");
            }
        }
        got_os = 1;
    }
    return platform_os;
}

static char platform_cpu[20];
static int got_cpu = 0;

char *platform_get_sco_runtime_cpu(void)
{
    struct utsname name;
    int retval = 0;
    int i = 0;
    int amount = 0;
    FILE *infile;
    char *tempfile = NULL;
    char *tempsystem = NULL;

    if (!got_cpu) {
        retval = uname(&name);
        if (!retval) {
            sprintf(platform_cpu, "%s", name.machine);
        } else {
            tempfile = archdep_tmpnam();
            tempsystem = util_concat("uname -m >", tempfile, NULL);
            system(tempsystem);
            infile = fopen(tempfile, "rb");
            if (infile) {
                amount = fread(platform_cpu, 1, 20, infile);
                if (amount) {
                    while (platform_cpu[i] != '\n') {
                        i++;
                    }
                    platform_cpu[i] = 0;
                }
                fclose(infile);
            }
            unlink(tempfile);
            lib_free(tempfile);
            lib_free(tempsystem);
        }
        got_cpu = 1;
    }
    return platform_cpu;
}
#endif
