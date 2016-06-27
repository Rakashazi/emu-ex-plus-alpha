/*
 * platform_os2_runtime_os.c - OS/2 and EComStation runtime version discovery.
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
   - OS/2 4.0 (tested with all fixpacks)
   - OS/2 4.52
   - OS/2 4.52 Server
   - EComStation
*/

#include "vice.h"

#ifdef __OS2__

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>

static int read_file_into_buffer(char *file, char *buffer)
{
    int sl = 0;

    sl = open(file, O_RDONLY | O_BINARY);
    if (sl == -1) {
        return 0;
    }
    if (read(sl, buffer, 8192) < 1) {
        close(sl);
        return 0;
    }
    close(sl);
    return 1;
}

static char *os = NULL;

char *platform_get_os2_runtime_os(void)
{
    char buffer[8192];
    int i = 0;
    int found = 0;

    if (!os) {
        if (read_file_into_buffer("c:\\os2\\install\\syslevel.os2", buffer) == 0) {
            os = "Unknown OS/2 version";
        } else {

            while (i < (8191 - 15) && found == 0) {
                if (buffer[i] == 'X' && buffer[i + 1] == 'R') {
                    found = 1;
                } else {
                    i++;
                }
            }

            if (!strncmp(buffer + i, "XRUM010_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 10";
            } else if (!strncmp(buffer + i, "XRUM011_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 11";
            } else if (!strncmp(buffer + i, "XRUM012_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 12";
            } else if (!strncmp(buffer + i, "XRUM013_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 13";
            } else if (!strncmp(buffer + i, "XRUM014_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 14";
            } else if (!strncmp(buffer + i, "XRUM015_XRU4000", 15)) {
                os = "OS/2 4.0 fixpack 15";
            } else if (!strncmp(buffer + i, "XR04500_XR04500", 15)) {
                os = "OS/2 4.5";
            } else if (!strncmp(buffer + i, "XR04501_XR04501", 15)) {
                os = "OS/2 4.51";
            } else if (!strncmp(buffer + i, "XR04502_XR04502", 15)) {
                os = "OS/2 4.52";
            } else if (!strncmp(buffer + i, "XR04503_XR04503", 15)) {
                os = "OS/2 Server 4.52";
            } else if (!strncmp(buffer + i, "XR0C004_XR04503", 15)) {
                os = "EComStation";
            } else if (!strncmp(buffer + i, "XRU4000_XRU4000", 15)) {
                /* OS/2 4.0 with either no fixpack or fixpack 9 and below */
                if (read_file_into_buffer("c:\\os2\\install\\syslevel.fpk", buffer) == 0) {
                    os = "OS/2 4.0";
                } else {

                    while (i < (8191 - 15) && found == 0) {
                        if (buffer[i] == 'X' && buffer[i + 1] == 'R') {
                            found = 1;
                        } else {
                            i++;
                        }
                    }

                    if (!strncmp(buffer + i, "XR0M001_XR0M001", 15)) {
                        os = "OS/2 4.0 fixpack 1";
                    } else if (!strncmp(buffer + i, "XR0M002_XR0M002", 15)) {
                        os = "OS/2 4.0 fixpack 2";
                    } else if (!strncmp(buffer + i, "XR0M003_XR0M003", 15)) {
                        os = "OS/2 4.0 fixpack 3";
                    } else if (!strncmp(buffer + i, "XR0M004_XR0M004", 15)) {
                        os = "OS/2 4.0 fixpack 4";
                    } else if (!strncmp(buffer + i, "XR0M005_XR0M005", 15)) {
                        os = "OS/2 4.0 fixpack 5";
                    } else if (!strncmp(buffer + i, "XR0M006_XR0M006", 15)) {
                        os = "OS/2 4.0 fixpack 6";
                    } else if (!strncmp(buffer + i, "XR0M007_XR0M007", 15)) {
                        os = "OS/2 4.0 fixpack 7";
                    } else if (!strncmp(buffer + i, "XR0M008_XR0M008", 15)) {
                        os = "OS/2 4.0 fixpack 8";
                    } else if (!strncmp(buffer + i, "XR0M009_XR0M009", 15)) {
                        os = "OS/2 4.0 fixpack 9";
                    } else {
                        os = "Unknown OS/2 version";
                    }
                }
            }
        }
    }
    return os;
}
#endif
