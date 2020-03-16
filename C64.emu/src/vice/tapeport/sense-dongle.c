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
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"

#include "sense-dongle.h"


static int sense_dongle_enabled = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void sense_dongle_reset(void);
static int sense_dongle_write_snapshot(struct snapshot_s *s, int write_image);
static int sense_dongle_read_snapshot(struct snapshot_s *s);

static tapeport_device_t sense_dongle_device = {
    TAPEPORT_DEVICE_SENSE_DONGLE, /* device id */
    "Sense dongle",               /* device name */
    0,                            /* order of the device, filled in by the tapeport system when the device is attached */
    "TapeSenseDongle",            /* resource used by the device */
    NULL,                         /* NO device shutdown function */
    sense_dongle_reset,           /* device specific reset function */
    NULL,                         /* NO set motor line function */
    NULL,                         /* NO set write line function */
    NULL,                         /* NO set sense line function */
    NULL,                         /* NO set read line function */
    NULL,                         /* NO passthrough flux change function */
    NULL,                         /* NO passthrough sense read function */
    NULL,                         /* NO passthrough write line function */
    NULL                          /* NO passthrough motor line function */
};

static tapeport_snapshot_t sense_dongle_snapshot = {
    TAPEPORT_DEVICE_SENSE_DONGLE,
    sense_dongle_write_snapshot,
    sense_dongle_read_snapshot
};

static tapeport_device_list_t *sense_dongle_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_sense_dongle_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (sense_dongle_enabled == val) {
        return 0;
    }

    if (val) {
        sense_dongle_list_item = tapeport_device_register(&sense_dongle_device);
        if (sense_dongle_list_item == NULL) {
            return -1;
        }
        tapeport_set_tape_sense(1, sense_dongle_device.id);
    } else {
        tapeport_device_unregister(sense_dongle_list_item);
        sense_dongle_list_item = NULL;
    }

    sense_dongle_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "TapeSenseDongle", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &sense_dongle_enabled, set_sense_dongle_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int sense_dongle_resources_init(void)
{
    tapeport_snapshot_register(&sense_dongle_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-tapesensedongle", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapeSenseDongle", (resource_value_t)1,
      NULL, "Enable tape sense dongle" },
    { "+tapesensedongle", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapeSenseDongle", (resource_value_t)0,
      NULL, "Disable tape sense dongle" },
    CMDLINE_LIST_END
};

int sense_dongle_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static void sense_dongle_reset(void)
{
    tapeport_set_tape_sense(1, sense_dongle_device.id);
}

/* ---------------------------------------------------------------------*/

static int sense_dongle_write_snapshot(struct snapshot_s *s, int write_image)
{
    /* No data to write */
    return 0;
}

static int sense_dongle_read_snapshot(struct snapshot_s *s)
{
    /* No data to read, we use this to enable the device when reading a snapshot */

    /* enable device */
    set_sense_dongle_enabled(1, NULL);

    return 0;
}
