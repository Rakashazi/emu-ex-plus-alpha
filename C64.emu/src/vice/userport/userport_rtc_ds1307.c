/*
 * userport_rtc_ds1307.c - Generic userport RTC (DS1307) emulation.
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

/* Userport RTC DS1307 (C64/C128/CBM2/PET/VIC20)

C64/C128 | CBM2 | PET | VIC20 | NAME
------------------------------------
    C    |  14  |  C  |   C   | PB0 <-> SDA
    D    |  13  |  D  |   D   | PB1 <-> SCL
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "ds1307.h"
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "uiapi.h"
#include "userport.h"
#include "userport_rtc_ds1307.h"


int userport_rtc_ds1307_enabled = 0;

/* rtc context */
static rtc_ds1307_t *ds1307_context = NULL;

/* rtc save */
static int ds1307_rtc_save;

static uint8_t ds1307_pb0_sda = 1;
static uint8_t ds1307_pb1_scl = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_rtc_read_pbx(void);
static void userport_rtc_store_pbx(uint8_t value);
static int userport_rtc_write_snapshot_module(snapshot_t *s);
static int userport_rtc_read_snapshot_module(snapshot_t *s);

static userport_device_t rtc_device = {
    USERPORT_DEVICE_RTC_DS1307, /* device id */
    "Userport RTC (DS1307)",    /* device name */
    userport_rtc_read_pbx,      /* read pb0-pb7 function */
    userport_rtc_store_pbx,     /* store pb0-pb7 function */
    NULL,                       /* NO read pa2 pin function */
    NULL,                       /* NO store pa2 pin function */
    NULL,                       /* NO read pa3 pin function */
    NULL,                       /* NO store pa3 pin function */
    0,                          /* pc pin is NOT needed */
    NULL,                       /* NO store sp1 pin function */
    NULL,                       /* NO read sp1 pin function */
    NULL,                       /* NO store sp2 pin function */
    NULL,                       /* NO read sp2 pin function */
    "UserportRTC",              /* resource used by the device */
    0xff,                       /* return value from a read, to be filled in by the device */
    0x03,                       /* validity mask of the device, doesn't change */
    0,                          /* device involved in a read collision, to be filled in by the collision detection system */
    0                           /* a tag to indicate the order of insertion */
};

static userport_snapshot_t rtc_snapshot = {
    USERPORT_DEVICE_RTC_DS1307,
    userport_rtc_write_snapshot_module,
    userport_rtc_read_snapshot_module
};

static userport_device_list_t *userport_rtc_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_userport_rtc_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (userport_rtc_ds1307_enabled == val) {
        return 0;
    }

    if (val) {
        ds1307_context = ds1307_init("USERDS1307");
        userport_rtc_list_item = userport_device_register(&rtc_device);
        if (userport_rtc_list_item == NULL) {
            return -1;
        }
        ds1307_set_data_line(ds1307_context, 1);
        ds1307_set_clk_line(ds1307_context, 1);
    } else {
        if (ds1307_context) {
            ds1307_destroy(ds1307_context, ds1307_rtc_save);
            ds1307_context = NULL;
        }
        userport_device_unregister(userport_rtc_list_item);
        userport_rtc_list_item = NULL;
    }

    userport_rtc_ds1307_enabled = val;
    return 0;
}

static int set_userport_rtc_save(int val, void *param)
{
    ds1307_rtc_save = val ? 1 : 0;

    return 0;
}


static const resource_int_t resources_int[] = {
    { "UserportRTCDS1307", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &userport_rtc_ds1307_enabled, set_userport_rtc_enabled, NULL },
    { "UserportRTCDS1307Save", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ds1307_rtc_save, set_userport_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int userport_rtc_ds1307_resources_init(void)
{
    userport_snapshot_register(&rtc_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userportrtcds1307", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTCDS1307", (resource_value_t)1,
      NULL, "Enable Userport RTC (DS1307)" },
    { "+userportrtcds1307", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTCDS1307", (resource_value_t)0,
      NULL, "Disable Userport RTC (DS1307)" },
    { "-userportrtcds1307save", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTCDS1307Save", (resource_value_t)1,
      NULL, "Enable saving of the Userport RTC (DS1307) data when changed." },
    { "+userportrtcds1307save", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTCDS1307Save", (resource_value_t)0,
      NULL, "Disable saving of the Userport RTC (DS1307) data when changed." },
    CMDLINE_LIST_END
};

int userport_rtc_ds1307_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

void userport_rtc_ds1307_resources_shutdown(void)
{
    if (ds1307_context) {
        ds1307_destroy(ds1307_context, ds1307_rtc_save);
        ds1307_context = NULL;
    }
}

/* ---------------------------------------------------------------------*/

static void userport_rtc_store_pbx(uint8_t value)
{
    uint8_t rtcdata = (value & 1) ? 1 : 0;
    uint8_t rtcclk = (value & 2) ? 1 : 0;

    if (rtcdata != ds1307_pb0_sda) {
        ds1307_set_data_line(ds1307_context, rtcdata);
        ds1307_pb0_sda = rtcdata;
    }
    if (rtcclk != ds1307_pb1_scl) {
        ds1307_set_clk_line(ds1307_context, rtcclk);
        ds1307_pb1_scl = rtcclk;
    }
}

static void userport_rtc_read_pbx(void)
{
    uint8_t retval = ds1307_pb1_scl << 1;

    retval |= (ds1307_read_data_line(ds1307_context) & 1);

    rtc_device.retval = retval;
}

/* ---------------------------------------------------------------------*/

/* UP_RTC_DS1307 snapshot module format:

   type  | name | description
   --------------------------
   BYTE  | SDA  | SDA line state
   BYTE  | SCL  | SCL line state
 */

static char snap_module_name[] = "UP_RTC_DS1307";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int userport_rtc_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, ds1307_pb0_sda) < 0
        || SMW_B(m, ds1307_pb1_scl) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    return ds1307_write_snapshot(ds1307_context, s);
}

static int userport_rtc_read_snapshot_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_rtc_enabled(1, NULL);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &ds1307_pb0_sda) < 0
        || SMR_B(m, &ds1307_pb1_scl) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    return ds1307_read_snapshot(ds1307_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
