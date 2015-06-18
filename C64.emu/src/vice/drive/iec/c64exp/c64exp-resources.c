/*
 * c64exp-resources.c
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

#include "c64exp-resources.h"
#include "drive.h"
#include "drivemem.h"
#include "lib.h"
#include "profdos.h"
#include "resources.h"
#include "supercard.h"
#include "util.h"


static char *profdos_1571_name = NULL;
static char *supercard_name = NULL;


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
        case DRIVE_PC_DD3:
        case DRIVE_PC_FORMEL64:
            break;
        default:
            return -1;
    }

    drive->parallel_cable = val;
    set_drive_ram(vice_ptr_to_uint(param));

    return 0;
}

static int set_drive_profdos(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->profdos = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));

    return 0;
}

static int set_profdos_1571_name(const char *val, void *param)
{
    if (util_string_set(&profdos_1571_name, val)) {
        return 0;
    }

    return profdos_load_1571(profdos_1571_name);
}

static int set_drive_supercard(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->supercard = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));

    return 0;
}

static int set_supercard_name(const char *val, void *param)
{
    if (util_string_set(&supercard_name, val)) {
        return 0;
    }

    return supercard_load(supercard_name);
}

static resource_int_t res_drive[] = {
    { NULL, DRIVE_PC_NONE, RES_EVENT_SAME, NULL,
      NULL, set_drive_parallel_cable, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_profdos, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_supercard, NULL },
    { NULL }
};

static const resource_string_t resources_string[] =
{
    { "DriveProfDOS1571Name", "", RES_EVENT_NO, NULL,
      &profdos_1571_name, set_profdos_1571_name, NULL },
    { "DriveSuperCardName", "", RES_EVENT_NO, NULL,
      &supercard_name, set_supercard_name, NULL },
    { NULL }
};

int c64exp_resources_init(void)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        res_drive[0].name = lib_msprintf("Drive%iParallelCable", dnr + 8);
        res_drive[0].value_ptr = &(drive->parallel_cable);
        res_drive[0].param = uint_to_void_ptr(dnr);
        res_drive[1].name = lib_msprintf("Drive%iProfDOS", dnr + 8);
        res_drive[1].value_ptr = &(drive->profdos);
        res_drive[1].param = uint_to_void_ptr(dnr);
        res_drive[2].name = lib_msprintf("Drive%iSuperCard", dnr + 8);
        res_drive[2].value_ptr = &(drive->supercard);
        res_drive[2].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free((char *)(res_drive[0].name));
        lib_free((char *)(res_drive[1].name));
        lib_free((char *)(res_drive[2].name));
    }

    return resources_register_string(resources_string);
}

void c64exp_resources_shutdown(void)
{
    lib_free(profdos_1571_name);
    lib_free(supercard_name);
}
