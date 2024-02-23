/*
 * mouse_quadrature.c - Quadrature mouse handling
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

/* This file contains the implementation for all quadrature based devices. The
   device puts quadrature signals at the digital lines, and uses other digital
   lines and/or POT lines for buttons */

/* Control port <--> mouse/paddles/pad connections:

   cport | amiga         | I/O
   ---------------------------
     1   | V-pulse       |  I
     2   | H-pulse       |  I
     3   | VQ-pulse      |  I
     4   | HQ-pulse      |  I
     5   | middle button |  I
     6   | left button   |  I
     7   | +5VDC         |  Power
     8   | GND           |  Ground
     9   | right button  |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0/xplus4)
   - inception joystick adapter ports (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | cx22        | I/O
   -------------------------
     1   | X direction |  I
     2   | X motion    |  I
     3   | Y direction |  I
     4   | Y motion    |  I
     6   | button      |  I
     7   | +5VDC       |  Power
     8   | GND         |  Ground

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0/xplus4)
   - inception joystick adapter ports (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | atari-st     | I/O
   --------------------------
     1   | XB           |  I
     2   | XA           |  I
     3   | YA           |  I
     4   | YB           |  I
     6   | left button  |  I
     7   | +5VDC        |  Power
     8   | GND          |  Ground
     9   | right button |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0/xplus4)
   - inception joystick adapter ports (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
*/

/* #define DEBUG_QUADRATURE */

#ifdef DEBUG_QUADRATURE
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include <stdlib.h> /* abs */

#include "vice.h"

#include "joyport.h"
#include "maincpu.h"
#include "log.h"
#include "machine.h"
#include "mouse.h"
#include "mousedrv.h"
#include "snapshot.h"
#include "vsyncapi.h"

#include "mouse_quadrature.h"

/******************************************************************************/

static int16_t last_mouse_x;
static int16_t last_mouse_y;

static uint8_t mouse_digital_val = 0;

/* The current emulated quadrature state of the polled mouse, range is [0,3] */
static uint8_t quadrature_x = 0;
static uint8_t quadrature_y = 0;

static int amiga_and_atarist_buttons = 0;

static uint8_t polled_joyval = 0xff;

/******************************************************************************/
static const uint8_t amiga_mouse_table[4] = { 0x0, 0x1, 0x5, 0x4 };
static const uint8_t st_mouse_table[4] = { 0x0, 0x2, 0x3, 0x1 };

static uint8_t joyport_mouse_amiga_st_read(int port)
{
    int mouse_sx = mouse_get_mouse_sx();
    int mouse_sy = mouse_get_mouse_sy();

    mouse_get_last_int16(&last_mouse_x, &last_mouse_y);

    if ((quadrature_x != ((last_mouse_x >> 1) & 3)) || (quadrature_y != ((~last_mouse_y >> 1) & 3))) {
        /* keep within range */
        quadrature_x = (last_mouse_x >> 1) & 3;
        quadrature_y = (~last_mouse_y >> 1) & 3;

        switch (mouse_type) {
            case MOUSE_TYPE_AMIGA:
                polled_joyval = (uint8_t)((amiga_mouse_table[quadrature_x] << 1) | amiga_mouse_table[quadrature_y] | 0xf0);
                break;
            case MOUSE_TYPE_CX22:
                polled_joyval = (uint8_t)(((quadrature_y & 1) << 3) | ((mouse_sy > 0) << 2) | ((quadrature_x & 1) << 1) | (mouse_sx > 0) | 0xf0);
                break;
            case MOUSE_TYPE_ST:
                polled_joyval =(uint8_t)(st_mouse_table[quadrature_x] | (st_mouse_table[quadrature_y] << 2) | 0xf0);
                break;
            default:
                polled_joyval = 0xff;
        }
    }

    return polled_joyval;
}

static uint8_t joyport_mouse_poll_value(int port)
{
    uint8_t retval = 0xff;

    if (_mouse_enabled) {
        mouse_poll();
        retval = (uint8_t)((~mouse_digital_val) & joyport_mouse_amiga_st_read(port));
        if (retval != (uint8_t)~mouse_digital_val) {
            joyport_display_joyport(port, mouse_type_to_id(mouse_type), (uint16_t)(~retval));
        }
    }
    return retval;
}

static uint8_t joyport_mouse_amiga_st_read_potx(int port)
{
    return _mouse_enabled ? ((amiga_and_atarist_buttons & 1) ? 0xff : 0) : 0xff;
}

static uint8_t joyport_mouse_amiga_st_read_poty(int port)
{
    return _mouse_enabled ? ((amiga_and_atarist_buttons & 2) ? 0xff : 0) : 0xff;
}

void mouse_amiga_st_button_right(int pressed)
{
    if (pressed) {
        amiga_and_atarist_buttons |= JOYPORT_UP;
    } else {
        amiga_and_atarist_buttons &= ~JOYPORT_UP;
    }
}

void mouse_amiga_st_button_left(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_FIRE_1;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_FIRE_1;
    }
}

void  mouse_amiga_st_button_middle(int pressed)
{
    if (pressed) {
        amiga_and_atarist_buttons |= JOYPORT_DOWN;
    } else {
        amiga_and_atarist_buttons &= ~JOYPORT_DOWN;
    }
}

static int joyport_mouse_set_enabled(int port, int joyportid)
{
    int mt;

    mouse_reset();

    if (joyportid == JOYPORT_ID_NONE) {
        /* disabled, set mouse type to none */
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

void mouse_amiga_st_init(void)
{
    amiga_and_atarist_buttons = 0;
}

/* Some prototypes are needed */
static int mouse_amiga_write_snapshot(struct snapshot_s *s, int port);
static int mouse_amiga_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_amiga_joyport_device = {
    "Mouse (Amiga)",                  /* name of the device */
    JOYPORT_RES_ID_MOUSE,             /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,          /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,             /* device uses the potentiometer lines for the right and middle buttons, but could work without it */
    JOYPORT_5VDC_REQUIRED,            /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,         /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,             /* device is a Mouse */
    0,                                /* NO output bits */
    joyport_mouse_set_enabled,        /* device enable/disable function */
    joyport_mouse_poll_value,         /* digital line read function */
    NULL,                             /* NO digital line store function */
    joyport_mouse_amiga_st_read_potx, /* pot-x read function */
    joyport_mouse_amiga_st_read_poty, /* pot-y read function */
    NULL,                             /* NO powerup function */
    mouse_amiga_write_snapshot,       /* device write snapshot function */
    mouse_amiga_read_snapshot,        /* device read snapshot function */
    NULL,                             /* NO device hook function */
    0                                 /* NO device hook function mask */
};

/* Some prototypes are needed */
static int mouse_cx22_write_snapshot(struct snapshot_s *s, int port);
static int mouse_cx22_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_cx22_joyport_device = {
    "Trackball (Atari CX-22)", /* name of the device */
    JOYPORT_RES_ID_MOUSE,      /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,   /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,      /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,     /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,  /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,      /* device is a Mouse/Trackball */
    0,                         /* NO output bits */
    joyport_mouse_set_enabled, /* device enable/disable function */
    joyport_mouse_poll_value,  /* digital line read function */
    NULL,                      /* NO digital line store function */
    NULL,                      /* NO pot-x read function */
    NULL,                      /* NO pot-y read function */
    NULL,                      /* NO powerup function */
    mouse_cx22_write_snapshot, /* device write snapshot function */
    mouse_cx22_read_snapshot,  /* device read snapshot function */
    NULL,                      /* NO device hook function */
    0                          /* NO device hook function mask */
};


/* Some prototypes are needed */
static int mouse_st_write_snapshot(struct snapshot_s *s, int port);
static int mouse_st_read_snapshot(struct snapshot_s *s, int port);

static joyport_t mouse_st_joyport_device = {
    "Mouse (Atari ST)",               /* name of the device */
    JOYPORT_RES_ID_MOUSE,             /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,          /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,             /* device uses the potentiometer lines for the right button, but could work without it */
    JOYPORT_5VDC_REQUIRED,            /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,         /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_MOUSE,             /* device is a Mouse */
    0,                                /* NO output bits */
    joyport_mouse_set_enabled,        /* device enable/disable function */
    joyport_mouse_poll_value,         /* digital line read function */
    NULL,                             /* NO digital line store function */
    joyport_mouse_amiga_st_read_potx, /* pot-x read function */
    joyport_mouse_amiga_st_read_poty, /* pot-y read function */
    NULL,                             /* NO powerup function */
    mouse_st_write_snapshot,          /* device write snapshot function */
    mouse_st_read_snapshot,           /* device read snapshot function */
    NULL,                             /* NO device hook function */
    0                                 /* NO device hook function mask */
};

/* --------------------------------------------------------- */

int mouse_amiga_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_AMIGA, &mouse_amiga_joyport_device);
}

int mouse_cx22_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_CX22, &mouse_cx22_joyport_device);
}

int mouse_st_register(void)
{
    return joyport_device_register(JOYPORT_ID_MOUSE_ST, &mouse_st_joyport_device);
}

/* --------------------------------------------------------- */

static int write_poll_val_snapshot(snapshot_module_t *m)
{
    if (0
        || SMW_B(m, quadrature_x) < 0
        || SMW_B(m, quadrature_y) < 0
        || SMW_B(m, polled_joyval) < 0) {
        return -1;
    }
    return 0;
}

static int read_poll_val_snapshot(snapshot_module_t *m)
{
    if (0
        || SMR_B(m, &quadrature_x) < 0
        || SMR_B(m, &quadrature_y) < 0
        || SMR_B(m, &polled_joyval) < 0) {
        return -1;
    }

    return 0;
}

/* MOUSE_AMIGA snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   DWORD  | buttons                | buttons state
 */

static const char mouse_amiga_snap_module_name[] = "MOUSE_AMIGA";
#define MOUSE_AMIGA_VER_MAJOR   1
#define MOUSE_AMIGA_VER_MINOR   0

static int mouse_amiga_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_amiga_snap_module_name, MOUSE_AMIGA_VER_MAJOR, MOUSE_AMIGA_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_B(m, mouse_digital_val) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_DW(m, (uint32_t)amiga_and_atarist_buttons) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_amiga_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_amiga_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_AMIGA_VER_MAJOR, MOUSE_AMIGA_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_B(m, &mouse_digital_val) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_DW_INT(m, &amiga_and_atarist_buttons) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_CX22 snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
 */

static const char mouse_cx22_snap_module_name[] = "MOUSE_CX22";
#define MOUSE_CX22_VER_MAJOR   1
#define MOUSE_CX22_VER_MINOR   0

static int mouse_cx22_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_cx22_snap_module_name, MOUSE_CX22_VER_MAJOR, MOUSE_CX22_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_B(m, mouse_digital_val) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_cx22_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_cx22_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_CX22_VER_MAJOR, MOUSE_CX22_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_B(m, &mouse_digital_val) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/* --------------------------------------------------------- */

/* MOUSE_ST snapshot module format:

   type   | name                   | description
   ---------------------------------------------
   BYTE   | digital value          | digital pins return value
   BYTE   | quadrature X           | quadrature X
   BYTE   | quadrature Y           | quadrature Y
   BYTE   | polled joyval          | polled joyval
   DWORD  | buttons                | buttons state
 */

static const char mouse_st_snap_module_name[] = "MOUSE_ST";
#define MOUSE_ST_VER_MAJOR   1
#define MOUSE_ST_VER_MINOR   0

static int mouse_st_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, mouse_st_snap_module_name, MOUSE_ST_VER_MAJOR, MOUSE_ST_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_B(m, mouse_digital_val) < 0) {
        goto fail;
    }

    if (write_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (SMW_DW(m, (uint32_t)amiga_and_atarist_buttons) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int mouse_st_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, mouse_st_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, MOUSE_ST_VER_MAJOR, MOUSE_ST_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_B(m, &mouse_digital_val) < 0) {
        goto fail;
    }

    if (read_poll_val_snapshot(m) < 0) {
        goto fail;
    }

    if (SMR_DW_INT(m, &amiga_and_atarist_buttons) < 0) {
        goto fail;
    }
    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
