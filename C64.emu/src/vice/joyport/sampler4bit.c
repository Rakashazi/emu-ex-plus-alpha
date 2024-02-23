/*
 * sampler4bit.c - 4bit joyport sampler emulation.
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
#include "sampler.h"

#include "sampler4bit.h"

/* Control port <--> 4bit sampler connections:

   cport | 4bit sampler | I/O
   --------------------------
     1   | D0           |  I
     2   | D1           |  I
     3   | D2           |  I
     4   | D3           |  I
     7   | +5VDC        |  Power
     8   | GND          |  Ground

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0/xplus4)
   - inception joystick adapter ports (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
 */

static int sampler_enabled = 0;

static int joyport_sampler_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == sampler_enabled) {
        return 0;
    }

    if (new_state) {
        /* enabled, start the sampler module in mono mode */
        sampler_start(SAMPLER_OPEN_MONO, "4bit control port sampler");
    } else {
        /* disabled, stop the sampler module */
        sampler_stop();
    }

    /* set current state */
    sampler_enabled = new_state;

    return 0;
}

static uint8_t joyport_sampler_read(int port)
{
    uint8_t retval = 0;

    if (sampler_enabled) {
        /* get 8bit sample and only keep the 4 highest bits */
        retval = sampler_get_sample(SAMPLER_CHANNEL_DEFAULT) >> 4;
        joyport_display_joyport(port, JOYPORT_ID_SAMPLER_4BIT, (uint16_t)retval);
        return (uint8_t)(~retval);
    }
    return 0xff;
}

static joyport_t joyport_sampler_device = {
    "Sampler (4bit)",            /* name of the device */
    JOYPORT_RES_ID_SAMPLER,      /* device is a sampler, only 1 sampler can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,     /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,        /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,       /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,    /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_SAMPLER,      /* device is a Sampler */
    0,                           /* NO output bits */
    joyport_sampler_set_enabled, /* device enable function */
    joyport_sampler_read,        /* digital line read function */
    NULL,                        /* NO digital line store function */
    NULL,                        /* NO pot-x read function */
    NULL,                        /* NO pot-x read function */
    NULL,                        /* NO powerup function */
    NULL,                        /* NO device write snapshot function */
    NULL,                        /* NO device read snapshot function */
    NULL,                        /* NO device hook function */
    0                            /* NO device hook function mask */
};

/* currently only used to register the joyport device */
int joyport_sampler4bit_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_SAMPLER_4BIT, &joyport_sampler_device);
}
