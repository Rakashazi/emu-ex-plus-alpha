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
#include "translate.h"
#include "uiapi.h"
#include "userport.h"
#include "userport_rtc_ds1307.h"


int userport_rtc_ds1307_enabled = 0;

/* rtc context */
static rtc_ds1307_t *ds1307_context = NULL;

/* rtc save */
static int ds1307_rtc_save;

static BYTE ds1307_pb0_sda = 1;
static BYTE ds1307_pb1_scl = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_rtc_read_pbx(void);
static void userport_rtc_store_pbx(BYTE value);
static int userport_rtc_write_snapshot_module(snapshot_t *s);
static int userport_rtc_read_snapshot_module(snapshot_t *s);

static userport_device_t rtc_device = {
    USERPORT_DEVICE_RTC_DS1307,
    "Userport RTC (DS1307)",
    IDGS_USERPORT_DS1307,
    userport_rtc_read_pbx,
    userport_rtc_store_pbx,
    NULL, /* NO pa2 read */
    NULL, /* NO pa2 write */
    NULL, /* NO pa3 read */
    NULL, /* NO pa3 write */
    0, /* NO pc pin needed */
    NULL, /* NO sp1 write */
    NULL, /* NO sp1 read */
    NULL, /* NO sp2 write */
    NULL, /* NO sp2 read */
    "UserportRTC",
    0xff,
    0x3, /* validity mask doesn't change */
    0,
    0
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
    { "-userportrtcds1307", SET_RESOURCE, 0,
      NULL, NULL, "UserportRTCDS1307", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_RTC_DS1307,
      NULL, NULL },
    { "+userportrtcds1307", SET_RESOURCE, 0,
      NULL, NULL, "UserportRTCDS1307", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_RTC_DS1307,
      NULL, NULL },
    { "-userportrtcds1307save", SET_RESOURCE, 0,
      NULL, NULL, "UserportRTCDS1307Save", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_RTC_DS1307_SAVE,
      NULL, NULL },
    { "+userportrtcds1307save", SET_RESOURCE, 0,
      NULL, NULL, "UserportRTCDS1307Save", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_RTC_DS1307_SAVE,
      NULL, NULL },
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

static void userport_rtc_store_pbx(BYTE value)
{
    BYTE rtcdata = (value & 1) ? 1 : 0;
    BYTE rtcclk = (value & 2) ? 1 : 0;

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
    BYTE retval = ds1307_pb1_scl << 1;

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
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_userport_rtc_enabled(1, NULL);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
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
