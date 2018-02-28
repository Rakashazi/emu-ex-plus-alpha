/*
 * cp-clockf83.c - CP Clock F83 RTC (PCF8583) emulation.
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

/* CP Clock F83

TAPE PORT | PCF8583 | I/O
------------------------------------
  MOTOR   |   SDA   |  O
  SENSE   |   SDA   |  I
  WRITE   |   SCL   |  O
*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "pcf8583.h"
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"
#include "translate.h"

static int tapertc_enabled = 0;

/* rtc context */
static rtc_pcf8583_t *tapertc_context = NULL;

/* rtc save */
static int tapertc_save = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void tapertc_store_sda(int flag);
static void tapertc_store_scl(int write_bit);
static int tapertc_write_snapshot(struct snapshot_s *s, int write_image);
static int tapertc_read_snapshot(struct snapshot_s *s);

static tapeport_device_t tapertc_device = {
    TAPEPORT_DEVICE_CP_CLOCK_F83,
    "Tape RTC (PCF8583)",
    IDGS_TAPE_RTC,
    0,
    "CPClockF83",
    NULL,
    tapertc_store_sda,
    tapertc_store_scl,
    NULL, /* no sense out */
    NULL, /* no read out */
    NULL, /* no passthrough */
    NULL, /* no passthrough */
    NULL, /* no passthrough */
    NULL  /* no passthrough */
};

static tapeport_snapshot_t tapertc_snapshot = {
    TAPEPORT_DEVICE_CP_CLOCK_F83,
    tapertc_write_snapshot,
    tapertc_read_snapshot,
};

static tapeport_device_list_t *tapertc_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int set_tapertc_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (tapertc_enabled == val) {
        return 0;
    }

    if (val) {
        tapertc_list_item = tapeport_device_register(&tapertc_device);
        if (tapertc_list_item == NULL) {
            return -1;
        }
        tapertc_context = pcf8583_init("TAPERTC", 2);
        pcf8583_set_data_line(tapertc_context, 1);
        pcf8583_set_clk_line(tapertc_context, 1);
    } else {
        if (tapertc_context) {
            pcf8583_destroy(tapertc_context, tapertc_save);
            tapertc_context = NULL;
        }
        tapeport_device_unregister(tapertc_list_item);
        tapertc_list_item = NULL;
    }

    tapertc_enabled = val;
    return 0;
}

static int set_tapertc_save(int val, void *param)
{
    tapertc_save = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "CPClockF83", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tapertc_enabled, set_tapertc_enabled, NULL },
    { "CPClockF83Save", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tapertc_save, set_tapertc_save, NULL },
    RESOURCE_INT_LIST_END
};

int tapertc_resources_init(void)
{
    tapeport_snapshot_register(&tapertc_snapshot);

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-cpclockf83", SET_RESOURCE, 0,
      NULL, NULL, "CPClockF83", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TAPERTC,
      NULL, NULL },
    { "+cpclockf83", SET_RESOURCE, 0,
      NULL, NULL, "CPClockF83", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TAPERTC,
      NULL, NULL },
    { "-cpclockf83save", SET_RESOURCE, 0,
      NULL, NULL, "CPClockF83Save", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TAPERTC_SAVE,
      NULL, NULL },
    { "+cpclockf83save", SET_RESOURCE, 0,
      NULL, NULL, "CPClockF83Save", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TAPERTC_SAVE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int tapertc_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

void tapertc_resources_shutdown(void)
{
    if (tapertc_context) {
        pcf8583_destroy(tapertc_context, tapertc_save);
        tapertc_context = NULL;
    }
}

/* ---------------------------------------------------------------------*/

static BYTE motor_state;

static void check_sense(void)
{
    int sense_from_rtc;

    sense_from_rtc = pcf8583_read_data_line(tapertc_context);

    if (!sense_from_rtc) {
        tapeport_set_tape_sense(0, tapertc_device.id);
    }
    else if (motor_state) {
        tapeport_set_tape_sense(0, tapertc_device.id);
    } else {
        tapeport_set_tape_sense(1, tapertc_device.id);
    }
}

static void tapertc_store_sda(int flag)
{
    motor_state = flag;

    pcf8583_set_data_line(tapertc_context, (BYTE)!motor_state);
    check_sense();
}

static void tapertc_store_scl(int write_bit)
{
    BYTE val = write_bit ? 1 : 0;

    pcf8583_set_clk_line(tapertc_context, val);
    check_sense();
}

/* ---------------------------------------------------------------------*/

/* TP_CP_CLOCK_F83 snapshot module format:

   type  | name  | description
   ---------------------------
   BYTE  | motor | motor state
 */

static char snap_module_name[] = "TP_CP_CLOCK_F83";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int tapertc_write_snapshot(struct snapshot_s *s, int write_image)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
 
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, motor_state) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    return pcf8583_write_snapshot(tapertc_context, s);
}

static int tapertc_read_snapshot(struct snapshot_s *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* enable device */
    set_tapertc_enabled(1, NULL);

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B(m, &motor_state) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    return pcf8583_read_snapshot(tapertc_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
