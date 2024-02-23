/*
 * bbrtc.c - joyport RTC (DS1602) emulation.
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
#include "ds1602.h"
#include "joyport.h"
#include "resources.h"
#include "snapshot.h"

#include "bbrtc.h"

/* Control port <--> bbrtc connections:

   cport | bbrtc | I/O
   -------------------
     1   | RST   |  O
     2   | DQ    | I/O
     4   | CLK   |  O
     7   | +5VDC | Power
     8   | GND   | Ground

   Works on:
   - native port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
 */

/* rtc context */
static rtc_ds1602_t *bbrtc_context[JOYPORT_MAX_PORTS] = { NULL };

/* rtc save */
static int bbrtc_save;

static int bbrtc_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t rst_line[JOYPORT_MAX_PORTS] = {1};
static uint8_t clk_line[JOYPORT_MAX_PORTS] = {1};
static uint8_t data_line[JOYPORT_MAX_PORTS] = {1};

static int joyport_bbrtc_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == bbrtc_enabled[port]) {
        return 0;
    }

    if (new_state) {
        /* enabled, create new DS1602 RTC context for the RTC on this port */
        bbrtc_context[port] = ds1602_init("BBRTC", 220953600);
    } else {
        /* disabled, if there is a DS1602 RTC context for this port, destroy it */
        if (bbrtc_context[port]) {
            ds1602_destroy(bbrtc_context[port], bbrtc_save);
            bbrtc_context[port] = NULL;
        }
    }

    /* set current enabled state */
    bbrtc_enabled[port] = new_state;

    return 0;
}

static uint8_t bbrtc_read(int port)
{
    uint8_t retval;

    /* get data line value from the DS1602 RTC */
    retval = (ds1602_read_data_line(bbrtc_context[port]) ? 0 : 1) << 1;

    joyport_display_joyport(port, JOYPORT_ID_BBRTC, (uint16_t)~retval);

    return (uint8_t)(~retval);
}

static void bbrtc_store(int port, uint8_t val)
{
    uint8_t rst_val = JOYPORT_BIT_BOOL(val, JOYPORT_UP_BIT);       /* reset line is on the joyport 'up' pin */
    uint8_t data_val = JOYPORT_BIT_BOOL(val, JOYPORT_DOWN_BIT);    /* data line is on the joyport 'down' pin */
    uint8_t clk_val = JOYPORT_BIT_BOOL(val, JOYPORT_RIGHT_BIT);    /* clock line is on the joyport 'right' pin */

    /* check if the reset line has changed and pass to DS1602 RTC reset line */
    if (rst_val != rst_line[port]) {
        ds1602_set_reset_line(bbrtc_context[port], rst_val);
        rst_line[port] = rst_val;
    }

    /* check if the clock line has changed and pass to DS1602 RTC clock line */
    if (clk_val != clk_line[port]) {
        ds1602_set_clk_line(bbrtc_context[port], clk_val);
        clk_line[port] = clk_val;
    }

    /* check if the data line has changed and pass to DS1602 RTC data line */
    if (data_val != data_line[port]) {
        ds1602_set_data_line(bbrtc_context[port], data_val);
        data_line[port] = data_val;
    }
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int bbrtc_write_snapshot(struct snapshot_s *s, int port);
static int bbrtc_read_snapshot(struct snapshot_s *s, int port);

static joyport_t joyport_bbrtc_device = {
    "RTC (BBRTC)",             /* name of the device */
    JOYPORT_RES_ID_NONE,       /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,   /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,      /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,     /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,  /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_RTC,        /* device is an RTC */
    0x0B,                      /* bits 3, 1 and 0 are output */
    joyport_bbrtc_set_enabled, /* device enable/disable function */
    bbrtc_read,                /* digital line read function */
    bbrtc_store,               /* digital line store function */
    NULL,                      /* NO pot-x read function */
    NULL,                      /* NO pot-y read function */
    NULL,                      /* NO powerup function */
    bbrtc_write_snapshot,      /* device snapshot write function */
    bbrtc_read_snapshot,       /* device snapshot read function */
    NULL,                      /* NO device hook function */
    0                          /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

static int set_bbrtc_save(int enabled, void *param)
{
    bbrtc_save = enabled ? 1 : 0;

    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_int[] = {
    { "BBRTCSave", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &bbrtc_save, set_bbrtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int joyport_bbrtc_resources_init(void)
{
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    return joyport_device_register(JOYPORT_ID_BBRTC, &joyport_bbrtc_device);
}

void joyport_bbrtc_resources_shutdown(void)
{
    int i;

    /* destroy any DS1602 RTC contexts */
    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (bbrtc_context[i]) {
            ds1602_destroy(bbrtc_context[i], bbrtc_save);
            bbrtc_context[i] = NULL;
        }
    }
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-bbrtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BBRTCSave", (resource_value_t)1,
      NULL, "Enable saving of the BBRTC data when changed." },
    { "+bbrtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BBRTCSave", (resource_value_t)0,
      NULL, "Disable saving of the BBRTC data when changed." },
    CMDLINE_LIST_END
};

int joyport_bbrtc_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* BBRTC snapshot module format:

   type  | name | description
   --------------------------------------
   BYTE  | RST  | reset line state
   BYTE  | CLK  | clock line state
   BYTE  | DATA | data line state
 */

static const char snap_module_name[] = "BBRTC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int bbrtc_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, rst_line[port]) < 0
        || SMW_B(m, clk_line[port]) < 0
        || SMW_B(m, data_line[port]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    snapshot_module_close(m);

    return ds1602_write_snapshot(bbrtc_context[port], s);
}

static int bbrtc_read_snapshot(struct snapshot_s *s, int port)
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
        || SMR_B(m, &rst_line[port]) < 0
        || SMR_B(m, &clk_line[port]) < 0
        || SMR_B(m, &data_line[port]) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ds1602_read_snapshot(bbrtc_context[port], s);

fail:
    snapshot_module_close(m);
    return -1;
}
