/*
 * userport_spt_joystick.c - Userport Stupid Pet Tricks joystick adapter emulation.
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

/* Userport Stupid Pet Tricks joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | VIC20 | NAME
------------------------------------
    D    |  13  |  D  |   D   | LEFT (PB0)
    E    |  12  |  E  |   E   | RIGHT (PB1)
    F    |  11  |  F  |   F   | UP (PB2)
    H    |  10  |  H  |   H   | DOWN (PB3)
    K    |   8  |  K  |   K   | FIRE (PB5)

Note that the userport +5VDC is NOT connected to the joystick +5VDC pin.

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
#include "userport_spt_joystick.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

/* SPT joystick bits */
#define SPT_JOYSTICK_LEFT_BIT    0
#define SPT_JOYSTICK_RIGHT_BIT   1
#define SPT_JOYSTICK_UP_BIT      2
#define SPT_JOYSTICK_DOWN_BIT    3
#define SPT_JOYSTICK_FIRE_BIT    5

/* SPT joystick bit values */
#define SPT_JOYSTICK_LEFT  (1 << SPT_JOYSTICK_LEFT_BIT)
#define SPT_JOYSTICK_RIGHT (1 << SPT_JOYSTICK_RIGHT_BIT)
#define SPT_JOYSTICK_UP    (1 << SPT_JOYSTICK_UP_BIT)
#define SPT_JOYSTICK_DOWN  (1 << SPT_JOYSTICK_DOWN_BIT)
#define SPT_JOYSTICK_FIRE  (1 << SPT_JOYSTICK_FIRE_BIT)

static int userport_spt_joystick_enabled = 0;

/* Some prototypes are needed */
static uint8_t userport_spt_joystick_read_pbx(uint8_t orig);
static void userport_spt_joystick_store_pbx(uint8_t value, int pulse);
static int userport_spt_joystick_write_snapshot_module(snapshot_t *s);
static int userport_spt_joystick_read_snapshot_module(snapshot_t *s);
static int userport_spt_joystick_enable(int value);

static userport_device_t userport_spt_joystick_device = {
    "Userport Stupid Pet Tricks joystick adapter", /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,          /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,         /* device is a joystick adapter */
    userport_spt_joystick_enable,                  /* enable function */
    userport_spt_joystick_read_pbx,                /* read pb0-pb7 function */
    userport_spt_joystick_store_pbx,               /* store pb0-pb7 function */
    NULL,                                          /* NO read pa2 pin function */
    NULL,                                          /* NO store pa2 pin function */
    NULL,                                          /* NO read pa3 pin function */
    NULL,                                          /* NO store pa3 pin function */
    0,                                             /* pc pin is NOT needed */
    NULL,                                          /* NO store sp1 pin function */
    NULL,                                          /* NO read sp1 pin function */
    NULL,                                          /* NO store sp2 pin function */
    NULL,                                          /* NO read sp2 pin function */
    NULL,                                          /* NO reset function */
    NULL,                                          /* NO powerup function */
    userport_spt_joystick_write_snapshot_module,   /* snapshot write function */
    userport_spt_joystick_read_snapshot_module     /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_spt_joystick_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_spt_joystick_enabled == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("%s is a joystick adapter, but joystick adapter %s is already active", userport_spt_joystick_device.name, joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, userport_spt_joystick_device.name);

        /* Enable 1 extra joystick port, without +5VDC support */
        joystick_adapter_set_ports(1, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_spt_joystick_enabled = val;
    return 0;
}

int userport_spt_joystick_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_SPT_JOYSTICK, &userport_spt_joystick_device);
}

/* ---------------------------------------------------------------------*/

static void userport_spt_joystick_store_pbx(uint8_t value, int pulse)
{
    uint8_t output = 0;

    /* convert from spt joystick pins to joyport pins */
    output = ((value & SPT_JOYSTICK_LEFT) >> SPT_JOYSTICK_LEFT_BIT) << JOYPORT_LEFT_BIT;
    output |= ((value & SPT_JOYSTICK_RIGHT) >> SPT_JOYSTICK_RIGHT_BIT) << JOYPORT_RIGHT_BIT;
    output |= ((value & SPT_JOYSTICK_DOWN) >> SPT_JOYSTICK_DOWN_BIT) << JOYPORT_DOWN_BIT;
    output |= ((value & SPT_JOYSTICK_UP) >> SPT_JOYSTICK_UP_BIT) << JOYPORT_UP_BIT;
    output |= ((value & SPT_JOYSTICK_FIRE) >> SPT_JOYSTICK_FIRE_BIT) << JOYPORT_FIRE_BIT;

    store_joyport_dig(JOYPORT_3, output, 0x1f);
}

static uint8_t userport_spt_joystick_read_pbx(uint8_t orig)
{
    uint8_t retval = 0;

    uint8_t portval = ~read_joyport_dig(JOYPORT_3);

    /* convert from joyport pins to spt joystick pins */
    retval |= ((portval & JOYPORT_LEFT) >> JOYPORT_LEFT_BIT) << SPT_JOYSTICK_LEFT_BIT;
    retval |= ((portval & JOYPORT_RIGHT) >> JOYPORT_RIGHT_BIT) << SPT_JOYSTICK_RIGHT_BIT;
    retval |= ((portval & JOYPORT_DOWN) >> JOYPORT_DOWN_BIT) << SPT_JOYSTICK_DOWN_BIT;
    retval |= ((portval & JOYPORT_UP) >> JOYPORT_UP_BIT) << SPT_JOYSTICK_UP_BIT;
    retval |= ((portval & JOYPORT_FIRE) >> JOYPORT_FIRE_BIT) << SPT_JOYSTICK_FIRE_BIT;

    return ~(retval);
}

/* ---------------------------------------------------------------------*/

static int userport_spt_joystick_write_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_write_module(s, JOYPORT_3);
}

static int userport_spt_joystick_read_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_read_module(s, JOYPORT_3);
}
