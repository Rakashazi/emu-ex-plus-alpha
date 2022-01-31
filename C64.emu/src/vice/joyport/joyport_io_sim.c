/*
 * joyport_io_sim.c - Joyport I/O simulation.
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
#include "joyport_io_sim.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"

/* - Control port I/O simulation

   This device provides a way of simulating the data input and
   output lines of a joyport device, can be used to test the
   joyport system.
*/

/* #define HOST_HARDWARE_IO */

/* Enable above define to be able to use host hardware I/O.
   In this mode you will need to write your own code to access
   the host hardware.
*/

static int joyport_io_sim_enabled[JOYPORT_MAX_PORTS] = {0};

#ifndef HOST_HARDWARE_IO
static uint8_t joyport_io_sim_data_out[JOYPORT_MAX_PORTS] = {0};
static uint8_t joyport_io_sim_data_in[JOYPORT_MAX_PORTS] = {0};

static uint8_t joyport_io_sim_potx[JOYPORT_MAX_PORTS] = {0};
static uint8_t joyport_io_sim_poty[JOYPORT_MAX_PORTS] = {0};
#endif

/* ------------------------------------------------------------------------- */

#ifdef HOST_HARDWARE_IO
static joyport_t joyport_io_hw_device;
#else
static joyport_t joyport_io_sim_device;
#endif

static int joyport_io_sim_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == joyport_io_sim_enabled[port]) {
        return 0;
    }

#ifndef HOST_HARDWARE_IO
    if (val) {
        joyport_io_sim_data_out[port] = 0;
        joyport_io_sim_data_in[port] = 0;
        joyport_io_sim_potx[port] = 0;
        joyport_io_sim_poty[port] = 0;
    }
#endif

    joyport_io_sim_enabled[port] = val;

    return 0;
}

#ifdef HOST_HARDWARE_IO
static uint8_t joyport_io_hw_read(int p)
{
    /* Create your own code that reads a byte from joystick port
       'p' of a host hardware joystick port. */
    return 0xff;
}

static void joyport_io_hw_store(int p, uint8_t val)
{
    /* Create your own code that sets the joystick port 'p' lines
       of a host hardware joystick port */
}

static uint8_t joyport_io_hw_read_potx(int p)
{
    /* Create your own code that reads a byte from joystick port
       'p' pot-x line of a host hardware joystick port, handling
       of the sampling will have to be done in hardware or host
       level software. */
    return 0xff;
}

static uint8_t joyport_io_hw_read_poty(int p)
{
    /* Create your own code that reads a byte from joystick port
       'p' pot-y line of a host hardware joystick port, handling
       of the sampling will have to be done in hardware or host
       level software. */
    return 0xff;
}
#else
static uint8_t joyport_io_sim_read(int port)
{
    return joyport_io_sim_data_out[port];
}

static void joyport_io_sim_store(int port, uint8_t val)
{
    joyport_io_sim_data_in[port] = val;
}

static uint8_t joyport_io_sim_read_potx(int port)
{
    return joyport_io_sim_potx[port];
}

static uint8_t joyport_io_sim_read_poty(int port)
{
    return joyport_io_sim_poty[port];
}
#endif

/* ------------------------------------------------------------------------- */

#ifndef HOST_HARDWARE_IO
static int joyport_io_sim_write_snapshot(struct snapshot_s *s, int p);
static int joyport_io_sim_read_snapshot(struct snapshot_s *s, int p);
#endif

#ifdef HOST_HARDWARE_IO
static joyport_t joyport_io_hw_device = {
    "Joyport host I/O hardware",  /* name of the device */
    JOYPORT_RES_ID_NONE,          /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,      /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,         /* device does NOT use the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE,     /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_IO_SIMULATION, /* device is a SNES adapter */
    0,                            /* output bits are programmable */
    joyport_io_sim_enable,        /* device enable function */
    joyport_io_hw_read,           /* digital line read function */
    joyport_io_hw_store,          /* digital line store function */
    joyport_io_hw_read_potx,      /* pot-x read function */
    joyport_io_hw_read_poty,      /* pot-y read function */
    NULL,                         /* NO powerup function */
    NULL,                         /* NO device write snapshot function */
    NULL,                         /* NO device read snapshot function */
    NULL,                         /* NO device hook function */
    0                             /* device hook function mask */
};
#else
static joyport_t joyport_io_sim_device = {
    "Joyport I/O simulation",      /* name of the device */
    JOYPORT_RES_ID_NONE,           /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,       /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,          /* device does NOT use the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE,      /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_IO_SIMULATION,  /* device is a SNES adapter */
    0,                             /* output bits are programmable */
    joyport_io_sim_enable,         /* device enable function */
    joyport_io_sim_read,           /* digital line read function */
    joyport_io_sim_store,          /* digital line store function */
    joyport_io_sim_read_potx,      /* pot-x read function */
    joyport_io_sim_read_poty,      /* pot-y read function */
    NULL,                          /* NO powerup function */
    joyport_io_sim_write_snapshot, /* device write snapshot function */
    joyport_io_sim_read_snapshot,  /* device read snapshot function */
    NULL,                          /* NO device hook function */
    0                              /* device hook function mask */
};
#endif

/* ------------------------------------------------------------------------- */

int joyport_io_sim_resources_init(void)
{
#ifdef HOST_HARDWARE_IO
    return joyport_device_register(JOYPORT_ID_IO_SIMULATION, &joyport_io_hw_device);
#else
    return joyport_device_register(JOYPORT_ID_IO_SIMULATION, &joyport_io_sim_device);
#endif
}

/* ------------------------------------------------------------------------- */

#ifndef HOST_HARDWARE_IO
void joyport_io_sim_set_out_lines(uint8_t val, int port)
{
    joyport_io_sim_data_out[port] = val & 0x1f;
}

uint8_t joyport_io_sim_get_out_lines(int port)
{
    return joyport_io_sim_data_out[port];
}

uint8_t joyport_io_sim_get_in_lines(int port)
{
    return joyport_io_sim_data_in[port];
}

void joyport_io_sim_set_potx(uint8_t val, int port)
{
    joyport_io_sim_potx[port] = val;
}

void joyport_io_sim_set_poty(uint8_t val, int port)
{
    joyport_io_sim_poty[port] = val;
}

uint8_t joyport_io_sim_get_potx(int port)
{
    return joyport_io_sim_potx[port];
}

uint8_t joyport_io_sim_get_poty(int port)
{
    return joyport_io_sim_poty[port];
}

/* ------------------------------------------------------------------------- */

/* JOYPORT I/O SIMULATION snapshot module format:

   type  |   name      | description
   --------------------------------------
   BYTE  | DATA OUT | data out value
   BYTE  | DATA IN  | data in value
   BYTE  | POTX     | pot-x state
   BYTE  | POTY     | pot-y state
 */

static const char snap_module_name[] = "JPIOSIM";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int joyport_io_sim_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, joyport_io_sim_data_out[p]) < 0
        || SMW_B(m, joyport_io_sim_data_in[p]) < 0
        || SMW_B(m, joyport_io_sim_potx[p]) < 0
        || SMW_B(m, joyport_io_sim_poty[p]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int joyport_io_sim_read_snapshot(struct snapshot_s *s, int p)
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
        || SMR_B(m, &joyport_io_sim_data_out[p]) < 0
        || SMR_B(m, &joyport_io_sim_data_in[p]) < 0
        || SMR_B(m, &joyport_io_sim_potx[p]) < 0
        || SMR_B(m, &joyport_io_sim_poty[p]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
#endif
