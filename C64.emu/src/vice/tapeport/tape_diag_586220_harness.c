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
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "tape_diag_586220_harness.h"
#include "tapeport.h"
#include "translate.h"
#include "util.h"

/* Device enabled */
static int tape_diag_586220_harness_enabled = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void tape_diag_586220_harness_set_motor(int flag);
static void tape_diag_586220_harness_toggle_write_bit(int write_bit);
static void tape_diag_586220_harness_set_sense_out(int sense);
static void tape_diag_586220_harness_set_read_out(int val);

static tapeport_device_t tape_diag_586220_harness_device = {
    TAPEPORT_DEVICE_TAPE_DIAG_586220_HARNESS,
    "Tape 586220 diagnostics harness module",
    IDGS_TAPE_DIAG_586220_HARNESS,
    0,
    "TapeDiag586220Harness",
    NULL,
    tape_diag_586220_harness_set_motor,
    tape_diag_586220_harness_toggle_write_bit,
    tape_diag_586220_harness_set_sense_out,
    tape_diag_586220_harness_set_read_out,
    NULL, /* no passthrough */
    NULL, /* no passthrough */
    NULL, /* no passthrough */
    NULL  /* no passthrough */
};

static tapeport_device_list_t *tape_diag_586220_harness_list_item = NULL;

/* ------------------------------------------------------------------------- */

static void tape_diag_586220_harness_set_motor(int flag)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_MOTOR, (BYTE)flag);
}

static void tape_diag_586220_harness_toggle_write_bit(int write_bit)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_WRITE, (BYTE)write_bit);
}

static void tape_diag_586220_harness_set_sense_out(int sense)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_SENSE, (BYTE)sense);
}

static void tape_diag_586220_harness_set_read_out(int read)
{
    c64_diag_586220_store_tapeport(C64_DIAG_TAPEPORT_READ, (BYTE)read);
}

/* ------------------------------------------------------------------------- */

static int set_tape_diag_586220_harness_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (tape_diag_586220_harness_enabled == val) {
        return 0;
    }

    if (val) {
        tape_diag_586220_harness_list_item = tapeport_device_register(&tape_diag_586220_harness_device);
        if (tape_diag_586220_harness_list_item == NULL) {
            return -1;
        }
    } else {
        tapeport_device_unregister(tape_diag_586220_harness_list_item);
        tape_diag_586220_harness_list_item = NULL;
    }

    tape_diag_586220_harness_enabled = val;
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_int[] = {
    { "TapeDiag586220Harness", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tape_diag_586220_harness_enabled, set_tape_diag_586220_harness_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int tape_diag_586220_harness_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-tapediag586220harness", SET_RESOURCE, 0,
      NULL, NULL, "TapeDiag586220Harness", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TAPE_DIAG_586220_HARNESS,
      NULL, NULL },
    { "+tapediag586220harness", SET_RESOURCE, 0,
      NULL, NULL, "TapeDiag586220Harness", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TAPE_DIAG_586220_HARNESS,
      NULL, NULL },
    CMDLINE_LIST_END
};

int tape_diag_586220_harness_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
