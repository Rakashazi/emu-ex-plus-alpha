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
#include "translate.h"

/* ------------------------------------------------------------------------- */

static int joyport_vizawrite64_dongle_enabled = 0;

static int counter = 0;

static BYTE values[6] = {
    0x55, 0x55, 0xaa, 0xaa, 0xff, 0xff
};

static int joyport_vizawrite64_dongle_enable(int port, int value)
{
    int val = value ? 1 : 0;

    joyport_vizawrite64_dongle_enabled = val;

    return 0;
}

static BYTE vizawrite64_dongle_read_potx(void)
{
    BYTE retval = values[counter++];

    if (counter == 6) {
        counter = 0;
    }

    return retval;
}

static BYTE vizawrite64_dongle_read_poty(void)
{
    BYTE retval = values[counter++];

    if (counter == 6) {
        counter = 0;
    }

    return retval;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_vizawrite64_dongle_device = {
    "VizaWrite 64 dongle",
    IDGS_VIZAWRITE64_DONGLE,
    JOYPORT_RES_ID_VIZAWRITE64,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_vizawrite64_dongle_enable,
    NULL,               /* no dig read */
    NULL,               /* no dig write */
    vizawrite64_dongle_read_potx,
    vizawrite64_dongle_read_poty,
    NULL,               /* no write snapshot */
    NULL                /* no read snapshot */
};

/* ------------------------------------------------------------------------- */

int joyport_vizawrite64_dongle_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_VIZAWRITE64_DONGLE, &joyport_vizawrite64_dongle_device);
}
