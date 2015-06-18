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
#include "translate.h"

static const cmdline_option_t cmdline_options[] = {
    { "-truedrive", SET_RESOURCE, 0,
      NULL, NULL, "DriveTrueEmulation", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TRUE_DRIVE,
      NULL, NULL },
    { "+truedrive", SET_RESOURCE, 0,
      NULL, NULL, "DriveTrueEmulation", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TRUE_DRIVE,
      NULL, NULL },
    { "-drivesound", SET_RESOURCE, 0,
      NULL, NULL, "DriveSoundEmulation", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_SOUND,
      NULL, NULL },
    { "+drivesound", SET_RESOURCE, 0,
      NULL, NULL, "DriveSoundEmulation", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_SOUND,
      NULL, NULL },
    { "-drivesoundvolume", SET_RESOURCE, 1,
      NULL, NULL, "DriveSoundEmulationVolume", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VOLUME, IDCLS_SET_DRIVE_SOUND_VOLUME,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t cmd_drive[] = {
    { NULL, SET_RESOURCE, 1,
      NULL, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_TYPE, IDCLS_SET_DRIVE_TYPE,
      NULL, NULL },
    { NULL, SET_RESOURCE, 1,
      NULL, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_SET_DRIVE_EXTENSION_POLICY,
      NULL, NULL },
    { NULL, SET_RESOURCE, 1,
      NULL, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_SET_IDLE_METHOD,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t cmd_drive_rtc[] = {
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RTC_SAVE,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RTC_SAVE,
      NULL, NULL },
    { NULL }
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
    ", 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250",
    ", 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1551: CBM 1551, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000)",
    ", 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1573: CBM 1571CR, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000, 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250)",
    ", 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000)",
    ", 1540: CBM 1540, 1541: CBM 1541, 1542: CBM 1541-II, 1570: CBM 1570, 1571: CBM 1571, 1581: CBM 1581, 2000: CMD FD-2000, 4000: CMD FD-4000, 2031: CBM 2031, 2040: CBM 2040, 3040: CBM 3040, 4040: CBM 4040, 1001: CBM 1001, 8050: CBM 8050, 8250: CBM 8250)"
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
            cmd_drive[0].description = ")";
        }
        cmd_drive[1].name = lib_msprintf("-drive%iextend", dnr + 8);
        cmd_drive[1].resource_name
            = lib_msprintf("Drive%iExtendImagePolicy", dnr + 8);
        cmd_drive[2].name = lib_msprintf("-drive%iidle", dnr + 8);
        cmd_drive[2].resource_name
            = lib_msprintf("Drive%iIdleMethod", dnr + 8);

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

        for (i = 0; i < 3; i++) {
            lib_free((char *)cmd_drive[i].name);
            lib_free((char *)cmd_drive[i].resource_name);
        }

        if (has_iec) {
            lib_free((char *)cmd_drive_rtc[0].name);
            lib_free((char *)cmd_drive_rtc[0].resource_name);
            lib_free((char *)cmd_drive_rtc[1].name);
            lib_free((char *)cmd_drive_rtc[1].resource_name);
        }
    }

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return machine_drive_cmdline_options_init();
}
