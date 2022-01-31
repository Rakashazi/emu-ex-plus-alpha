/*
 * trapthem_snespad.c - Trapthem SNES PAD emulation.
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
#include "resources.h"
#include "snapshot.h"
#include "snespad.h"
#include "trapthem_snespad.h"

#include "log.h"

/* Control port <--> SNES PAD connections:

   cport |  SNES PAD  | I/O
   ------------------------
     3   |  DATA PAD  |  I
     4   |   CLOCK    |  O
     6   |   LATCH    |  O

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/x64dtv/xcbm5x0/xvic)
   - hit userport joystick adapter port 1 (x64/x64sc/xscpu64/x128)
   - kingsoft userport joystick adapter port 1 (x64/x64sc/xscpu64/x128)
   - starbyte userport joystick adapter port 2 (x64/x64sc/xscpu64/x128)
   - hummer userport joystick adapter port (x64dtv)
   - oem userport joystick adapter port (xvic)
   - sidcart joystick adapter port (xplus4)

 */

static int snespad_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t counter[JOYPORT_MAX_PORTS] = {0};

static uint8_t clock_line[JOYPORT_MAX_PORTS] = {0};
static uint8_t latch_line[JOYPORT_MAX_PORTS] = {0};

/* ------------------------------------------------------------------------- */

static joyport_t joyport_snespad_device;

static int joyport_snespad_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == snespad_enabled[port]) {
        return 0;
    }

    if (val) {
        counter[port] = 0;
        joystick_set_snes_mapping(port);
    } else {
        joyport_clear_mapping(port);
    }

    snespad_enabled[port] = val;

    return 0;
}

static uint8_t snespad_read(int port)
{
    uint8_t retval;
    uint16_t joyval = get_joystick_value(port);

    switch (counter[port]) {
        case SNESPAD_BUTTON_A:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_A_BIT, JOYPORT_LEFT_BIT));   /* output button a on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_B:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_B_BIT, JOYPORT_LEFT_BIT));   /* output button b on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_X:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_X_BIT, JOYPORT_LEFT_BIT));   /* output button x on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_Y:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_Y_BIT, JOYPORT_LEFT_BIT));   /* output button y on joyport 'left' pin */
            break;
        case SNESPAD_BUMPER_LEFT:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_LEFT_BUMBER_BIT, JOYPORT_LEFT_BIT));   /* output left bumper button on joyport 'left' pin */
            break;
        case SNESPAD_BUMPER_RIGHT:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_RIGHT_BUMBER_BIT, JOYPORT_LEFT_BIT));   /* output right bumper button on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_SELECT:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_SELECT_BIT, JOYPORT_LEFT_BIT));   /* output select button on joyport 'left' pin */
            break;
        case SNESPAD_BUTTON_START:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_BUTTON_START_BIT, JOYPORT_LEFT_BIT));   /* output start button on joyport 'left' pin */
            break;
        case SNESPAD_UP:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_UP_BIT, JOYPORT_LEFT_BIT));   /* output up on joyport 'left' pin */
            break;
        case SNESPAD_DOWN:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_DOWN_BIT, JOYPORT_LEFT_BIT));   /* output down on joyport 'left' pin */
            break;
        case SNESPAD_LEFT:
            retval = (uint8_t)(joyval & JOYPORT_LEFT); /* output left on joyport 'left' pin */
            break;
        case SNESPAD_RIGHT:
            retval = (uint8_t)(JOYPORT_BIT_SHIFT(joyval, JOYPORT_RIGHT_BIT, JOYPORT_LEFT_BIT));   /* output right on joyport 'left' pin */
            break;
        case SNESPAD_BIT_12_1:
        case SNESPAD_BIT_13_1:
        case SNESPAD_BIT_14_1:
        case SNESPAD_BIT_15_1:
            retval = 4;
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

    if (latch_line[port] && !new_latch) {
        counter[port] = 0;
    }

    if (clock_line[port] && !new_clock) {
        if (counter[port] != SNESPAD_EOS) {
            counter[port]++;
        }
    }

    latch_line[port] = new_latch;
    clock_line[port] = new_clock;
}

static void snespad_powerup(int port)
{
    counter[port] = 0;
}

/* ------------------------------------------------------------------------- */

static int trapthem_snespad_write_snapshot(struct snapshot_s *s, int p);
static int trapthem_snespad_read_snapshot(struct snapshot_s *s, int p);

static joyport_t joyport_snespad_device = {
    "SNES Pad Adapter (Trapthem)",     /* name of the device */
    JOYPORT_RES_ID_NONE,               /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,           /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,              /* device does NOT use the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE,          /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_SNES_ADAPTER,       /* device is a SNES adapter */
    0x1C,                              /* bits 4 and 3 are output bits */
    joyport_snespad_enable,            /* device enable function */
    snespad_read,                      /* digital line read function */
    snespad_store,                     /* digital line store function */
    NULL,                              /* NO pot-x read function */
    NULL,                              /* NO pot-y read function */
    snespad_powerup,                   /* powerup function */
    trapthem_snespad_write_snapshot,   /* device write snapshot function */
    trapthem_snespad_read_snapshot,    /* device read snapshot function */
    NULL,                              /* NO device hook function */
    0                                  /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_trapthem_snespad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_TRAPTHEM_SNESPAD, &joyport_snespad_device);
}

/* ------------------------------------------------------------------------- */

/* TRAPTHEMSNESPAD snapshot module format:

   type  |   name  | description
   ----------------------------------
   BYTE  | COUNTER | counter value
   BYTE  | LATCH   | latch line state
   BYTE  | CLOCK   | clock line state
 */

static const char snap_module_name[] = "TRAPTHEMSNESPAD";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int trapthem_snespad_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, counter[p]) < 0
        || SMW_B(m, latch_line[p]) < 0
        || SMW_B(m, clock_line[p]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int trapthem_snespad_read_snapshot(struct snapshot_s *s, int p)
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
        || SMR_B(m, &counter[p]) < 0
        || SMR_B(m, &latch_line[p]) < 0
        || SMR_B(m, &clock_line[p]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
