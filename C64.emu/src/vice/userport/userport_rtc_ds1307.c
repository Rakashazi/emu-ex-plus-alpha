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
#include "joyport.h"
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
static uint8_t userport_rtc_read_pbx(uint8_t orig);
static void userport_rtc_store_pbx(uint8_t value, int pulse);
static int userport_rtc_write_snapshot_module(snapshot_t *s);
static int userport_rtc_read_snapshot_module(snapshot_t *s);
static int userport_rtc_enable(int value);

static userport_device_t rtc_device = {
    "Userport RTC (DS1307)",            /* device name */
    JOYSTICK_ADAPTER_ID_NONE,           /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_RTC,           /* device is an RTC */
    userport_rtc_enable,                /* enable function */
    userport_rtc_read_pbx,              /* read pb0-pb7 function */
    userport_rtc_store_pbx,             /* store pb0-pb7 function */
    NULL,                               /* NO read pa2 pin function */
    NULL,                               /* NO store pa2 pin function */
    NULL,                               /* NO read pa3 pin function */
    NULL,                               /* NO store pa3 pin function */
    0,                                  /* pc pin is NOT needed */
    NULL,                               /* NO store sp1 pin function */
    NULL,                               /* NO read sp1 pin function */
    NULL,                               /* NO store sp2 pin function */
    NULL,                               /* NO read sp2 pin function */
    NULL,                               /* NO reset function */
    NULL,                               /* NO powerup function */
    userport_rtc_write_snapshot_module, /* snapshot write function */
    userport_rtc_read_snapshot_module   /* snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int userport_rtc_enable(int value)
{
    int val = value ? 1 : 0;

    if (userport_rtc_ds1307_enabled == val) {
        return 0;
    }

    if (val) {
        ds1307_context = ds1307_init("USERDS1307");
        ds1307_set_data_line(ds1307_context, 1);
        ds1307_set_clk_line(ds1307_context, 1);
    } else {
        if (ds1307_context) {
            ds1307_destroy(ds1307_context, ds1307_rtc_save);
            ds1307_context = NULL;
        }
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
    { "UserportRTCDS1307Save", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ds1307_rtc_save, set_userport_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int userport_rtc_ds1307_resources_init(void)
{
    if (userport_device_register(USERPORT_DEVICE_RTC_DS1307, &rtc_device) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
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

static void userport_rtc_store_pbx(uint8_t value, int pulse)
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

static uint8_t userport_rtc_read_pbx(uint8_t orig)
{
    uint8_t retval = ds1307_pb1_scl << 1;

    retval |= (ds1307_read_data_line(ds1307_context) & 1);

    return retval;
}

/* ---------------------------------------------------------------------*/

/* UP_RTC_DS1307 snapshot module format:

   type  |   name   | description
   ------------------------------
   BYTE  |   SDA    | SDA line state
   BYTE  |   SCL    | SCL line state
   BYTE  | rtc save | save rtc offset upon detacht
 */

static const char snap_module_name[] = "UPRTCDS1307";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int userport_rtc_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, ds1307_pb0_sda) < 0
        || SMW_B(m, ds1307_pb1_scl) < 0
        || SMW_B(m, (uint8_t)ds1307_rtc_save) < 0) {
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
        || SMR_B(m, &ds1307_pb1_scl) < 0
        || SMR_B_INT(m, &ds1307_rtc_save) < 0) {
       goto fail;
    }
    snapshot_module_close(m);

    return ds1307_read_snapshot(ds1307_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
