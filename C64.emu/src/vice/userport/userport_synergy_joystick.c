/*
 * userport_synergy_joystick.c - Userport Synergy 3-joy joystick adapter emulation.
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

/* Synergy userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | I/O | NOTES
---------------------------------------------------
    C    |  14  |  C  |   B   |   C   |  I  | PB0 <- JOY1/2/3 UP
    D    |  13  |  D  |   K   |   D   |  I  | PB1 <- JOY1/2/3 DOWN
    E    |  12  |  E  |   4   |   E   |  I  | PB2 <- JOY1/2/3 LEFT
    F    |  11  |  F  |   5   |   F   |  I  | PB3 <- JOY1/2/3 RIGHT
    H    |  10  |  H  |   6   |   H   |  I  | PB4 <- JOY1/2/3 FIRE
    J    |   9  |  J  |   7   |   J   |  O  | PB5 -> JOY1 SELECT
    K    |   8  |  K  |   J   |   K   |  O  | PB6 -> JOY2 SELECT
    L    |   7  |  L  |   F   |   L   |  O  | PB7 -> JOY3 SELECT
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
#include "userport_synergy_joystick.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

static int userport_joy_synergy_enable = 0;
static int userport_joystick_synergy_select = 0;

/* Some prototypes are needed */
static uint8_t userport_joystick_synergy_read_pbx(uint8_t orig);
static void userport_joystick_synergy_store_pbx(uint8_t value, int pulse);
static int userport_joystick_synergy_write_snapshot_module(snapshot_t *s);
static int userport_joystick_synergy_read_snapshot_module(snapshot_t *s);
static int userport_joystick_synergy_enable(int value);

static userport_device_t synergy_device = {
    "Synergy userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,            /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,           /* device is a joystick adapter */
    userport_joystick_synergy_enable,                /* enable function */
    userport_joystick_synergy_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_synergy_store_pbx,             /* store pb0-pb7 function */
    NULL,                                            /* NO read pa2 pin function */
    NULL,                                            /* NO store pa2 pin function */
    NULL,                                            /* NO read pa3 pin function */
    NULL,                                            /* NO store pa3 pin function */
    0,                                               /* pc pin is NOT needed */
    NULL,                                            /* NO store sp1 pin function */
    NULL,                                            /* NO read sp1 pin function */
    NULL,                                            /* NO store sp2 pin function */
    NULL,                                            /* NO read sp2 pin function */
    NULL,                                            /* NO reset function */
    NULL,                                            /* NO powerup function */
    userport_joystick_synergy_write_snapshot_module, /* snapshot write function */
    userport_joystick_synergy_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_joystick_synergy_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_synergy_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport Synergy joystick adapter");

        /* Enable 3 extra joystick ports, without +5VDC support */
        joystick_adapter_set_ports(3, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_synergy_enable = val;

    return 0;
}

int userport_joystick_synergy_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_SYNERGY, &synergy_device);
}

/* ---------------------------------------------------------------------*/

static uint8_t userport_joystick_synergy_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint8_t jv3 = ~read_joyport_dig(JOYPORT_3);
    uint8_t jv4 = ~read_joyport_dig(JOYPORT_4);
    uint8_t jv5 = ~read_joyport_dig(JOYPORT_5);

    if (userport_joystick_synergy_select == 0) {
        retval = (uint8_t)~(jv3 & 0x1f);
    } else if (userport_joystick_synergy_select == 1) {
        retval = (uint8_t)~(jv4 & 0x1f);
    } else {
        retval = (uint8_t)~(jv5 & 0x1f);
    }

    return retval;
}

static void userport_joystick_synergy_store_pbx(uint8_t value, int pulse)
{
    uint8_t j1 = value & 0x1f;

    uint8_t j5_select = (value & 0x80) ? 1 : 0;
    uint8_t j4_select = (value & 0x40) ? 1 : 0;
    uint8_t j3_select = (value & 0x20) ? 1 : 0;

    /* sanity check, only 1 value should be 0 */
    if (j3_select + j4_select + j5_select == 2) {
        if (j3_select == 0) {
            userport_joystick_synergy_select = 0;
        } else if (j4_select == 0) {
            userport_joystick_synergy_select = 1;
        } else {
            userport_joystick_synergy_select = 2;
        }
    }

    if (userport_joystick_synergy_select == 0) {
        store_joyport_dig(JOYPORT_3, j1, 0x1f);
    } else if (userport_joystick_synergy_select == 1) {
        store_joyport_dig(JOYPORT_4, j1, 0x1f);
    } else {
        store_joyport_dig(JOYPORT_5, j1, 0x1f);
    }
}

/* ---------------------------------------------------------------------*/

/* UP_JOY_SYNERGY snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | select | joyport select
 */

static const char synergy_module_name[] = "UPJOYSYNERGY";
#define SYNERGY_VER_MAJOR   0
#define SYNERGY_VER_MINOR   1

static int userport_joystick_synergy_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, synergy_module_name, SYNERGY_VER_MAJOR, SYNERGY_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)userport_joystick_synergy_select) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0
        || joyport_snapshot_write_module(s, JOYPORT_5) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_synergy_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, synergy_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SYNERGY_VER_MAJOR, SYNERGY_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &userport_joystick_synergy_select) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0
        || joyport_snapshot_read_module(s, JOYPORT_5) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
