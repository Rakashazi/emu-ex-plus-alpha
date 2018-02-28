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
#include "translate.h"

/* Control port <--> bbrtc connections:

   cport | bbrtc | I/O
   -------------------
     1   | RST   |  O
     2   | DQ    | I/O
     4   | CLK   |  O
 */

/* rtc context */
static rtc_ds1602_t *bbrtc_context = NULL;

/* rtc save */
static int bbrtc_save;

static int bbrtc_enabled = 0;

static BYTE rst_line = 1;
static BYTE clk_line = 1;
static BYTE data_line = 1;

static int joyport_bbrtc_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == bbrtc_enabled) {
        return 0;
    }

    if (val) {
        bbrtc_context = ds1602_init("BBRTC", 220953600);
    } else {
        if (bbrtc_context) {
            ds1602_destroy(bbrtc_context, bbrtc_save);
            bbrtc_context = NULL;
        }
    }

    bbrtc_enabled = val;

    return 0;
}

static BYTE bbrtc_read(int port)
{
    BYTE retval;

    retval = (ds1602_read_data_line(bbrtc_context) ? 0 : 1) << 1;
    joyport_display_joyport(JOYPORT_ID_BBRTC, retval);
    return (BYTE)(~retval);
}

static void bbrtc_store(BYTE val)
{
    BYTE rst_val = val & 1;
    BYTE data_val = (BYTE)((val & 2) >> 1);
    BYTE clk_val = (BYTE)((val & 8) >> 3);

    if (rst_val != rst_line) {
        ds1602_set_reset_line(bbrtc_context, rst_val);
        rst_line = rst_val;
    }

    if (clk_val != clk_line) {
        ds1602_set_clk_line(bbrtc_context, clk_val);
        clk_line = clk_val;
    }

    if (data_val != data_line) {
        ds1602_set_data_line(bbrtc_context, data_val);
        data_line = data_val;
    }
}

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int bbrtc_write_snapshot(struct snapshot_s *s, int port);
static int bbrtc_read_snapshot(struct snapshot_s *s, int port);

static joyport_t joyport_bbrtc_device = {
    "BBRTC",
    IDGS_BBRTC,
    JOYPORT_RES_ID_RTC,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_bbrtc_enable,
    bbrtc_read,
    bbrtc_store,
    NULL,                   /* no pot-x read */
    NULL,                   /* no pot-y read */
    bbrtc_write_snapshot,
    bbrtc_read_snapshot
};

/* ------------------------------------------------------------------------- */

static int set_bbrtc_save(int val, void *param)
{
    bbrtc_save = val ? 1 : 0;

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
    if (bbrtc_context) {
        ds1602_destroy(bbrtc_context, bbrtc_save);
        bbrtc_context = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-bbrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "BBRTCSave", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_BBRTC_SAVE,
      NULL, NULL },
    { "+bbrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "BBRTCSave", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_BBRTC_SAVE,
      NULL, NULL },
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

static char snap_module_name[] = "BBRTC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int bbrtc_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, rst_line) < 0
        || SMW_B(m, clk_line) < 0
        || SMW_B(m, data_line) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    snapshot_module_close(m);

    return ds1602_write_snapshot(bbrtc_context, s);
}

static int bbrtc_read_snapshot(struct snapshot_s *s, int port)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

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
        || SMR_B(m, &rst_line) < 0
        || SMR_B(m, &clk_line) < 0
        || SMR_B(m, &data_line) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ds1602_read_snapshot(bbrtc_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
