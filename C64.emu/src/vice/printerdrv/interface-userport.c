/*
 * interface-userport.c - Userport printer interface.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "driver-select.h"
#include "interface-userport.h"
#include "output-select.h"
#include "printer.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "userport.h"

/* 
C64/C128 | CBM2 | PET | VIC20 | CENTRONICS  | NOTES
---------------------------------------------------
    B    |  6   |  B  |   B   |     11      | FLAG2 <- BUSY
    C    | 14   |  C  |   C   |      2      | PB0 -> DATA0
    D    | 13   |  D  |   D   |      3      | PB1 -> DATA1
    E    | 12   |  E  |   E   |      4      | PB2 -> DATA2
    F    | 11   |  F  |   F   |      5      | PB3 -> DATA3
    H    | 10   |  H  |   H   |      6      | PB4 -> DATA4
    J    |  9   |  J  |   J   |      7      | PB5 -> DATA5
    K    |  8   |  K  |   K   |      8      | PB6 -> DATA6
    L    |  7   |  L  |   L   |      9      | PB7 -> DATA7
    M    |  2   |  M  |   M   |      1      | PA2 -> STROBE
*/

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_printer_store_pbx(BYTE b);
static void userport_printer_store_pa2(BYTE s);
static int userport_printer_write_snapshot_module(snapshot_t *s);
static int userport_printer_read_snapshot_module(snapshot_t *s);

static userport_device_t printer_device = {
    USERPORT_DEVICE_PRINTER,
    "Userport printer",
    IDGS_USERPORT_PRINTER,
    NULL, /* NO pbx read */
    userport_printer_store_pbx,
    NULL, /* NO pa2 read */
    userport_printer_store_pa2,
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "PrinterUserport",
    0xff,
    0xff, /* validity mask doesn't change */
    0,
    0
};

static userport_snapshot_t printer_snapshot = {
    USERPORT_DEVICE_PRINTER,
    userport_printer_write_snapshot_module,
    userport_printer_read_snapshot_module
};

static userport_device_list_t *userport_printer_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int userport_printer_enabled = 0;

#define USERPORT_OUTPUT         (NUM_OUTPUT_SELECT - 1)

static int set_up_enabled(int val, void *param)
{
    int newval = val ? 1 : 0;

    if (newval && !userport_printer_enabled) {
        /* Switch printer on.  */
        if (driver_select_open(USERPORT_OUTPUT, 4) >= 0) {
            userport_printer_list_item = userport_device_register(&printer_device);
            if (userport_printer_list_item == NULL) {
                return -1;
            }
            userport_printer_enabled = 1;
        }
    }
    if (userport_printer_enabled && !newval) {
        userport_device_unregister(userport_printer_list_item);
        userport_printer_list_item = NULL;
        driver_select_close(USERPORT_OUTPUT, 4);
        userport_printer_enabled = 0;
    }

    return 0;
}

static const resource_int_t resources_int[] = {
    { "PrinterUserport", 0, RES_EVENT_STRICT, (resource_value_t)0,
      (void *)&userport_printer_enabled, set_up_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int interface_userport_init_resources(void)
{
    userport_snapshot_register(&printer_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
    { "-pruser", SET_RESOURCE, 0,
      NULL, NULL, "PrinterUserport", (resource_value_t) 1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_PRINTER,
      NULL, NULL },
    { "+pruser", SET_RESOURCE, 0,
      NULL, NULL, "PrinterUserport", (resource_value_t) 0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_PRINTER,
      NULL, NULL },
    CMDLINE_LIST_END
};

int interface_userport_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static BYTE value; /* userport value */
static BYTE strobe;

static void userport_printer_store_pbx(BYTE b)
{
    value = b;
}

static void userport_printer_store_pa2(BYTE s)
{
    if (userport_printer_enabled && strobe && !s) {     /* hi->lo on strobe */
        driver_select_putc(USERPORT_OUTPUT, 4, (BYTE)value);

        set_userport_flag(1); /* signal lo->hi */
        set_userport_flag(0); /* signal hi->lo */
    }
    strobe = s;
}

/* ------------------------------------------------------------------------- */

/* USERPORT_PRINTER snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | value  | return value
   BYTE  | strobe | strobe flag
 */

static char snap_module_name[] = "USERPORT_PRINTER";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int userport_printer_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, value) < 0
        || SMW_B(m, strobe) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_printer_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_up_enabled(1, NULL);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &value) < 0
        || SMR_B(m, &strobe) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
