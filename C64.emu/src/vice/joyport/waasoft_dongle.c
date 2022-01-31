/*
 * waasoft_dongle.c - Waasoft Dongle emulation.
 *
 * Written by
 *  Zer0-X
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

/* Control port <--> waasoft dongle connections:

   cport | waasoft dongle | I/O
   ----------------------------
     1   | Reset          |  O
     2   | Clock          |  O
     5   | Data           |  I
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joyport.h"
#include "keyboard.h"

#include "waasoft_dongle.h"

/* ------------------------------------------------------------------------- */

static int joyport_waasoft_dongle_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t counter[JOYPORT_MAX_PORTS] = {0};

static uint8_t waasoft_reset_line[JOYPORT_MAX_PORTS] = {1};
static uint8_t waasoft_clock_line[JOYPORT_MAX_PORTS] = {1};

static const uint8_t waasoft_values[15] = {
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00
};

static int joyport_waasoft_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    joyport_waasoft_dongle_enabled[port] = val;

    return 0;
}

static uint8_t waasoft_dongle_read_poty(int port)
{
    return waasoft_values[counter[port]];
}

static void waasoft_dongle_store_dig(int port, uint8_t val)
{
    uint8_t reset = val & 1;
    uint8_t clock = val & 2;

    if (clock != waasoft_clock_line[port]) {
        if (!clock) {
            counter[port]++;
            if (counter[port] == 15) {
                counter[port] = 0;
            }
        }
    }

    waasoft_clock_line[port] = clock;

    if (reset != waasoft_reset_line[port]) {
        if (!reset) {
            counter[port] = 0;
        }
    }

    waasoft_reset_line[port] = reset;
}

static void waasoft_powerup(int port)
{
    counter[port] = 0;
}

/* ------------------------------------------------------------------------- */

static int waasoft_write_snapshot(struct snapshot_s *s, int p);
static int waasoft_read_snapshot(struct snapshot_s *s, int p);

static joyport_t joyport_waasoft_dongle_device = {
    "Dongle (WaaSoft)",            /* name of the device */
    JOYPORT_RES_ID_NONE,           /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,       /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,          /* device uses the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE,      /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_C64_DONGLE,     /* device is a C64 Dongle */
    0x03,                          /* bits 1 and 0 are output bits */
    joyport_waasoft_dongle_enable, /* device enable function */
    NULL,                          /* NO digital line read function */
    waasoft_dongle_store_dig,      /* digital line store function */
    NULL,                          /* NO pot-x read function */
    waasoft_dongle_read_poty,      /* pot-y read function */
    waasoft_powerup,               /* powerup function */
    waasoft_write_snapshot,        /* device write snapshot function */
    waasoft_read_snapshot,         /* device read snapshot function */
    NULL,                          /* NO device hook function */
    0                              /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_waasoft_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_WAASOFT_DONGLE, &joyport_waasoft_dongle_device);
}

/* ------------------------------------------------------------------------- */

/* WAASOFT snapshot module format:

   type  |   name  | description
   ----------------------------------
   BYTE  | COUNTER | counter value
   BYTE  | RESET   | reset line state
   BYTE  | CLOCK   | clock line state
 */

static const char snap_module_name[] = "WAASOFT";
#define SNAP_MAJOR   1
#define SNAP_MINOR   0

static int waasoft_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, counter[port]) < 0
        || SMW_B(m, waasoft_reset_line[port]) < 0
        || SMW_B(m, waasoft_clock_line[port]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int waasoft_read_snapshot(struct snapshot_s *s, int port)
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
        || SMR_B(m, &counter[port]) < 0
        || SMR_B(m, &waasoft_reset_line[port]) < 0
        || SMR_B(m, &waasoft_clock_line[port]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
