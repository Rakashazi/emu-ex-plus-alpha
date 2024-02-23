/*
 * coplin_keypad.c - Coplin keypad emulation.
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
#include "keyboard.h"

#include "coplin_keypad.h"

/* Control port <--> coplin keypad connections:

   cport | keypad | I/O
   -------------------------
     1   | KEY0   |  I
     2   | KEY1   |  I
     3   | KEY2   |  I
     4   | KEY3   |  I
     6   | KEY4   |  I

Works on:
- native port(s) (x64/x64sc/xscpu64/x64dtv/xvic/xplus4)
- inception joystick adapter ports (x64/x64sc/xscpu64/xvic)
- multijoy joystick adapter ports (x64/x64sc/xscpu64)
- spaceballs joystick adapter ports (x64/x64sc/xscpu64)
- cga userport joystick adapter ports (x64/x64sc/xscpu64)
- hit userport joystick adapter ports (x64/x64sc/xscpu64)
- hummer userport joystick adapter port (x64/x64sc/xscpu64)
- kingsoft userport joystick adapter ports (x64/x64sc/xscpu64)
- oem userport joystick adapter port (x64/x64sc/xscpu64)
- pet userport joystick adapter port (x64/x64sc/xscpu64)
- starbyte userport joystick adapter ports (x64/x64sc/xscpu64)
- synergy userport joystick adapter ports (x64/x64sc/xscpu64)
- stupid pet tricks userport joystick adapter port (x64/x64sc/xscpu64)
- wheel of joysticks userport joystick adapter ports (x64/x64sc/xscpu64)
- sidcart joystick adapter port (xplus4)

The keypad has the following layout:

KEYPAD             KEYMAP KEYS
-------------   ----------------
| 7 | 8 | 9 |   |  1 |  2 |  3 |
-------------   ----------------
| 4 | 5 | 6 |   |  6 |  7 |  8 |
-------------   ----------------
| 1 | 2 | 3 |   | 11 | 12 | 13 |
-------------   ----------------
| 0 | P | R |   | 16 | 17 | 18 |
-------------   ----------------

P = pr
R = return

The keypad grounds a line when a key is pressed.

The following logic is used:

KEY0 = P && 9 && 7 && 8
KEY1 = 0 && 3 && 1 && 2
KEY2 = 4 && 7 && P && 5 && 0 && 1
KEY3 = 6 && 9 && 3 && 0 && P && 5
KEY4 = r;

which results in:

KEY PRESSED   RP0123456789   KEY4 KEY3 KEY2 KEY1 KEY0
-----------   ------------   ---- ---- ---- ---- ----
<no key>      111111111111     1    1    1    1    1
9             111111111110     1    0    1    1    0
8             111111111101     1    1    1    1    0
7             111111111011     1    1    0    1    0
6             111111110111     1    0    1    1    1
5             111111101111     1    0    0    1    1
4             111111011111     1    1    0    1    1
3             111110111111     1    0    1    0    1
2             111101111111     1    1    1    0    1
1             111011111111     1    1    0    0    1
0             110111111111     1    0    0    0    1
PR            101111111111     1    0    0    1    0
RETURN        011111111111     0    1    1    1    1
 */

#define ROW_COL(x, y) ((x * 3) + y)

#define KEYPAD_KEY_7 ROW_COL(0,0)
#define KEYPAD_KEY_8 ROW_COL(0,1)
#define KEYPAD_KEY_9 ROW_COL(0,2)
#define KEYPAD_KEY_4 ROW_COL(1,0)
#define KEYPAD_KEY_5 ROW_COL(1,1)
#define KEYPAD_KEY_6 ROW_COL(1,2)
#define KEYPAD_KEY_1 ROW_COL(2,0)
#define KEYPAD_KEY_2 ROW_COL(2,1)
#define KEYPAD_KEY_3 ROW_COL(2,2)
#define KEYPAD_KEY_0 ROW_COL(3,0)
#define KEYPAD_KEY_P ROW_COL(3,1)
#define KEYPAD_KEY_R ROW_COL(3,2)

#define KEYPAD_KEYS_NUM  12

static int coplin_keypad_enabled = 0;

static unsigned int keys[KEYPAD_KEYS_NUM];

/* ------------------------------------------------------------------------- */

static void handle_keys(int row, int col, int pressed)
{
    /* sanity check for row and col, row should be 0-3, and col should be 1-3 */
    if (row < 0 || row > 3 || col < 1 || col > 3) {
        return;
    }

    /* change the state of the key that the row/col is wired to */
    keys[(row * 3) + col - 1] = (unsigned int)pressed;
}

/* ------------------------------------------------------------------------- */

static int joyport_coplin_keypad_set_enabled(int port, int enabled)
{
    int new_state = enabled ? 1 : 0;

    if (new_state == coplin_keypad_enabled) {
        return 0;
    }

    if (new_state) {
        /* enabled, clear keys and register the keypad */
        memset(keys, 0, KEYPAD_KEYS_NUM * sizeof(unsigned int));
        keyboard_register_joy_keypad(handle_keys);
    } else {
        /* disabled, unregister the keypad */
        keyboard_register_joy_keypad(NULL);
    }

    /* set the current state */
    coplin_keypad_enabled = new_state;

    return 0;
}

static uint8_t coplin_keypad_read(int port)
{
    unsigned int retval = 0;
    unsigned int tmp;

    /* KEY4 */
    tmp = !keys[KEYPAD_KEY_R] << JOYPORT_FIRE_BIT;   /* output key 4 on the joyport 'fire' pin */
    retval |= tmp;

    /* KEY3 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_6] & !keys[KEYPAD_KEY_9] & !keys[KEYPAD_KEY_3] & !keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_5]);
    tmp <<= JOYPORT_RIGHT_BIT;   /* output key 3 on the joyport 'right' pin */
    retval |= tmp;

    /* KEY2 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_4] & !keys[KEYPAD_KEY_7] & !keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_5] & !keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_1]);
    tmp <<= JOYPORT_LEFT_BIT;   /* output key 2 on the joyport 'left' pin */
    retval |= tmp;

    /* KEY1 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_3] & !keys[KEYPAD_KEY_1] & !keys[KEYPAD_KEY_2]);
    tmp <<= JOYPORT_DOWN_BIT;   /* output key 1 on the joyport 'down' pin */
    retval |= tmp;

    /* KEY0 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_9] & !keys[KEYPAD_KEY_7] & !keys[KEYPAD_KEY_8]);
    retval |= tmp;   /* output key 0 on the joyport 'up' pin */

    retval |= 0xe0;

    joyport_display_joyport(port, JOYPORT_ID_COPLIN_KEYPAD, (uint16_t)~retval);

    return (uint8_t)retval;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_coplin_keypad_device = {
    "Keypad (Coplin)",                 /* name of the device */
    JOYPORT_RES_ID_KEYPAD,             /* device is a keypad, only 1 keypad can be active at the same time */
    JOYPORT_IS_NOT_LIGHTPEN,           /* device is NOT a lightpen */
    JOYPORT_POT_OPTIONAL,              /* device does NOT use the potentiometer lines */
    JOYPORT_5VDC_NOT_NEEDED,           /* device does NOT need +5VDC to work */
    JOYSTICK_ADAPTER_ID_NONE,          /* device is NOT a joystick adapter */
    JOYPORT_DEVICE_KEYPAD,             /* device is a Keypad */
    0,                                 /* No output bits */
    joyport_coplin_keypad_set_enabled, /* device enable/disable function */
    coplin_keypad_read,                /* digital line read function */
    NULL,                              /* NO digital line store function */
    NULL,                              /* NO pot-x read function */
    NULL,                              /* NO pot-y read function */
    NULL,                              /* NO powerup function */
    NULL,                              /* NO device write snapshot function */
    NULL,                              /* NO device read snapshot function */
    NULL,                              /* NO device hook function */
    0                                  /* NO device hook function mask */
};

/* ------------------------------------------------------------------------- */

int joyport_coplin_keypad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_COPLIN_KEYPAD, &joyport_coplin_keypad_device);
}
