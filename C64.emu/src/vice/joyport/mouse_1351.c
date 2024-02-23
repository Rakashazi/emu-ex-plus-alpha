/*
 * mouse_1351.c - C1351-like mouse handling
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Andreas Boose <viceteam@t-online.de>
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

/* This file contains the implementation of all devices which work like the
   1351 mouse, ie the device puts movement deltas on the POT inputs, and uses
   the digital lines for additional buttons. */

/* Control port <--> mouse/paddles/pad connections:

   cport | 1351         | I/O
   --------------------------
     1   | right button |  I ("Up")
     5   | Y-position   |  I
     6   | left button  |  I ("Fire")
     7   | +5VDC        |  Power
     8   | GND          |  Ground
     9   | X-position   |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | smart mouse  | I/O
   --------------------------
     1   | right button |  I
     2   | RTC clock    |  O
     3   | RTC I/O      | I/O
     4   | RTC RST      | I/O
     5   | Y-position   |  I
     6   | left button  |  I
     7   | +5VDC        |  Power
     8   | GND          |  Ground
     9   | X-position   |  I

   Works on:
   - Native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | micromys      | I/O
   ---------------------------
     1   | right button  |  I
     2   | middle button |  I
     3   | wheel up      |  I
     4   | wheel down    |  I
     5   | Y-position    |  I
     6   | left button   |  I
     7   | +5VDC         |  Power
     8   | GND           |  Ground
     9   | X-position    |  I

   Works on:
   - Native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
*/

/* #define DEBUG_1351 */

#ifdef DEBUG_1351
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include <stdlib.h> /* abs */

#include "vice.h"

#include "cmdline.h"
#include "joyport.h"
#include "maincpu.h"
#include "log.h"
#include "mouse.h"
#include "mousedrv.h"
#include "resources.h"
#include "snapshot.h"
#include "vsyncapi.h"

#include "ds1202_1302.h"
#include "mouse_1351.h"

/******************************************************************************/

static int16_t last_mouse_x;
static int16_t last_mouse_y;
static uint8_t mouse_digital_val = 0;

static CLOCK up_down_pulse_end = 0;    /* in cpu cycle units */

static rtc_ds1202_1302_t *ds1202; /* smartmouse */
static int ds1202_rtc_save; /* smartmouse rtc data save */

/******************************************************************************/

/*
    note: for the expected behaviour look at testprogs/SID/paddles/readme.txt

    "The Final Cartridge 3" (and possibly others?) do not work if the mouse
    is inserted (as in enabled) after it has started. so either enable mouse
    emulation on the commandline, or (re)start (or reset incase of a cart)
    after enabling it in the gui. (see testprogs/SID/paddles/fc3detect.asm)
*/

static uint8_t mouse_get_1351_x(int port)
{
    mouse_get_last_int16(&last_mouse_x, &last_mouse_y);
    return (uint8_t)((last_mouse_x & 0x7f) + 0x40);
}

static uint8_t mouse_get_1351_y(int port)
{
    mouse_get_last_int16(&last_mouse_x, &last_mouse_y);
    return (uint8_t)((last_mouse_y & 0x7f) + 0x40);
}

void mouse_1351_button_right(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_UP;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_UP;
    }
}

void mouse_1351_button_left(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_FIRE_1;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_FIRE_1;
    }
}

static uint8_t joyport_mouse_value(int port)
{
    uint8_t retval = 0xff;

    if (_mouse_enabled) {
        retval = ~mouse_digital_val;
    }

    joyport_display_joyport(port, JOYPORT_ID_MOUSE_1351, (uint16_t)(~retval));

    return retval;
}

static int mouse_1351_set_enabled(int port, int joyportid)
{
    int mt;

    mouse_reset();

    if (joyportid == JOYPORT_ID_NONE) {
        /* disabled, destroy the DS1202 RTC context if needed (smartmouse) */
        if (ds1202) {
            ds1202_1302_destroy(ds1202, ds1202_rtc_save);
            ds1202 = NULL;
        }
        mouse_type = -1;
        return 0;
    }

    /* convert joyport ID to mouse type */
    mt = mouse_id_to_type(joyportid);

    if (mt == -1) {
        return -1;
    }

    if (mt == mouse_type) {
        return 0;
    }

    mouse_type = mt;

    if (mt == MOUSE_TYPE_SMART) {
        /* smartmouse enabled, create DS1202 RTC context */
        ds1202 = ds1202_1302_init("SM", 1202);
    }

    return 0;
}

/* Some prototypes are needed */
static int mouse_1351_write_snapshot(struct snapshot_s *s, int port);
static int mouse_1351_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_1351_joyport_device = {
    "Mouse (1351)",            /* name of the device */
    JOYPORT_RES_ID_MOUSE,      /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,   /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,      /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,     /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,  /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,      /* device is a Mouse */
    0,                         /* NO output bits */
    mouse_1351_set_enabled,    /* device enable/disable function */
    joyport_mouse_value,       /* digital line read function */
    NULL,                      /* NO digital line store function */
    mouse_get_1351_x,          /* pot-x read function */
    mouse_get_1351_y,          /* pot-y read function */
    NULL,                      /* NO powerup function */
    mouse_1351_write_snapshot, /* device write snapshot function */
    mouse_1351_read_snapshot,  /* device read snapshot function */
    NULL,                      /* NO device hook function */
    0                          /* NO device hook function mask */
};

/* FIXME: remove unused stuff from the snapshot */

/* MOUSE_1351 snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
 */

static const char mouse_1351_snap_module_name[] = "MOUSE_1351";
#define MOUSE_1351_VER_MAJOR   1
#define MOUSE_1351_VER_MINOR   0

static int mouse_1351_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_1351_snap_module_name, MOUSE_1351_VER_MAJOR, MOUSE_1351_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_B(m, mouse_digital_val) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_1351_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_1351_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_1351_VER_MAJOR, MOUSE_1351_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_B(m, &mouse_digital_val) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/*****************************************************************************/

static int up_down_counter = 0;

/* http://wiki.icomp.de/wiki/Micromys_Protocol */
uint8_t micromys_mouse_read(void)
{
    /* update wheel until we're ahead */
    while (up_down_counter && up_down_pulse_end <= maincpu_clk) {
        up_down_counter += (up_down_counter < 0) * 2 - 1;
        up_down_pulse_end += 512 * 98; /* 50 ms counted from POT input (98 A/D cycles) */
    }
    if (up_down_counter & 1) {
        return (uint8_t)(~(4 << (up_down_counter < 0)));
    }

    return 0xff;
}

void micromys_mouse_button_up(int pressed)
{
    if (pressed) {
        if (!up_down_counter) {
            up_down_pulse_end = maincpu_clk;
        }
        up_down_counter += 2;
    }
}

void micromys_mouse_button_down(int pressed)
{
    if (pressed) {
        if (!up_down_counter) {
            up_down_pulse_end = maincpu_clk;
        }
        up_down_counter -= 2;
    }
}

void micromys_mouse_button_middle(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_DOWN;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_DOWN;
    }
}

static uint8_t joyport_mouse_micromys_value(int port)
{
    uint8_t retval = 0xff;

    if (_mouse_enabled) {
        retval = (uint8_t)((~mouse_digital_val) & micromys_mouse_read());
        joyport_display_joyport(port, JOYPORT_ID_MOUSE_MICROMYS, (uint16_t)(~retval));
    }
    return retval;
}

/* Some prototypes are needed */
static int mouse_micromys_write_snapshot(struct snapshot_s *s, int port);
static int mouse_micromys_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_micromys_joyport_device = {
    "Mouse (Micromys)",            /* name of the device */
    JOYPORT_RES_ID_MOUSE,          /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,       /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,          /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,         /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,      /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,          /* device is a Mouse */
    0,                             /* NO output bits */
    mouse_1351_set_enabled,        /* device enable/disable function */
    joyport_mouse_micromys_value,  /* digital line read function */
    NULL,                          /* NO digital line store function */
    mouse_get_1351_x,              /* pot-x read function */
    mouse_get_1351_y,              /* pot-y read function */
    NULL,                          /* NO powerup function */
    mouse_micromys_write_snapshot, /* device write snapshot function */
    mouse_micromys_read_snapshot,  /* device read snapshot function */
    NULL,                          /* NO device hook function */
    0                              /* NO device hook function mask */
};

/* MOUSE_MICROMYS snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   DWORD  | up down counter        | up down counter
   DWORD  | up down pulse end      | up down pulse end
 */

static const char mouse_micromys_snap_module_name[] = "MOUSE_MICROMYS";
#define MOUSE_MICROMYS_VER_MAJOR   1
#define MOUSE_MICROMYS_VER_MINOR   0

static int mouse_micromys_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_micromys_snap_module_name, MOUSE_MICROMYS_VER_MAJOR, MOUSE_MICROMYS_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMW_B(m, mouse_digital_val) < 0
        || SMW_DW(m, (uint32_t)up_down_counter) < 0
        || SMW_DW(m, (uint32_t)up_down_pulse_end) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_micromys_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    uint32_t tmpc1;

    m = snapshot_module_open(s, mouse_micromys_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_MICROMYS_VER_MAJOR, MOUSE_MICROMYS_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMR_B(m, &mouse_digital_val) < 0
        || SMR_DW_INT(m, &up_down_counter) < 0
        || SMR_DW(m, &tmpc1) < 0) {
        goto fail;
    }

    up_down_pulse_end = (CLOCK)tmpc1;

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/******************************************************************************/

static uint8_t joyport_mouse_smart_value(int port)
{
    uint8_t retval = 0xff;

    if (_mouse_enabled) {
        retval = (uint8_t)((~mouse_digital_val) & smart_mouse_read());
        if (retval != (uint8_t)~mouse_digital_val) {
            joyport_display_joyport(port, JOYPORT_ID_MOUSE_SMART, (uint16_t)(~retval));
        }
    }
    return retval;
}

void smart_mouse_store(int port, uint8_t val)
{
    ds1202_1302_set_lines(ds1202, !(val & JOYPORT_RIGHT), !!(val & JOYPORT_DOWN), !!(val & JOYPORT_LEFT));
}

uint8_t smart_mouse_read(void)
{
    return ds1202_1302_read_data_line(ds1202) ? 0xff : 0xfb;
}

void smart_mouse_shutdown(void)
{
    if (ds1202) {
        ds1202_1302_destroy(ds1202, ds1202_rtc_save);
        ds1202 = NULL;
    }
}

/* Some prototypes are needed */
static int mouse_smart_write_snapshot(struct snapshot_s *s, int port);
static int mouse_smart_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_smart_joyport_device = {
    "Mouse (SmartMouse)",       /* name of the device */
    JOYPORT_RES_ID_MOUSE,       /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,    /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,       /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,      /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,   /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,       /* device is a Mouse */
    0x0E,                       /* bits 3, 2 and 1 are output bits */
    mouse_1351_set_enabled,     /* device enable/disable function */
    joyport_mouse_smart_value,  /* digital line read function */
    smart_mouse_store,          /* digital line store function */
    mouse_get_1351_x,           /* pot-x read function */
    mouse_get_1351_y,           /* pot-y read function */
    NULL,                       /* NO powerup function */
    mouse_smart_write_snapshot, /* device write snapshot function */
    mouse_smart_read_snapshot,  /* device read snapshot function */
    NULL,                       /* NO device hook function */
    0                           /* NO device hook function mask */
};

static int set_smart_mouse_rtc_save(int val, void *param)
{
    ds1202_rtc_save = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_extra_int[] = {
    { "SmartMouseRTCSave", 0, RES_EVENT_SAME, NULL,
      &ds1202_rtc_save, set_smart_mouse_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int smart_mouse_resources_init(void)
{
    return resources_register_int(resources_extra_int);
}

static const cmdline_option_t cmdline_extra_option[] =
{
    { "-smartmousertcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SmartMouseRTCSave", (void *)1,
      NULL, "Enable saving of smart mouse RTC data when changed." },
    { "+smartmousertcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SmartMouseRTCSave", (void *)0,
      NULL, "Disable saving of smart mouse RTC data when changed." },
    CMDLINE_LIST_END
};

/*--------------------------------------------------------------------------*/

int mouse_1351_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_1351, &mouse_1351_joyport_device);
}

int mouse_micromys_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_MICROMYS, &mouse_micromys_joyport_device);
}

int mouse_smartmouse_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_SMART, &mouse_smart_joyport_device);
}


/* --------------------------------------------------------- */

int smart_mouse_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_extra_option);
}

/* MOUSE_SMART snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
 */

static const char mouse_smart_snap_module_name[] = "MOUSE_SMART";
#define MOUSE_SMART_VER_MAJOR   1
#define MOUSE_SMART_VER_MINOR   0

static int mouse_smart_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_smart_snap_module_name, MOUSE_SMART_VER_MAJOR, MOUSE_SMART_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_B(m, mouse_digital_val) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ds1202_1302_write_snapshot(ds1202, s);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_smart_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_smart_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_SMART_VER_MAJOR, MOUSE_SMART_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_B(m, &mouse_digital_val) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return ds1202_1302_read_snapshot(ds1202, s);

fail:
    snapshot_module_close(m);
    return -1;
}

