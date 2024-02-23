/*
 * userport_petscii_snespad.c - Userport Petscii SNES pad emulation.
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

/* - PETSCII SNES PAD (C64/C128/PET/VIC20)

C64/C128 |   PET   |  VIC20  | CBM2 | SNES PAD | I/O
-------------------------------------------------------
 F (PB3) | F (PA3) | F (PB3) |  11  |   CLOCK  |  O
 J (PB5) | J (PA5) | J (PB5) |   9  |   LATCH  |  O
 K (PB6) | K (PA6) | K (PB6) |   8  |   DATA   |  I
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
#include "snespad.h"
#include "userport.h"
#include "userport_petscii_snespad.h"
#include "machine.h"
#include "uiapi.h"

#include "log.h"

static int userport_snespad_enabled = 0;

static uint8_t counter = 0;

static uint8_t clock_line = 0;
static uint8_t latch_line = 0;

/* Some prototypes are needed */
static uint8_t userport_snespad_read_pbx(uint8_t orig);
static void userport_snespad_store_pbx(uint8_t value, int pulse);
static int userport_petscii_write_snapshot_module(snapshot_t *s);
static int userport_petscii_read_snapshot_module(snapshot_t *s);
static int userport_petscii_enable(int val);
static void userport_petscii_powerup(void);

static userport_device_t userport_snespad_device = {
    "Userport Petscii SNES pad",               /* device name */
    JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES, /* this is a joystick adapter */
    USERPORT_DEVICE_TYPE_JOYSTICK_ADAPTER,     /* device is a joystick adapter */
    userport_petscii_enable,                   /* enable function */
    userport_snespad_read_pbx,                 /* read pb0-pb7 function */
    userport_snespad_store_pbx,                /* store pb0-pb7 function */
    NULL,                                      /* NO read pa2 pin function */
    NULL,                                      /* NO store pa2 pin function */
    NULL,                                      /* NO read pa3 pin function */
    NULL,                                      /* NO store pa3 pin function */
    0,                                         /* pc pin is NOT needed */
    NULL,                                      /* NO store sp1 pin function */
    NULL,                                      /* NO read sp1 pin function */
    NULL,                                      /* NO store sp2 pin function */
    NULL,                                      /* NO read sp2 pin function */
    NULL,                                      /* NO reset function */
    userport_petscii_powerup,                  /* powerup function */
    userport_petscii_write_snapshot_module,    /* snapshot write function */
    userport_petscii_read_snapshot_module      /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_petscii_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_snespad_enabled == val) {
        return 0;
    }

    if (val) {
        /* check if a different joystick adapter is already active */
        if (joystick_adapter_get_id()) {
            ui_error("%s is a joystick adapter, but joystick adapter %s is already active", userport_snespad_device.name, joystick_adapter_get_name());
            return -1;
        }
        counter = 0;
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_USERPORT_PETSCII_SNES, userport_snespad_device.name);

        /* Enable 1 extra joystick port, without +5VDC support */
        joystick_adapter_set_ports(1, 0);
        joystick_set_snes_mapping(JOYPORT_3);
    } else {
        joystick_adapter_deactivate();
        joyport_clear_mapping(JOYPORT_3);
    }

    userport_snespad_enabled = val;
    return 0;
}

int userport_petscii_snespad_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_PETSCII_SNESPAD, &userport_snespad_device);
}

/* ---------------------------------------------------------------------*/

static void userport_petscii_powerup(void)
{
    counter = 0;
}

static void userport_snespad_store_pbx(uint8_t value, int pulse)
{
    uint8_t new_clock = 0;
    uint8_t new_latch = 0;

    new_clock = (value & 0x08) >> 3;
    new_latch = (value & 0x20) >> 4;

    if (latch_line && !new_latch) {
        counter = 0;
    }

    if (clock_line && !new_clock) {
        if (counter != SNESPAD_EOS) {
            counter++;
        }
    }

    latch_line = new_latch;
    clock_line = new_clock;
}

static uint8_t userport_snespad_read_pbx(uint8_t orig)
{
    uint8_t retval;
    uint16_t portval = get_joystick_value(JOYPORT_3);

    switch (counter) {
        case SNESPAD_BUTTON_A:
            retval = (uint8_t)((portval & 0x10) >> 4);
            break;
        case SNESPAD_BUTTON_B:
            retval = (uint8_t)((portval & 0x20) >> 5);
            break;
        case SNESPAD_BUTTON_X:
            retval = (uint8_t)((portval & 0x40) >> 6);
            break;
        case SNESPAD_BUTTON_Y:
            retval = (uint8_t)((portval & 0x80) >> 7);
            break;
        case SNESPAD_BUMPER_LEFT:
            retval = (uint8_t)((portval & 0x100) >> 8);
            break;
        case SNESPAD_BUMPER_RIGHT:
            retval = (uint8_t)((portval & 0x200) >> 9);
            break;
        case SNESPAD_BUTTON_SELECT:
            retval = (uint8_t)((portval & 0x400) >> 10);
            break;
        case SNESPAD_BUTTON_START:
            retval = (uint8_t)((portval & 0x800) >> 11);
            break;
        case SNESPAD_UP:
            retval = (uint8_t)(portval & 1);
            break;
        case SNESPAD_DOWN:
            retval = (uint8_t)((portval & 2) >> 1);
            break;
        case SNESPAD_LEFT:
            retval = (uint8_t)((portval & 4) >> 2);
            break;
        case SNESPAD_RIGHT:
            retval = (uint8_t)((portval & 8) >> 3);
            break;
        case SNESPAD_BIT_12_1:
        case SNESPAD_BIT_13_1:
        case SNESPAD_BIT_14_1:
        case SNESPAD_BIT_15_1:
            retval = 1;
            break;
        case SNESPAD_EOS:
            retval = 1;
            break;
        default:
            retval = 0;
    }

    retval <<= 6;

    return (~retval);
}

/* ---------------------------------------------------------------------*/

/* USERPORT_PETSCII_SNESPAD snapshot module format:

   type  | name    | description
   -----------------------------
   BYTE  | COUNTER | current count
   BYTE  | CLOCK   | clock line state
   BYTE  | LATCH   | latch line state
 */

static const char snap_module_name[] = "UPPETSCII";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_petscii_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, counter) < 0)
        || (SMW_B(m, clock_line) < 0)
        || (SMW_B(m, latch_line) < 0)) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_petscii_read_snapshot_module(snapshot_t *s)
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

    if (0
        || (SMR_B(m, &counter) < 0)
        || (SMR_B(m, &clock_line) < 0)
        || (SMR_B(m, &latch_line) < 0)) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
