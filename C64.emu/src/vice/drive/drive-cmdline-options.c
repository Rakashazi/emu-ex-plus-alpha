/*
 * drive-cmdline-options.c - Hardware-level disk drive emulation,
 *                           command line options module.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "drive-cmdline-options.h"
#include "drive.h"
#include "lib.h"
#include "machine.h"
#include "machine-drive.h"

static const cmdline_option_t cmdline_options[] =
{
    { "-truedrive", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DriveTrueEmulation", (void *)1,
      NULL, "Enable hardware-level emulation of disk drives" },
    { "+truedrive", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DriveTrueEmulation", (void *)0,
      NULL, "Disable hardware-level emulation of disk drives" },
    { "-drivesound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DriveSoundEmulation", (void *)1,
      NULL, "Enable sound emulation of disk drives" },
    { "+drivesound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DriveSoundEmulation", (void *)0,
      NULL, "Disable sound emulation of disk drives" },
    { "-drivesoundvolume", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DriveSoundEmulationVolume", NULL,
      "<Volume>", "Set volume for disk drive sound emulation (0-4000)" },
    CMDLINE_LIST_END
};

static cmdline_option_t cmd_drive[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Type>", NULL },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<method>", "Set drive 40 track extension policy (0: never, 1: ask, 2: on access)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<method>", "Set drive idling method (0: no traps, 1: skip cycles, 2: trap idle)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<RPM>", "Set drive rpm (30000 = 300rpm)" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      "<Wobble>", "Set drive wobble (100 = +/-0.5rpm)" },
    CMDLINE_LIST_END
};

static cmdline_option_t cmd_drive_rtc[] = {
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable saving of FD2000/4000 RTC data when changed." },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable saving of FD2000/4000 RTC data when changed." },
    CMDLINE_LIST_END
};

typedef struct machine_drives_s {
    int machine;
    int drives_index;
} machine_drives_t;

#define DRIVES_IEEE   0
#define DRIVES_PLUS4  1
#define DRIVES_C128   2
#define DRIVES_C64DTV 3
#define DRIVES_C64    4

static char *drives[] = {
    "Set drive type (0: no drive, 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250)",
    "Set drive type (0: no drive, 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1551: CBM 1551, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000)",
    "Set drive type (0: no drive, 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1573: CBM 1571CR, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000, 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250)",
    "Set drive type (0: no drive, 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000)",
    "Set drive type (0: no drive, 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000, 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250)"
};

static machine_drives_t machine_drives[] = {
    { VICE_MACHINE_C64,    DRIVES_C64    },
    { VICE_MACHINE_C128,   DRIVES_C128   },
    { VICE_MACHINE_VIC20,  DRIVES_C64    },
    { VICE_MACHINE_PET,    DRIVES_IEEE   },
    { VICE_MACHINE_CBM5x0, DRIVES_IEEE   },
    { VICE_MACHINE_CBM6x0, DRIVES_IEEE   },
    { VICE_MACHINE_PLUS4,  DRIVES_PLUS4  },
    { VICE_MACHINE_C64DTV, DRIVES_C64DTV },
    { VICE_MACHINE_C64SC,  DRIVES_C64    },
    { VICE_MACHINE_VSID,   DRIVES_C64DTV },
    { VICE_MACHINE_SCPU64, DRIVES_C64    },
    { 0, 0 },
};

int drive_cmdline_options_init(void)
{
    unsigned int dnr, i, j;
    char *found_string = NULL;
    int has_iec;

    switch (machine_class) {
        case VICE_MACHINE_NONE:
        case VICE_MACHINE_PET:
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
        case VICE_MACHINE_VSID:
            has_iec = 0;
            break;
        default:
            has_iec = 1;
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        cmd_drive[0].name = lib_msprintf("-drive%itype", dnr + 8);
        cmd_drive[0].resource_name = lib_msprintf("Drive%iType", dnr + 8);
        for (j = 0; machine_drives[j].machine != 0; j++) {
            if (machine_drives[j].machine == machine_class) {
                found_string = drives[machine_drives[j].drives_index];
            }
        }
        if (found_string) {
            cmd_drive[0].description = found_string;
        } else {
            cmd_drive[0].description = "Set drive type (0: no drive)";
        }
        cmd_drive[1].name = lib_msprintf("-drive%iextend", dnr + 8);
        cmd_drive[1].resource_name
            = lib_msprintf("Drive%iExtendImagePolicy", dnr + 8);
        cmd_drive[2].name = lib_msprintf("-drive%iidle", dnr + 8);
        cmd_drive[2].resource_name
            = lib_msprintf("Drive%iIdleMethod", dnr + 8);
        cmd_drive[3].name = lib_msprintf("-drive%irpm", dnr + 8);
        cmd_drive[3].resource_name
            = lib_msprintf("Drive%iRPM", dnr + 8);
        cmd_drive[4].name = lib_msprintf("-drive%iwobble", dnr + 8);
        cmd_drive[4].resource_name
            = lib_msprintf("Drive%iWobble", dnr + 8);

        if (has_iec) {
            cmd_drive_rtc[0].name = lib_msprintf("-drive%irtcsave", dnr + 8);
            cmd_drive_rtc[0].resource_name
                = lib_msprintf("Drive%iRTCSave", dnr + 8);
            cmd_drive_rtc[1].name = lib_msprintf("+drive%irtcsave", dnr + 8);
            cmd_drive_rtc[1].resource_name
                = lib_msprintf("Drive%iRTCSave", dnr + 8);
            if (cmdline_register_options(cmd_drive_rtc) < 0) {
                return -1;
            }
        }
        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 5; i++) {
            lib_free(cmd_drive[i].name);
            lib_free(cmd_drive[i].resource_name);
        }

        if (has_iec) {
            lib_free(cmd_drive_rtc[0].name);
            lib_free(cmd_drive_rtc[0].resource_name);
            lib_free(cmd_drive_rtc[1].name);
            lib_free(cmd_drive_rtc[1].resource_name);
        }
    }

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return machine_drive_cmdline_options_init();
}
