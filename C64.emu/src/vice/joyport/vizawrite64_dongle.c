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


/* ------------------------------------------------------------------------- */

static int joyport_vizawrite64_dongle_enabled = 0;

static int counter = 0;

static uint8_t values[6] = {
    0x55, 0x55, 0xaa, 0xaa, 0xff, 0xff
};

static int joyport_vizawrite64_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    joyport_vizawrite64_dongle_enabled = val;

    return 0;
}

static uint8_t vizawrite64_dongle_read_potx(void)
{
    uint8_t retval = values[counter++];

    if (counter == 6) {
        counter = 0;
    }

    return retval;
}

static uint8_t vizawrite64_dongle_read_poty(void)
{
    uint8_t retval = values[counter++];

    if (counter == 6) {
        counter = 0;
    }

    return retval;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_vizawrite64_dongle_device = {
    "VizaWrite 64 dongle",             /* name of the device */
    JOYPORT_RES_ID_VIZAWRITE64,        /* device is of the vizawrite64 type, only 1 of this type can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,           /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,              /* device uses the potentiometer lines */
    joyport_vizawrite64_dongle_enable, /* device enable function */
    NULL,                              /* NO digital line read function */
    NULL,                              /* NO digital line store function */
    vizawrite64_dongle_read_potx,      /* pot-x read function */
    vizawrite64_dongle_read_poty,      /* pot-y read function */
    NULL,                              /* NO device write snapshot function */
    NULL                               /* NO device read snapshot function */
};

/* ------------------------------------------------------------------------- */

int joyport_vizawrite64_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_VIZAWRITE64_DONGLE, &joyport_vizawrite64_dongle_device);
}
