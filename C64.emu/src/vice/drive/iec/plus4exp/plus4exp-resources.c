/*
 * plus4exp-resources.c
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

#include "drive.h"
#include "drivemem.h"
#include "lib.h"
#include "plus4exp-resources.h"
#include "resources.h"


static void set_drive_ram(unsigned int dnr)
{
    drive_t *drive = drive_context[dnr]->drive;

    if (drive->type != DRIVE_TYPE_1570 && drive->type != DRIVE_TYPE_1571
        && drive->type != DRIVE_TYPE_1571CR) {
        return;
    }

    drivemem_init(drive_context[dnr], drive->type);

    return;
}

static int set_drive_parallel_cable(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    switch (val) {
        case DRIVE_PC_NONE:
        case DRIVE_PC_STANDARD:
            break;
        default:
            return -1;
    }

    drive->parallel_cable = val;
    set_drive_ram(vice_ptr_to_uint(param));

    return 0;
}


static resource_int_t res_drive[] = {
    { NULL, DRIVE_PC_NONE, RES_EVENT_SAME, NULL,
      NULL, set_drive_parallel_cable, NULL },
    { NULL }
};

int plus4exp_resources_init(void)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        res_drive[0].name = lib_msprintf("Drive%iParallelCable", dnr + 8);
        res_drive[0].value_ptr = &(drive->parallel_cable);
        res_drive[0].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free((char *)(res_drive[0].name));
    }

    return 0;
}

void plus4exp_resources_shutdown(void)
{
}
