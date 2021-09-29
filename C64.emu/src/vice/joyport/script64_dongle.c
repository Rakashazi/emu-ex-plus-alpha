/*
 * script64_dongle.c - Script 64 Dongle emulation.
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
#include "keyboard.h"

#include "script64_dongle.h"


/* Control port <--> Script 64 dongle connections:

   cport | resistor value to pin 5
   -------------------------------
     7   | 100 Kohm
     9   | 27 Kohm
 */

#define POTX_RETURN   0xC0
#define POTY_RETURN   0x80

/* ------------------------------------------------------------------------- */

static int joyport_script64_dongle_enabled = 0;

static int joyport_script64_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    joyport_script64_dongle_enabled = val;

    return 0;
}

static uint8_t script64_dongle_read_potx(int port)
{
    return POTX_RETURN;
}

static uint8_t script64_dongle_read_poty(int port)
{
    return POTY_RETURN;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_script64_dongle_device = {
    "Script 64 dongle",             /* name of the device */
    JOYPORT_RES_ID_SCRIPT64,        /* device is of the script64 type, only 1 of this kind can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,        /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,           /* device uses the potentiometer lines */
    joyport_script64_dongle_enable, /* device enable function */
    NULL,                           /* NO digital line read function */
    NULL,                           /* NO digital line store function */
    script64_dongle_read_potx,      /* pot-x read function */
    script64_dongle_read_poty,      /* pot-y read function */
    NULL,                           /* NO device write snapshot function */
    NULL                            /* NO device read snapshot function */
};

/* ------------------------------------------------------------------------- */

int joyport_script64_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_SCRIPT64_DONGLE, &joyport_script64_dongle_device);
}
