/*
 * paperclip64c.c - Paperclip 64 joyport dongle emulation.
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
#include "paperclip64.h"
#include "resources.h"
#include "snapshot.h"

/* Control port <--> paperclip64 connections:

   cport | paperclip64 | I/O
   -------------------------
     1   |    PROM O0  |  I
     2   |    PROM O1  |  I
     3   | COUNTER CLK |  O
     4   | COUNTER CLR |  O
     6   |   PROM CE   |  O
 */

/* Paperclip64D Dongle description:

   This emulation currently does not work for the software using it,
   help/more information is needed to fix this.

   Documentation/information used for making the emulation:

   http://c64preservation.com/c64pp/bbs/dm.php?thread=1579

   The current emulation does the following:

   1- wait for the bit 4, 3 and 2 of the joystick port to become xxx000xx
   2- wait for the bit 4, 3 and 2 of the joystick port to become xxx010xx
   3- wait for the bit 4, 3 and 2 of the joystick port to become xxx111xx
   4- wait for the bit 4, 3 and 2 of the joystick port to become xxx010xx
   5- set the first bit-pair of the key on bits 1 and 0 of the joystick port.

   The key sequence is : 02 00 00 01 03 02 01 03 02 01 02 01 02 01 02 00
                         01 02 00 01 03 03 02 00 00 00 00 00 00 00 01 03
                         03 02 00 01 02 00 01 02 01 02 01 02 01 03 02 01
                         03 02 00 00 01 03 03 03 03 03 03 03 03 03 03 03

   After setting the first bits of the sequence the following needs to happen before setting the next bit:

   1- wait for the bit 4, 3 and 2 of the joystick port to become xxx111xx
   2- wait for the bit 4, 3 and 2 of the joystick port to become xxx010xx

   Be aware that the current emulation keeps the 'old' bits set between steps 1 and 2.

   The emulation 'shows' the correct bit pattern when using the monitor to check what the code gets from the port,
   but it does not work (good enough) for the actual software.
*/

static int paperclip64_enabled = 0;

static int counter = 0;

static uint8_t command = 0xff;
static uint8_t output_enable = 0;

static uint8_t keys[64] = {
    3, 2, 0, 0, 1, 3, 2, 1,
    3, 2, 1, 2, 1, 2, 1, 2,
    0, 1, 2, 0, 1, 3, 3, 2,
    0, 0, 0, 0, 0, 0, 0, 1,
    3, 3, 2, 0, 1, 2, 0, 1,
    2, 1, 2, 1, 2, 1, 3, 2,
    1, 3, 2, 0, 0, 1, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3
};

/* ------------------------------------------------------------------------- */

static int joyport_paperclip64_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == paperclip64_enabled) {
        return 0;
    }

    if (val) {
        command = 0;
    }

    paperclip64_enabled = val;

    return 0;
}

static uint8_t paperclip64_read(int port)
{
    uint8_t retval = 0xff;

    if (output_enable) {
        retval &= (keys[counter] | 0xfc);
        joyport_display_joyport(JOYPORT_ID_BBRTC, (uint8_t)(~retval & 3));
    }
    return retval;
}

static void paperclip64_store(uint8_t val)
{
    uint8_t new_command = val & 0x1c;
    uint8_t reset;
    uint8_t clk;
    uint8_t old_clk;

    if (new_command == command) {
        return;
    }

    output_enable = !(val & 0x10);

    reset = !(val & 8);

    if (reset) {
        counter = 0;
    } else {
        clk = val & 4;
        old_clk = command & 4;

        if (old_clk && !clk) {
            counter++;
            if (counter == 0x3c) {
                counter = 0;
            }
        }
   }
   command = new_command;
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int paperclip64_write_snapshot(struct snapshot_s *s, int port);
static int paperclip64_read_snapshot(struct snapshot_s *s, int port);

static joyport_t joyport_paperclip64_device = {
    "Paperclip64 dongle",       /* name of the device */
    JOYPORT_RES_ID_PAPERCLIP64, /* device is of the paperclip64 type, only 1 device of this kind can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,    /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,       /* device does NOT use the potentiometer lines */
    joyport_paperclip64_enable, /* device enable function */
    paperclip64_read,           /* digital line read function */
    paperclip64_store,          /* digital line store function */
    NULL,                       /* NO pot-x read function */
    NULL,                       /* NO pot-y read function */
    paperclip64_write_snapshot, /* device write snapshot function */
    paperclip64_read_snapshot   /* device read snapshot function */
};

/* ------------------------------------------------------------------------- */

int joyport_paperclip64_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_PAPERCLIP64, &joyport_paperclip64_device);
}

/* ------------------------------------------------------------------------- */

/* PAPERCLIP64 snapshot module format:

   type  | name    | description
   -----------------------------
   DWORD | counter | counter
   BYTE  | command | command
   BYTE  | state   | state
 */

static char snap_module_name[] = "PAPERCLIP64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int paperclip64_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DW(m, (uint32_t)counter) < 0
        || SMW_B(m, command) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int paperclip64_read_snapshot(struct snapshot_s *s, int port)
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
        || SMR_DW_INT(m, &counter) < 0
        || SMR_B(m, &command) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
