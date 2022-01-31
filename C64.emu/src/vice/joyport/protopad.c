/*
 * protopad.c - Protopad emulation.
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

#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "maincpu.h"
#include "protopad.h"
#include "resources.h"
#include "snapshot.h"

#include "log.h"

/* Control port <--> Protopad connections:

   Compatibility mode:

   cport |  pin name | protopad   | I/O
   ------------------------------------
     1   |  JOY0     | dpad up    |  I
     2   |  JOY1     | dpad down  |  I
     3   |  JOY2     | dpad left  |  I
     4   |  JOY3     | dpad right |  I
     5   |  POTY     | A button   |  I
     6   |  JOY4     | Y button   |  I
     9   |  POTX     | X button   |  I

   Works on:
   - native joystick port(s) (x64/x64sc/xscpu64/x128/x64dtv/xcbm5x0/xvic)
   - hit userport joystick adapter port 1 (x64/x64sc/xscpu64/x128)
   - kingsoft userport joystick adapter port 1 (x64/x64sc/xscpu64/x128)
   - starbyte userport joystick adapter port 2 (x64/x64sc/xscpu64/x128)
   - hummer userport joystick adapter port (x64dtv)
   - oem userport joystick adapter port (xvic)
   - sidcart joystick adapter port (xplus4)

   in compatibility mode the following extra mappings exist:

   Select toggles dpad up on/off
   Start toggles permanent rapid fire on/off
   L button cycles through the rapid fire speeds
   R button rapid fires which ever fire button is pressed at the same time


   Native mode:

   cport |  pin name | protopad    | I/O
   -------------------------------------
     1   |  JOY0     | data bit 0  |  I
     2   |  JOY1     | data bit 1  |  I
     3   |  JOY2     | data bit 2  |  I
     4   |  JOY3     | clock       |  O
     6   |  JOY4     | native mode |  O

    Keeping JOY4 low enables native mode
 */

static int protopad_enabled[JOYPORT_MAX_PORTS] = {0};

static uint8_t counter[JOYPORT_MAX_PORTS] = {0};

static uint8_t clock_line[JOYPORT_MAX_PORTS] = {0};
static uint8_t mode_line[JOYPORT_MAX_PORTS] = {0};
static uint8_t dpad_mode[JOYPORT_MAX_PORTS] = {0};
static uint8_t rapid_button[JOYPORT_MAX_PORTS] = {0};
static uint8_t permanent_rapid[JOYPORT_MAX_PORTS] = {0};
static uint8_t rapid_speed[JOYPORT_MAX_PORTS] = {0};

/* FIXME: These speed selections are just a guess,
   more information is needed to make this part correct */
static int speed_selection[] = { 2, 4, 8, 16, 32, -1 };   /* Speed selections in flips per second, -1 means end of list */

/* ------------------------------------------------------------------------- */

static joyport_t joyport_protopad_device;

static int joyport_protopad_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == protopad_enabled[port]) {
        return 0;
    }

    if (val) {
        counter[port] = 0;
        clock_line[port] = 0;
        mode_line[port] = 1;
        dpad_mode[port] = 0;
        rapid_button[port] = 0;
        permanent_rapid[port] = 0;
        rapid_speed[port] = 0;
        joystick_set_snes_mapping(port);
    } else {
        joyport_clear_mapping(port);
    }

    protopad_enabled[port] = val;

    return 0;
}

static int protopad_get_autofire_on_off(int port)
{
    uint32_t second_cycles = (uint32_t)(maincpu_clk % machine_get_cycles_per_second());
    uint32_t cycles_per_flip = (uint32_t)(machine_get_cycles_per_second() / speed_selection[rapid_speed[port]]);
    uint32_t flip_part = second_cycles / cycles_per_flip;

    if (flip_part & 1) {
        return 0;
    }
    return 1;
}

static uint8_t protopad_read(int port)
{
    uint8_t retval = 0;
    uint16_t joyval = get_joystick_value(port);
    int button_up;
    int button_down;
    int button_left;
    int button_right;
    int button_fire;

    /* if the mode line is high we have compat mode, if low we have native mode */
    if (mode_line[port]) {
        button_up = JOYPORT_BIT_BOOL(joyval, JOYPORT_UP_BIT);
        button_down = JOYPORT_BIT_BOOL(joyval, JOYPORT_DOWN_BIT);
        button_left = JOYPORT_BIT_BOOL(joyval, JOYPORT_LEFT_BIT);
        button_right = JOYPORT_BIT_BOOL(joyval, JOYPORT_RIGHT_BIT);
        button_fire = JOYPORT_BIT_BOOL(joyval, JOYPORT_FIRE_BIT);

        /* check if up is pressed */
        if (button_up) {
            /* check if up needs to be ignored */
            if (!dpad_mode[port]) {
                /* check if auto-fire button is also pressed */
                if (rapid_button[port]) {
                    /* get up from autofire function */
                    button_up = protopad_get_autofire_on_off(port);
                }
            } else {
                button_up = 0;
            }
        }

        /* check if down is pressed */
        if (button_down) {
            /* check if auto-fire button is also pressed */
            if (rapid_button[port]) {
                /* get down from autofire function */
                button_down = protopad_get_autofire_on_off(port);
            }
        }

        /* check if left is pressed */
        if (button_left) {
            /* check if auto-fire button is also pressed */
            if (rapid_button[port]) {
                /* get left from autofire function */
                button_left = protopad_get_autofire_on_off(port);
            }
        }

        /* check if right is pressed */
        if (button_right) {
            /* check if auto-fire button is also pressed */
            if (rapid_button[port]) {
                /* get right from autofire function */
                button_right = protopad_get_autofire_on_off(port);
            }
        }

        /* check if permanent fire is on */
        if (permanent_rapid[port]) {
            /* check if fire is pressed */
            if (button_fire) {
                /* check if auto-fire is also pressed */
                if (rapid_button[port]) {
                    /* get fire from autofire function */
                    button_fire = protopad_get_autofire_on_off(port);
                }
            } else {
                /* get fire from autofire function */
                button_fire = protopad_get_autofire_on_off(port);
            }
        } else {
            /* check if fire is pressed */
            if (button_fire) {
                /* check if auto-fire is also pressed */
                if (rapid_button[port]) {
                    /* get fire from autofire function */
                    button_fire = protopad_get_autofire_on_off(port);
                }
            }
        }

        /* construct retval from the state of the buttons */
        retval = (button_up ? JOYPORT_UP : 0);
        retval |= (button_down ? JOYPORT_DOWN : 0);
        retval |= (button_left ? JOYPORT_LEFT : 0);
        retval |= (button_right ? JOYPORT_RIGHT : 0);
        retval |= (button_fire ? JOYPORT_FIRE : 0);
    } else {
        switch (counter[port]) {
            case PROTOPAD_TRIPPLE_0: /* return B A Right */
                retval = (uint8_t)((joyval & (JOYPORT_BUTTON_B | JOYPORT_BUTTON_A | JOYPORT_RIGHT)) >> 3);
                break;
            case PROTOPAD_TRIPPLE_1: /* return Left Down Up */
                retval = (uint8_t)(joyval & (JOYPORT_LEFT | JOYPORT_DOWN | JOYPORT_UP));
                break;
            case PROTOPAD_TRIPPLE_2: /* return Start Select Bumber-R */
                retval = (uint8_t)((joyval & (JOYPORT_BUTTON_START | JOYPORT_BUTTON_SELECT | JOYPORT_BUTTON_RIGHT_BUMBER)) >> 9);
                break;
            case PROTOPAD_TRIPPLE_3: /* return Bumber-L Y X */
                retval = (uint8_t)((joyval & (JOYPORT_BUTTON_LEFT_BUMBER | JOYPORT_BUTTON_Y | JOYPORT_BUTTON_X)) >> 6);
                break;
            default:
                retval = 0xff;
        }
    }

    return ~(retval & 0x1f);
}

static void protopad_store(int port, uint8_t val)
{
    uint8_t new_mode = JOYPORT_BIT_BOOL(val, JOYPORT_FIRE_BIT);     /* mode line is on joyport 'fire' pin */
    uint8_t new_clock = JOYPORT_BIT_BOOL(val, JOYPORT_RIGHT_BIT);   /* clock line is on joyport 'right' pin */

    if (clock_line[port] != new_clock) {
        counter[port]++;
        if (counter[port] == PROTOPAD_COUNT_MAX) {
            counter[port] = PROTOPAD_TRIPPLE_0;
        }
    }

    if (mode_line[port] && !new_mode) {
        if (!new_mode) {
            counter[port] = PROTOPAD_HANDSHAKE;
        }
    }

    mode_line[port] = new_mode;
    clock_line[port] = new_clock;
}

static uint8_t protopad_read_potx(int port)
{
    int button = get_joystick_value(port) & JOYPORT_FIRE_POTX;

    /* check if button is pressed */
    if (button) {
        /* check if autofire button is also pressed */
        if (rapid_button[port]) {
            /* get right from autofire function */
            button = protopad_get_autofire_on_off(port);
        }
    }

    return (uint8_t)(button ? 0x00 : 0xff);
}

static uint8_t protopad_read_poty(int port)
{
    int button = get_joystick_value(port) & JOYPORT_FIRE_POTY;

    /* check if button is pressed */
    if (button) {
        /* check if autofire button is also pressed */
        if (rapid_button[port]) {
            /* get right from autofire function */
            button = protopad_get_autofire_on_off(port);
        }
    }

    return (uint8_t)(button ? 0x00 : 0xff);
}

static void protopad_powerup(int port)
{
        counter[port] = 0;
        dpad_mode[port] = 0;
        rapid_button[port] = 0;
        permanent_rapid[port] = 0;
        rapid_speed[port] = 0;
}

/* ------------------------------------------------------------------------- */

static int protopad_lb_state[JOYPORT_MAX_PORTS] = {0};
static int protopad_select_state[JOYPORT_MAX_PORTS] = {0};
static int protopad_start_state[JOYPORT_MAX_PORTS] = {0};

static void protopad_state_button_hook(int port, uint16_t state)
{
    int new_lb_state;
    int new_select_state;
    int new_start_state;

    /* only handle state changes when in compatibility mode */
    if (mode_line[port]) {
        rapid_button[port] = JOYPORT_BIT_BOOL(state, JOYPORT_BUTTON_RIGHT_BUMBER_BIT);

        /* check if the select button / dpad mode selection button has been pressed */
        new_select_state = JOYPORT_BIT_BOOL(state, JOYPORT_BUTTON_SELECT_BIT);
        if (protopad_select_state[port] != new_select_state) {
            /* only toggle when pressed, release does not change the state */
            if (new_select_state) {
                dpad_mode[port] = !dpad_mode[port];
            }
            protopad_select_state[port] = new_select_state;
        }

        /* check if the start button / permanent fire mode selection button has been pressed */
        new_start_state = JOYPORT_BIT_BOOL(state, JOYPORT_BUTTON_START_BIT);
        if (protopad_start_state[port] != new_start_state) {
            /* only toggle when pressed, release does not change the state */
            if (new_start_state) {
                permanent_rapid[port] = !permanent_rapid[port];
            }
            protopad_start_state[port] = new_start_state;
        }

        /* check if the left bumper / fire speed selection button has been pressed */
        new_lb_state = JOYPORT_BIT_BOOL(state, JOYPORT_BUTTON_LEFT_BUMBER_BIT);
        if (protopad_lb_state[port] != new_lb_state) {
            /* only toggle when pressed, release does not change the state */
            if (new_lb_state) {
                rapid_speed[port]++;
                if (speed_selection[rapid_speed[port]] == -1) {
                    rapid_speed[port] = 0;
                }
            }
            protopad_lb_state[port] = new_lb_state;
        }
    }
}

/* ------------------------------------------------------------------------- */

static int protopad_write_snapshot(struct snapshot_s *s, int p);
static int protopad_read_snapshot(struct snapshot_s *s, int p);

#define LB     JOYPORT_BUTTON_LEFT_BUMBER
#define RB     JOYPORT_BUTTON_RIGHT_BUMBER
#define SELECT JOYPORT_BUTTON_SELECT
#define START  JOYPORT_BUTTON_START

static joyport_t joyport_protopad_device = {
    "Protopad",                  /* name of the device */
    JOYPORT_RES_ID_NONE,         /* device can be used in multiple ports at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,     /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,        /* device does NOT use the potentiometer lines */
    JOYSTICK_ADAPTER_ID_NONE,    /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_SNES_ADAPTER, /* device is a SNES adapter */
    0x18,                        /* bits 4, and 3 are output bits */
    joyport_protopad_enable,     /* device enable function */
    protopad_read,               /* digital line read function */
    protopad_store,              /* digital line store function */
    protopad_read_potx,          /* pot-x read function */
    protopad_read_poty,          /* pot-y read function */
    protopad_powerup,            /* powerup function */
    protopad_write_snapshot,     /* device write snapshot function */
    protopad_read_snapshot,      /* device read snapshot function */
    protopad_state_button_hook,  /* device hook function */
    LB | RB | SELECT | START     /* device hook function mask */

};

/* ------------------------------------------------------------------------- */

int joyport_protopad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_PROTOPAD, &joyport_protopad_device);
}

/* ------------------------------------------------------------------------- */

/* PROTOPAD snapshot module format:

   type  |   name      | description
   --------------------------------------
   BYTE  | COUNTER     | counter value
   BYTE  | CLOCK       | clock line state
   BYTE  | MODE        | mode line state
   BYTE  | DPADMODE    | dpad up mode state
   BYTE  | PERMRAPID   | permanent rapid mode state
   BYTE  | RAPIDSPEED  | rapid speed selection
 */

static const char snap_module_name[] = "PROTOPAD";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

static int protopad_write_snapshot(struct snapshot_s *s, int p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0 
        || SMW_B(m, counter[p]) < 0
        || SMW_B(m, clock_line[p]) < 0
        || SMW_B(m, mode_line[p]) < 0
        || SMW_B(m, dpad_mode[p]) < 0
        || SMW_B(m, permanent_rapid[p]) < 0
        || SMW_B(m, rapid_speed[p]) < 0) {
            snapshot_module_close(m);
            return -1;
    }
    return snapshot_module_close(m);
}

static int protopad_read_snapshot(struct snapshot_s *s, int p)
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
        || SMR_B(m, &counter[p]) < 0
        || SMR_B(m, &clock_line[p]) < 0
        || SMR_B(m, &mode_line[p]) < 0
        || SMR_B(m, &dpad_mode[p]) < 0
        || SMR_B(m, &permanent_rapid[p]) < 0
        || SMR_B(m, &rapid_speed[p]) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
