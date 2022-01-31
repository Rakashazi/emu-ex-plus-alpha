/*
 * dtlbasic-dongle.c - dtl basic tape port dongle emulation.
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
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"

#include "dtl-basic-dongle.h"


/* DTL Basic Dongle description:

   This emulation currently does not work for the software using it,
   help/more information is needed to fix this.

   Documentation/information used for making the emulation:

   http://sid.fi/~tnt/dtl/

   The current emulation does the following:

   1- wait for the sense line to go high.
   2- wait for the sense line to go low.
   3- wait for the write line to go high.
   4- wait for the write line to go low.
   5- set the first bit of the bit sequence 0010 0100 0000 0010 (0x2402) on the read line.

   After setting the first bit of the sequence the following needs to happen before setting the next bit:

   1- wait for the write line to go high.
   2- wait for the write line to go low.

   Be aware that the current emulation keeps the 'old' bit set between steps 1 and 2.

   The emulation 'shows' the correct bit pattern when using the 'dongle2.prg' file, but does not work (good enough) for the actual software.
*/

#define DTLBASIC_DONGLE_IDLE       0
#define DTLBASIC_DONGLE_SENSE_HIGH 1
#define DTLBASIC_DONGLE_SENSE_LOW  2
#define DTLBASIC_DONGLE_WRITE_HIGH 3
#define DTLBASIC_DONGLE_ACTIVE     4

static int dtlbasic_dongle_enabled = 0;

static int dtlbasic_counter = -1;

static const uint8_t dtlbasic_key[20] = { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };

static int write_status = -1;
static int sense_status = -1;

static int dtlbasic_state = DTLBASIC_DONGLE_IDLE;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void dtlbasic_powerup(int port);
static void dtlbasic_write(int port, int write_bit);
static void dtlbasic_sense_out(int port, int sense);
static int dtlbasic_write_snapshot(int port, struct snapshot_s *s, int write_image);
static int dtlbasic_read_snapshot(int port, struct snapshot_s *s);
static int dtlbasic_enable(int port, int val);

#define VICE_MACHINE_MASK (VICE_MACHINE_C64|VICE_MACHINE_C64SC|VICE_MACHINE_C128)

static tapeport_device_t dtlbasic_dongle_device = {
    "DTL BASIC dongle",          /* device name */
    TAPEPORT_DEVICE_TYPE_DONGLE, /* device is a 'dongle' type device */
    VICE_MACHINE_MASK,           /* device works on x64/x64sc/x128 machines */
    TAPEPORT_PORT_1_MASK,        /* device only works on port 1 */
    dtlbasic_enable,             /* device enable function */
    dtlbasic_powerup,            /* device specific hard reset function */
    NULL,                        /* NO device shutdown function */
    NULL,                        /* NO set motor line function */
    dtlbasic_write,              /* set write line function */
    dtlbasic_sense_out,          /* set sense line function */
    NULL,                        /* NO set read line function */
    dtlbasic_write_snapshot,     /* device snapshot write function */
    dtlbasic_read_snapshot       /* device snapshot read function */
};


/* ------------------------------------------------------------------------- */

static int dtlbasic_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (dtlbasic_dongle_enabled == val) {
        return 0;
    }

    if (val) {
        dtlbasic_counter = -1;
        dtlbasic_state = DTLBASIC_DONGLE_IDLE;
    }

    dtlbasic_dongle_enabled = val;
    return 0;
}

int dtlbasic_dongle_resources_init(int amount)
{
    return tapeport_device_register(TAPEPORT_DEVICE_DTL_BASIC_DONGLE, &dtlbasic_dongle_device);
}

/* ---------------------------------------------------------------------*/

static void dtlbasic_powerup(int port)
{
    dtlbasic_state = DTLBASIC_DONGLE_IDLE;
    dtlbasic_counter = -1;
    write_status = -1;
    sense_status = -1;
}

static void dtlbasic_write(int port, int write_bit)
{
    if (write_bit == write_status) {
        return;
    }

    write_status = write_bit;

    if (dtlbasic_state == DTLBASIC_DONGLE_SENSE_LOW && write_bit) {
        dtlbasic_state = DTLBASIC_DONGLE_WRITE_HIGH;
        return;
    }

    if (dtlbasic_state == DTLBASIC_DONGLE_WRITE_HIGH && !write_bit) {
        dtlbasic_state = DTLBASIC_DONGLE_ACTIVE;
        dtlbasic_counter = 1;
        return;
    }

    if (!write_bit) {
        if (dtlbasic_counter != -1) {
            if (dtlbasic_key[dtlbasic_counter]) {
                tapeport_trigger_flux_change(1, TAPEPORT_PORT_1);
            }
            ++dtlbasic_counter;
            if (dtlbasic_counter == 20) {
                dtlbasic_counter = -1;
            }
        }
    }
}

static void dtlbasic_sense_out(int port, int sense)
{
    if (sense == sense_status) {
        return;
    }

    sense_status = sense;

    if (dtlbasic_state == DTLBASIC_DONGLE_IDLE && sense) {
        dtlbasic_state = DTLBASIC_DONGLE_SENSE_HIGH;
        return;
    }

    if (dtlbasic_state == DTLBASIC_DONGLE_SENSE_HIGH && !sense) {
        dtlbasic_state = DTLBASIC_DONGLE_SENSE_LOW;
        return;
    }
}

/* ---------------------------------------------------------------------*/

/* TP_DTLBASIC snapshot module format:

   type  | name    | description
   -----------------------------
   DWORD | counter | counter
   DWORD | write   | write line state
   DWORD | sense   | sense line state
   DWORD | state   | device state
 */

static const char snap_module_name[] = "TP_DTLBASIC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int dtlbasic_write_snapshot(int port, struct snapshot_s *s, int write_image)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DW(m, (uint32_t)dtlbasic_counter) < 0
        || SMW_DW(m, (uint32_t)write_status) < 0
        || SMW_DW(m, (uint32_t)sense_status) < 0
        || SMW_DW(m, (uint32_t)dtlbasic_state) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int dtlbasic_read_snapshot(int port, struct snapshot_s *s)
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
        || SMR_DW_INT(m, &dtlbasic_counter) < 0
        || SMR_DW_INT(m, &write_status) < 0
        || SMR_DW_INT(m, &sense_status) < 0
        || SMR_DW_INT(m, &dtlbasic_state) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
