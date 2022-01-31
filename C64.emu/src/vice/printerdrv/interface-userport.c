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
#include "types.h"
#include "joyport.h"
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
static void userport_printer_store_pbx(uint8_t b, int pulse);
static void userport_printer_store_pa2(uint8_t s);
static int userport_printer_write_snapshot_module(snapshot_t *s);
static int userport_printer_read_snapshot_module(snapshot_t *s);
static int userport_printer_enable(int val);

static userport_device_t printer_device = {
    "Userport printer",                     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,               /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_PRINTER,           /* device is a printer */
    userport_printer_enable,                /* enable function */
    NULL,                                   /* NO read pb0-pb7 function */
    userport_printer_store_pbx,             /* store pb0-pb7 function */
    NULL,                                   /* NO read pa2 pin function */
    userport_printer_store_pa2,             /* store pa2 pin function */
    NULL,                                   /* NO read pa3 pin function */
    NULL,                                   /* NO store pa3 pin function */
    0,                                      /* pc pin is NOT needed */
    NULL,                                   /* NO store sp1 pin function */
    NULL,                                   /* NO read sp1 pin function */
    NULL,                                   /* NO store sp2 pin function */
    NULL,                                   /* NO read sp2 pin function */
    NULL,                                   /* NO reset function */
    NULL,                                   /* NO power toggle function */
    userport_printer_write_snapshot_module, /* snapshot write function */
    userport_printer_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_printer_enabled = 0;

#define USERPORT_OUTPUT         (NUM_OUTPUT_SELECT - 1)

static int userport_printer_enable(int val)
{
    int newval = val ? 1 : 0;

    if (newval && !userport_printer_enabled) {
        /* Switch printer on.  */
        if (driver_select_open(USERPORT_OUTPUT, 4) >= 0) {
            userport_printer_enabled = 1;
        }
    }
    if (userport_printer_enabled && !newval) {
        driver_select_close(USERPORT_OUTPUT, 4);
        userport_printer_enabled = 0;
    }

    return 0;
}

int interface_userport_init_resources(void)
{
    return userport_device_register(USERPORT_DEVICE_PRINTER, &printer_device);
}

/* ------------------------------------------------------------------------- */

static uint8_t value; /* userport value */
static uint8_t strobe;

static void userport_printer_store_pbx(uint8_t b, int pulse)
{
    value = b;
}

static void userport_printer_store_pa2(uint8_t s)
{
    if (userport_printer_enabled && strobe && !s) {     /* hi->lo on strobe */
        driver_select_putc(USERPORT_OUTPUT, 4, (uint8_t)value);

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

static char snap_module_name[] = "UPPRINTER";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_printer_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, value) < 0
            || SMW_B(m, strobe) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_printer_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B(m, &value) < 0
            || SMR_B(m, &strobe) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
