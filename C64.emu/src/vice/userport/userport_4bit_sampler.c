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
#include "translate.h"
#include "userport.h"
#include "userport_4bit_sampler.h"

int userport_4bit_sampler_enabled = 0;

int userport_4bit_sampler_read = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_4bit_sampler_read_pbx(void);
static void userport_4bit_sampler_store_pa2(BYTE value);
static int userport_4bit_sampler_write_snapshot_module(snapshot_t *s);
static int userport_4bit_sampler_read_snapshot_module(snapshot_t *s);

static userport_device_t sampler_device = {
    USERPORT_DEVICE_4BIT_SAMPLER,
    "Userport 4bit sampler",
    IDGS_USERPORT_4BIT_SAMPLER,
    userport_4bit_sampler_read_pbx,
    NULL, /* NO pbx store */
    NULL, /* NO pa2 read */
    userport_4bit_sampler_store_pa2,
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp1 write */
    NULL, /* NO sp2 read */
    "Userport4bitSampler",
    0xff,
    0xf0, /* valid mask doesn't change */
    0,
    0
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
    { "-userport4bitsampler", SET_RESOURCE, 0,
      NULL, NULL, "Userport4bitSampler", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_4BIT_SAMPLER,
      NULL, NULL },
    { "+userport4bitsampler", SET_RESOURCE, 0,
      NULL, NULL, "Userport4bitSampler", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_4BIT_SAMPLER,
      NULL, NULL },
    CMDLINE_LIST_END
};

int userport_4bit_sampler_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static void userport_4bit_sampler_store_pa2(BYTE value)
{
    userport_4bit_sampler_read = value & 1;
}

static void userport_4bit_sampler_read_pbx(void)
{
    BYTE retval = 0xf0;

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
