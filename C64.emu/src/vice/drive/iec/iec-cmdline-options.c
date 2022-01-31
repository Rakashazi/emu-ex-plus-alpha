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

static const cmdline_option_t cmdline_options[] =
{
    { "-dos1540", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1540", NULL,
      "<Name>", "Specify name of 1540 DOS ROM image" },
    { "-dos1541", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1541", NULL,
      "<Name>", "Specify name of 1541 DOS ROM image" },
    { "-dos1541II", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1541II", NULL,
      "<Name>", "Specify name of 1541-II DOS ROM image" },
    { "-dos1570", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1570", NULL,
      "<Name>", "Specify name of 1570 DOS ROM image" },
    { "-dos1571", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1571", NULL,
      "<Name>", "Specify name of 1571 DOS ROM image" },
    { "-dos1581", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName1581", NULL,
      "<Name>", "Specify name of 1581 DOS ROM image" },
    { "-dos2000", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName2000", NULL,
      "<Name>", "Specify name of 2000 DOS ROM image" },
    { "-dos4000", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosName4000", NULL,
      "<Name>", "Specify name of 4000 DOS ROM image" },
    { "-dosCMDHD", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DosNameCMDHD", NULL,
      "<Name>", "Specify name of CMD HD Boot ROM image" },
    CMDLINE_LIST_END
};

static cmdline_option_t cmd_drive[] =
{
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable 8KiB RAM expansion at $2000-$3FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable 8KiB RAM expansion at $2000-$3FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable 8KiB RAM expansion at $4000-$5FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable 8KiB RAM expansion at $4000-$5FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable 8KiB RAM expansion at $6000-$7FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable 8KiB RAM expansion at $6000-$7FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable 8KiB RAM expansion at $8000-$9FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable 8KiB RAM expansion at $8000-$9FFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)1,
      NULL, "Enable 8KiB RAM expansion at $A000-$BFFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, NULL, (void *)0,
      NULL, "Disable 8KiB RAM expansion at $A000-$BFFF" },
    { NULL, SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, NULL, NULL,
      NULL, "Fixed Disk Size" },
    CMDLINE_LIST_END
};

int iec_cmdline_options_init(void)
{
    unsigned int dnr, i;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
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
        cmd_drive[10].name = lib_msprintf("-drive%ifixedsize", dnr + 8);
        cmd_drive[10].resource_name
            = lib_msprintf("Drive%iFixedSize", dnr + 8);

        if (cmdline_register_options(cmd_drive) < 0) {
            return -1;
        }

        for (i = 0; i < 11; i++) {
            lib_free(cmd_drive[i].name);
            lib_free(cmd_drive[i].resource_name);
        }
    }

    return cmdline_register_options(cmdline_options);
}
