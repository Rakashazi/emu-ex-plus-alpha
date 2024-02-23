/*
 * paperclip2.c - Paperclip 2 joyport dongle emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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
#include "paperclip2.h"
#include "resources.h"
#include "snapshot.h"

/* Control port <--> paperclip2 connections:

   cport | paperclip2  | I/O
   -------------------------
     1   |    PROM O0  |  I
     2   |    PROM O1  |  I
     3   | COUNTER CLK |  O
     4   | COUNTER CLR |  O
     6   |   PROM CE   |  O
     7   |    +5VDC    |  Power
     8   |     GND     |  Ground

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128)
 */

/* Paperclip 2 Dongle description:

   This emulation works only for the "paperclip ii" files.

   Documentation/information used for making the emulation:

   https://c64preservation.com/index.php/bbs/dm.php?thread=1579

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

   The "key" sequence is found below in the code; it has 64 values.
*/

static int paperclip2_enabled[JOYPORT_MAX_PORTS] = {0};

static int counter[JOYPORT_MAX_PORTS] = {0};

static uint8_t command[JOYPORT_MAX_PORTS] = {0xff};
static uint8_t output_enable[JOYPORT_MAX_PORTS] = {0};

static const uint8_t keys[64] = {
   0, 3, 2, 3, 1, 1, 2, 3,
   2, 2, 0, 2, 2, 3, 2, 3,
   0, 1, 2, 2, 1, 2, 3, 0,
   3, 3, 3, 1, 0, 1, 3, 1,
   1, 3, 2, 1, 1, 0, 1, 3,
   1, 3, 2, 3, 1, 2, 0, 2,
   1, 2, 0, 0, 0, 1, 3, 0,
   0, 2, 3, 0, 3, 2, 3, 2
};

/* ------------------------------------------------------------------------- */

static int joyport_paperclip2_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == paperclip2_enabled[port]) {
        return 0;
    }

    if (new_state) {
        /* enabled, set command to 0 */
        command[port] = 0;
    }

    /* set current state */
    paperclip2_enabled[port] = new_state;

    return 0;
}

static uint8_t paperclip2_read(int port)
{
    uint8_t retval = 0xff;

    if (output_enable[port]) {
        /* if the output is enabled, return current key bits */
        retval &= (keys[counter[port]] | 0xfc);
    }
    return retval;
}

static void paperclip2_store(int port, uint8_t val)
{
    uint8_t new_command = val & 0x1c;
    uint8_t reset;
    uint8_t clk;
    uint8_t old_clk;

    if (new_command == command[port]) {
        return;
    }

    output_enable[port] = !(val & JOYPORT_FIRE);   /* output enable line is on joyport 'fire' pin */

    reset = !(val & 8);

    if (reset) {
        /* reset line asserted, set counter to 0 */
        counter[port] = 0;
    } else {
        clk = val & JOYPORT_LEFT;   /* clock line is on joyport 'left' pin */
        old_clk = command[port] & JOYPORT_LEFT;

        if (old_clk && !clk) {
            /* clock line asserted, increment key position */
            counter[port]++;
            if (counter[port] == 64) {
                counter[port] = 0;
            }
        }
   }
   command[port] = new_command;
}

static void paperclip2_powerup(int port)
{
    /* reset counter */
    counter[port] = 0;
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int paperclip2_write_snapshot(struct snapshot_s *s, int port);
static int paperclip2_read_snapshot(struct snapshot_s *s, int port);

static joyport_t joyport_paperclip2_device = {
    "Dongle (Paperclip2)",         /* name of the device */
    JOYPORT_RES_ID_NONE,           /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,       /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,          /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,         /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,      /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_C64_DONGLE,     /* device is a C64 Dongle */
    0x1C,                          /* bits 4, 3 and 2 are output bits */
    joyport_paperclip2_set_enabled,/* device enable/disable function */
    paperclip2_read,               /* digital line read function */
    paperclip2_store,              /* digital line store function */
    NULL,                          /* NO pot-x read function */
    NULL,                          /* NO pot-y read function */
    paperclip2_powerup,            /* powerup function */
    paperclip2_write_snapshot,     /* device write snapshot function */
    paperclip2_read_snapshot,      /* device read snapshot function */
    NULL,                          /* NO device hook function */
    0                              /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_paperclip2_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_PAPERCLIP2, &joyport_paperclip2_device);
}

/* ------------------------------------------------------------------------- */

/* PAPERCLIP2 snapshot module format:

   type  | name    | description
   -----------------------------
   DWORD | counter | counter
   BYTE  | command | command
   BYTE  | state   | state
 */

static const char snap_module_name[] = "PAPERCLIP2";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int paperclip2_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DW(m, (uint32_t)counter[port]) < 0
        || SMW_B(m, command[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int paperclip2_read_snapshot(struct snapshot_s *s, int port)
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
        || SMR_DW_INT(m, &counter[port]) < 0
        || SMR_B(m, &command[port]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
