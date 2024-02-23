/*
 * userport_joystick.c - Generic userport joystick emulation.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "userport.h"
#include "userport_joystick.h"
#include "uiapi.h"

/* CGA userport joystick adapter (C64/C128/CBM2/PET/VIC20)

C64/C128 | CBM2 | PET | VIC20 | 74LS257 | JOY1 | JOY2 | NOTES
-------------------------------------------------------------
    C    |  14  |  C  |   C   |    4    |  -   |  -   | PB0 <- JOYx UP OUTPUT
    D    |  13  |  D  |   D   |    7    |  -   |  -   | PB1 <- JOYx DOWN OUTPUT
    E    |  12  |  E  |   E   |    9    |  -   |  -   | PB2 <- JOYx LEFT OUTPUT
    F    |  11  |  F  |   F   |   12    |  -   |  -   | PB3 <- JOYx RIGHT OUTPUT
    H    |  10  |  H  |   H   |    -    |  -   |  6   | PB4 <- JOY2 FIRE
    J    |   9  |  J  |   J   |    -    |  6   |  -   | PB5 <- JOY1 FIRE
    L    |   7  |  L  |   L   |    1    |  -   |  -   | PB7 -> INPUT SELECT
    -    |   -  |  -  |   -   |    2    |  1   |  -   | JOYx UP INPUT 0 <- JOY1 UP
    -    |   -  |  -  |   -   |    5    |  2   |  -   | JOYx DOWN INPUT 0 <- JOY1 DOWN
    -    |   -  |  -  |   -   |   11    |  3   |  -   | JOYx LEFT INPUT 0 <- JOY1 LEFT
    -    |   -  |  -  |   -   |   14    |  4   |  -   | JOYx RIGHT INPUT 0 <- JOY1 RIGHT
    -    |   -  |  -  |   -   |    3    |  -   |  1   | JOYx UP INPUT 1 <- JOY2 UP
    -    |   -  |  -  |   -   |    6    |  -   |  2   | JOYx DOWN INPUT 1 <- JOY2 DOWN
    -    |   -  |  -  |   -   |   10    |  -   |  3   | JOYx LEFT INPUT 1 <- JOY2 LEFT
    -    |   -  |  -  |   -   |   13    |  -   |  4   | JOYx RIGHT INPUT 1 <- JOY2 RIGHT
*/

/* PET userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | JOY1 | JOY2 | NOTES
-----------------------------------------------------------
    C    |  14  |  C  |   B   |   C   |  1   |  -   | PB0 <- JOY1 UP
    D    |  13  |  D  |   K   |   D   |  2   |  -   | PB1 <- JOY1 DOWN
    E    |  12  |  E  |   4   |   E   |  3   |  -   | PB2 <- JOY1 LEFT
    F    |  11  |  F  |   5   |   F   |  4   |  -   | PB3 <- JOY1 RIGHT
    H    |  10  |  H  |   6   |   H   |  -   |  1   | PB4 <- JOY2 UP
    J    |   9  |  J  |   7   |   J   |  -   |  2   | PB5 <- JOY2 DOWN
    K    |   8  |  K  |   J   |   K   |  -   |  3   | PB6 <- JOY2 LEFT
    L    |   7  |  L  |   F   |   L   |  -   |  4   | PB7 <-> JOY2 RIGHT

JOY1 FIRE is hooked up to pull down both JOY1 LEFT & JOY1 RIGHT
JOY2 FIRE is hooked up to pull down both JOY2 LEFT & JOY2 RIGHT
*/

/* VIC20 OEM userport joystick adapter (C64/C128/CBM2/PET/PLUS4/VIC20)

C64/C128 | CBM2 | PET | PLUS4 | VIC20 | JOY | NOTES
---------------------------------------------------
    F    |  11  |  F  |   5   |   F   |  6  | PB3 <- JOY FIRE
    H    |  10  |  H  |   6   |   H   |  4  | PB4 <- JOY RIGHT
    J    |   9  |  J  |   7   |   J   |  3  | PB5 <- JOY LEFT
    K    |   8  |  K  |   J   |   K   |  2  | PB6 <- JOY DOWN
    L    |   7  |  L  |   F   |   L   |  1  | PB7 <- JOY UP
*/

/* ------------------------------------------------------------------------- */

static int userport_joy_cga_enable = 0;
static int userport_joy_pet_enable = 0;
static int userport_joy_oem_enable = 0;

static int userport_joystick_cga_select = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t userport_joystick_cga_read_pbx(uint8_t orig);
static void userport_joystick_cga_store_pbx(uint8_t value, int pulse);
static int userport_joystick_cga_write_snapshot_module(snapshot_t *s);
static int userport_joystick_cga_read_snapshot_module(snapshot_t *s);
static int userport_joystick_cga_enable(int value);

static uint8_t userport_joystick_pet_read_pbx(uint8_t orig);
static void userport_joystick_pet_store_pbx(uint8_t value, int pulse);
static int userport_joystick_pet_write_snapshot_module(snapshot_t *s);
static int userport_joystick_pet_read_snapshot_module(snapshot_t *s);
static int userport_joystick_pet_enable(int value);

static uint8_t userport_joystick_oem_read_pbx(uint8_t orig);
static void userport_joystick_oem_store_pbx(uint8_t value, int pulse);
static int userport_joystick_oem_write_snapshot_module(snapshot_t *s);
static int userport_joystick_oem_read_snapshot_module(snapshot_t *s);
static int userport_joystick_oem_enable(int value);

static userport_device_t cga_device = {
    "CGA userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,        /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,       /* device is a joystick adapter */
    userport_joystick_cga_enable,                /* enable function */
    userport_joystick_cga_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_cga_store_pbx,             /* store pb0-pb7 function */
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
    userport_joystick_cga_write_snapshot_module, /* snapshot write function */
    userport_joystick_cga_read_snapshot_module   /* snapshot read function */
};

static userport_device_t pet_device = {
    "PET userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,        /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,       /* device is a joystick adapter */
    userport_joystick_pet_enable,                /* enable function */
    userport_joystick_pet_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_pet_store_pbx,             /* store pb0-pb7 function */
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
    userport_joystick_pet_write_snapshot_module, /* snapshot write function */
    userport_joystick_pet_read_snapshot_module   /* snapshot read function */
};

static userport_device_t oem_device = {
    "OEM userport joy adapter",                  /* device name */
    JOYSTICK_ADAPTER_ID_GENERIC_USERPORT,        /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,       /* device is a joystick adapter */
    userport_joystick_oem_enable,                /* enable function */
    userport_joystick_oem_read_pbx,              /* read pb0-pb7 function */
    userport_joystick_oem_store_pbx,             /* store pb0-pb7 function */
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
    userport_joystick_oem_write_snapshot_module, /* snapshot write function */
    userport_joystick_oem_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_joystick_pet_output_check(int port, uint8_t output_bits)
{
    if (output_bits & 0x10) {   /* no output on 'fire' pin */
        return 0;
    }
    return 1;
}

static int userport_joystick_cga_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_cga_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport CGA joystick adapter");

        /* Enable 2 extra joystick ports, without +5VDC support */
        joystick_adapter_set_ports(2, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_cga_enable = val;

    return 0;
}

static int userport_joystick_pet_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_pet_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport PET joystick adapter");
        joystick_adapter_set_output_check_function(userport_joystick_pet_output_check);

        /* Enable 2 extra joystick ports, without +5VDC support */
        joystick_adapter_set_ports(2, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_pet_enable = val;

    return 0;
}

static int userport_joystick_oem_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_joy_oem_enable == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("Joystick adapter %s is already active", joystick_adapter_get_name());
            return -1;
        }
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_GENERIC_USERPORT, "Userport OEM joystick adapter");

        /* Enable 1 extra joystick port, without +5VDC support */
        joystick_adapter_set_ports(1, 0);
    } else {
        joystick_adapter_deactivate();
    }

    userport_joy_oem_enable = val;

    return 0;
}

int userport_joystick_cga_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_CGA, &cga_device);
}

int userport_joystick_pet_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_PET, &pet_device);
}

int userport_joystick_oem_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_JOYSTICK_OEM, &oem_device);
}

/* ------------------------------------------------------------------------- */

static uint8_t userport_joystick_cga_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint8_t jv3 = ~read_joyport_dig(JOYPORT_3);
    uint8_t jv4 = ~read_joyport_dig(JOYPORT_4);

    if (userport_joystick_cga_select) {
        retval = (uint8_t)~((jv4 & 0xf) | (jv3 & 0x10) | ((jv4 & 0x10) << 1));
    } else {
        retval = (uint8_t)~((jv3 & 0xf) | (jv3 & 0x10) | ((jv4 & 0x10) << 1));
    }
    return retval;
}

static void userport_joystick_cga_store_pbx(uint8_t value, int pulse)
{
    userport_joystick_cga_select = (value & 0x80) ? 0 : 1;
}

/* ------------------------------------------------------------------------- */

static uint8_t userport_joystick_pet_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint8_t jv3 = ~read_joyport_dig(JOYPORT_3);
    uint8_t jv4 = ~read_joyport_dig(JOYPORT_4);

    retval = ((jv3 & 0xf) | ((jv4 & 0xf) << 4));
    retval |= (jv3 & 0x10) ? 3 : 0;
    retval |= (jv4 & 0x10) ? 0x30 : 0;
    retval = (uint8_t)~retval;

    return retval;
}

static void userport_joystick_pet_store_pbx(uint8_t value, int pulse)
{
    uint8_t j1 = value & 0xf;
    uint8_t j2 = (value & 0xf0) >> 4;

    store_joyport_dig(JOYPORT_3, j1, 0xf);
    store_joyport_dig(JOYPORT_4, j2, 0xf);
}

/* ------------------------------------------------------------------------- */

static uint8_t userport_joystick_oem_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint8_t jv3 = ~read_joyport_dig(JOYPORT_3);

    retval = ((jv3 & 1) << 7);
    retval |= ((jv3 & 2) << 5);
    retval |= ((jv3 & 4) << 3);
    retval |= ((jv3 & 8) << 1);
    retval |= ((jv3 & 16) >> 1);
    retval = (uint8_t)~retval;

    return retval;
}

static void userport_joystick_oem_store_pbx(uint8_t value, int pulse)
{
    uint8_t j1 = (value & 8) << 1;

    j1 |= (value & 0x10) >> 1;
    j1 |= (value & 0x20) >> 3;
    j1 |= (value & 0x40) >> 5;
    j1 |= (value & 0x80) >> 7;

    store_joyport_dig(JOYPORT_3, j1, 0x1f);
}

/* ------------------------------------------------------------------------- */

/* UP_JOY_CGA snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | select | joyport select
 */

static const char cga_module_name[] = "UPJOYCGA";
#define CGA_VER_MAJOR   0
#define CGA_VER_MINOR   1

static int userport_joystick_cga_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, cga_module_name, CGA_VER_MAJOR, CGA_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)userport_joystick_cga_select) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_cga_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, cga_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, CGA_VER_MAJOR, CGA_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &userport_joystick_cga_select) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

/* ------------------------------------------------------------------------- */

static int userport_joystick_pet_write_snapshot_module(snapshot_t *s)
{
    if (0
        || joyport_snapshot_write_module(s, JOYPORT_3) < 0
        || joyport_snapshot_write_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

static int userport_joystick_pet_read_snapshot_module(snapshot_t *s)
{
    if (0
        || joyport_snapshot_read_module(s, JOYPORT_3) < 0
        || joyport_snapshot_read_module(s, JOYPORT_4) < 0) {
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static int userport_joystick_oem_write_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_write_module(s, JOYPORT_3);
}

static int userport_joystick_oem_read_snapshot_module(snapshot_t *s)
{
    return joyport_snapshot_read_module(s, JOYPORT_3);
}
