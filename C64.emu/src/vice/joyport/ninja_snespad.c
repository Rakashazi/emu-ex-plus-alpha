/*
 * ninja_snespad.c - Ninja SNES PAD emulation.
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

#include "joyport.h"
#include "joystick.h"
#include "ninja_snespad.h"
#include "resources.h"
#include "snapshot.h"
#include "snespad.h"

#include "log.h"

/* Control port <--> SNES PAD connections:

   cport |   SNES PAD   | I/O
   -------------------------
     1   |   DATA PAD 1 |  I
     2   |   DATA PAD 2 |  I
     3   |   DATA PAD 3 |  I
     4   |     CLOCK    |  O
     6   |     LATCH    |  O
     7   |     +5VDC    | Power
     8   |     GND      | Ground

   Works on:
   - native joystick ports (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
 */

/* Since there are currently no games that use more than 3 snes pads,
   only 3 snes pads are emulated, this is easily extendable in the
   future.
 */

static int snespad_enabled = 0;

static uint8_t counter = 0;

static uint8_t clock_line = 0;
static uint8_t latch_line = 0;

/* ------------------------------------------------------------------------- */

static joyport_t joyport_snespad_device;

static int joyport_snespad_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == snespad_enabled) {
        return 0;
    }

    if (new_state) {
        /* enabled, activate joystick adapter and set amount of ports to 3 */
        joystick_adapter_activate(JOYSTICK_ADAPTER_ID_NINJA_SNES, joyport_snespad_device.name);
        counter = 0;

        /* enable 3 extra ports, since this is a snes adapter no +5VDC support */
        joystick_adapter_set_ports(3, 0);

        /* enabled, enable snes mapping on ports 3, 4 and 5 */
        joystick_set_snes_mapping(JOYPORT_3);
        joystick_set_snes_mapping(JOYPORT_4);
        joystick_set_snes_mapping(JOYPORT_5);
    } else {
        /* disabled, disable snes mapping on ports 3, 4 and 5 */
        joyport_clear_mapping(JOYPORT_3);
        joyport_clear_mapping(JOYPORT_4);
        joyport_clear_mapping(JOYPORT_5);

        /* disabled, deactivate joystick adapter */
        joystick_adapter_deactivate();
    }

    snespad_enabled = new_state;

    return 0;
}

static uint8_t snespad_read(int port)
{
    uint8_t retval;
    uint16_t joyval1 = get_joystick_value(JOYPORT_3);
    uint16_t joyval2 = get_joystick_value(JOYPORT_4);
    uint16_t joyval3 = get_joystick_value(JOYPORT_5);

    switch (counter) {
        case SNESPAD_BUTTON_A:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_A_BIT, JOYPORT_UP_BIT);      /* output snespad 1 button a on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_A_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 button a on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_A_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 button a on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_B:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_B_BIT, JOYPORT_UP_BIT);      /* output snespad 1 button b on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_B_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 button b on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_B_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 button b on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_X:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_X_BIT, JOYPORT_UP_BIT);      /* output snespad 1 button x on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_X_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 button x on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_X_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 button x on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_Y:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_Y_BIT, JOYPORT_UP_BIT);      /* output snespad 1 button y on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_Y_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 button y on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_Y_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 button y on joyport 'left' pin */
            break;
        case SNESPAD_BUMPER_LEFT:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_LEFT_BUMBER_BIT, JOYPORT_UP_BIT);      /* output snespad 1 left bumper on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_LEFT_BUMBER_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 left bumper on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_LEFT_BUMBER_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 left bumper on joyport 'left' pin */
            break;
        case SNESPAD_BUMPER_RIGHT:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_RIGHT_BUMBER_BIT, JOYPORT_UP_BIT);      /* output snespad 1 right bumper on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_RIGHT_BUMBER_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 right bumper on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_RIGHT_BUMBER_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 right bumper on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_SELECT:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_SELECT_BIT, JOYPORT_UP_BIT);      /* output snespad 1 select button on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_SELECT_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 select button on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_SELECT_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 select button on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_START:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_BUTTON_START_BIT, JOYPORT_UP_BIT);      /* output snespad 1 start button on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_BUTTON_START_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 start button on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_BUTTON_START_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 start button on joyport 'left' pin */
            break;
        case SNESPAD_UP:
            retval = (uint8_t)JOYPORT_BIT_BOOL(joyval1, JOYPORT_UP_BIT);                   /* output snespad 1 up on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_UP_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 up on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_UP_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 up on joyport 'left' pin */
            break;
        case SNESPAD_DOWN:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_DOWN_BIT, JOYPORT_UP_BIT);      /* output snespad 1 down on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_DOWN_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 down on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_DOWN_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 down on joyport 'left' pin */
            break;
        case SNESPAD_LEFT:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_LEFT_BIT, JOYPORT_UP_BIT);      /* output snespad 1 left on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_LEFT_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 left on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_LEFT_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 left on joyport 'left' pin */
            break;
        case SNESPAD_RIGHT:
            retval = (uint8_t)JOYPORT_BIT_SHIFT(joyval1, JOYPORT_RIGHT_BIT, JOYPORT_UP_BIT);      /* output snespad 1 right on joyport 'up' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval2, JOYPORT_RIGHT_BIT, JOYPORT_DOWN_BIT);   /* output snespad 2 right on joyport 'down' pin */
            retval |= (uint8_t)JOYPORT_BIT_SHIFT(joyval3, JOYPORT_RIGHT_BIT, JOYPORT_LEFT_BIT);   /* output snespad 3 right on joyport 'left' pin */
            break;
        case SNESPAD_BIT_12_1:
        case SNESPAD_BIT_13_1:
        case SNESPAD_BIT_14_1:
        case SNESPAD_BIT_15_1:
            /* part of the snes sequence, but unused, return 1 on each line */
            retval = 7;
            break;
        case SNESPAD_EOS:
            retval = 0;
            break;
        default:
            retval = 1;
    }

    return ~(retval);
}

static void snespad_store(int port, uint8_t val)
{
    uint8_t new_clock = JOYPORT_BIT_BOOL(val, JOYPORT_RIGHT_BIT);   /* clock line is on joyport 'right' pin */
    uint8_t new_latch = JOYPORT_BIT_BOOL(val, JOYPORT_FIRE_BIT);    /* latch line is on joyport 'fire' pin */

    if (latch_line && !new_latch) {
        /* latch line asserted, set counter to 0 */
        counter = 0;
    }

    if (clock_line && !new_clock) {
        /* clock line asserted, increment the counter if we are not at the end of the sequence */
        if (counter != SNESPAD_EOS) {
            counter++;
        }
    }

    latch_line = new_latch;
    clock_line = new_clock;
}

static void snespad_powerup(int port)
{
    counter = 0;
}

/* ------------------------------------------------------------------------- */

static int ninja_snespad_write_snapshot(struct snapshot_s *s, int p);
static int ninja_snespad_read_snapshot(struct snapshot_s *s, int p);

static joyport_t joyport_snespad_device = {
    "SNES Pad Adapter (Ninja)",       /* name of the device */
    JOYPORT_RES_ID_NONE,              /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,          /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,             /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,            /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NINJA_SNES,   /* device is a joystick adapter */
    JOYPORT_DEVICE_SNES_ADAPTER,      /* device is a SNES adapter */
    0,                                /* NO output bits */
    joyport_snespad_set_enabled,      /* device enable/disable function */
    snespad_read,                     /* digital line read function */
    snespad_store,                    /* digital line store function */
    NULL,                             /* NO pot-x read function */
    NULL,                             /* NO pot-y read function */
    snespad_powerup,                  /* powerup function */
    ninja_snespad_write_snapshot,     /* device write snapshot function */
    ninja_snespad_read_snapshot,      /* device read snapshot function */
    NULL,                             /* NO device hook function */
    0                                 /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_ninja_snespad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_NINJA_SNESPAD, &joyport_snespad_device);
}

/* ------------------------------------------------------------------------- */

/* NINJASNESPAD snapshot module format:

   type  |   name  | description
   ----------------------------------
   BYTE  | COUNTER | counter value
   BYTE  | LATCH   | latch line state
   BYTE  | CLOCK   | clock line state
 */

static const char snap_module_name[] = "NINJASNESPAD";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int ninja_snespad_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, counter) < 0
        || SMW_B(m, latch_line) < 0
        || SMW_B(m, clock_line) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int ninja_snespad_read_snapshot(struct snapshot_s *s, int p)
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
        || SMR_B(m, &counter) < 0
        || SMR_B(m, &latch_line) < 0
        || SMR_B(m, &clock_line) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
