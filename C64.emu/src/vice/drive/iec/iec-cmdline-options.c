/*
 * iec-cmdline-options.c
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
#include "drive.h"
#include "iec-cmdline-options.h"
#include "lib.h"
#include "translate.h"

static const cmdline_option_t cmdline_options[] = {
    { "-dos1541", SET_RESOURCE, 1,
      NULL, NULL, "DosName1541", "dos1541",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_1541_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos1541II", SET_RESOURCE, 1,
      NULL, NULL, "DosName1541II", "d1541II",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_1541_II_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos1570", SET_RESOURCE, 1,
      NULL, NULL, "DosName1570", "dos1570",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_1570_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos1571", SET_RESOURCE, 1,
      NULL, NULL, "DosName1571", "dos1571",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_1571_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos1581", SET_RESOURCE, 1,
      NULL, NULL, "DosName1581", "dos1581",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_1581_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos2000", SET_RESOURCE, 1,
      NULL, NULL, "DosName2000", "dos2000",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_2000_DOS_ROM_NAME,
      NULL, NULL },
    { "-dos4000", SET_RESOURCE, 1,
      NULL, NULL, "DosName4000", "dos4000",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_4000_DOS_ROM_NAME,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t cmd_drive[] = {
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RAM_2000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RAM_2000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RAM_4000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RAM_4000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RAM_6000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RAM_6000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RAM_8000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RAM_8000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DRIVE_RAM_A000,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DRIVE_RAM_A000,
      NULL, NULL },
    { NULL }
};

int iec_cmdline_options_init(void)
{
    unsigned int dnr, i;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        cmd_drive[0].name = lib_msprintf("-drive%iram2000", dnr + 8);
        cmd_drive[0].resource_name
            = lib_msprintf("Drive%iRAM2000", dnr + 8);
        cmd_drive[1].name = lib_msprintf("+drive%iram2000", dnr + 8);
        cmd_drive[1].resource_name
            = lib_msprintf("Drive%iRAM2000", dnr + 8);
        cmd_drive[2].name = lib_msprintf("-drive%iram4000", dnr + 8);
        cmd_drive[2].resource_name
            = lib_msprintf("Drive%iRAM4000", dnr + 8);
        cmd_drive[3].name = lib_msprintf("+drive%iram4000", dnr + 8);
        cmd_drive[3].resource_name
            = lib_msprintf("Drive%iRAM4000", dnr + 8);
        cmd_drive[4].name = lib_msprintf("-drive%iram6000", dnr + 8);
        cmd_drive[4].resource_name
            = lib_msprintf("Drive%iRAM6000", dnr + 8);
        cmd_drive[5].name = lib_msprintf("+drive%iram6000", dnr + 8);
        cmd_drive[5].resource_name
            = lib_msprintf("Drive%iRAM6000", dnr + 8);
        cmd_drive[6].name = lib_msprintf("-drive%iram8000", dnr + 8);
        cmd_drive[6].resource_name
            = lib_msprintf("Drive%iRAM8000", dnr + 8);
        cmd_drive[7].name = lib_msprintf("+drive%iram8000", dnr + 8);
        cmd_drive[7].resource_name
            = lib_msprintf("Drive%iRAM8000", dnr + 8);
        cmd_drive[8].name = lib_msprintf("-drive%irama000", dnr + 8);
        cmd_drive[8].resource_name
            = lib_msprintf("Drive%iRAMA000", dnr + 8);
        cmd_drive[9].name = lib_msprintf("+drive%irama000", dnr + 8);
        cmd_drive[9].resource_name
            = lib_msprintf("Drive%iRAMA000", dnr + 8);

        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 10; i++) {
            lib_free((char *)cmd_drive[i].name);
            lib_free((char *)cmd_drive[i].resource_name);
        }
    }

    return cmdline_register_options(cmdline_options);
}
