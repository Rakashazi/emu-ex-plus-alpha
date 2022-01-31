/*
 * userport_4bit_sampler.c - Generic userport 4bit sampler emulation.
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

/* - 4bit sampler (C64/C128/CBM2)

C64/C128 | CBM2 | ADC | NOTES
-----------------------------
    H    |  10  | 14  | PB4 <- D4
    J    |   9  | 15  | PB5 <- D5
    K    |   8  | 16  | PB6 <- D6
    L    |   7  | 17  | PB7 <- D7
    M    |   2  |  8  | PA2 -> /RD
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "resources.h"
#include "sampler.h"
#include "snapshot.h"
#include "joyport.h"
#include "userport.h"
#include "userport_4bit_sampler.h"

int userport_4bit_sampler_enabled = 0;

int userport_4bit_sampler_read = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t userport_4bit_sampler_read_pbx(uint8_t orig);
static void userport_4bit_sampler_store_pa2(uint8_t value);
static int userport_4bit_sampler_enable(int value);

static userport_device_t sampler_device = {
    "Userport 4bit sampler",         /* device name */
    JOYSTICK_ADAPTER_ID_NONE,        /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_SAMPLER,    /* device is a sampler */
    userport_4bit_sampler_enable,    /* enable function */
    userport_4bit_sampler_read_pbx,  /* read pb0-pb7 function */
    NULL,                            /* NO store pb0-pb7 function */
    NULL,                            /* NO read pa2 pin function */
    userport_4bit_sampler_store_pa2, /* store pa2 pin function */
    NULL,                            /* NO read pa3 pin function */
    NULL,                            /* NO store pa3 pin function */
    0,                               /* pc pin is NOT needed */
    NULL,                            /* NO store sp1 pin function */
    NULL,                            /* NO read sp1 pin function */
    NULL,                            /* NO store sp2 pin function */
    NULL,                            /* NO read sp2 pin function */
    NULL,                            /* NO reset function */
    NULL,                            /* NO powerup function */
    NULL,                            /* NO snapshot write function */
    NULL                             /* NO snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_4bit_sampler_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_4bit_sampler_enabled == val) {
        return 0;
    }

    if (val) {
        sampler_start(SAMPLER_OPEN_MONO, "4bit userport sampler");
    } else {
        sampler_stop();
    }

    userport_4bit_sampler_enabled = val;
    return 0;
}

int userport_4bit_sampler_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_4BIT_SAMPLER, &sampler_device);
}

/* ---------------------------------------------------------------------*/

static void userport_4bit_sampler_store_pa2(uint8_t value)
{
    userport_4bit_sampler_read = value & 1;
}

static uint8_t userport_4bit_sampler_read_pbx(uint8_t orig)
{
    uint8_t retval = orig;

    if (!userport_4bit_sampler_read) {
        retval = sampler_get_sample(SAMPLER_CHANNEL_DEFAULT) & 0xf0;
    }
    return retval;
}
