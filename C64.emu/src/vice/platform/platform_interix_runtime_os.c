/*
 * platform_interix_runtime_os.c - Interix runtime version discovery.
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
 - Windows NT 4.0 Embedded Workstation (x86)
 - Windows NT 4.0 Workstation (x86)
 - Windows NT 4.0 Embedded Server (x86)
 - Windows NT 4.0 Server (x86)
 - Windows NT 4.0 Small Business Server 4.0 (x86)
 - Windows NT 4.0 Small Business Server 4.5 (x86)
 - Windows NT 4.0 Enterprise Server (x86)
 - Windows 2000 Pro (x86)
 - Windows 2000 Powered (x86)
 - Windows 2000 Server (x86)
 - Windows 2000 Small Business Server (x86)
 - Windows 2000 Advanced Server (x86)
 - Windows 2000 Datacenter Server (x86)
 - Windows XP Embedded (x86)
 - Windows XP FLP (x86)
 - Windows XP Pro (x86)
 - Windows XP Tablet PC (x86)
 - Windows XP MCE 2004 (x86)
 - Windows XP MCE 2005 (x86)
 - Windows XP MCE 2005 R1 (x86)
 - Windows XP MCE 2005 R2 (x86)
 - Windows 2003 Web Server (x86)
 - Windows 2003 Standard Server (x86)
 - Windows 2003 Small Business Server (x86)
 - Windows 2003 Enterprise Server (x86)
 - Windows 2003 Datacenter Server (x86)
 - Windows Home Server (x86)
 - Windows 2003 R2 Standard Server (x86)
 - Windows 2003 R2 Small Business Server (x86)
 - Windows 2003 R2 Enterprise Server (x86/x64)
 - Windows 2003 R2 Datacenter Server (x64)
 - Windows Vista Enterprise (x86)
 - Windows Vista Ultimate (x86)
 - Windows 2008 Foundation Server (x64)
 - Windows 2008 Standard Server (x86/x64)
 - Windows 2008 Enterprise Server (x86/x64)
 - Windows 2008 Datacenter Server (x86/x64)
 - Windows 2008 Basic Storage Server (x64)
 - Windows 2008 Workgroup Storage Server (x64)
 - Windows 2008 Enterprise Storage Server (x64)
 - Windows Thin PC (x86)
 - Windows 7 Embedded POSReady (x86)
 - Windows 7 Embedded Standard (x86)
 - Windows 7 Enterprise (x86/x64)
 - Windows 7 Ultimate (x86/x64)
 - Windows 2008 R2 Foundation Server (x64)
 - Windows 2008 R2 Enterprise Server (x64)
 - Windows 2008 R2 Datacenter Server (x64)
 - Windows 2008 R2 Workgroup Storage Server (x64)
 - Windows 2008 R2 Standard Storage Sever (x64)
 - Windows 2008 R2 Enterprise Storage Server (x64)
 - Windows 2009 POSReady (x86)
 - Windows Home Server 2011 (x64)
 - Windows 2011 Standard Multipoint Server (x64)
 - Windows 2012 Standard Server (x64)
 - Windows 8 Enterprise (x86/x64)
*/

#include "vice.h"

#ifdef __INTERIX

#include <stdio.h>
#include <sys/utsname.h>
#include <interix/interix.h>
#include <interix/registry.h>

#define NT_SERVER_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ProductOptions"
#define NT_SERVER_VALUE L"ProductType"

#define POSREADY_KEY L"\\Registry\\Machine\\Software\\Microsoft\\POSReady"
#define POSREADY_VALUE L"Version"

#define NTSBS_KEY L"\\Registry\\Machine\\Software\\Microsoft\\BackOffice"
#define NTSBS_VALUE L"SuiteVersion"

#define NT_PRODUCT_SUITE_PATH "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ProductOptions\\ProductSuite"

#define NT_FLP_PATH "\\Registry\\Machine\\System\\WPA\\Fundamentals\\Installed"

#define NT_TABLET_PATH "\\Registry\\Machine\\System\\WPA\\TabletPC\\Installed"

#define NT_MCE_PATH "\\Registry\\Machine\\System\\WPA\\MediaCenter\\Installed"

#define MCE_VERSION_KEY L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Media Center"
#define MCE_VERSION_VALUE L"Ident"

#define NT_THINPC_PATH "\\Registry\\Machine\\Software\\Microsoft\\ThinPC\\Version"

typedef struct winver_s {
    char *name;
    char *windows_name;
    int server;
    int flags;
} winver_t;

static winver_t windows_versions[] = {
    { "Windows NT 4 Embedded Workstation",         "Microsoft Windows NT 4",                     0, 1 },
    { "Windows NT 4 Workstation",                  "Microsoft Windows NT 4",                     0, 0 },
    { "Windows NT 4 Embedded Server",              "Microsoft Windows NT 4",                     8, 0 },
    { "Windows NT 4 Server",                       "Microsoft Windows NT 4",                     1, 0 },
    { "Windows NT 4 Small Business Server 4.0",    "Microsoft Windows NT 4",                     3, 0 },
    { "Windows NT 4 Small Business Server 4.5",    "Microsoft Windows NT 4",                     1, 2 },
    { "Windows NT 4 Enterprise Server",            "Microsoft Windows NT 4",                     5, 0 },
    { "Windows 2000 Pro",                          "Microsoft Windows 2000",                     0, 0 },
    { "Windows 2000 Server",                       "Microsoft Windows 2000",                     1, 0 },
    { "Windows 2000 Advanced Server",              "Microsoft Windows 2000",                     5, 0 },
    { "Windows 2000 Datacenter Server",            "Microsoft Windows 2000",                     6, 0 },
    { "Windows 2000 Powered",                      "Microsoft Windows 2000",                     4, 0 },
    { "Windows 2000 Small Business Server",        "Microsoft Windows 2000",                     3, 0 },
    { "Windows XP Embedded",                       "Microsoft Windows XP",                       0, 10 },
    { "Windows XP Pro",                            "Microsoft Windows XP",                       0, 0 },
    { "Windows XP Tablet PC",                      "Microsoft Windows XP",                       0, 1 },
    { "Windows XP Media Center 2004",              "Microsoft Windows XP",                       0, 6 },
    { "Windows XP Media Center 2005",              "Microsoft Windows XP",                       0, 7 },
    { "Windows XP Media Center 2005 R1",           "Microsoft Windows XP",                       0, 8 },
    { "Windows XP Media Center 2005 R2",           "Microsoft Windows XP",                       0, 9 },
    { "Windows Fundamentals for Legacy PCs",       "Microsoft Windows XP",                       0, 3 },
    { "Windows POSReady 2009",                     "Microsoft Windows XP",                       0, 5 },
    { "Windows 2003 Web Server",                   "Microsoft Windows Server 2003",              2, 0 },
    { "Windows 2003 Standard Server",              "Microsoft Windows Server 2003",              1, 0 },
    { "Windows 2003 Small Business Server",        "Microsoft Windows Server 2003",              3, 0 },
    { "Windows 2003 Enterprise Server",            "Microsoft Windows Server 2003",              5, 0 },
    { "Windows 2003 Datacenter Server",            "Microsoft Windows Server 2003",              6, 0 },
    { "Windows Home Server",                       "Microsoft Windows Server 2003",              7, 0 },
    { "Windows 2003 R2 Standard Server",           "Microsoft Windows Server 2003 R2",           1, 0 },
    { "Windows 2003 R2 Enterprise Server",         "Microsoft Windows Server 2003 R2",           5, 0 },
    { "Windows 2003 R2 Datacenter Server",         "Microsoft Windows Server 2003 R2",           6, 0 },
    { "Windows Vista Enterprise",                  "Windows (TM) Vista Enterprise",              0, 0 },
    { "Windows Vista Enterprise",                  "Windows Vista (TM) Enterprise",              0, 0 },
    { "Windows Vista Ultimate",                    "Windows (TM) Vista Ultimate",                0, 0 },
    { "Windows Vista Ultimate",                    "Windows Vista (TM) Ultimate",                0, 0 },
    { "Windows 2008 Foundation Server",            "Windows (R) Small Business Server 2008",     1, 0 },
    { "Windows 2008 Standard Server",              "Windows (R) Standard Server 2008",           1, 0 },
    { "Windows 2008 Standard Server",              "Windows Server (R) 2008 Standard",           1, 0 },
    { "Windows 2008 Enterprise Server",            "Windows (R) Enterprise Server 2008",         5, 0 },
    { "Windows 2008 Enterprise Server",            "Windows Server (R) 2008 Enterprise",         5, 0 },
    { "Windows 2008 Datacenter Server",            "Windows (R) Datacenter Server 2008",         6, 0 },
    { "Windows 2008 Datacenter Server",            "Windows Server (R) 2008 Datacenter",         6, 0 },
    { "Windows 2008 HPC Server",                   "Windows Server (R) 2008 HPC Edition",        1, 0 },
    { "Windows 2008 Basic Storage Server",         "Windows (R) Storage Server 2008 Basic",      1, 0 },
    { "Windows 2008 Workgroup Storage Server",     "Windows (R) Storage Server 2008 Workgroup",  1, 0 },
    { "Windows 2008 Enterprise Storage Server",    "Windows (R) Storage Server 2008 Enterprise", 1, 0 },
    { "Windows Thin PC",                           "Windows Embedded Standard",                  0, 1 },
    { "Windows 7 Embedded POSReady",               "Windows Embedded Standard",                  0, 2 },
    { "Windows 7 Embedded Standard",               "Windows Embedded Standard",                  0, 0 },
    { "Windows 7 Enterprise",                      "Windows 7 Enterprise",                       0, 0 },
    { "Windows 7 Ultimate",                        "Windows 7 Ultimate",                         0, 0 },
    { "Windows 2008 R2 Standard Server",           "Windows Server 2008 R2 Standard",            1, 0 },
    { "Windows 2008 R2 Enterprise Server",         "Windows Server 2008 R2 Enterprise",          5, 0 },
    { "Windows 2008 R2 Datacenter Server",         "Windows Server 2008 R2 Datacenter",          6, 0 },
    { "Windows 2008 R2 Workgroup Storage Server",  "Windows Storage Server 2008 R2 Workgroup",   1, 0 },
    { "Windows 2008 R2 Workgroup Storage Server",  "Windows Storage Server 2008 R2 Workgroup",   5, 0 },
    { "Windows 2008 R2 Workgroup Storage Server",  "Windows Storage Server 2008 R2 Workgroup",   6, 0 },
    { "Windows 2008 R2 Standard Storage Server",   "Windows Storage Server 2008 R2 Standard",    1, 0 },
    { "Windows 2008 R2 Standard Storage Server",   "Windows Storage Server 2008 R2 Standard",    5, 0 },
    { "Windows 2008 R2 Standard Storage Server",   "Windows Storage Server 2008 R2 Standard",    6, 0 },
    { "Windows 2008 R2 Enterprise Storage Server", "Windows Storage Server 2008 R2 Enterprise",  1, 0 },
    { "Windows 2008 R2 Enterprise Storage Server", "Windows Storage Server 2008 R2 Enterprise",  5, 0 },
    { "Windows 2008 R2 Enterprise Storage Server", "Windows Storage Server 2008 R2 Enterprise",  6, 0 },
    { "Windows 2008 R2 Foundation Server",         "Windows Server 2008 R2 Foundation",          1, 0 },
    { "Windows 2008 R2 Foundation Server",         "Windows Server 2008 R2 Foundation",          5, 0 },
    { "Windows 2008 R2 Foundation Server",         "Windows Server 2008 R2 Foundation",          6, 0 },
    { "Windows Home Server 2011",                  "Windows Home Server 2011",                   1, 0 },
    { "Windows 2011 Multipoint Server",            "Windows MultiPoint Server 2011",             1, 0 },
    { "Windows 2011 Multipoint Server",            "Windows MultiPoint Server 2011",             5, 0 },
    { "Windows 2011 Multipoint Server",            "Windows MultiPoint Server 2011",             6, 0 },
    { "Windows 8 Enterprise",                      "Windows 8 Enterprise",                       0, 0 },
    { "Windows 2012 Standard Server",              "Windows Server 2012 Standard",               1, 0 },
    { NULL,                                        NULL,                                         0, 0 }
};

static int widelen(char *text)
{
    char *p = text;
    int len = 0;

    while (p[0] != 0 || p[1] != 0) {
        p += 2;
        len += 2;
    }
    return len;
}

static void wide2single(char *wide, char *single)
{
    int i;

    for (i = 0; wide[i * 2] != 0 || wide[(i * 2) + 1] != 0; i++) {
        single[i] = wide[i * 2];
    }
    single[i] = 0;
}

static char *get_windows_version(void)
{
    char windows_name[100];
    char server_name[20];
    char nt_version[10];
    char product_suite[200];
    char temp[200];
    int rcode;
    int i = 0;
    int windows_server = 0;
    int windows_flags = 0;
    int type;
    int found = 0;
    int suite = 0;
    size_t size = 200;
    char *p;
    unsigned long wpa = 0;
    size_t wpa_size = 4;

    rcode = getreg_strvalue((PCWSTR)NT_VERSION_KEY, (PCWSTR)NT_PRODUCT_NAME_VALUE, windows_name, 100);
    if (rcode) {
        rcode = getreg_strvalue((PCWSTR)NT_VERSION_KEY, (PCWSTR)NT_VERSION_VALUE, nt_version, 10);
        if (!strcmp("4.0", nt_version)) {
            sprintf(windows_name, "Microsoft Windows NT 4");
        }
    }

    /* 0 = professional
       1 = tablet pc
       2 = MCE 2002
       3 = flp
       4 = POSReady 7
       5 = POSReady 2009
       6 = MCE 2004
       7 = MCE 2005
       8 = MCE 2005 R1
       9 = MCE 2005 R2
      10 = Embedded
     */
    if (!strcmp(windows_name, "Microsoft Windows XP")) {
        rcode = getreg(NT_FLP_PATH, &type, &wpa, &wpa_size);
        if (!rcode) {
            if (wpa) {
                windows_flags = 3;
            }
        }
        rcode = getreg(NT_TABLET_PATH, &type, &wpa, &wpa_size);
        if (!rcode) {
            if (wpa) {
                windows_flags = 1;
            }
        }
        rcode = getreg(NT_MCE_PATH, &type, &wpa, &wpa_size);
        if (!rcode) {
            if (wpa) {
                windows_flags = 2;
            }
        }
        rcode = getreg(NT_THINPC_PATH, &type, &wpa, &wpa_size);
        if (!rcode) {
            if (wpa) {
                windows_flags = 3;
            }
        }
        rcode = getreg_strvalue((PCWSTR)POSREADY_KEY, (PCWSTR)POSREADY_VALUE, nt_version, 10);
        if (!rcode) {
            if (!strcmp("2.0", nt_version)) {
                windows_flags = 5;
            } else {
                windows_flags = 4;
            }
        }
        rcode = getreg(NT_PRODUCT_SUITE_PATH, &type, &product_suite, &size);
        if (!rcode) {
            p = product_suite;
            while (!found) {
                wide2single(p, temp);
                if (!strcmp(temp, "EmbeddedNT") && windows_flags != 3 && windows_flags != 5) {
                    windows_flags = 10;
                }
                p += widelen(p);
                p += 2;
                if (p[0] == 0 && p[1] == 0) {
                    found = 1;
                }
            }
        }
        if (windows_flags == 2) {
            rcode = getreg_strvalue((PCWSTR)MCE_VERSION_KEY, (PCWSTR)MCE_VERSION_VALUE, nt_version, 10);
            if (!rcode) {
                if (!strcmp("2.7", nt_version)) {
                    windows_flags = 6;
                } else if (!strcmp("2.8", nt_version)) {
                    windows_flags = 6;
                } else if (!strcmp("3.0", nt_version)) {
                    windows_flags = 7;
                } else if (!strcmp("3.1", nt_version)) {
                    windows_flags = 8;
                } else if (!strcmp("4.0", nt_version)) {
                    windows_flags = 9;
                }
            }
        }
    }

    /* 0 = embedded, 1 = thin pc, 2 = posready */
    if (!strcmp(windows_name, "Windows Embedded Standard")) {
        rcode = getreg(NT_THINPC_PATH, &type, &temp, &size);
        if (!rcode) {
            windows_flags = 1;
        }
        rcode = getreg_strvalue((PCWSTR)POSREADY_KEY, (PCWSTR)POSREADY_VALUE, nt_version, 10);
        if (!rcode) {
            windows_flags = 2;
        }
    }

    /* 1 = nt4 SBS 4.0, 2 = nt4 SBS 4.5 */
    if (!strcmp(windows_name, "Microsoft Windows NT 4")) {
        rcode = getreg_strvalue((PCWSTR)NTSBS_KEY, (PCWSTR)NTSBS_VALUE, nt_version, 10);
        if (!rcode) {
            if (!strcmp("4.0", nt_version)) {
                windows_flags = 1;
            } else if (!strcmp("4.5", nt_version)) {
                windows_flags = 2;
            }
        }
    }
    

    /* 0 = workstation
       1 = standard server
       2 = blade server
       3 = small business server
       4 = appliance server
       5 = enterprise server
       6 = datacenter server
       7 = home server
       8 = embedded server
    */
    rcode = getreg_strvalue((PCWSTR)NT_SERVER_KEY, (PCWSTR)NT_SERVER_VALUE, server_name, 20);
    if (!rcode) {
        if (!strcmp("ServerNT", server_name) || !strcmp("LanmanNT", server_name)) {
            rcode = getreg(NT_PRODUCT_SUITE_PATH, &type, &product_suite, &size);
            if (rcode) {
                windows_server = 1;
            } else {
                p = product_suite;
                while (!found) {
                    wide2single(p, temp);
                    if (!strcmp(temp, "Blade")) {
                        suite |= 1;
                    }
                    if (!strcmp(temp, "Small Business")) {
                        suite |= 2;
                    }
                    if (!strcmp(temp, "Server Appliance")) {
                        suite |= 4;
                    }
                    if (!strcmp(temp, "Enterprise")) {
                        suite |= 8;
                    }
                    if (!strcmp(temp, "DataCenter")) {
                        suite |= 16;
                    }
                    if (!strcmp(temp, "WH Server")) {
                        suite |= 32;
                    }
                    if (!strcmp(temp, "EmbeddedNT")) {
                        suite |= 64;
                    }
                    p += widelen(p);
                    p += 2;
                    if (p[0] == 0 && p[1] == 0) {
                        found = 1;
                    }
                }
                if (suite >= 64) {
                    windows_server = 8;
                } else if (suite >= 32) {
                    windows_server = 7;
                } else if (suite >= 16) {
                    windows_server = 6;
                } else if (suite >= 8) {
                    windows_server = 5;
                } else if (suite >= 4) {
                    windows_server = 4;
                } else if (suite >= 2) {
                    windows_server = 3;
                } else if (suite >= 1) {
                    windows_server = 2;
                } else {
                    windows_server = 1;
                }
            }
        } else {
            rcode = getreg(NT_PRODUCT_SUITE_PATH, &type, &product_suite, &size);
            if (!rcode) {
                p = product_suite;
                while (!found) {
                    wide2single(p, temp);
                    if (!strcmp(temp, "EmbeddedNT") && windows_flags != 3 && windows_flags != 5 && windows_flags != 10) {
                        suite |= 1;
                    }
                    p += widelen(p);
                    p += 2;
                    if (p[0] == 0 && p[1] == 0) {
                        found = 1;
                    }
                }
                if (suite == 1) {
                    windows_flags = 1;
                }
            }
        }
    }

    /* Check the table for a matching entry */
    for (i = 0; windows_versions[i].name; i++) {
        if (!strcmp(windows_versions[i].windows_name, windows_name)) {
            if (windows_versions[i].server == windows_server) {
                 if (windows_versions[i].flags == windows_flags) {
                    return windows_versions[i].name;
                }
            }
        }
    }

#ifdef DEBUG_PLATFORM
    printf("name: %s, server: %d, flags: %d\n", windows_name, windows_server, windows_flags);
#endif

    return "Unknown Windows version";
}

static char interix_platform_version[100];
static int got_version = 0;

char *platform_get_interix_runtime_os(void)
{
    char service_pack[100];
    int rcode;
    struct utsname name;

    if (!got_version) {
        uname(&name);
        sprintf(interix_platform_version, "Interix %s", name.release);
        rcode = getreg_strvalue((PCWSTR)NT_SERVICEPACK_KEY, (PCWSTR)NT_SERVICEPACK_VALUE, service_pack, 100);
        if (!rcode) {
            sprintf(interix_platform_version, "%s (%s %s)", interix_platform_version, get_windows_version(), service_pack);
        } else {
            sprintf(interix_platform_version, "%s (%s)", interix_platform_version, get_windows_version());
        }
        got_version = 1;
    }
    return interix_platform_version;
}
#endif
