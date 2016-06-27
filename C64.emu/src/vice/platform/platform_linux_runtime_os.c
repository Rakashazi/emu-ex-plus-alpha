/*
 * platform_linux_runtime_os.c - Linux runtime version discovery.
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
   cpu          | libc
   --------------------------
   aarch64      | glibc-2.17
   alpha        | glibc-2.1.3
   amd64        | glibc-2.13
   amd64        | dietlibc
   amd64        | musl
   armeb        | glibc-2.3.2
   armel        | glibc-2.13
   armel        | musl
   armhf        | glibc-2.13
   ia64         | glibc-2.3.1
   m68k         | glibc-2.2.5
   m68k         | glibc-2.3.2
   microblazeel | glibc-2.3.6
   mipseb       | glibc-2.11.3
   mips64eb     | glibc-2.11.3
   mipsel       | glibc-2.11.3
   mips64el     | glibc-2.11.3
   openrisc     | uclibc
   ppc          | glibc-2.13
   ppc          | dietlibc
   ppc64        | glibc-2.13
   s390         | glibc-2.2.4
   s390x        | glibc-2.2.4
   sh4          | glibc-2.17
   sparc        | glibc-2.3.6
   sparc64      | glibc-2.13
   x86          | libc4
   x86          | libc5
   x86          | glibc-1.09
   x86          | glibc-2.0
   x86          | glibc-2.0.2
   x86          | glibc-2.0.3
   x86          | glibc-2.0.4
   x86          | glibc-2.0.5
   x86          | glibc-2.0.6
   x86          | glibc-2.0.7
   x86          | glibc-2.1.1
   x86          | glibc-2.1.2
   x86          | glibc-2.1.3
   x86          | glibc-2.1.92
   x86          | glibc-2.2
   x86          | glibc-2.2.1
   x86          | glibc-2.2.2
   x86          | glibc-2.2.3
   x86          | glibc-2.2.4
   x86          | glibc-2.2.5
   x86          | glibc-2.2.93
   x86          | glibc-2.3.1
   x86          | glibc-2.3.2
   x86          | glibc-2.3.3
   x86          | glibc-2.3.4
   x86          | glibc-2.3.5
   x86          | glibc-2.3.6
   x86          | glibc-2.4
   x86          | glibc-2.5
   x86          | glibc-2.5.1
   x86          | glibc-2.6
   x86          | glibc-2.6.1
   x86          | glibc-2.7
   x86          | glibc-2.8
   x86          | glibc-2.8.90
   x86          | glibc-2.9
   x86          | glibc-2.10.1
   x86          | glibc-2.10.2
   x86          | glibc-2.11
   x86          | glibc-2.11.1
   x86          | glibc-2.11.2
   x86          | glibc-2.11.3
   x86          | glibc-2.12
   x86          | glibc-2.12.1
   x86          | glibc-2.12.2
   x86          | glibc-2.12.90
   x86          | glibc-2.13
   x86          | glibc-2.13.90
   x86          | glibc-2.14
   x86          | glibc-2.14.1
   x86          | glibc-2.14.90
   x86          | glibc-2.15
   x86          | glibc-2.16
   x86          | glibc-2.17
   x86          | glibc-2.18
   x86          | glibc-2.19
   x86          | glibc-2.21
   x86          | dietlibc
   x86          | newlib
   x86          | musl
   x86          | uclibc
   x86          | l4linux
   x86          | openserver (lxrun)
   x86          | unixware (LKP)
   x86          | solaris (lxrun)
   x86          | netbsd (emulation layer)
   x86          | freebsd (emulation layer)
   x86          | openbsd (emulation layer)
 */

#include "vice.h"

#if defined(__linux) && !defined(__ANDROID__) && !defined(AMIGA_AROS)

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "archdep.h"
#include "lib.h"
#include "platform.h"
#include "util.h"
#include "log.h"

#if defined(__GLIBC__) && (__GLIBC__==2) && (__GLIBC_MINOR__>0) && !defined(__UCLIBC__)
#  include <gnu/libc-version.h>
#endif

static char linux_version[100];
static char linux_cpu[100];
static int got_linux_version = 0;
static int got_linux_cpu = 0;

char *platform_get_linux_runtime_cpu(void)
{
    FILE *cpuinfo = NULL;
    char *buffer = NULL;
    char *loc1 = NULL;
    char *loc2 = NULL;
    char *loc3 = NULL;
    char *tempfile = NULL;
    char *tempsystem = NULL;
    size_t size1 = 0;
    size_t size2 = 0;
    struct utsname name;

    if (!got_linux_cpu) {
        sprintf(linux_cpu, "Unknown CPU");
        cpuinfo = fopen("/proc/cpuinfo", "rb");
        if (cpuinfo) {
            fclose(cpuinfo);
            cpuinfo = NULL;
            tempfile = archdep_tmpnam();
            tempsystem = util_concat("cat /proc/cpuinfo >", tempfile, NULL);
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
                if (!loc1) {
                    loc1 = strstr(buffer, "cpu type");
                }
                if (!loc1) {
                    loc1 = strstr(buffer, "cpu model");
                }
                if (!loc1) {
                    loc1 = strstr(buffer, "Processor");
                }
                if (!loc1) {
                    loc1 = strstr(buffer, "cpu");
                    if (loc1 && loc1 != buffer) {
                        loc1--;
                        if (*loc1 != '\n' && isspace(*loc1)) {
                            loc1 = NULL;
                        }
                    }
                }
                if (!loc1) {
                    loc1 = strstr(buffer, "CPU");
                }
                if (!loc1) {
                    loc1 = strstr(buffer, "vendor_id");
                }
                if (loc1) {
                    loc2 = strstr(loc1, ":");
                    if (loc2) {
                        loc2 += 2;
                        while (isspace(*loc2)) {
                            loc2++;
                        }
                        loc3 = strstr(loc2, "\n");
                        if (loc3) {
                            *loc3 = 0;
                            sprintf(linux_cpu, "%s", loc2);
                            got_linux_cpu = 1;
                        }
                    }
                }
            }
            fclose(cpuinfo);
            unlink(tempfile);
            lib_free(tempfile);
            lib_free(tempsystem);
            if (buffer) {
                free(buffer);
            }
        }
#ifndef PLATFORM_NO_X86_ASM
        if (!got_linux_cpu) {
            sprintf(linux_cpu, "%s", platform_get_x86_runtime_cpu());
            got_linux_cpu = 1;
        }
#endif
        if (!got_linux_cpu) {
            uname(&name);
            sprintf(linux_cpu, "%s", name.machine);
            got_linux_cpu = 1;
        }
    }
    return linux_cpu;
}

char *platform_get_linux_runtime_os(void)
{
    struct utsname name;
    FILE *bsd_emul_test;
    int is_bsd = 0;
    int ret;
    int i = 0;

    if (!got_linux_version) {
        unlink("emultest.sh");
        unlink("emultest.netbsd");
        bsd_emul_test = fopen("emultest.sh", "wb");
        if (bsd_emul_test) {
            fprintf(bsd_emul_test, "#!/bin/sh\n");
            fprintf(bsd_emul_test, "if test -f /proc/self/emul; then\n");
            fprintf(bsd_emul_test, "  echo emulation >emultest.netbsd\n");
            fprintf(bsd_emul_test, "fi\n");
            fclose(bsd_emul_test);
            chmod("emultest.sh", S_IRWXU);
            ret = system("./emultest.sh");
            if (!ret) {
                unlink("emultest.sh");
            }
            bsd_emul_test = fopen("emultest.netbsd", "rb");
            if (bsd_emul_test) {
                sprintf(linux_version, "NetBSD");
                fclose(bsd_emul_test);
                unlink("emultest.netbsd");
                is_bsd = 1;
            }
        }

        if (!is_bsd) {
            uname(&name);

            if (!strcasecmp(name.sysname, "SCO_SV")) {
                if (name.version[0] == '5' || name.version[0] == '6') {
                    sprintf(linux_version, "lxrun openserver %s", name.version);
                } else if (name.version[0] == '7') {
                    sprintf(linux_version, "lxrun unixware %s", name.version);
                } else {
                    sprintf(linux_version, "lxrun sco");
                }
            } else if (!strncasecmp(name.version, "FreeBSD", 7)) {
                while (name.version[i] != '-' && name.version[i] != 0) {
                    linux_version[i] = name.version[i];
                    i++;
                }
                linux_version[i] = 0;
            } else {
                sprintf(linux_version, "%s %s", name.sysname, name.release);
            }
        }

#ifdef WATCOM_COMPILE
#define CLIB_HANDLED
        sprintf(linux_version, "%s (openwatcom)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && defined(__dietlibc__)
#define CLIB_HANDLED
        sprintf(linux_version, "%s (dietlibc)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && defined(_NEWLIB_VERSION)
#define CLIB_HANDLED
        sprintf(linux_version, "%s (newlib %s)", linux_version, _NEWLIB_VERSION);
#endif

#if !defined(CLIB_HANDLED) && defined(__UCLIBC__)
#define CLIB_HANDLED
        sprintf(linux_version, "%s (uClibc)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && defined(__GLIBC__)
#  define CLIB_HANDLED
#  if (__GLIBC__==2)
#    if (__GLIBC_MINOR__>0)
        sprintf(linux_version, "%s (glibc %s)", linux_version, gnu_get_libc_version());
#    else
        sprintf(linux_version, "%s (glibc 2.x)", linux_version);
#    endif
#  else
        sprintf(linux_version, "%s (glibc 1.x)", linux_version);
#  endif
#endif

#if !defined(CLIB_HANDLED) && defined(_LINUX_C_LIB_VERSION)
#  define CLIB_HANDLED
        sprintf(linux_version, "%s (libc %s)", linux_version, _LINUX_C_LIB_VERSION);
#endif

#if !defined(CLIB_HANDLED) && (VICE_LINUX_CLIB_VERSION_MAJOR==1)
#  define CLIB_HANDLED
        sprintf(linux_version, "%s (glibc 1.x)", linux_version);
#endif

#if !defined(CLIB_HANDLED) && (VICE_LINUX_CLIB_VERSION_MAJOR==6)
#  define CLIB_HANDLED
        sprintf(linux_version, "%s (glibc 2.x)", linux_version);
#endif

#ifndef CLIB_HANDLED
#  include <sys/ucontext.h>
#  ifdef _UCONTEXT_H
#    define CLIB_HANDLED
        sprintf(linux_version, "%s (musl)", linux_version);
#  endif
#endif

#ifndef CLIB_HANDLED
        sprintf(linux_version, "%s (unknown libc)", linux_version);
#endif

        got_linux_version = 1;
    }

    return linux_version;
}
#endif
