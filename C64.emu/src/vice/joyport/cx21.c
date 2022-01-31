/*
 * cx21.c - Atari CX21 keypad emulation.
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
#include "keyboard.h"
#include "snapshot.h"

#include "cx21.h"

/* Control port <--> CX21 keypad connections:

   cport | keypad | I/O
   -------------------------
     1   | KEYROW1 |  O
     2   | KEYROW2 |  O
     3   | KEYROW3 |  O
     4   | KEYROW4 |  O
     5   | KEYCOL3 |  I
     6   | KEYCOL2 |  I
     9   | KEYCOL1 |  I

Works on:
- native port(s) (x64/x64sc/xscpu64/xvic)
- sidcart joystick adapter port (xplus4)


The keypad has the following layout:

KEYPAD          KEYMAP KEYS
-------------   ----------------
| 3 | 2 | 1 |   |  1 |  2 |  3 |
-------------   ----------------
| 6 | 5 | 4 |   |  6 |  7 |  8 |
-------------   ----------------
| 9 | 8 | 7 |   | 11 | 12 | 13 |
-------------   ----------------
| # | 0 | * |   | 16 | 17 | 18 |
-------------   ----------------

The keypad connects a row to a column.

The following logic is used:

key pin pin comments
--- --- --- --------
 3   1   6  A0 <-> FIRE
 2   1   9  A0 <-> POT AX
 1   1   5  A0 <-> POT AY
 6   2   6  A1 <-> FIRE
 5   2   9  A1 <-> POT AX
 4   2   5  A1 <-> POT AY
 9   3   6  A2 <-> FIRE
 8   3   9  A2 <-> POT AX
 7   3   5  A2 <-> POT AY
 #   4   6  A3 <-> FIRE
 0   4   9  A3 <-> POT AX
 *   4   5  A3 <-> POT AY
 */

#define KEYPAD_KEY_3      0
#define KEYPAD_KEY_2      1
#define KEYPAD_KEY_1      2

#define KEYPAD_KEY_6      3
#define KEYPAD_KEY_5      4
#define KEYPAD_KEY_4      5

#define KEYPAD_KEY_9      6
#define KEYPAD_KEY_8      7
#define KEYPAD_KEY_7      8

#define KEYPAD_KEY_HASH   9
#define KEYPAD_KEY_0      10
#define KEYPAD_KEY_MULT   11

#define KEYPAD_NUM_KEYS   12

static int cx21_enabled = 0;

static unsigned int keys[KEYPAD_NUM_KEYS];
static uint8_t port = 0;

/* ------------------------------------------------------------------------- */

static void handle_keys(int row, int col, int pressed)
{
    if (row < 0 || row > 3 || col < 1 || col > 3) {
        return;
    }

    keys[(row * 3) + col - 1] = (unsigned int)pressed;
}

/* ------------------------------------------------------------------------- */

static int joyport_cx21_enable(int prt, int value)
{
    int val = value ? 1 : 0;

    if (val == cx21_enabled) {
        return 0;
    }

    if (val) {
        memset(keys, 0, KEYPAD_NUM_KEYS * sizeof(unsigned int));
        keyboard_register_joy_keypad(handle_keys);
    } else {
        keyboard_register_joy_keypad(NULL);
    }

    cx21_enabled = val;

    return 0;
}

static uint8_t cx21_read_dig(int p)
{
    uint8_t retval = 0;

    if (keys[KEYPAD_KEY_3]) {
        if (port & 1) {
            retval = JOYPORT_FIRE;   /* output on joyport 'fire' pin */
        }
    }

    if (keys[KEYPAD_KEY_6]) {
        if (port & 2) {
            retval = JOYPORT_FIRE;   /* output on joyport 'fire' pin */
        }
    }

    if (keys[KEYPAD_KEY_9]) {
        if (port & 4) {
            retval = JOYPORT_FIRE;   /* output on joyport 'fire' pin */
        }
    }

    if (keys[KEYPAD_KEY_HASH]) {
        if (port & 8) {
            retval = JOYPORT_FIRE;   /* output on joyport 'fire' pin */
        }
    }

    joyport_display_joyport(JOYPORT_ID_CX21_KEYPAD, (uint8_t)retval);

    return (uint8_t)~retval;
}

static void cx21_store_dig(int prt, uint8_t val)
{
    port = (uint8_t)~val;
}

static uint8_t cx21_read_potx(int joyport)
{
    if (keys[KEYPAD_KEY_2]) {
        if (port & 1) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_5]) {
        if (port & 2) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_8]) {
        if (port & 4) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_0]) {
        if (port & 8) {
            return 0;
        }
    }

    return 0xff;
}

static uint8_t cx21_read_poty(int joyport)
{
    if (keys[KEYPAD_KEY_1]) {
        if (port & 1) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_4]) {
        if (port & 2) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_7]) {
        if (port & 4) {
            return 0;
        }
    }

    if (keys[KEYPAD_KEY_MULT]) {
        if (port & 8) {
            return 0;
        }
    }

    return 0xff;
}

/* ------------------------------------------------------------------------- */

static int cx21_write_snapshot(struct snapshot_s *s, int port);
static int cx21_read_snapshot(struct snapshot_s *s, int port);

static joyport_t joyport_cx21_device = {
    "Keypad (Atari CX21)",    /* name of the device */
    JOYPORT_RES_ID_KEYPAD,    /* device is a keypad, only 1 keypad can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,  /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,     /* device uses the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE, /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_KEYPAD,    /* device is a Keypad */
    0x0F,                     /* bits 3, 2, 1 and 0 are output bits */
    joyport_cx21_enable,      /* device enable function */
    cx21_read_dig,            /* digital line read function */
    cx21_store_dig,           /* digital line store function */
    cx21_read_potx,           /* pot-x read function */
    cx21_read_poty,           /* pot-y read function */
    NULL,                     /* NO powerup function */
    cx21_write_snapshot,      /* device write snapshot function */
    cx21_read_snapshot,       /* device read snapshot function */
    NULL,                     /* NO device hook function */
    0                         /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_cx21_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_CX21_KEYPAD, &joyport_cx21_device);
}

/* ------------------------------------------------------------------------- */

/* CX21 snapshot module format:

   type  | name | description
   ----------------------------------
   BYTE  | PORT | PORT register state
 */

static const char snap_module_name[] = "CX21";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int cx21_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, port) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int cx21_read_snapshot(struct snapshot_s *s, int p)
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
        || SMR_B(m, &port) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
