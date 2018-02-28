/*
 * sampler2bit.c - 2bit joyport sampler emulation.
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
#include "translate.h"

/* Control port <--> 2bit sampler connections:

   cport | 2bit sampler | I/O
   --------------------------
     1   | D0           |  I
     2   | D1           |  I

 */

static int sampler_enabled = 0;

static int joyport_sampler_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == sampler_enabled) {
        return 0;
    }

    if (val) {
        sampler_start(SAMPLER_OPEN_MONO, "2bit control port sampler");
    } else {
        sampler_stop();
    }

    sampler_enabled = val;

    return 0;
}

static BYTE joyport_sampler_read(int port)
{
    BYTE retval = 0;

    if (sampler_enabled) {
        retval = sampler_get_sample(SAMPLER_CHANNEL_DEFAULT) >> 6;
        joyport_display_joyport(JOYPORT_ID_SAMPLER_2BIT, retval);
        return (BYTE)(~retval);
    }
    return 0xff;
}

static joyport_t joyport_sampler_device = {
    "Sampler (2bit)",
    IDGS_SAMPLER_2BIT,
    JOYPORT_RES_ID_SAMPLER,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_sampler_enable,
    joyport_sampler_read,
    NULL,               /* no store digital */
    NULL,               /* no pot-x read */
    NULL,               /* no pot-y read */
    NULL,               /* no data for a snapshot */
    NULL                /* no data for a snapshot */
};

/* currently only used to register the joyport device */
int joyport_sampler2bit_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_SAMPLER_2BIT, &joyport_sampler_device);
}
