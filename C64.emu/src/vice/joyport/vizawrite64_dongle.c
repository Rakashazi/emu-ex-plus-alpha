/*
 * vizawrite64_dongle.c - VizaWrite 64 Dongle emulation.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joyport.h"
#include "keyboard.h"

#include "vizawrite64_dongle.h"

/* Control port <--> Vizawrite64 dongle connections:

   cport | I/O
   ------------
     5   | O
     9   | O

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128)
 */

/* ------------------------------------------------------------------------- */

static int joyport_vizawrite64_dongle_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t counter[JOYPORT_MAX_PORTS] = {0};

static const uint8_t values[6] = {
    0x55, 0x55, 0xaa, 0xaa, 0xff, 0xff
};

static int joyport_vizawrite64_dongle_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    joyport_vizawrite64_dongle_enabled[port] = new_state;

    return 0;
}

static uint8_t vizawrite64_dongle_read_potx(int port)
{
    uint8_t retval = values[counter[port]++];

    if (counter[port] == 6) {
        counter[port] = 0;
    }

    return retval;
}

static uint8_t vizawrite64_dongle_read_poty(int port)
{
    uint8_t retval = values[counter[port]++];

    if (counter[port] == 6) {
        counter[port] = 0;
    }

    return retval;
}

static void vizawrite64_powerup(int port)
{
    counter[port] = 0;
}

/* ------------------------------------------------------------------------- */

static int vizawrite64_write_snapshot(struct snapshot_s *s, int p);
static int vizawrite64_read_snapshot(struct snapshot_s *s, int p);

static joyport_t joyport_vizawrite64_dongle_device = {
    "Dongle (VizaWrite 64)",               /* name of the device */
    JOYPORT_RES_ID_NONE,                   /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,               /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,                  /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,                 /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,              /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_C64_DONGLE,             /* device is a C64 Dongle */
    0,                                     /* NO output bits */
    joyport_vizawrite64_dongle_set_enabled, /* device enable/disable function */
    NULL,                                  /* NO digital line read function */
    NULL,                                  /* NO digital line store function */
    vizawrite64_dongle_read_potx,          /* pot-x read function */
    vizawrite64_dongle_read_poty,          /* pot-y read function */
    vizawrite64_powerup,                   /* powerup function */
    vizawrite64_write_snapshot,            /* device write snapshot function */
    vizawrite64_read_snapshot,             /* device read snapshot function */
    NULL,                                  /* NO device hook function */
    0                                      /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_vizawrite64_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_VIZAWRITE64_DONGLE, &joyport_vizawrite64_dongle_device);
}

/* ------------------------------------------------------------------------- */

/* VIZAWRITE64 snapshot module format:

   type  |   name  | description
   ----------------------------------
   BYTE  | COUNTER | counter value
 */

static const char snap_module_name[] = "VIZAWRITE64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int vizawrite64_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, counter[port]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int vizawrite64_read_snapshot(struct snapshot_s *s, int port)
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
        || SMR_B(m, &counter[port]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
