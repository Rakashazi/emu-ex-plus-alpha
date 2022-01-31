/*
 * userport_rtc_58321a.c - Generic userport RTC (58321a) emulation.
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

/* Userport RTC 58321a (C64/C128/CBM2/PET/VIC20)

C64/C128 | CBM2 | PET | VIC20 | NAME
------------------------------------
    C    |  14  |  C  |   C   | PB0 <-> D0
    D    |  13  |  D  |   D   | PB1 <-> D1
    E    |  12  |  E  |   E   | PB2 <-> D2
    F    |  11  |  F  |   F   | PB3 <-> D3
    H    |  10  |  H  |   H   | PB4 -> ADDRESS /DATA
    J    |   9  |  J  |   J   | PB5 -> READ
    K    |   8  |  K  |   K   | PB6 -> WRITE
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "rtc-58321a.h"
#include "snapshot.h"
#include "uiapi.h"
#include "joyport.h"
#include "userport.h"
#include "userport_rtc_58321a.h"

int userport_rtc_58321a_enabled = 0;

/* rtc context */
static rtc_58321a_t *rtc58321a_context = NULL;

/* rtc save */
static int rtc58321a_rtc_save;

static int read_line_active = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t userport_rtc_read_pbx(uint8_t orig);
static void userport_rtc_store_pbx(uint8_t value, int pulse);
static int userport_rtc_write_snapshot_module(snapshot_t *s);
static int userport_rtc_read_snapshot_module(snapshot_t *s);
static int userport_rtc_enable(int value);

static userport_device_t rtc_device = {
    "Userport RTC (RTC58321A)",         /* device name */
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

    if (userport_rtc_58321a_enabled == val) {
        return 0;
    }

    if (val) {
        rtc58321a_context = rtc58321a_init("USER");
    } else {
        if (rtc58321a_context) {
            rtc58321a_destroy(rtc58321a_context, rtc58321a_rtc_save);
            rtc58321a_context = NULL;
        }
    }

    userport_rtc_58321a_enabled = val;
    return 0;
}

static int set_userport_rtc_save(int val, void *param)
{
    rtc58321a_rtc_save = val ? 1 : 0;

    return 0;
}


static const resource_int_t resources_int[] = {
    { "UserportRTC58321aSave", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &rtc58321a_rtc_save, set_userport_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int userport_rtc_58321a_resources_init(void)
{
    if (userport_device_register(USERPORT_DEVICE_RTC_58321A, &rtc_device) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userportrtc58321asave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTC58321aSave", (resource_value_t)1,
      NULL, "Enable saving of the Userport RTC (58321a) data when changed." },
    { "+userportrtc58321asave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UserportRTC58321aSave", (resource_value_t)0,
      NULL, "Disable saving of the Userport RTC (58321a) data when changed." },
    CMDLINE_LIST_END
};

int userport_rtc_58321a_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

void userport_rtc_58321a_resources_shutdown(void)
{
    if (rtc58321a_context) {
        rtc58321a_destroy(rtc58321a_context, rtc58321a_rtc_save);
        rtc58321a_context = NULL;
    }
}

/* ---------------------------------------------------------------------*/

static void userport_rtc_store_pbx(uint8_t value, int pulse)
{
    if (value & 0x10) {
        rtc58321a_write_address(rtc58321a_context, (uint8_t)(value & 0xf));
    }
    if (value & 0x20) {
        read_line_active = 1;
    } else {
        read_line_active = 0;
    }
    if (value & 0x40) {
        rtc58321a_write_data(rtc58321a_context, (uint8_t)(value & 0xf));
    }
}

static uint8_t userport_rtc_read_pbx(uint8_t orig)
{
    uint8_t retval = 0xf;

    if (read_line_active) {
        retval = rtc58321a_read(rtc58321a_context);
    }
    return retval;
}

/* ---------------------------------------------------------------------*/

/* UP_RTC_58321A snapshot module format:

   type  | name | description
   --------------------------
   BYTE  | read    | read line active
   BYTE  | rtcsave | save rtc offset when detaching
 */

static const char snap_module_name[] = "UP_RTC_58321A";
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
        || (SMW_B(m, (uint8_t)read_line_active) < 0)
        || (SMW_B(m, (uint8_t)rtc58321a_rtc_save) < 0)) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    return rtc58321a_write_snapshot(rtc58321a_context, s);
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
        || (SMR_B_INT(m, &read_line_active) < 0)
        || (SMR_B_INT(m, &rtc58321a_rtc_save) < 0)) {
        goto fail;
    }
    snapshot_module_close(m);

    return rtc58321a_read_snapshot(rtc58321a_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
