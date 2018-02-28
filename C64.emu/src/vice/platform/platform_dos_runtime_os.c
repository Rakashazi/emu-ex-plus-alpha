/*
 * platform_dos_runtime_os.c - DOS runtime version discovery.
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

/* Tested and confirmed working on the following DOS systems:
 - Compaq DOS 3.31
 - Concurrent DOS XM 6.0
 - DOSBox 0.71
 - DOSBox 0.73
 - DOSBox 0.74
 - DR-DOS 5.0
 - DR-DOS 6.0
 - DR-DOS 7.03
 - DR-DOS 7.05
 - DR-DOS 8.0
 - DR-DOS 8.1
 - FreeDOS 0.3
 - FreeDOS 0.4
 - FreeDOS 0.5
 - FreeDOS 0.6
 - FreeDOS 0.7
 - FreeDOS 0.8
 - FreeDOS 0.9
 - FreeDOS 1.1
 - MSDOS 3.10 (Compaq OEM)
 - MSDOS 3.10 (Epson OEM)
 - MSDOS 3.10 (Olivetti OEM)
 - MSDOS 3.20 (Generic)
 - MSDOS 3.20 (Amstrad OEM)
 - MSDOS 3.20 (Data General OEM)
 - MSDOS 3.20 (HP OEM)
 - MSDOS 3.20 (Zenith OEM)
 - MSDOS 3.21 (Hyosung OEM)
 - MSDOS 3.21 (Kaypro OEM)
 - MSDOS 3.21 (Generic)
 - MSDOS 3.30 (Toshiba OEM)
 - MSDOS 3.30 (Generic)
 - MSDOS 3.30A (AT&T OEM)
 - MSDOS 3.30A (Generic)
 - MSDOS 4.01 (Thoshiba OEM)
 - MSDOS 4.01 (Generic)
 - MSDOS 5.00 (Generic)
 - MSDOS 5.00 (AST OEM)
 - MSDOS 5.00 (Compaq OEM)
 - MSDOS 5.00 (Olivetti OEM)
 - MSDOS 5.00 (Toshiba OEM)
 - MSDOS 6.00 (Generic)
 - MSDOS 6.20 (Generic)
 - MSDOS 6.21 (Generic)
 - MSDOS 7.00 (Stand Alone)
 - MSDOS 7.10 (Stand Alone)
 - MSDOS 8.0 (Stand Alone)
 - Novell DOS 7
 - PCDOS 3.00
 - PCDOS 3.10
 - PCDOS 4.00
 - PCDOS 5.00
 - PCDOS 5.02
 - PCDOS 6.10
 - PCDOS 7.10
 - PCDOS 2000
 - REAL32 7.6
 - ROMDOS 6.22
 - ROMDOS 7.1
*/

/* Tested and confirmed working on the following DOS GUI's:
 - DesqView 2.70
 - DesqView 2.71
 - DesqView 2.80
 - Windows 3.0
 - Windows 3.10
 - Windows 3.10 (Windows For Workgroups)
 - Windows 3.11
 - Windows 3.11 (Windows For Workgroups)
*/

/* Tested and confirmed working on the following DOS on Windows systems:
 - Windows 95 Original
 - Windows 95A
 - Windows 95B
 - Windows 95C
 - Windows 98
 - Windows 98 Secure
 - Windows 98SE
 - Windows 98SE Secure
 - Windows ME
 - Windows ME Secure
 - Windows NT 3.50 Workstation
 - Windows NT 3.50 Server
 - Windows NT 3.51 Workstation
 - Windows NT 3.51 Server
 - Windows NT 4.0 Embedded Workstation
 - Windows NT 4.0 Workstation
 - Windows NT 4.0 Embedded Server
 - Windows NT 4.0 Server
 - Windows NT 4.0 Terminal Server
 - Windows NT 4.0 Small Business Server 4.0
 - Windows NT 4.0 Small Business Server 4.5
 - Windows NT 4.0 Enterprise Server
 - Windows Neptune
 - Windows 2000 Pro
 - Windows 2000 Powered
 - Windows 2000 Server
 - Windows 2000 Small Business Server
 - Windows 2000 Advanced Server
 - Windows 2000 Datacenter Server
 - Windows XP PE
 - Windows XP Embedded
 - Windows XP FLP
 - Windows XP Starter
 - Windows XP Home
 - Windows XP Pro
 - Windows XP Tablet PC
 - Windows XP MCE 2002
 - Windows XP MCE 2004
 - Windows XP MCE 2005
 - Windows XP MCE 2005 R1
 - Windows XP MCE 2005 R2
 - Windows 2003 Web Server
 - Windows 2003 Standard Server
 - Windows 2003 Small Business Server
 - Windows 2003 Enterprise Server
 - Windows 2003 Datacenter Server
 - Windows Home Server
 - Windows 2003 R2 Standard Server
 - Windows 2003 R2 Small Business Server
 - Windows 2003 R2 Enterprise Server
 - Windows Vista Starter
 - Windows Vista Home Basic
 - Windows Vista Home Premium
 - Windows 7 Starter
 - Windows 7 Home Basic
 - Windows 7 Home Premium
 - Windows 7 Pro
 - Windows 2009 Embedded Standard
 - Windows 2009 Embedded POSReady
 - Windows 8
 - Windows 8 Pro
 - Windows 8.1 Embedded Industry Pro
 - Windows 8.1 Embedded Industry Enterprise
 - Windows 8.1 Pro
 - Windows 8.1 Enterprise
 - Windows 10 Education
 - Windows 10 Pro
*/

/* Tested and confirmed working on the following other DOS emulations:
 - OS/2 2.0
 - OS/2 2.1
 - OS/2 3.0
 - OS/2 4.0
 - OS/2 4.52
 - EComStation 2.0 RC5
 - EComStation 2.0 RC6a
 - EComStation 2.0 RC7 Silver Release
*/

#include "vice.h"

#ifdef __MSDOS__

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <dpmi.h>

#include "lib.h"
#include "util.h"

static char archdep_os_version[128];
static int got_os = 0;

typedef struct dos_version_s {
    char *name;
    char *flavor;
    int major;
    int minor;
    int oem;
    char *command_string;
    char *ver_string;
    char *comspec_string;
    char *env_string;
} dos_version_t;

static dos_version_t dos_versions[] = {
    { "Compaq DOS 3.31",               "IBMPcDos",    3, 31,  -1, "The COMPAQ Personal Computer MS-DOS ",                       "COMPAQ Personal Computer DOS Version  3.31 ",            NULL, NULL },
    { "Concurrent DOS XM 6.0",         "IBMPcDos",    3, 30,  -1, NULL,                                                         NULL,                                                     NULL, "CDOS 6.0" },
    { "DOSBox 0.71",                   "MS-DOS",      5,  0, 255, NULL,                                                         "DOSBox version 0.71. Reported DOS version 5.0.",         NULL, NULL },
    { "DOSBox 0.73",                   "MS-DOS",      5,  0, 255, NULL,                                                         "DOSBox version 0.73. Reported DOS version 5.00.",        NULL, NULL },
    { "DOSBox 0.74",                   "MS-DOS",      5,  0, 255, NULL,                                                         "DOSBox version 0.74. Reported DOS version 5.00.",        NULL, NULL },
    { "DR-DOS 5.0",                    "IBMPcDos",    3, 31,  -1, NULL,                                                         "DR DOS Release 5.0",                                     NULL, NULL },
    { "DR-DOS 6.0",                    "IBMPcDos",    3, 31,  -1, NULL,                                                         "DR DOS Release 6.0",                                     NULL, NULL },
    { "DR-DOS 7.03",                   "IBMPcDos",    6,  0,   0, NULL,                                                         "Caldera DR-DOS 7.03",                                    NULL, NULL },
    { "DR-DOS 7.05",                   "IBMPcDos",    7, 10,   0, NULL,                                                         "Caldera DR-DOS 7.05",                                    NULL, NULL },
    { "DR-DOS 8.0",                    "IBMPcDos",    6,  0,   0, NULL,                                                         "DeviceLogics DR-DOS 8.0 ",                               NULL, NULL },
    { "DR-DOS 8.1",                    "IBMPcDos",    6,  0,   0, NULL,                                                         NULL,                                                     NULL, "DrDOS 8.1" },
    { "FreeDOS 0.3",                   "??Unknown??", 3, 31,  -1, NULL,                                                         "Environment size set to 240 bytes.",                     NULL, NULL },
    { "FreeDOS 0.4",                   "??Unknown??", 4,  0,  -1, NULL,                                                         "FreeCom version 0.76b [Mar 24 1999]",                    NULL, NULL },
    { "FreeDOS 0.5",                   "??Unknown??", 4,  0,  -1, NULL,                                                         "FreeCom version 0.79a [Mar 31 2000]",                    NULL, NULL },
    { "FreeDOS 0.6",                   "??Unknown??", 5,  0, 253, NULL,                                                         "FreeCom version 0.82 [Mar 10 2001]",                     NULL, NULL },
    { "FreeDOS 0.7",                   "??Unknown??", 5,  0, 253, NULL,                                                         "FreeCom version 0.83 Beta 28 [Aug 15 2001]",             NULL, NULL },
    { "FreeDOS 0.8",                   "??Unknown??", 5,  0, 253, NULL,                                                         NULL,                                                     NULL, NULL },
    { "FreeDOS 0.9",                   "??Unknown??", 7, 10, 253, NULL,                                                         NULL,                                                     NULL, NULL },
    { "FreeDOS 1.1",                   "??Unknown??", 7, 10, 254, NULL,                                                         NULL,                                                     NULL, NULL },
    { "MSDOS 3.10 (Compaq OEM)",       "IBMPcDos",    3, 10,  -1, "The COMPAQ Personal Computer MS-DOS",                        "COMPAQ Personal Computer DOS Version  3.10 ",            NULL, NULL },
    { "MSDOS 3.10 (Epson OEM)",        "MS-DOS",      3, 10,  -1, "Microsoft MS-DOS Version 3.10",                              "MS-DOS Version 3.10",                                    NULL, NULL },
    { "MSDOS 3.10 (Olivetti OEM)",     "MS-DOS",      3, 10,  -1, "Microsoft MS-DOS",                                           "Microsoft MS-DOS Version 3.10 ",                         NULL, NULL },
    { "MSDOS 3.20 (Amstrad OEM)",      "MS-DOS",      3, 20,  -1, "Microsoft(R) MS-DOS(R)  Version 3.20",                       "MS-DOS Version 3.20",                                    NULL, NULL },
    { "MSDOS 3.20 (Data General OEM)", "MS-DOS",      3, 20,  -1, "Data General Corporation MS-DOS",                            "Data General Corp MS-DOS  Version  3.20 ",               NULL, NULL },
    { "MSDOS 3.20 (HP OEM)",           "IBMPcDos",    3, 20,  -1, "Microsoft(R) MS-DOS(R)  Version 3.20",                       "MS-DOS Version 3.20",                                    NULL, NULL },
    { "MSDOS 3.20 (Zenith OEM)",       "ZenitDOS",    3, 20,  -1, "MS-DOS Version 3.20",                                        NULL,                                                     NULL, NULL },
    { "MSDOS 3.20 (Generic)",          "MS-DOS",      3, 20,  -1, "Microsoft MS-DOS Version 3.20",                              "MS-DOS Version 3.20",                                    NULL, NULL },
    { "MSDOS 3.21 (Hyosung OEM)",      "MS-DOS",      3, 21,  -1, "HYOSUNG MS-DOS Ver 3.21",                                    "HYOSUNG MS-DOS Ver 3.21",                                NULL, NULL },
    { "MSDOS 3.21 (Generic)",          "MS-DOS",      3, 21,  -1, "Microsoft(R) MS-DOS(R)  Version 3.21",                       "MS-DOS Version 3.21",                                    NULL, NULL },
    { "MSDOS 3.30 (Toshiba OEM)",      "IBMPcDos",    3, 30,  -1, "MS-DOS(R) Version 3.30 ",                                    "Toshiba MS-DOS Version 3.30 / R3CE0US      ",            NULL, NULL },
    { "MSDOS 3.30",                    "IBMPcDos",    3, 30,  -1, "Microsoft(R) MS-DOS(R)  Version 3.30",                       "MS-DOS Version 3.30                     ",               NULL, NULL },
    { "MSDOS 3.30A (AT&T OEM)",        "OlivtDOS",    3, 30,  -1, "Microsoft(R) MS-DOS(R)  Version 3.30a",                      "AT&T Personal Computer MS-DOS Version 3.30a  Rev. 1.01", NULL, NULL },
    { "MSDOS 3.30A (Generic)",         "IBMPcDos",    3, 30,  -1, "Microsoft(R) MS-DOS(R)  Version 3.30A",                      "MS-DOS Version 3.30                     ",               NULL, NULL },
    { "MSDOS 4.01 (Toshiba OEM)",      "MS-DOS",      4,  0,  -1, "MS-DOS(R) Version 4.01",                                     "Toshiba MS-DOS Version 4.01 / R4A60US    ",              NULL, NULL },
    { "MSDOS 4.01 (Generic)",          "MS-DOS",      4,  0,  -1, "Microsoft(R) MS-DOS(R) Version 4.01",                        "MS-DOS Version 4.01",                                    NULL, NULL },
    { "MSDOS 5.00 (Generic)",          "MS-DOS",      5,  0, 255, "Microsoft(R) MS-DOS(R) Version 5.00",                        "MS-DOS Version 5.00",                                    NULL, NULL },
    { "MSDOS 5.00 (AST OEM)",          "IBMPcDos",    5,  0,   0, "Microsoft(R) MS-DOS(R) Version 5.00 from AST RESEARCH INC.", "MS-DOS Version 5.00 from AST RESEARCH INC.",             NULL, NULL },
    { "MSDOS 5.00 (Compaq OEM)",       "IBMPcDos",    5,  0,   0, "COMPAQ MS-DOS Version 5.00   ",                              "COMPAQ MS-DOS Version 5.00   ",                          NULL, NULL },
    { "MSDOS 5.00 (Olivetti OEM)",     "OlivtDOS",    5,  0,  35, NULL,                                                         NULL,                                                     NULL, NULL },
    { "MSDOS 5.00 (Toshiba OEM)",      "MS-DOS",      5,  0, 255, "TOSHIBA Personal Computer  MS-DOS Version 5.00",             "Toshiba MS-DOS Version 5.00 / R5B80SC   ",               NULL, NULL },
    { "MSDOS 6.0",                     "MS-DOS",      6,  0, 255, "Microsoft(R) MS-DOS(R) Version 6",                           "MS-DOS Version 6.00",                                    NULL, NULL },
    { "MSDOS 6.20",                    "MS-DOS",      6, 20, 255, "Microsoft(R) MS-DOS(R) Version 6.20",                        "MS-DOS Version 6.20",                                    NULL, NULL },
    { "MSDOS 6.21",                    "MS-DOS",      6, 20, 255, "Microsoft(R) MS-DOS(R) Version 6.21",                        "MS-DOS Version 6.21",                                    NULL, NULL },
    { "MSDOS 7.0",                     "MS-DOS",      7,  0, 255, NULL,                                                         "Windows 95. [Version 4.00.950]",                         NULL, NULL },
    { "MSDOS 7.10",                    "MS-DOS",      7, 10, 255, NULL,                                                         "MS-DOS 7.1 [Version 7.10.1999]",                         NULL, NULL },
    { "MSDOS 7.10",                    "MS-DOS",      7, 10, 255, NULL,                                                         "Windows 95. [Version 4.00.1111]",                        NULL, NULL },
    { "MSDOS 7.10",                    "MS-DOS",      7, 10, 255, NULL,                                                         "Windows 98 [Version 4.10.1998]",                         NULL, NULL },
    { "MSDOS 7.10",                    "MS-DOS",      7, 10, 255, NULL,                                                         "Windows 98 [Version 4.10.2222]",                         NULL, NULL },
    { "MSDOS 8.0",                     "MS-DOS",      8,  0, 255, NULL,                                                         "Windows Millennium [Version 4.90.3000]",                 NULL, NULL },
    { "Novell DOS 7",                  "IBMPcDos",    6,  0,   0, NULL,                                                         "Novell DOS 7",                                           NULL, NULL },
    { "OS/2 2.0",                      "IBMPcDos",   20,  0,   0, NULL,                                                         "The Operating System/2 Version is 2.00 ",                NULL, NULL },
    { "OS/2 2.1",                      "IBMPcDos",   20, 10,   0, NULL,                                                         "The Operating System/2 Version is 2.10 ",                NULL, NULL },
    { "OS/2 3.0",                      "IBMPcDos",   20, 30,   0, NULL,                                                         "The Operating System/2 Version is 3.00 ",                NULL, NULL },
    { "OS/2 4.0",                      "IBMPcDos",   20, 40,   0, NULL,                                                         "The Operating System/2 Version is 4.00 ",                NULL, NULL },
    { "OS/2 4.52 / EComStation",       "IBMPcDos",   20, 45,   0, NULL,                                                         "The Operating System/2 Version is 4.50 ",                NULL, NULL },
    { "PCDOS 3.00",                    "IBMPcDos",    3,  0,  -1, "The IBM Personal Computer DOS",                              "IBM Personal Computer DOS Version  3.00 ",               NULL, NULL },
    { "PCDOS 3.10",                    "IBMPcDos",    3, 10,  -1, "The IBM Personal Computer DOS",                              "IBM Personal Computer DOS Version  3.10 ",               NULL, NULL },
    { "PCDOS 4.00",                    "IBMPcDos",    4,  0,  -1, "IBM DOS Version 4.00",                                       "IBM DOS Version 4.00",                                   NULL, NULL },
    { "PCDOS 5.00",                    "IBMPcDos",    5,  0,   0, "IBM DOS Version 5.00",                                       "IBM DOS Version 5.00",                                   NULL, NULL },
    { "PCDOS 5.02",                    "IBMPcDos",    5,  2,   0, NULL,                                                         "IBM DOS Version 5.02",                                   NULL, NULL },
    { "PCDOS 6.10",                    "IBMPcDos",    6,  0,   0, "IBM DOS Version 6.10",                                       "IBM DOS Version 6.1",                                    NULL, NULL },
    { "PCDOS 7.10",                    "IBMPcDos",    7, 10,   0, "PC DOS Version 7.10",                                        "PC DOS Version 7.1",                                     NULL, NULL },
    { "PCDOS 2000",                    "IBMPcDos",    7,  0,   0, "PC DOS Version 7.00",                                        "PC DOS Version 7.0",                                     NULL, NULL },
    { "REAL32 7.6",                    "IBMPcDos",    3, 31,  -1, NULL,                                                         NULL,                                                     NULL, "REAL32 7.6" },
    { "ROMDOS 6.22",                   "MS-DOS",      6, 22, 255, NULL,                                                         "Datalight ROM-DOS Version 6.22SU",                       NULL, NULL },
    { "ROMDOS 7.1",                    "MS-DOS",      7, 10, 255, NULL,                                                         "Datalight ROM-DOS Version 7.1SU",                        NULL, NULL },
    { NULL,                            NULL,         -1, -1,  -1, NULL,                                                         NULL,                                                     NULL, NULL }
};

typedef struct dos_win_version_s {
    char *name;
    char *cmd_version;
    char *prod_version;
} dos_win_version_t;

static dos_win_version_t dos_win_versions[] = {
    { "Windows NT 3.50",                           "Windows NT Version 3.50 ",                   NULL },
    { "Windows NT 3.51",                           "Windows NT Version 3.51 ",                   NULL },
    { "Windows NT 4.0",                            "Windows NT Version 4.0  ",                   NULL },
    { "Windows Neptune",                           "Microsoft Windows 2000 [Version 5.00.5111]", "Windows 2000 Professional" },
    { "Windows 2000 Pro",                          "Microsoft Windows 2000 [Version 5.00.2195]", "Windows 2000 Professional" },
    { "Windows 2000 Powered",                      "Microsoft Windows 2000 [Version 5.00.2195]", "Windows Powered" },
    { "Windows 2000 Server",                       "Microsoft Windows 2000 [Version 5.00.2195]", "Windows 2000 Server" },
    { "Windows 2000 Advanced Server",              "Microsoft Windows 2000 [Version 5.00.2195]", "Windows 2000 Advanced Server" },
    { "Windows 2000 Datacenter Server",            "Microsoft Windows 2000 [Version 5.00.2195]", "Windows 2000 Datacenter Server" },
    { "Windows XP Starter / Home",                 "Microsoft Windows XP [Version 5.1.2600]",    "Windows XP Home Edition" },
    { "Windows XP Pro",                            "Microsoft Windows XP [Version 5.1.2600]",    "Windows XP Professional" },
    { "Windows 2003 Web Server",                   "Microsoft Windows [Version 5.2.3790]",       "Windows Server 2003, Web Edition" },
    { "Windows 2003 Standard Server",              "Microsoft Windows [Version 5.2.3790]",       "Windows Server 2003, Standard Edition" },
    { "Windows 2003 SBS / Home Server",            "Microsoft Windows [Version 5.2.3790]",       "Windows Server 2003 for Small Business Server"},
    { "Windows 2003 Enterprise Server",            "Microsoft Windows [Version 5.2.3790]",       "Windows Server 2003, Enterprise Edition" },
    { "Windows 2003 Datacenter Server",            "Microsoft Windows [Version 5.2.3790]",       "Windows Server 2003, Datacenter Edition" },
    { "Windows Vista",                             "Microsoft Windows [Version 6.0.6000]",       NULL },
    { "Windows 7",                                 "Microsoft Windows [Version 6.1.7601]",       NULL },
    { "Windows XP FLP / 2009 Embedded",            "Microsoft Windows XP [Version 5.1.2600]",    NULL },
    { "Windows 8",                                 "Microsoft Windows [Version 6.2.9200]",       NULL },
    { "Windows 8.1",                               "Microsoft Windows [Version 6.3.9600]",       NULL },
    { "Windows 10",                                "Microsoft Windows [Version 10.0.10240]",     NULL },
    { NULL,                                        NULL,                                         NULL }
};

static char *illegal_strings[] = {
    "The shell is about to be terminated, though, this is",
    NULL
};

static int check_illegal_string(char *string)
{
    int i;

    for (i = 0; illegal_strings[i]; ++i) {
        if (!strcmp(string, illegal_strings[i])) {
            return 1;
        }
    }
    return 0;
}

static char *get_prod_spec_string(char *command)
{
    char buffer[160];
    FILE *infile = NULL;
    char *retval = NULL;
    int found = 0;

    infile = popen(command, "r");
    if (infile) {
        do {
            if (!fgets(buffer, 159, infile)) {
                found = 2;
            } else {
                if (!strncmp(buffer, "Product=", 8)) {
                    found = 1;
                }
            }
        } while (!found);
        pclose(infile);
        if (found == 1) {
            buffer[strlen(buffer) - 1] = 0;
            retval = lib_stralloc(buffer + 8);
        }

    }
    if (retval) {
        if (check_illegal_string(retval)) {
            lib_free(retval);
            retval = NULL;
        }
    }
    return retval;
}

static char *get_cmd_ver_string(char *command)
{
    char buffer[160];
    FILE *infile = NULL;
    char *retval = NULL;
    int found = 0;
    int i;

    infile = popen(command, "r");
    if (infile) {
        do {
            if (!fgets(buffer, 159, infile)) {
                found = 2;
            } else {
                if (strlen(buffer) > 1) {
                    for (i = 0; buffer[i]; ++i) {
                        if (buffer[i] != ' ' && buffer[i] != '\r' && buffer[i] != '\n') {
                            found = 1;
                        }
                    }
                }
            }
        } while (!found);
        pclose(infile);
        if (found == 1) {
            buffer[strlen(buffer) - 1] = 0;
            retval = lib_stralloc(buffer);
        }

    }
    if (retval) {
        if (check_illegal_string(retval)) {
            lib_free(retval);
            retval = NULL;
        }
    }
    return retval;
}

static char *get_command_com_string(void)
{
    FILE *infile = NULL;
    char buffer[65280];
    char *ptr = NULL;
    char *ptr2 = NULL;
    int i;
    int found = 0;
    char *retval = NULL;
    char *comspec = NULL;

    comspec = getenv("COMSPEC");

    if (!comspec) {
        comspec = "C:\\COMMAND.COM";
    }

    if (!strcmp(comspec, "C:\\BIN\\CMDXSWP.COM")) {
        return lib_stralloc("FreeDOS 0.9");
    }

    infile = fopen(comspec, "rb");
    if (infile) {
        memset(buffer, 0, 65280);
        fread(buffer, 1, 65280, infile);
        fclose(infile);
        for (i = 0; !found; ++i) {
            if (i == 65280 - 4) {
                found = 2;
            } else if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') {
                found = 1;
            }
        }
        if (found == 1) {
            ptr = buffer + i + 3;
            ptr2 = strstr(ptr, "\r");
            if (ptr2) {
                ptr2[0] = 0;
                retval = lib_stralloc(ptr);
            }
        }
    }

    if (retval) {
        if (check_illegal_string(retval)) {
            lib_free(retval);
            retval = NULL;
        }
    }

    return retval;
}

static char *get_version_from_env(void)
{
    char *os = getenv("OS");
    char *ver = getenv("VER");
    char *retval = NULL;

    if (os && ver) {
        retval = util_concat(os, " ", ver, NULL);
    }
    return retval;
}

static int get_real32_ver(void)
{
    __dpmi_regs r;

    r.h.cl = 0xa3;
    __dpmi_int(0xe0, &r);

    return (int)r.x.ax;
}

static int desqview_present(void)
{
    __dpmi_regs r;

    r.x.ax = 0xde00;
    __dpmi_int(0x2f, &r);

    return (r.h.al == 0xff);
}

static void get_desqview_version(int *major, int *minor)
{
    __dpmi_regs r;

    r.h.ah = 0x2b;
    r.x.bx = 0;
    r.x.cx = 0x4445;
    r.x.dx = 0x5351;
    r.h.al = 0x01;
    __dpmi_int(0x21, &r);

    *major = r.h.bh;
    *minor = r.h.bl;
}

static int get_dos_oem_nr(void)
{
    __dpmi_regs r;

    r.h.ah = 0x30;
    r.h.al = 0x00;
    __dpmi_int(0x21, &r);

    return (int)r.h.bh;
}

static int get_windows_version(int *major, int *minor, int *mode)
{
    __dpmi_regs r;

    r.x.ax = 0x160a;
    __dpmi_int(0x2f, &r);

    if (r.x.cx == 2 || r.x.cx == 3) {
        *major = r.h.bh;
        *minor = r.h.bl;
        *mode = r.x.cx;
        return 1;
    }

    r.x.ax = 0x4680;
    __dpmi_int(0x2f, &r);
    if (r.x.ax == 0) {
        r.x.ax = 0x4B02;
        r.x.bx = 0;
        r.x.es = 0;
        r.x.di = 0;
        __dpmi_int(0x2f, &r);
        if (r.x.es == 0 && r.x.di == 0) {
            *major = 3;
            *minor = 0;
            return 1;
        } else {
            return 0;
        }
    }

    r.x.ax = 0x1600;
    __dpmi_int(0x2f, &r);
    if (r.h.al == 1 || r.h.al == 0xff) {
        *major = 2;
        *minor = 0;
        return 1;
    }
    if (r.h.al & 0x7f) {
        *major = 3;
        *minor = 0;
        return 1;
    }

    return 0;
}

char *platform_get_dos_runtime_os(void)
{
    unsigned short real_version;

    int version_major = -1;
    int version_minor = -1;
    int version_oem = -1;
    int win_major = 0;
    int win_minor = 0;
    int win_mode = 0;
    int real32_version = 0;
    const char *version_flavor = _os_flavor;
    char *version_ver_string = NULL;
    char *command_ver_string = NULL;
    char *comspec_ver_string = NULL;
    char *env_ver_string = NULL;
    char *cmd_ver_string = NULL;
    char *prodver_string = NULL;
    char *systemroot = NULL;
    char *prodver_command = NULL;
    char *real32_string = NULL;

    int i;
    int found = 0;

    if (!got_os) {
        real_version = _get_dos_version(1);
        version_major = real_version >> 8;
        version_minor = real_version & 0xff;

        if (version_major >= 5) {
            version_oem = get_dos_oem_nr();
        }

        comspec_ver_string = getenv("COMSPEC");

        command_ver_string = get_command_com_string();
        if ((version_major != 7 || version_minor != 10 || version_oem != 253) && comspec_ver_string && strcmp(comspec_ver_string, "C:\\CDOS.COM")) {
            version_ver_string = get_cmd_ver_string("ver");
        }
        env_ver_string = get_version_from_env();

        if (comspec_ver_string) {
            comspec_ver_string = lib_stralloc(comspec_ver_string);
        }

        if (version_major == 5 && version_minor == 50) {
            if (!cmd_ver_string) {
                cmd_ver_string = get_cmd_ver_string("ver");
                if (!strcmp(cmd_ver_string, "MS-DOS Version 5.00.500")) {
                    lib_free(cmd_ver_string);
                    cmd_ver_string = get_cmd_ver_string("command.com /c cmd /c ver");
                }
            }

            for (i = 0; !found && dos_win_versions[i].name; ++i) {
                if (!strcmp(cmd_ver_string, dos_win_versions[i].cmd_version)) {
                    if (!prodver_string && dos_win_versions[i].prod_version) {
                        if (!systemroot) {
                            systemroot = getenv("SYSTEMROOT");
                        }
                        prodver_command = util_concat("type ", systemroot, "\\system32\\prodspec.ini", NULL);
                        prodver_string = get_prod_spec_string(prodver_command);
                        lib_free(prodver_command);
                        prodver_command = NULL;
                    }
                    if (!strcmp(prodver_string, dos_win_versions[i].prod_version)) {
                        found = 1;
                    }
                }
            }
            if (!found) {
                sprintf(archdep_os_version, "Unknown DOS on Windows version");
            } else {
                sprintf(archdep_os_version, "DOS on %s", dos_win_versions[i - 1].name);
            }
            if (cmd_ver_string) {
                lib_free(cmd_ver_string);
                cmd_ver_string = NULL;
            }
            if (prodver_string) {
                lib_free(prodver_string);
                prodver_string = NULL;
            }
        } else {
            for (i = 0; !found && dos_versions[i].name; ++i) {
                if (!strcmp(version_flavor, dos_versions[i].flavor)) {
                    if (version_major == dos_versions[i].major) {
                        if (version_minor == dos_versions[i].minor) {
                            if (version_oem == dos_versions[i].oem) {
                                if ((dos_versions[i].command_string && command_ver_string && !strcmp(dos_versions[i].command_string, command_ver_string)) || !dos_versions[i].command_string) {
                                    if ((dos_versions[i].ver_string && version_ver_string && !strcmp(dos_versions[i].ver_string, version_ver_string)) || !dos_versions[i].ver_string) {
                                        if ((dos_versions[i].comspec_string && comspec_ver_string && !strcmp(dos_versions[i].comspec_string, comspec_ver_string)) || !dos_versions[i].comspec_string) {
                                            if ((dos_versions[i].env_string && env_ver_string && !strcmp(dos_versions[i].env_string, env_ver_string)) || !dos_versions[i].env_string) {
                                                found = 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!found) {
                sprintf(archdep_os_version, "Unknown DOS version: %s %d %d %d %s %s %s %s",
                    version_flavor,
                    version_major,
                    version_minor,
                    version_oem,
                    (command_ver_string) ? command_ver_string : "(N/A)",
                    (version_ver_string) ? version_ver_string : "(N/A)",
                    (env_ver_string) ? env_ver_string : "(N/A)",
                    (comspec_ver_string) ? comspec_ver_string : "(N/A)");
            } else {
                sprintf(archdep_os_version, dos_versions[i - 1].name);
                if (desqview_present()) {
                    get_desqview_version(&win_major, &win_minor);
                    if (!win_major && !win_minor) {
                        sprintf(archdep_os_version, "%s [DESQView]", archdep_os_version);
                    } else {
                        sprintf(archdep_os_version, "%s [DESQView %d.%d]", archdep_os_version, win_major, win_minor);
                    }
                }
                if (get_windows_version(&win_major, &win_minor, &win_mode)) {
                    if (win_major == 4 && win_minor == 0) {
                        sprintf(archdep_os_version, "%s [Windows 95]", archdep_os_version);
                    } else if (win_major == 4 && win_minor == 10) {
                        sprintf(archdep_os_version, "%s [Windows 98]", archdep_os_version);
                    } else if (win_major == 4 && win_minor == 90) {
                        sprintf(archdep_os_version, "%s [Windows ME]", archdep_os_version);
                    } else {
                        sprintf(archdep_os_version, "%s [Windows %d.%d]", archdep_os_version, win_major, win_minor);
                    }
                }
                if (!strcmp(archdep_os_version, "REAL/32")) {
                    real32_version = get_real32_ver();
                    if (real32_version >> 8 == 0x14) {
                        switch (real32_version & 0xff) {
                            case 0x32:
                                real32_string = "DR Concurrent PC DOS 3.2";
                                break;
                            case 0x41:
                                real32_string = "DR Concurrent DOS 4.1";
                                break;
                            case 0x50:
                                real32_string = "DR Concurrent DOS/XM 5.0";
                                break;
                            case 0x63:
                                real32_string = "DR Multiuser DOS 5.0";
                                break;
                            case 0x65:
                                real32_string = "DR Multiuser DOS 5.01";
                                break;
                            case 0x66:
                                real32_string = "DR Multiuser DOS 5.1";
                                break;
                            case 0x67:
                                real32_string = "IMS Multiuser DOS 7.0/7.1";
                                break;
                            case 0x68:
                                real32_string = "IMS REAL/32 7.50/7.51";
                                break;
                            case 0x69:
                                real32_string = "IMS REAL/32 7.52/7.53";
                                break;
                            default:
                                real32_string = "IMS REAL/32 7.6";
                        }
                        sprintf(archdep_os_version, real32_string);
                    }
                }
            }
        }
        got_os = 1;
        if (command_ver_string) {
            lib_free(command_ver_string);
            command_ver_string = NULL;
        }
        if (version_ver_string) {
            lib_free(version_ver_string);
            version_ver_string = NULL;
        }
        if (env_ver_string) {
            lib_free(env_ver_string);
            env_ver_string = NULL;
        }
        if (comspec_ver_string) {
            lib_free(comspec_ver_string);
            comspec_ver_string = NULL;
        }
    }
    return archdep_os_version;
}
#endif
