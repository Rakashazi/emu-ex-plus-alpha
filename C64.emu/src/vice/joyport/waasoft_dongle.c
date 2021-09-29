/*
 * waasoft_dongle.c - WaaVizaWrite 64 Dongle emulation.
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

static int joyport_waasoft_dongle_enabled = 0;

static int counter = 0;

static uint8_t waasoft_reset_line = 1;
static uint8_t waasoft_clock_line  = 1;

static uint8_t waasoft_values[15] = {
	0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00
};

static int joyport_waasoft_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    joyport_waasoft_dongle_enabled = val;

    return 0;
}

static uint8_t waasoft_dongle_read_poty(int port)
{
    return waasoft_values[counter];
}

static void waasoft_dongle_store_dig(uint8_t val)
{
    uint8_t reset = val & 1;
    uint8_t clock = val & 2;

    if (clock != waasoft_clock_line) {
        if (!clock) {
            counter++;
            if (counter == 15) {
                counter = 0;
            }
        }
    }

    waasoft_clock_line = clock;

    if (reset != waasoft_reset_line) {
        if (!reset) {
            counter = 0;
        }
    }

    waasoft_reset_line = reset;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_waasoft_dongle_device = {
    "WaaSoft dongle",              /* name of the device */
    JOYPORT_RES_ID_WAASOFT,        /* device is of the waasoft type, only 1 of this type can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,       /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,          /* device uses the potentiometer lines */
    joyport_waasoft_dongle_enable, /* device enable function */
    NULL,                          /* NO digital line read function */
    waasoft_dongle_store_dig,      /* digital line store function */
    NULL,                          /* NO pot-x read function */
    waasoft_dongle_read_poty,      /* pot-y read function */
    NULL,                          /* NO device write snapshot function */
    NULL                           /* NO device read snapshot function */
};

/* ------------------------------------------------------------------------- */

int joyport_waasoft_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_WAASOFT_DONGLE, &joyport_waasoft_dongle_device);
}
