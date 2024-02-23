/*
 * mouse_neos.c - NEOS mouse handling
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

/* This file contains the implementation for the NEOS mouse, which uses the
   digital lines for a custom protocol, and additionally POTX for one of the
   buttons. */

/* Control port <--> mouse/paddles/pad connections:

   cport | neos         | I/O
   --------------------------
     1   | D0           |  I
     2   | D1           |  I
     3   | D2           |  I
     4   | D3           |  I
     6   | strobe       |  O
     6   | left button  |  I    ("Fire")
     7   | +5VDC        |  Power
     8   | GND          |  Ground
     9   | right button |  I    ("POTX")

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
*/

/* #define DEBUG_NEOS */

#ifdef DEBUG_NEOS
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include <stdlib.h> /* abs */

#include "vice.h"

#include "joyport.h"
#include "maincpu.h"
#include "log.h"
#include "mouse.h"
#include "mousedrv.h"
#include "snapshot.h"
#include "vsyncapi.h"

#include "mouse_neos.h"

/******************************************************************************/

static uint8_t mouse_digital_val = 0;

static int16_t mouse_latest_x;
static int16_t mouse_latest_y;

static CLOCK neos_last_trigger = 0;
static CLOCK neos_time_out_cycles = 232;

static int neos_buttons;
static int neos_prev;

static uint8_t neos_x;
static uint8_t neos_y;
static uint8_t neos_lastx;
static uint8_t neos_lasty;

enum {
    NEOS_XH = 0,
    NEOS_XL,
    NEOS_YH,
    NEOS_YL
} neos_state = NEOS_YL;

/******************************************************************************/

void neos_mouse_set_machine_parameter(long clock_rate)
{
    neos_time_out_cycles = (CLOCK)((clock_rate / 10000) * 2);
}

static void neos_get_new_movement(void)
{
    int16_t new_x16, new_y16;
    uint8_t new_x, new_y;

    mouse_get_raw_int16(&new_x16, &new_y16);
    new_x = (uint8_t)(new_x16 >> 1);
    new_y = (uint8_t)(new_y16 >> 1);

    neos_x = (uint8_t)(neos_lastx - new_x);
    neos_lastx = new_x;

    neos_y = (uint8_t)(new_y - neos_lasty);
    neos_lasty = new_y;

    /* DBG(("neos_get_new_movement %dx%d", new_x16, new_y16)); */
}

void neos_mouse_store(int port, uint8_t val)
{
    if ((neos_prev & 16) != (val & 16)) {
        /* each change on the strobe line advances to the next state */
        switch (neos_state) {
            case NEOS_XH:
                if (val & JOYPORT_FIRE) {
                    neos_state = NEOS_XL;
                }
                break;
            case NEOS_XL:
                if (neos_prev & JOYPORT_FIRE) {
                    neos_state = NEOS_YH;
                }
                break;
            case NEOS_YH:
                if (val & JOYPORT_FIRE) {
                    neos_state = NEOS_YL;
                }
                break;
            case NEOS_YL:
                if (neos_prev & JOYPORT_FIRE) {
                    neos_state = NEOS_XH;
                    neos_get_new_movement();
                }
                break;
            default:
                /* never reaches here */
                break;
        }
        /* every change on the strobe line resets the timeout */
        neos_last_trigger = maincpu_clk;
        neos_prev = val;
    }
}

uint8_t neos_mouse_read(void)
{
    if ((neos_state != NEOS_XH) && (maincpu_clk > (neos_last_trigger + neos_time_out_cycles))) {
        neos_state = NEOS_XH;
        neos_get_new_movement();
    }

    switch (neos_state) {
        case NEOS_XH:
            return ((neos_x >> 4) & 0xf) | 0xf0;   /* output high nibble of X on joyport direction pins */
            break;
        case NEOS_XL:
            return (neos_x & 0xf) | 0xf0;          /* output low nibble of X on joyport direction pins */
            break;
        case NEOS_YH:
            return ((neos_y >> 4) & 0xf) | 0xf0;   /* output high nibble of Y on joyport direction pins */
            break;
        case NEOS_YL:
            return (neos_y & 0xf) | 0xf0;          /* output low nibble of X on joyport direction pins */
            break;
        default:
            /* never reaches here */
            return 0xff;
            break;
    }
}

static uint8_t joyport_mouse_neos_value(int port)
{
    uint8_t retval = 0xff;

    if (_mouse_enabled) {

        /* we need to poll here, else the mouse can not move if the C64 code
           does not read POTX */
        mouse_poll();

        retval = (uint8_t)((~mouse_digital_val) & neos_mouse_read());
        /* on a real NEOS mouse the left mouse button is connected to FIRE and
           will interfere with the STROBE line which is connected to the same
           I/O bit. in this case all direction bits will go inactive/high and
           the mouse can not be moved */
        if (mouse_digital_val & 0x10) {
            retval &= ~0x0f;
        }
        if (retval != (uint8_t)~mouse_digital_val) {
            joyport_display_joyport(port, JOYPORT_ID_MOUSE_NEOS, (uint16_t)(~retval));
        }
    }
    return retval;
}

void mouse_neos_button_right(int pressed)
{
    if (pressed) {
        neos_buttons |= JOYPORT_UP;
    } else {
        neos_buttons &= ~JOYPORT_UP;
    }
}

void mouse_neos_button_left(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_FIRE_1;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_FIRE_1;
    }
}

static uint8_t joyport_mouse_neos_potx(int port)
{
    return _mouse_enabled ? ((neos_buttons & 1) ? 0xff : 0) : 0xff;
}

void mouse_neos_set_enabled(int enabled)
{
    if (enabled) {
        mouse_get_raw_int16(&mouse_latest_x, &mouse_latest_y);
        neos_lastx = (uint8_t)(mouse_latest_x >> 1);
        neos_lasty = (uint8_t)(mouse_latest_y >> 1);
    }
}

static int joyport_mouse_neos_set_enabled(int port, int joyportid)
{
    int mt;

    mouse_reset();
    neos_lastx = (uint8_t)(mouse_latest_x >> 1);
    neos_lasty = (uint8_t)(mouse_latest_y >> 1);

    if (joyportid == JOYPORT_ID_NONE) {
        /* disabled, set mouse type to invalid/none */
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

    return 0;
}


/* Some prototypes are needed */
int mouse_neos_write_snapshot(struct snapshot_s *s, int port);
int mouse_neos_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_neos_joyport_device = {
    "Mouse (NEOS)",                 /* name of the device */
    JOYPORT_RES_ID_MOUSE,           /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,        /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,           /* device uses the potentiometer line for the right button, but could work without it */
    JOYPORT_5VDC_REQUIRED,          /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,       /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,           /* device is a Mouse */
    0x10,                           /* bit 4 is an output bit */
    joyport_mouse_neos_set_enabled, /* device enable/disable function */
    joyport_mouse_neos_value,       /* digital line read function */
    neos_mouse_store,               /* digital line store function */
    joyport_mouse_neos_potx,        /* pot-x read function */
    NULL,                           /* NO pot-y read function */
    NULL,                           /* NO powerup function */
    mouse_neos_write_snapshot,      /* device write snapshot function */
    mouse_neos_read_snapshot,       /* device read snapshot function */
    NULL,                           /* NO device hook function */
    0                               /* NO device hook function mask */
};

int mouse_neos_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_NEOS, &mouse_neos_joyport_device);
}


void mouse_neos_init(void)
{
    neos_buttons = 0;
    neos_prev = 0xff;
}

/* --------------------------------------------------------- */

/* MOUSE_NEOS snapshot module format:

   type  | name                 | description
   ------------------------------------------
   BYTE  | digital value        | digital pins return value
   DWORD | buttons              | buttons state
   BYTE  | neos x               | neos X
   BYTE  | neos y               | neos Y
   BYTE  | neos last x          | neos last X
   BYTE  | neos last y          | neos last Y
   DWORD | neos state           | state
   DWORD | neos prev            | previous state
   DWORD | last trigger         | last trigger clock
   DWORD | neos time out cycles | time out cycles
 */

static const char mouse_neos_snap_module_name[] = "MOUSE_NEOS";
#define MOUSE_NEOS_VER_MAJOR   1
#define MOUSE_NEOS_VER_MINOR   0

int mouse_neos_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_neos_snap_module_name, MOUSE_NEOS_VER_MAJOR, MOUSE_NEOS_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMW_B(m, mouse_digital_val) < 0
        || SMW_DW(m, (uint32_t)neos_buttons) < 0
        || SMW_B(m, neos_x) < 0
        || SMW_B(m, neos_y) < 0
        || SMW_B(m, neos_lastx) < 0
        || SMW_B(m, neos_lasty) < 0
        || SMW_DW(m, (uint32_t)neos_state) < 0
        || SMW_DW(m, (uint32_t)neos_prev) < 0
        || SMW_DW(m, (uint32_t)neos_last_trigger) < 0
        || SMW_DW(m, (uint32_t)neos_time_out_cycles) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

int mouse_neos_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    uint32_t tmpc1;
    uint32_t tmpc2;
    int tmp_neos_state;

    m = snapshot_module_open(s, mouse_neos_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_NEOS_VER_MAJOR, MOUSE_NEOS_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (0
        || SMR_B(m, &mouse_digital_val) < 0
        || SMR_DW_INT(m, &neos_buttons) < 0
        || SMR_B(m, &neos_x) < 0
        || SMR_B(m, &neos_y) < 0
        || SMR_B(m, &neos_lastx) < 0
        || SMR_B(m, &neos_lasty) < 0
        || SMR_DW_INT(m, &tmp_neos_state) < 0
        || SMR_DW_INT(m, &neos_prev) < 0
        || SMR_DW(m, &tmpc1) < 0
        || SMR_DW(m, &tmpc2) < 0) {
        goto fail;
    }

    neos_last_trigger = (CLOCK)tmpc1;
    neos_time_out_cycles = (CLOCK)tmpc2;
    neos_state = tmp_neos_state;

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

