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
#include "userport.h"
#include "userport_4bit_sampler.h"

int userport_4bit_sampler_enabled = 0;

int userport_4bit_sampler_read = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_4bit_sampler_read_pbx(void);
static void userport_4bit_sampler_store_pa2(uint8_t value);
static int userport_4bit_sampler_write_snapshot_module(snapshot_t *s);
static int userport_4bit_sampler_read_snapshot_module(snapshot_t *s);

static userport_device_t sampler_device = {
    USERPORT_DEVICE_4BIT_SAMPLER,    /* device id */
    "Userport 4bit sampler",         /* device name */
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
    "Userport4bitSampler",           /* resource used by the device */
    0xff,                            /* return value from a read, to be filled in by the device */
    0xf0,                            /* validity mask of the device, doesn't change */
    0,                               /* device involved in a read collision, to be filled in by the collision detection system */
    0                                /* a tag to indicate the order of insertion */
};

static userport_snapshot_t sampler_snapshot = {
    USERPORT_DEVICE_4BIT_SAMPLER,
    userport_4bit_sampler_write_snapshot_module,
    userport_4bit_sampler_read_snapshot_module
};

static userport_device_list_t *userport_4bit_sampler_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_userport_4bit_sampler_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (userport_4bit_sampler_enabled == val) {
        return 0;
    }

    if (val) {
        sampler_start(SAMPLER_OPEN_MONO, "4bit userport sampler");
        userport_4bit_sampler_list_item = userport_device_register(&sampler_device);
        if (userport_4bit_sampler_list_item == NULL) {
            sampler_stop();
            return -1;
        }
    } else {
        userport_device_unregister(userport_4bit_sampler_list_item);
        userport_4bit_sampler_list_item = NULL;
        sampler_stop();
    }

    userport_4bit_sampler_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "Userport4bitSampler", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &userport_4bit_sampler_enabled, set_userport_4bit_sampler_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int userport_4bit_sampler_resources_init(void)
{
    userport_snapshot_register(&sampler_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userport4bitsampler", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Userport4bitSampler", (resource_value_t)1,
      NULL, "Enable Userport 4bit sampler" },
    { "+userport4bitsampler", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Userport4bitSampler", (resource_value_t)0,
      NULL, "Disable Userport 4bit sampler" },
    CMDLINE_LIST_END
};

int userport_4bit_sampler_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static void userport_4bit_sampler_store_pa2(uint8_t value)
{
    userport_4bit_sampler_read = value & 1;
}

static void userport_4bit_sampler_read_pbx(void)
{
    uint8_t retval = 0xf0;

    if (!userport_4bit_sampler_read) {
        retval = sampler_get_sample(SAMPLER_CHANNEL_DEFAULT) & 0xf0;
    }
    sampler_device.retval = retval;
}

/* ---------------------------------------------------------------------*/

static int userport_4bit_sampler_write_snapshot_module(snapshot_t *s)
{
    /* No data to save */
    return 0;
}

static int userport_4bit_sampler_read_snapshot_module(snapshot_t *s)
{
    /* No data to load, this is used to enable the device when loading a snapshot */
    set_userport_4bit_sampler_enabled(1, NULL);

    return 0;
}
