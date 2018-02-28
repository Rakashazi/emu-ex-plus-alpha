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
#include "translate.h"

/* Control port <--> coplin keypad connections:

   cport | keypad | I/O
   -------------------------
     1   | KEY0   |  I
     2   | KEY1   |  I
     3   | KEY2   |  I
     4   | KEY3   |  I
     6   | KEY4   |  I

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

static int coplin_keypad_enabled = 0;

static unsigned int keys[12];

/* ------------------------------------------------------------------------- */

#ifdef COMMON_KBD
static void handle_keys(int row, int col, int pressed)
{
    if (row < 0 || row > 3 || col < 1 || col > 3) {
        return;
    }

    keys[(row * 3) + col - 1] = (unsigned int)pressed;
}
#endif

/* ------------------------------------------------------------------------- */

static int joyport_coplin_keypad_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == coplin_keypad_enabled) {
        return 0;
    }

    if (val) {
        memset(keys, 0, 12);
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(handle_keys);
#endif
    } else {
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(NULL);
#endif
    }

    coplin_keypad_enabled = val;

    return 0;
}

static BYTE coplin_keypad_read(int port)
{
    unsigned int retval = 0;
    unsigned int tmp;

    /* KEY4 */
    tmp = !keys[KEYPAD_KEY_R] << 4;
    retval |= tmp;

    /* KEY3 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_6] & !keys[KEYPAD_KEY_9] & !keys[KEYPAD_KEY_3] & !keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_5]);
    tmp <<= 3;
    retval |= tmp;

    /* KEY2 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_4] & !keys[KEYPAD_KEY_7] & !keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_5] & !keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_1]);
    tmp <<= 2;
    retval |= tmp;

    /* KEY1 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_0] & !keys[KEYPAD_KEY_3] & !keys[KEYPAD_KEY_1] & !keys[KEYPAD_KEY_2]);
    tmp <<= 1;
    retval |= tmp;

    /* KEY0 */
    tmp = (unsigned int)(!keys[KEYPAD_KEY_P] & !keys[KEYPAD_KEY_9] & !keys[KEYPAD_KEY_7] & !keys[KEYPAD_KEY_8]);
    retval |= tmp;

    retval |= 0xe0;

    joyport_display_joyport(JOYPORT_ID_COPLIN_KEYPAD, (BYTE)~retval);

    return (BYTE)retval;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_coplin_keypad_device = {
    "Coplin Keypad",
    IDGS_COPLIN_KEYPAD,
    JOYPORT_RES_ID_KEYPAD,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_coplin_keypad_enable,
    coplin_keypad_read,
    NULL,               /* no digital store */
    NULL,               /* no pot-x read */
    NULL,               /* no pot-y read */
    NULL,               /* no write snapshot */
    NULL                /* no read snapshot */
};

/* ------------------------------------------------------------------------- */

int joyport_coplin_keypad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_COPLIN_KEYPAD, &joyport_coplin_keypad_device);
}
