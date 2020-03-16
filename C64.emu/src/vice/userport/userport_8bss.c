/*
 * userport_8bss.c - Userport 8bit stereo sampler emulation.
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

/* 8bit stereo sampler (C64/C128/CBM2)

C64/C128 | CBM2 | ADC0820-1 | ADC0820-2 | NOTES
-----------------------------------------------
    8    |   5  | 6         | 6         | /PC2 -> /WR & RDY
    9    |   4  | 13        | /13       | /PA3 -> /CS1 & CS2
    C    |  14  | 2         | 2         | PB0 <- DB0
    D    |  13  | 3         | 3         | PB1 <- DB1
    E    |  12  | 4         | 4         | PB2 <- DB2
    F    |  11  | 5         | 5         | PB3 <- DB3
    H    |  10  | 6         | 6         | PB4 <- DB4
    J    |   9  | 7         | 7         | PB5 <- DB5
    K    |   8  | 8         | 8         | PB6 <- DB6
    L    |   7  | 9         | 9         | PB7 <- DB7
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
#include "userport_8bss.h"

int userport_8bss_enabled = 0;

int userport_8bss_channel = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_8bss_read_pbx(void);
static void userport_8bss_store_pa3(uint8_t value);
static int userport_8bss_write_snapshot_module(snapshot_t *s);
static int userport_8bss_read_snapshot_module(snapshot_t *s);

static userport_device_t sampler_device = {
    USERPORT_DEVICE_8BSS,           /* device id */
    "Userport 8bit stereo sampler", /* device name */
    userport_8bss_read_pbx,         /* read pb0-pb7 function */
    NULL,                           /* NO store pb0-pb7 function */
    NULL,                           /* NO read pa2 pin function */
    NULL,                           /* NO store pa2 pin function */
    NULL,                           /* NO read pa3 pin function */
    userport_8bss_store_pa3,        /* store pa3 pin function */
    1,                              /* pc pin is needed */
    NULL,                           /* NO store sp1 pin function */
    NULL,                           /* NO read sp1 pin function */
    NULL,                           /* NO store sp2 pin function */
    NULL,                           /* NO read sp2 pin function */
    "Userport8BSS",                 /* resource used by the device */
    0xff,                           /* return value from a read, to be filled in by the device */
    0xff,                           /* validity mask of the device, doesn't change */
    0,                              /* device involved in a read collision, to be filled in by the collision detection system */
    0                               /* a tag to indicate the order of insertion */
};

static userport_snapshot_t sampler_snapshot = {
    USERPORT_DEVICE_8BSS,
    userport_8bss_write_snapshot_module,
    userport_8bss_read_snapshot_module
};

static userport_device_list_t *userport_8bss_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_userport_8bss_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (userport_8bss_enabled == val) {
        return 0;
    }

    if (val) {
        sampler_start(SAMPLER_OPEN_STEREO, "8bit userport stereo sampler");
        userport_8bss_list_item = userport_device_register(&sampler_device);
        if (userport_8bss_list_item == NULL) {
            sampler_stop();
            return -1;
        }
    } else {
        userport_device_unregister(userport_8bss_list_item);
        userport_8bss_list_item = NULL;
        sampler_stop();
    }

    userport_8bss_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "Userport8BSS", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &userport_8bss_enabled, set_userport_8bss_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int userport_8bss_resources_init(void)
{
    userport_snapshot_register(&sampler_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userport8bss", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Userport8BSS", (resource_value_t)1,
      NULL, "Enable Userport 8bit stereo sampler" },
    { "+userport8bss", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Userport8BSS", (resource_value_t)0,
      NULL, "Disable Userport 8bit stereo sampler" },
    CMDLINE_LIST_END
};

int userport_8bss_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static void userport_8bss_store_pa3(uint8_t value)
{
    userport_8bss_channel = value & 1;
}

static void userport_8bss_read_pbx(void)
{
    uint8_t retval;

    if (userport_8bss_channel) {
        retval = sampler_get_sample(SAMPLER_CHANNEL_1);
    } else {
        retval = sampler_get_sample(SAMPLER_CHANNEL_2);
    }
    sampler_device.retval = retval;
}

/* ---------------------------------------------------------------------*/

/* USERPORT_8BSS snapshot module format:

   type  | name    | description
   -----------------------------
   BYTE  | channel | channel flag
 */

static char snap_module_name[] = "USERPORT_8BSS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int userport_8bss_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)userport_8bss_channel) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

static int userport_8bss_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_8bss_enabled(1, NULL);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &userport_8bss_channel) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
