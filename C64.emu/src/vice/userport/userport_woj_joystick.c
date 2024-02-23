/*
 * userport_woj_joystick.c - Userport Wheel Of Joysticks 8-joy joystick adapter emulation.
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

/* WOJ userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | I/O | NOTES
---------------------------------------------------
    C    |  14  |  C  |   B   |   C   |  I  | PB0 <- JOY1/2/3 UP
    D    |  13  |  D  |   K   |   D   |  I  | PB1 <- JOY1/2/3 DOWN
    E    |  12  |  E  |   4   |   E   |  I  | PB2 <- JOY1/2/3 LEFT
    F    |  11  |  F  |   5   |   F   |  I  | PB3 <- JOY1/2/3 RIGHT
    H    |  10  |  H  |   6   |   H   |  I  | PB4 <- JOY1/2/3 FIRE
    J    |   9  |  J  |   7   |   J   |  O  | PB5 -> JOY SELECT BIT 0
    K    |   8  |  K  |   J   |   K   |  O  | PB6 -> JOY SELECT BIT 1
    L    |   7  |  L  |   F   |   L   |  O  | PB7 -> JOY SELECT BIT 2
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
#include "userport_woj_joystick.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

static int userport_joy_woj_enable = 0;
static int userport_joystick_woj_select = 0;

/* Some prototypes are needed */
static uint8_t userport_joystick_woj_read_pbx(uint8_t orig);
static void userport_joystick_woj_store_pbx(uint8_t value, int pulse);
static int userport_joystick_woj_write_snapshot_module(snapshot_t *s);
static int userport_joystick_woj_read_snapshot_module(snapshot_t *s);
static int userport_joystick_woj_enable(int value);

static userport_device_t woj_device = {
    "WOJ userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,        /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,       /* device is a joystick adapter */
    userport_joystick_woj_enable,                /* enable function */
    userport_joystick_woj_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_woj_store_pbx,             /* store pb0-pb7 function */
    NULL,                                        /* NO read pa2 pin function */
    NULL,                                        /* NO store pa2 pin function */
    NULL,                                        /* NO read pa3 pin function */
    NULL,                                        /* NO store pa3 pin function */
    0,                                           /* pc pin is NOT needed */
    NULL,                                        /* NO store sp1 pin function */
    NULL,                                        /* NO read sp1 pin function */
    NULL,                                        /* NO store sp2 pin function */
    NULL,                                        /* NO read sp2 pin function */
    NULL,                                        /* NO reset function */
    NULL,                                        /* NO powerup function */
    userport_joystick_woj_write_snapshot_module, /* snapshot write function */
    userport_joystick_woj_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_joystick_woj_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_woj_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport WOJ joystick adapter");

        /* Enable 8 extra joystick ports, without +5VDC support */
        joystick_adapter_set_ports(8, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_woj_enable = val;

    return 0;
}

int userport_joystick_woj_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_WOJ, &woj_device);
}

/* ---------------------------------------------------------------------*/

static uint8_t userport_joystick_woj_read_pbx(uint8_t orig)
{
    return ~read_joyport_dig(JOYPORT_3 + userport_joystick_woj_select);
}

static void userport_joystick_woj_store_pbx(uint8_t value, int pulse)
{
    uint8_t j1 = value & 0x1f;

    userport_joystick_woj_select = ((value & 0xE0) >> 5);

    store_joyport_dig(JOYPORT_3 + userport_joystick_woj_select, j1, 0x1f);
}

/* ---------------------------------------------------------------------*/

/* UP_JOY_WOJ snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | select | joyport select
 */

static const char woj_module_name[] = "UPJOYWOJ";
#define WOJ_VER_MAJOR   0
#define WOJ_VER_MINOR   1

static int userport_joystick_woj_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, woj_module_name, WOJ_VER_MAJOR, WOJ_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)userport_joystick_woj_select) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0
        || joyport_snapshot_write_module(s, JOYPORT_5) < 0
        || joyport_snapshot_write_module(s, JOYPORT_6) < 0
        || joyport_snapshot_write_module(s, JOYPORT_7) < 0
        || joyport_snapshot_write_module(s, JOYPORT_8) < 0
        || joyport_snapshot_write_module(s, JOYPORT_9) < 0
        || joyport_snapshot_write_module(s, JOYPORT_10) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_woj_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, woj_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, WOJ_VER_MAJOR, WOJ_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &userport_joystick_woj_select) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0
        || joyport_snapshot_read_module(s, JOYPORT_5) < 0
        || joyport_snapshot_read_module(s, JOYPORT_6) < 0
        || joyport_snapshot_read_module(s, JOYPORT_7) < 0
        || joyport_snapshot_read_module(s, JOYPORT_8) < 0
        || joyport_snapshot_read_module(s, JOYPORT_9) < 0
        || joyport_snapshot_read_module(s, JOYPORT_10) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
