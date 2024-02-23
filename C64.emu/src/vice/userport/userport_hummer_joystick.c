/*
 * userport_hummer_joystick.c - Userport Hummer joystick adapter emulation.
 *
 * Written by
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

/* C64DTV HUMMER userport joystick adapter (C64/C128/C64DTV/CBM2/PET/PLUS4/VIC20)

C64/C128 | C64DTV | CBM2 | PET | PLUS4 | VIC20 | JOY | NOTES
------------------------------------------------------------
    C    |  USR0  |  14  |  C  |   B   |   C   |  1  | PB0 <- JOY UP
    D    |  USR1  |  13  |  D  |   K   |   D   |  2  | PB1 <- JOY DOWN
    E    |  USR2  |  12  |  E  |   4   |   E   |  3  | PB2 <- JOY LEFT
    F    |  USR3  |  11  |  F  |   5   |   F   |  4  | PB3 <- JOY RIGHT
    H    |  USR4  |  10  |  H  |   6   |   H   |  6  | PB4 <- JOY FIRE

The c64dtv uses 3.3v, so NO +5VDC pin is supported on the userport.

*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "resources.h"
#include "joyport.h"
#include "joystick.h"
#include "snapshot.h"
#include "userport.h"
#include "userport_hummer_joystick.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

static int userport_joy_hummer_enable = 0;

/* Some prototypes are needed */
static uint8_t userport_joystick_hummer_read_pbx(uint8_t orig);
static void userport_joystick_hummer_store_pbx(uint8_t value, int pulse);
static int userport_joystick_hummer_write_snapshot_module(snapshot_t *s);
static int userport_joystick_hummer_read_snapshot_module(snapshot_t *s);
static int userport_joystick_hummer_enable(int value);

static userport_device_t hummer_device = {
    "Hummer userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,           /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,          /* device is a joystick adapter */
    userport_joystick_hummer_enable,                /* enable function */
    userport_joystick_hummer_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_hummer_store_pbx,             /* store pb0-pb7 function */
    NULL,                                           /* NO read pa2 pin function */
    NULL,                                           /* NO store pa2 pin function */
    NULL,                                           /* NO read pa3 pin function */
    NULL,                                           /* NO store pa3 pin function */
    0,                                              /* pc pin is NOT needed */
    NULL,                                           /* NO store sp1 pin function */
    NULL,                                           /* NO read sp1 pin function */
    NULL,                                           /* NO store sp2 pin function */
    NULL,                                           /* NO read sp2 pin function */
    NULL,                                           /* NO reset function */
    NULL,                                           /* NO powerup function */
    userport_joystick_hummer_write_snapshot_module, /* snapshot write function */
    userport_joystick_hummer_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_joystick_hummer_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_hummer_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport HUMMER joystick adapter");

        /* Enable 1 extra joystick port, without +5VDC support */
        joystick_adapter_set_ports(1, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_hummer_enable = val;

    return 0;
}

int userport_joystick_hummer_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_HUMMER, &hummer_device);
}

/* ---------------------------------------------------------------------*/

static uint8_t userport_joystick_hummer_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint8_t jv3 = ~read_joyport_dig(JOYPORT_3);

    retval = (uint8_t)~(jv3 & 0x1f);

    return retval;
}

static void userport_joystick_hummer_store_pbx(uint8_t value, int pulse)
{
    uint8_t j1 = value & 0x1f;

    store_joyport_dig(JOYPORT_3, j1, 0x1f);
}

/* ---------------------------------------------------------------------*/

static int userport_joystick_hummer_write_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_write_module(s, JOYPORT_3);
}

static int userport_joystick_hummer_read_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_read_module(s, JOYPORT_3);
}
