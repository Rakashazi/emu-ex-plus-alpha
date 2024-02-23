/*
 * tape_diag_586220_harness.c - Tapeport part of the 586220 diagnostic harness emulation.
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

/* tape_diag_586220_harness:

This device is attached to the tape port, it is the tapeport
part of a diagnostic harness.

PIN | CABLE | NOTES
-------------------
A-1 |   8   | SWa, SWb, SWc, SWd, SWa2 control (+5V)
C-3 |   7   | SWa, SWb, SWc, SWd, SWa2 control (MOTOR) can ground the line
D-4 |   6   | loops to 4 (READ <-> SENSE)
E-5 |   5   | SWa, SWb, SWc, SWd, SWa2 control (WRITE) can ground the line
F-6 |   4   | loops to 6 (SENSE <-> READ)
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64_diag_586220_harness.h"
#include "cmdline.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "tape_diag_586220_harness.h"
#include "tapeport.h"
#include "util.h"

#ifdef TAPEPORT_EXPERIMENTAL_DEVICES

/* Device enabled */
static int tape_diag_586220_harness_enabled = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void tape_diag_586220_harness_set_motor(int port, int flag);
static void tape_diag_586220_harness_toggle_write_bit(int port, int write_bit);
static void tape_diag_586220_harness_set_sense_out(int port, int sense);
static void tape_diag_586220_harness_set_read_out(int port, int val);
static int tape_diag_586220_harness_enable(int port, int val);

#define VICE_MACHINE_MASK (VICE_MACHINE_C64|VICE_MACHINE_C64SC)

static tapeport_device_t tape_diag_586220_harness_device = {
    "Tape 586220 diagnostics harness module",  /* device name */
    TAPEPORT_DEVICE_TYPE_HARNESS,              /* device is a 'harness' type device */
    VICE_MACHINE_MASK,                         /* device works on x64/x64sc machines */
    TAPEPORT_PORT_1_MASK,                      /* device only works on port 1 */
    tape_diag_586220_harness_enable,           /* device enable function */
    NULL,                                      /* NO device specific hard reset function */
    NULL,                                      /* NO device shutdown function */
    tape_diag_586220_harness_set_motor,        /* set motor line function */
    tape_diag_586220_harness_toggle_write_bit, /* set write line function */
    tape_diag_586220_harness_set_sense_out,    /* set sense line function */
    tape_diag_586220_harness_set_read_out,     /* set read line function */
    NULL,                                      /* NO device snapshot write function */
    NULL                                       /* NO device snapshot read function */
};

/* ------------------------------------------------------------------------- */

static void tape_diag_586220_harness_set_motor(int port, int flag)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_MOTOR, (uint8_t)flag);
}

static void tape_diag_586220_harness_toggle_write_bit(int port, int write_bit)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_WRITE, (uint8_t)write_bit);
}

static void tape_diag_586220_harness_set_sense_out(int port, int sense)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_SENSE, (uint8_t)sense);
}

static void tape_diag_586220_harness_set_read_out(int port, int read)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_READ, (uint8_t)read);
}

/* ------------------------------------------------------------------------- */

static int tape_diag_586220_harness_enable(int port, int value)
{
    int val = value ? 1 : 0;

    tape_diag_586220_harness_enabled = val;

    return 0;
}

/* ------------------------------------------------------------------------- */

int tape_diag_586220_harness_resources_init(int amount)
{
    return tapeport_device_register(TAPEPORT_DEVICE_TAPE_DIAG_586220_HARNESS, &tape_diag_586220_harness_device);
}

#endif
