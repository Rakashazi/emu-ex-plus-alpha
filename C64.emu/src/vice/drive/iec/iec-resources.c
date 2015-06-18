/*
 * iec-resources.c
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
#include "drivetypes.h"
#include "driverom.h"
#include "iec-resources.h"
#include "iecrom.h"
#include "lib.h"
#include "resources.h"
#include "traps.h"
#include "util.h"

static char *dos_rom_name_1540 = NULL;
static char *dos_rom_name_1541 = NULL;
static char *dos_rom_name_1541ii = NULL;
static char *dos_rom_name_1570 = NULL;
static char *dos_rom_name_1571 = NULL;
static char *dos_rom_name_1581 = NULL;
static char *dos_rom_name_2000 = NULL;
static char *dos_rom_name_4000 = NULL;

static void set_drive_ram(unsigned int dnr)
{
    drive_t *drive = drive_context[dnr]->drive;

    if (drive->type == DRIVE_TYPE_NONE) {
        return;
    }

    drivemem_init(drive_context[dnr], drive->type);

    return;
}

static int set_dos_rom_name_1540(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1540, val)) {
        return 0;
    }

    return iecrom_load_1540();
}

static int set_dos_rom_name_1541(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1541, val)) {
        return 0;
    }

    return iecrom_load_1541();
}

static int set_dos_rom_name_1541ii(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1541ii, val)) {
        return 0;
    }

    return iecrom_load_1541ii();
}

static int set_dos_rom_name_1570(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1570, val)) {
        return 0;
    }

    return iecrom_load_1570();
}

static int set_dos_rom_name_1571(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1571, val)) {
        return 0;
    }

    return iecrom_load_1571();
}

static int set_dos_rom_name_1581(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_1581, val)) {
        return 0;
    }

    return iecrom_load_1581();
}

static int set_dos_rom_name_2000(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_2000, val)) {
        return 0;
    }

    return iecrom_load_2000();
}

static int set_dos_rom_name_4000(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_4000, val)) {
        return 0;
    }

    return iecrom_load_4000();
}

static int set_drive_ram2(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->drive_ram2_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram4(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->drive_ram4_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram6(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->drive_ram6_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram8(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->drive_ram8_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_rama(int val, void *param)
{
    drive_t *drive = drive_context[vice_ptr_to_uint(param)]->drive;

    drive->drive_rama_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static const resource_string_t resources_string[] = {
    { "DosName1540", "dos1540", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &dos_rom_name_1540, set_dos_rom_name_1540, NULL },
    { "DosName1541", "dos1541", RES_EVENT_NO, NULL,
      &dos_rom_name_1541, set_dos_rom_name_1541, NULL },
    { "DosName1541ii", "d1541II", RES_EVENT_NO, NULL,
      &dos_rom_name_1541ii, set_dos_rom_name_1541ii, NULL },
    { "DosName1570", "dos1570", RES_EVENT_NO, NULL,
      &dos_rom_name_1570, set_dos_rom_name_1570, NULL },
    { "DosName1571", "dos1571", RES_EVENT_NO, NULL,
      &dos_rom_name_1571, set_dos_rom_name_1571, NULL },
    { "DosName1581", "dos1581", RES_EVENT_NO, NULL,
      &dos_rom_name_1581, set_dos_rom_name_1581, NULL },
    { "DosName2000", "dos2000", RES_EVENT_NO, NULL,
      &dos_rom_name_2000, set_dos_rom_name_2000, NULL },
    { "DosName4000", "dos4000", RES_EVENT_NO, NULL,
      &dos_rom_name_4000, set_dos_rom_name_4000, NULL },
    { NULL }
};

static resource_int_t res_drive[] = {
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_ram2, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_ram4, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_ram6, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_ram8, NULL },
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, set_drive_rama, NULL },
    { NULL }
};

int iec_resources_init(void)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        res_drive[0].name = lib_msprintf("Drive%iRAM2000", dnr + 8);
        res_drive[0].value_ptr = &(drive->drive_ram2_enabled);
        res_drive[0].param = uint_to_void_ptr(dnr);
        res_drive[1].name = lib_msprintf("Drive%iRAM4000", dnr + 8);
        res_drive[1].value_ptr = &(drive->drive_ram4_enabled);
        res_drive[1].param = uint_to_void_ptr(dnr);
        res_drive[2].name = lib_msprintf("Drive%iRAM6000", dnr + 8);
        res_drive[2].value_ptr = &(drive->drive_ram6_enabled);
        res_drive[2].param = uint_to_void_ptr(dnr);
        res_drive[3].name = lib_msprintf("Drive%iRAM8000", dnr + 8);
        res_drive[3].value_ptr = &(drive->drive_ram8_enabled);
        res_drive[3].param = uint_to_void_ptr(dnr);
        res_drive[4].name = lib_msprintf("Drive%iRAMA000", dnr + 8);
        res_drive[4].value_ptr = &(drive->drive_rama_enabled);
        res_drive[4].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free((char *)(res_drive[0].name));
        lib_free((char *)(res_drive[1].name));
        lib_free((char *)(res_drive[2].name));
        lib_free((char *)(res_drive[3].name));
        lib_free((char *)(res_drive[4].name));
    }

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return 0;
}

void iec_resources_shutdown(void)
{
    lib_free(dos_rom_name_1540);
    lib_free(dos_rom_name_1541);
    lib_free(dos_rom_name_1541ii);
    lib_free(dos_rom_name_1570);
    lib_free(dos_rom_name_1571);
    lib_free(dos_rom_name_1581);
    lib_free(dos_rom_name_2000);
    lib_free(dos_rom_name_4000);
}
