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
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

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
#include "cmdhd.h"

static char *dos_rom_name_1540 = NULL;
static char *dos_rom_name_1541 = NULL;
static char *dos_rom_name_1541ii = NULL;
static char *dos_rom_name_1570 = NULL;
static char *dos_rom_name_1571 = NULL;
static char *dos_rom_name_1581 = NULL;
static char *dos_rom_name_2000 = NULL;
static char *dos_rom_name_4000 = NULL;
static char *dos_rom_name_CMDHD = NULL;

static void set_drive_ram(unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];

    if (unit->type == DRIVE_TYPE_NONE) {
        return;
    }

    drivemem_init(unit);

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

static int set_dos_rom_name_CMDHD(const char *val, void *param)
{
    if (util_string_set(&dos_rom_name_CMDHD, val)) {
        return 0;
    }

    return iecrom_load_CMDHD();
}

static int set_drive_fixed(const char *val, void *param)
{
    char *end;
    int shift;
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];
    unsigned long long int work;
    char suffix;
    size_t len = 0;
    char *check_string = NULL;
    char last_char = 0;
    int i;
#if 0
    unsigned long long int calc;
    char text[50];
    int tp = 0;
#endif

    /* check if the given string is null */
    if (!util_check_null_string(val)) {

        /* duplicate the string so we can work on it */
        check_string = lib_strdup(val);

        /* remove any spaces */
        util_remove_spaces(check_string);

        /* check if last character is a k, m or g */
        len = strlen(check_string);
        if (!len) {
            lib_free(check_string);
            return -1;
        }
        last_char = util_toupper(check_string[len - 1]);
        if (last_char == 'K' || last_char == 'M' || last_char == 'G') {
            check_string[len - 1] = 0;
        }

        /* now check if the left over string is a number */
        len = strlen(check_string);
        for (i = 0; i < len; i++) {
            if (!isdigit((unsigned char)check_string[i])) {
                lib_free(check_string);
                return -1;
            }
        }

        /* string is a valid input, free the check string and proceed with the rest of the code */
        lib_free(check_string);
    }

    /* free existing ASCII value of resource */
    if (unit->fixed_size_text) {
        lib_free(unit->fixed_size_text);
    }


    /* turn whatever we are given into a number */
    errno = 0;
    work = strtoll(val, &end, 0);

    /* if it is good, and the remaining pointer is good, process any suffix */
    if (!errno && end) {
        /* skip any spaces */
        while (*end == ' ') {
            end++;
        }
        /* check for 3 suffixes */
        suffix = util_toupper(*end);
        if (suffix == 'K') {
            shift = 10;
        } else if (suffix == 'M') {
            shift = 20;
        } else if (suffix == 'G') {
            shift = 30;
        } else {
            shift = 0;
            suffix = 0;
        }
#if 0
        /* make my own ascii output since lib_msprintf("%"PRIu64"%c", work, suffix)
            doesn't work on windows at all */
        text[tp++] = 0;
        text[tp++] = suffix;

        /* generate ascii output in reverse order */
        calc = work;
        while (calc && tp < 48) {
            text[tp++] = (calc % 10) + '0';
            calc = calc / 10;
        }

        /* allocate and copy back in proper order */
        unit->fixed_size_text = lib_malloc(tp);
        for (i = 0; i < tp; i++) {
            unit->fixed_size_text[i] = text[tp - 1 - i];
        }
#else
        /* just copy what was passed */
        unit->fixed_size_text = lib_strdup(val);
#endif

        /* apply change */
        work = work << shift;
        /* make it terms of 512 byte units */
        unit->fixed_size = (unsigned int)(work >> 9);
        /* round up if need be */
        if (work & 511) {
            unit->fixed_size++;
        }
    } else {
        /* if any conversion errors happen, just make it 0 */
        unit->fixed_size = 0;
        /* generate a new ascii representation of the full value */
        unit->fixed_size_text = lib_msprintf("0");
    }

    /* tell the CMDHD, if there is one, the updated value */
    cmdhd_update_maxsize(unit->fixed_size, vice_ptr_to_uint(param) + 8);

    return 0;
}

static int set_drive_ram2(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];

    unit->drive_ram2_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram4(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];

    unit->drive_ram4_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram6(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];

    unit->drive_ram6_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_ram8(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];

    unit->drive_ram8_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static int set_drive_rama(int val, void *param)
{
    diskunit_context_t *unit = diskunit_context[vice_ptr_to_uint(param)];

    unit->drive_rama_enabled = val ? 1 : 0;
    set_drive_ram(vice_ptr_to_uint(param));
    return 0;
}

static const resource_string_t resources_string[] = {
    { "DosName1540", DRIVE_ROM1540_NAME, RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &dos_rom_name_1540, set_dos_rom_name_1540, NULL },
    { "DosName1541", DRIVE_ROM1541_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1541, set_dos_rom_name_1541, NULL },
    { "DosName1541ii", DRIVE_ROM1541II_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1541ii, set_dos_rom_name_1541ii, NULL },
    { "DosName1570", DRIVE_ROM1570_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1570, set_dos_rom_name_1570, NULL },
    { "DosName1571", DRIVE_ROM1571_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1571, set_dos_rom_name_1571, NULL },
    { "DosName1581", DRIVE_ROM1581_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_1581, set_dos_rom_name_1581, NULL },
    { "DosName2000", DRIVE_ROM2000_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_2000, set_dos_rom_name_2000, NULL },
    { "DosName4000", DRIVE_ROM4000_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_4000, set_dos_rom_name_4000, NULL },
    { "DosNameCMDHD", DRIVE_ROMCMDHD_NAME, RES_EVENT_NO, NULL,
      &dos_rom_name_CMDHD, set_dos_rom_name_CMDHD, NULL },
    RESOURCE_STRING_LIST_END
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
    RESOURCE_INT_LIST_END
};

static resource_string_t res_string[] = {
    { NULL, "8G", RES_EVENT_SAME, NULL,
      NULL, set_drive_fixed, NULL },
    RESOURCE_STRING_LIST_END
};

int iec_resources_init(void)
{
    int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        res_drive[0].name = lib_msprintf("Drive%iRAM2000", dnr + 8);
        res_drive[0].value_ptr = &(unit->drive_ram2_enabled);
        res_drive[0].param = uint_to_void_ptr(dnr);
        res_drive[1].name = lib_msprintf("Drive%iRAM4000", dnr + 8);
        res_drive[1].value_ptr = &(unit->drive_ram4_enabled);
        res_drive[1].param = uint_to_void_ptr(dnr);
        res_drive[2].name = lib_msprintf("Drive%iRAM6000", dnr + 8);
        res_drive[2].value_ptr = &(unit->drive_ram6_enabled);
        res_drive[2].param = uint_to_void_ptr(dnr);
        res_drive[3].name = lib_msprintf("Drive%iRAM8000", dnr + 8);
        res_drive[3].value_ptr = &(unit->drive_ram8_enabled);
        res_drive[3].param = uint_to_void_ptr(dnr);
        res_drive[4].name = lib_msprintf("Drive%iRAMA000", dnr + 8);
        res_drive[4].value_ptr = &(unit->drive_rama_enabled);
        res_drive[4].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free(res_drive[0].name);
        lib_free(res_drive[1].name);
        lib_free(res_drive[2].name);
        lib_free(res_drive[3].name);
        lib_free(res_drive[4].name);

        res_string[0].name = lib_msprintf("Drive%iFixedSize", dnr + 8);
        res_string[0].value_ptr = &(unit->fixed_size_text);
        res_string[0].param = uint_to_void_ptr(dnr);
        unit->fixed_size_text = NULL;
        unit->fixed_size = 0;

        if (resources_register_string(res_string) < 0) {
            return -1;
        }

        lib_free(res_string[0].name);

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
    lib_free(dos_rom_name_CMDHD);
}
