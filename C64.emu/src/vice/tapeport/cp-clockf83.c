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
#include "machine.h"
#include "pcf8583.h"
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"

#include "cp-clockf83.h"

static int tapertc_enabled[TAPEPORT_MAX_PORTS] = { 0 };

/* rtc context */
static rtc_pcf8583_t *tapertc_context[TAPEPORT_MAX_PORTS] = { NULL };

/* rtc save */
static int tapertc_save = 0;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void tapertc_store_sda(int port, int flag);
static void tapertc_store_scl(int port, int write_bit);
static int tapertc_write_snapshot(int port, struct snapshot_s *s, int write_image);
static int tapertc_read_snapshot(int port, struct snapshot_s *s);
static int tapertc_enable(int port, int val);

static tapeport_device_t tapertc_device = {
    "Tape RTC (PCF8583)",       /* device name */
    TAPEPORT_DEVICE_TYPE_RTC,   /* device is an 'RTC' type device */
    VICE_MACHINE_ALL,           /* device works on all machines */
    TAPEPORT_PORT_ALL_MASK,     /* device works on all ports */
    tapertc_enable,             /* device enable function */
    NULL,                       /* NO device specific hard reset function */
    tapertc_resources_shutdown, /* device shutdown function */
    tapertc_store_sda,          /* set motor line function */
    tapertc_store_scl,          /* NO set write line function */
    NULL,                       /* NO set sense line function */
    NULL,                       /* NO set read line function */
    tapertc_write_snapshot,     /* device snapshot write function */
    tapertc_read_snapshot       /* device snapshot read function */
};

/* ------------------------------------------------------------------------- */

static int tapertc_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (tapertc_enabled[port] == val) {
        return 0;
    }

    if (val) {
        tapertc_context[port] = pcf8583_init("TAPERTC", 2);
        pcf8583_set_data_line(tapertc_context[port], 1);
        pcf8583_set_clk_line(tapertc_context[port], 1);
    } else {
        if (tapertc_context[port]) {
            pcf8583_destroy(tapertc_context[port], tapertc_save);
            tapertc_context[port] = NULL;
        }
    }

    tapertc_enabled[port] = val;

    return 0;
}

static int set_tapertc_save(int val, void *param)
{
    tapertc_save = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "CPClockF83Save", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tapertc_save, set_tapertc_save, NULL },
    RESOURCE_INT_LIST_END
};

int tapertc_resources_init(int amount)
{
    if (tapeport_device_register(TAPEPORT_DEVICE_CP_CLOCK_F83, &tapertc_device) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-cpclockf83save", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPClockF83Save", (resource_value_t)1,
      NULL, "Enable saving of the CP Clock F83 (PCF8583 RTC) data when changed." },
    { "+cpclockf83save", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPClockF83Save", (resource_value_t)0,
      NULL, "Disable saving of the CP Clock F83 (PCF8583 RTC) data when changed." },
    CMDLINE_LIST_END
};

int tapertc_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

void tapertc_resources_shutdown(void)
{
    int i;

    for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
        if (tapertc_context[i]) {
            pcf8583_destroy(tapertc_context[i], tapertc_save);
            tapertc_context[i] = NULL;
        }
    }
}

/* ---------------------------------------------------------------------*/

static uint8_t motor_state[TAPEPORT_MAX_PORTS];

static void check_sense(int port)
{
    int sense_from_rtc;

    sense_from_rtc = pcf8583_read_data_line(tapertc_context[port]);

    if (!sense_from_rtc) {
        tapeport_set_tape_sense(0, port);
    } else if (motor_state[port]) {
        tapeport_set_tape_sense(0, port);
    } else {
        tapeport_set_tape_sense(1, port);
    }
}

static void tapertc_store_sda(int port, int flag)
{
    motor_state[port] = flag;

    pcf8583_set_data_line(tapertc_context[port], (uint8_t)!motor_state[port]);
    check_sense(port);
}

static void tapertc_store_scl(int port, int write_bit)
{
    uint8_t val = write_bit ? 1 : 0;

    pcf8583_set_clk_line(tapertc_context[port], val);
    check_sense(port);
}

/* ---------------------------------------------------------------------*/

/* TP_CP_CLOCK_F83 snapshot module format:

   type  | name  | description
   ---------------------------
   BYTE  | motor | motor state
 */

static const char snap_module_name[] = "TP_CP_CLOCK_F83";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

static int tapertc_write_snapshot(int port, struct snapshot_s *s, int write_image)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, motor_state[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    return pcf8583_write_snapshot(tapertc_context[port], s);
}

static int tapertc_read_snapshot(int port, struct snapshot_s *s)
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

    if (SMR_B(m, &motor_state[port]) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

    return pcf8583_read_snapshot(tapertc_context[port], s);

fail:
    snapshot_module_close(m);
    return -1;
}
