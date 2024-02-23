/*
 * mouse_paddle.c - Paddle-like devices
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

/* This file contains the implementation for all devices that work like paddles,
   ie the device puts absolute positions at the POT inputs, and uses the digital
   lines for additional buttons. */

/* Control port <--> mouse/paddles/pad connections:

   cport | paddles         | I/O
   -----------------------------
     3   | paddle X button |  I
     4   | paddle Y button |  I
     5   | paddle Y value  |  I
     7   |      +5VDC      |  Power
     8   |       GND       |  Ground
     9   | paddle X value  |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | koalapad     | I/O
   --------------------------
     3   | left button  |  I
     4   | right button |  I
     5   | Y-position   |  I
     7   |    +5VDC     |  Power
     8   |     GND      |  Ground
     9   | X-position   |  I

   Works on:
   - Native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)

   cport | microflyte joystick  | I/O
   ----------------------------------
     1   | Throttle up button   |  I
     2   | Throttle down button |  I
     3   | Brake button         |  I
     4   | Flaps button         |  I
     5   | up/down pot value    |  I
     6   | Reset button         |  I
     7   | +5VDC                |  Power
     8   | GND                  |  Ground
     9   | left/right pot value |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/xvic/xcbm5x0)
   - sidcart joystick adapter port (xplus4)
 */

/* #define DEBUG_PADDLE */

#ifdef DEBUG_PADDLE
#define DBG(_x_)  log_debug _x_
#else
#define DBG(_x_)
#endif

#include <stdlib.h> /* abs */

#include "vice.h"

#include "cmdline.h"
#include "joyport.h"
#include "joystick.h"
#include "maincpu.h"
#include "log.h"
#include "machine.h"
#include "mouse.h"
#include "mousedrv.h"
#include "resources.h"
#include "snapshot.h"
#include "vsyncapi.h"

#include "mouse_paddle.h"

/******************************************************************************/

static int paddles_p1_input = PADDLES_INPUT_MOUSE; /* host input source for paddles in port 1 */
static int paddles_p2_input = PADDLES_INPUT_MOUSE; /* host input source for paddles in port 2 */

static int16_t mouse_x;
static int16_t mouse_y;

/******************************************************************************/

/* FIXME: right now we only support one(!) paddle port */
#define MAXPORTS 2

typedef struct {
    uint8_t x, y;
} potvalues;

static potvalues pot_val[MAXPORTS] = {
    { 0x00, 0xff },
    { 0x00, 0xff }
};

typedef struct {
    int16_t x, y;
} potvalues_old;

static potvalues_old pot_old[MAXPORTS] = {
    { 0x00, 0xff },
    { 0x00, 0xff }
};

static uint8_t mouse_digital_val;

static inline uint8_t mouse_paddle_update(uint8_t paddle_v, int16_t *old_v, int16_t new_v)
{
    int16_t new_paddle = (int16_t)(paddle_v + new_v - *old_v);
    *old_v = new_v;

    if (new_paddle > 255) {
        new_paddle = 255;
    } else if (new_paddle < 0) {
        new_paddle = 0;
    }
    DBG(("mouse_paddle_update paddle:%d oldv:%d newv:%d ret:%d\n",
        paddle_v, *old_v, new_v, new_paddle));
    return (uint8_t)new_paddle;
}

/*
    note: for the expected behaviour look at testprogs/SID/paddles/readme.txt
*/

/* note: we divide mouse_x / mouse_y by two here, else paddle values will be
         changing too fast, making games unplayable */

#define PADDLE_DIV  2

static uint8_t mouse_get_paddle_x(int port)
{
    mouse_get_raw_int16(&mouse_x, &mouse_y);

    DBG(("mouse_get_paddle_x port:%d mouse enabled:%d mouse_x:%d mouse_y:%d\n",
         port, _mouse_enabled, mouse_x, mouse_y));

    if (port == JOYPORT_1 || (machine_class == VICE_MACHINE_PLUS4 && port == JOYPORT_PLUS4_SIDCART)) {
        /* Paddle on joystick port 1, or, joystick port 6 for plus4 which is the joystick port on the SID cartridge */
        if (paddles_p1_input == PADDLES_INPUT_JOY_AXIS) {
            /* return analog value of a host axis converted to 8bit */
            return joystick_get_axis_value(port, 0);
        } else {
            if (_mouse_enabled) {
                /* Use mouse for paddle value */
                pot_val[0].x = mouse_paddle_update(pot_val[0].x, &(pot_old[0].x), (int16_t)mouse_x / PADDLE_DIV);
                return (uint8_t)(0xff - pot_val[0].x);
            }
        }
    }

    if (port == JOYPORT_2) {
        /* Paddle on joystick port 2 */
        if (paddles_p2_input == PADDLES_INPUT_JOY_AXIS) {
            /* return analog value of a host axis converted to 8bit */
            return joystick_get_axis_value(port, 0);
        } else {
            if (_mouse_enabled) {
                /* Use mouse for paddle value */
                pot_val[0].x = mouse_paddle_update(pot_val[0].x, &(pot_old[0].x), (int16_t)mouse_x / PADDLE_DIV);
                return (uint8_t)(0xff - pot_val[0].x);
            }
        }
    }
    return 0xff;
}

static uint8_t mouse_get_paddle_y(int port)
{
    mouse_get_raw_int16(&mouse_x, &mouse_y);

    if (port == JOYPORT_1 || (machine_class == VICE_MACHINE_PLUS4 && port == JOYPORT_PLUS4_SIDCART)) {
        /* Paddle on joystick port 1, or, joystick port 6 for plus4 which is the joystick port on the SID cartridge */
        if (paddles_p1_input == PADDLES_INPUT_JOY_AXIS) {
            /* return analog value of a host axis converted to 8bit */
            return joystick_get_axis_value(port, 1);
        } else {
            if (_mouse_enabled) {
                /* Use mouse for paddle value */
                pot_val[0].y = mouse_paddle_update(pot_val[0].y, &(pot_old[0].y), (int16_t)mouse_y / PADDLE_DIV);
                return (uint8_t)(0xff - pot_val[0].y);
            }
        }
    }

    if (port == JOYPORT_2) {
        /* Paddle on joystick port 2 */
        if (paddles_p2_input == PADDLES_INPUT_JOY_AXIS) {
            /* return analog value of a host axis converted to 8bit */
            return joystick_get_axis_value(port, 1);
        } else {
            if (_mouse_enabled) {
                /* Use mouse for paddle value */
                pot_val[0].y = mouse_paddle_update(pot_val[0].y, &(pot_old[0].y), (int16_t)mouse_y / PADDLE_DIV);
                return (uint8_t)(0xff - pot_val[0].y);
            }
        }
    }
    return 0xff;
}

void paddles_button_left(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_LEFT;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_LEFT;
    }
}

void paddles_button_right(int pressed)
{
    if (pressed) {
        mouse_digital_val |= JOYPORT_RIGHT;
    } else {
        mouse_digital_val &= (uint8_t)~JOYPORT_RIGHT;
    }
}

static joyport_mapping_t paddles_mapping =  {
    "Paddle",   /* name of the device */
    NULL,       /* NO mapping of pin 0 (UP) */
    NULL,       /* NO mapping of pin 1 (DOWN) */
    NULL,       /* NO mapping of pin 2 (LEFT) */
    NULL,       /* NO mapping of pin 3 (RIGHT) */
    "Button 1", /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    "Button 2", /* name for the mapping of pin 5 (FIRE-2/SNES-B) */
    NULL,       /* NO mapping of pin 6 (FIRE-3/SNES-X) */
    NULL,       /* NO mapping of pin 7 (SNES-Y) */
    NULL,       /* NO mapping of pin 8 (SNES-LB) */
    NULL,       /* NO mapping of pin 9 (SNES-RB) */
    NULL,       /* NO mapping of pin 10 (SNES-SELECT) */
    NULL,       /* NO mapping of pin 11 (SNES-START) */
    "Paddle 1", /* name for the mapping of pot 1 (POT-X) */
    "Paddle 2"  /* name for the mapping of pot 2 (POT-Y) */
};

static uint8_t joyport_paddles_value(int port)
{
    uint16_t paddle_fire_buttons = get_joystick_value(port);
    uint8_t retval = 0xff;

    if (port == JOYPORT_1 || (machine_class == VICE_MACHINE_PLUS4 && port == JOYPORT_PLUS4_SIDCART)) {
        /* Paddle on joystick port 1, or, joystick port 6 for plus4 which is the joystick port on the SID cartridge */
        if (paddles_p1_input == PADDLES_INPUT_JOY_AXIS) {
            /* return joystick mapped button state */
            retval = (uint8_t)~((paddle_fire_buttons & 0x30) >> 2);
        } else {
            /* return mouse button */
            retval = _mouse_enabled ? (uint8_t)~mouse_digital_val : 0xff;
        }
    }

    if (port == JOYPORT_2) {
        /* Paddle on joystick port 2 */
        if (paddles_p2_input == PADDLES_INPUT_JOY_AXIS) {
            /* return joystick mapped button state */
            retval = (uint8_t)~((paddle_fire_buttons & 0x30) >> 2);
        } else {
            /* return mouse button */
            retval = _mouse_enabled ? (uint8_t)~mouse_digital_val : 0xff;
        }
    }

    joyport_display_joyport(port, mouse_type_to_id(mouse_type), (uint16_t)(~retval));
    return retval;
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

    /* convert joyport ID to mouse type*/
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

static int joyport_paddles_set_enabled(int port, int new_state)
{
    if (new_state) {
        /* enabled, set paddle mapping for the port */
        joyport_set_mapping(&paddles_mapping, port);
    } else {
        /* disabled, clear paddle mapping for the port */
        joyport_clear_mapping(port);
    }
    return joyport_mouse_set_enabled(port, new_state);
}

/* Some prototypes are needed */
static int paddles_write_snapshot(struct snapshot_s *s, int port);
static int paddles_read_snapshot(struct snapshot_s *s, int port);

static joyport_t paddles_joyport_device = {
    "Paddles",                   /* name of the device */
    JOYPORT_RES_ID_NONE,         /* device normally uses the mouse for input,
                                    but it can be mapped to a joystick axis too,
                                    therefor it is flagged as not using the mouse */
    JOYPORT_IS_NOT_LIGHTPEN,     /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,        /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,       /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,    /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_PADDLES,      /* device is a Paddle */
    0,                           /* NO output bits */
    joyport_paddles_set_enabled, /* device enable/disable function */
    joyport_paddles_value,       /* digital line read function */
    NULL,                        /* NO digital line store function */
    mouse_get_paddle_x,          /* pot-x read function */
    mouse_get_paddle_y,          /* pot-y read function */
    NULL,                        /* NO powerup function */
    paddles_write_snapshot,      /* device write snapshot function */
    paddles_read_snapshot,       /* device read snapshot function */
    NULL,                        /* NO device hook function */
    0                            /* NO device hook function mask */
};

static int set_paddles_p1_input(int axis_enabled, void *param)
{
    paddles_p1_input = axis_enabled ? PADDLES_INPUT_JOY_AXIS : PADDLES_INPUT_MOUSE;

    return 0;
}

static int set_paddles_p2_input(int axis_enabled, void *param)
{
    paddles_p2_input = axis_enabled ? PADDLES_INPUT_JOY_AXIS : PADDLES_INPUT_MOUSE;

    return 0;
}


static const resource_int_t resources_extra_int[] = {
    { "PaddlesInput1", PADDLES_INPUT_MOUSE, RES_EVENT_SAME, NULL,
      &paddles_p1_input, set_paddles_p1_input, NULL },
    { "PaddlesInput2", PADDLES_INPUT_MOUSE, RES_EVENT_SAME, NULL,
      &paddles_p2_input, set_paddles_p2_input, NULL },
    RESOURCE_INT_LIST_END
};

int paddles_resources_init(void)
{
    return resources_register_int(resources_extra_int);
}

static const cmdline_option_t cmdline_extra_option[] =
{
    { "-paddles1inputmouse", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PaddlesInput1", (void *)PADDLES_INPUT_MOUSE,
      NULL, "Use host mouse as input for paddles in port 1." },
    { "-paddles1inputjoyaxis", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PaddlesInput1", (void *)PADDLES_INPUT_JOY_AXIS,
      NULL, "Use host joystick axis as input for paddles in port 1." },
    { "-paddles2inputmouse", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PaddlesInput2", (void *)PADDLES_INPUT_MOUSE,
      NULL, "Use host mouse as input for paddles in port 2." },
    { "-paddles2inputjoyaxis", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PaddlesInput2", (void *)PADDLES_INPUT_JOY_AXIS,
      NULL, "Use host joystick axis as input for paddles in port 2." },
    CMDLINE_LIST_END
};

int paddles_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_extra_option);
}

/* PADDLES snapshot module format:

   type  | name                   | description
   ----------------------------------------
   BYTE  | digital value          | digital pins return value
   BYTE  | paddle 1 x-value       | paddle 1 x-value
   BYTE  | paddle 1 y-value       | paddle 1 y-value
   BYTE  | old paddle 1 x-value   | old paddle 1 x-value
   BYTE  | old paddle 1 y-value   | old paddle 1 y-value
 */

static int write_paddle_val_snapshot(snapshot_module_t *m)
{
    if (0
        || SMW_B(m, mouse_digital_val) < 0
        || SMW_B(m, pot_val[0].x) < 0
        || SMW_B(m, pot_val[0].y) < 0
        || SMW_W(m, (uint16_t)pot_old[0].x) < 0
        || SMW_W(m, (uint16_t)pot_old[0].y) < 0) {
        return -1;
    }
    return 0;
}

static int read_paddle_val_snapshot(snapshot_module_t *m)
{
    uint16_t paddle_old0;
    uint16_t paddle_old1;

    if (0
        || SMR_B(m, &mouse_digital_val) < 0
        || SMR_B(m, &pot_val[0].x) < 0
        || SMR_B(m, &pot_val[0].y) < 0
        || SMR_W(m, &paddle_old0) < 0
        || SMR_W(m, &paddle_old1) < 0) {
        return -1;
    }
    pot_old[0].x = (int16_t)paddle_old0;
    pot_old[0].y = (int16_t)paddle_old1;

    return 0;
}

static const char paddles_snap_module_name[] = "PADDLES";
#define PADDLES_VER_MAJOR   1
#define PADDLES_VER_MINOR   0

static int paddles_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, paddles_snap_module_name, PADDLES_VER_MAJOR, PADDLES_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (write_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int paddles_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, paddles_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, PADDLES_VER_MAJOR, PADDLES_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (read_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/*****************************************************************************/


static uint8_t joyport_koalapad_pot_x(int port)
{
    return _mouse_enabled ? (uint8_t)(255 - mouse_get_paddle_x(port)) : 0xff;
}

static uint8_t joyport_mouse_value(int port)
{
    uint8_t retval = 0xff;
    if (_mouse_enabled) {
        retval = (uint8_t)((~mouse_digital_val));
        joyport_display_joyport(port, mouse_type_to_id(mouse_type), (uint16_t)(~retval));
    }
    return retval;
}

/* Some prototypes are needed */
static int koalapad_write_snapshot(struct snapshot_s *s, int port);
static int koalapad_read_snapshot(struct snapshot_s *s, int port);

static joyport_t koalapad_joyport_device = {
    "KoalaPad",                 /* name of the device */
    JOYPORT_RES_ID_MOUSE,       /* device uses the mouse for input, only 1 mouse type device can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,    /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,       /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,      /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,   /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_DRAWING_PAD, /* device is a Drawing Tablet */
    0,                          /* NO output bits */
    joyport_mouse_set_enabled,  /* device enable/disable function */
    joyport_mouse_value,        /* digital line read function */
    NULL,                       /* NO digital line store function */
    joyport_koalapad_pot_x,     /* pot-x read function */
    mouse_get_paddle_y,         /* pot-y read function */
    NULL,                       /* NO powerup function */
    koalapad_write_snapshot,    /* device write snapshot function */
    koalapad_read_snapshot,     /* device read snapshot function */
    NULL,                       /* NO device hook function */
    0                           /* NO device hook function mask */
};

/* KOALAPAD snapshot module format:

   type  | name                   | description
   ----------------------------------------
   BYTE  | digital value          | digital pins return value
   BYTE  | paddle 1 x-value       | paddle 1 x-value
   BYTE  | paddle 1 y-value       | paddle 1 y-value
   BYTE  | old paddle 1 x-value   | old paddle 1 x-value
   BYTE  | old paddle 1 y-value   | old paddle 1 y-value
 */

static const char koalapad_snap_module_name[] = "KOALAPAD";
#define KOALAPAD_VER_MAJOR   0
#define KOALAPAD_VER_MINOR   0

static int koalapad_write_snapshot(struct snapshot_s *s, int port)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, koalapad_snap_module_name, KOALAPAD_VER_MAJOR, KOALAPAD_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (write_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (write_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

static int koalapad_read_snapshot(struct snapshot_s *s, int port)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, koalapad_snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major_version, minor_version, KOALAPAD_VER_MAJOR, KOALAPAD_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (read_mouse_common_snapshot(m) < 0) {
        goto fail;
    }

    if (read_paddle_val_snapshot(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

/*****************************************************************************/

static joyport_mapping_t mf_mapping =  {
    "Microflyte Joystick", /* name of the device on the port */
    "Throttle Up",         /* name for the mapping of pin 0 (UP) */
    "Throttle Down",       /* name for the mapping of pin 1 (DOWN) */
    "Brake",               /* name for the mapping of pin 2 (LEFT) */
    "Flaps",               /* name for the mapping of pin 3 (RIGHT) */
    "Reset",               /* name for the mapping of pin 4 (FIRE-1/SNES-A) */
    NULL,                  /* NO mapping of pin 5 (FIRE-2/SNES-B) */
    NULL,                  /* NO mapping of pin 6 (FIRE-3/SNES-X) */
    NULL,                  /* NO mapping of pin 7 (SNES-Y) */
    NULL,                  /* NO mapping of pin 8 (SNES-LB) */
    NULL,                  /* NO mapping of pin 9 (SNES-RB) */
    NULL,                  /* NO mapping of pin 10 (SNES-SELECT) */
    NULL,                  /* NO mapping of pin 11 (SNES-START) */
    "Up/Down",             /* name for the mapping of pot 1 (POT-X) */
    "Left/Right"           /* name for the mapping of pot 2 (POT-Y) */
};

static int joyport_mf_set_enabled(int port, int new_state)
{
    if (new_state) {
        /* enabled, set analog joystick mapping for the port */
        joyport_set_mapping(&mf_mapping, port);
    } else {
        /* disabled, clear analog joystick mapping for the port */
        joyport_clear_mapping(port);
    }
    return joyport_mouse_set_enabled(port, new_state);
}

static uint8_t joyport_mf_joystick_value(int port)
{
    uint16_t mf_fire_buttons = get_joystick_value(port);
    uint8_t retval = 0xff;

    if (port == JOYPORT_1 || (machine_class == VICE_MACHINE_PLUS4 && port == JOYPORT_PLUS4_SIDCART)) {
        /* Paddle on joystick port 1, or, joystick port 6 for plus4 which is the joystick port on the SID cartridge */
        if (paddles_p1_input == PADDLES_INPUT_JOY_AXIS) {
            /* return joystick mapped buttons */
            retval = (uint8_t)(~mf_fire_buttons);
        } else {
            /* return mouse buttons */
            retval = _mouse_enabled ? (uint8_t)~mouse_digital_val : 0xff;
        }
    }

    if (port == JOYPORT_2) {
        /* Paddle on joystick port 2 */
        if (paddles_p2_input == PADDLES_INPUT_JOY_AXIS) {
            /* return joystick mapped buttons */
            retval = (uint8_t)(~mf_fire_buttons);
        } else {
            /* return mouse buttons */
            retval = _mouse_enabled ? (uint8_t)~mouse_digital_val : 0xff;
        }
    }
    joyport_display_joyport(port, mouse_type_to_id(mouse_type), (uint16_t)(~retval));
    return retval;
}

static joyport_t mf_joystick_joyport_device = {
    "Microflyte Joystick",     /* name of the device */
    JOYPORT_RES_ID_NONE,       /* device normally uses the mouse for input,
                                 but it can be mapped to a joystick axis too,
                                 therefor it is flagged as not using the mouse */
    JOYPORT_IS_NOT_LIGHTPEN,   /* device is NOT a lightpen */
    JOYPORT_POT_REQUIRED,      /* device uses the potentiometer lines */
    JOYPORT_5VDC_REQUIRED,     /* device NEEDS +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,  /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_PADDLES,    /* device is a Paddle */
    0,                         /* NO output bits */
    joyport_mf_set_enabled,    /* device enable/disable function */
    joyport_mf_joystick_value, /* digital line read function */
    NULL,                      /* NO digital line store function */
    mouse_get_paddle_x,        /* pot-x read function */
    mouse_get_paddle_y,        /* pot-y read function */
    NULL,                      /* NO powerup function */
    paddles_write_snapshot,    /* device write snapshot function */
    paddles_read_snapshot,     /* device read snapshot function */
    NULL,                      /* NO device hook function */
    0                          /* NO device hook function mask */
};

int paddle_register(void)
{
    return joyport_device_register(JOYPORT_ID_PADDLES, &paddles_joyport_device);
}

int koalapad_register(void)
{
    return joyport_device_register(JOYPORT_ID_KOALAPAD, &koalapad_joyport_device);
}

int mf_joystick_register(void)
{
    return joyport_device_register(JOYPORT_ID_MF_JOYSTICK, &mf_joystick_joyport_device);
}
