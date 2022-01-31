/*
 * sense-dongle.c - tape port dongle that asserts the sense line.
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

#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"

#include "sense-dongle.h"


static int sense_dongle_enabled[TAPEPORT_MAX_PORTS] = { 0 };

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void sense_dongle_powerup(int port);
static int sense_dongle_enable(int port, int val);

static tapeport_device_t sense_dongle_device = {
    "Sense dongle",              /* device name */
    TAPEPORT_DEVICE_TYPE_DONGLE, /* device is a 'dongle' type device */
    VICE_MACHINE_ALL,            /* device works on all machines */
    TAPEPORT_PORT_ALL_MASK,      /* device works on all ports */
    sense_dongle_enable,         /* device enable function */
    sense_dongle_powerup,        /* device specific hard reset function */
    NULL,                        /* NO device shutdown function */
    NULL,                        /* NO set motor line function */
    NULL,                        /* NO set write line function */
    NULL,                        /* NO set sense line function */
    NULL,                        /* NO set read line function */
    NULL,                        /* NO device snapshot write function */
    NULL                         /* NO device snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int sense_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (sense_dongle_enabled[port] == val) {
        return 0;
    }

    if (val) {
        tapeport_set_tape_sense(1, port);
    }

    sense_dongle_enabled[port] = val;
    return 0;
}

int sense_dongle_resources_init(int amount)
{
    return tapeport_device_register(TAPEPORT_DEVICE_SENSE_DONGLE, &sense_dongle_device);
}

/* ---------------------------------------------------------------------*/

static void sense_dongle_powerup(int port)
{
    tapeport_set_tape_sense(1, port);
}
