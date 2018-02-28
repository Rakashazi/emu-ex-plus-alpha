/*
 * rushware_keypad.c - RushWare keypad emulation.
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

   cport | keypad  | I/O
   --------------------------
     1   | KEY0    |  I
     2   | KEY1    |  I
     3   | KEY2    |  I
     4   | KEY3    |  I
     6   | KEYDOWN |  I

The keypad has the following layout:

KEYPAD             KEYMAP KEYS
-----------------   ---------------------
| 7 | 8 | 9 | * |   |  1 |  2 |  3 |  4 |
-----------------   ---------------------
| 4 | 5 | 6 | / |   |  6 |  7 |  8 |  9 |
-----------------   ---------------------
| 1 | 2 | 3 | - |   | 11 | 12 | 13 | 14 |
-----------------   ---------------------
| . | 0 | E | + |   | 16 | 17 | 18 | 19 |
-----------------   ---------------------

E = enter

The keypad has a bit pattern for each key
and grounds pin 6 when a key is pressed.

which results in:

KEY PRESSED   KEYDOWN KEY3 KEY2 KEY1 KEY0
-----------   ------- ---- ---- ---- ----
<no key>         1     x    x    x    x
ENTER            0     0    0    0    0
.                0     0    0    0    1
*                0     0    0    1    0
/                0     0    0    1    1
-                0     0    1    0    0
+                0     0    1    0    1
9                0     0    1    1    0
8                0     0    1    1    1
7                0     1    0    0    0
6                0     1    0    0    1
5                0     1    0    1    0
4                0     1    0    1    1
3                0     1    1    0    0
2                0     1    1    0    1
1                0     1    1    1    0
0                0     1    1    1    1
*/

#define ROW_COL(x, y) ((x * 4) + y)

#define KEYPAD_KEY_7    ROW_COL(0,0)
#define KEYPAD_KEY_8    ROW_COL(0,1)
#define KEYPAD_KEY_9    ROW_COL(0,2)
#define KEYPAD_KEY_MULT ROW_COL(0,4)
#define KEYPAD_KEY_4    ROW_COL(1,0)
#define KEYPAD_KEY_5    ROW_COL(1,1)
#define KEYPAD_KEY_6    ROW_COL(1,2)
#define KEYPAD_KEY_DIV  ROW_COL(1,3)
#define KEYPAD_KEY_1    ROW_COL(2,0)
#define KEYPAD_KEY_2    ROW_COL(2,1)
#define KEYPAD_KEY_3    ROW_COL(2,2)
#define KEYPAD_KEY_MIN  ROW_COL(2,3)
#define KEYPAD_KEY_DOT  ROW_COL(3,0)
#define KEYPAD_KEY_0    ROW_COL(3,1)
#define KEYPAD_KEY_ENT  ROW_COL(3,2)
#define KEYPAD_KEY_PLUS ROW_COL(3,3)

static int rushware_keypad_enabled = 0;

static int keys[16];

/* ------------------------------------------------------------------------- */

#ifdef COMMON_KBD
static void handle_keys(int row, int col, int pressed)
{
    if (row < 0 || row > 3 || col < 1 || col > 4) {
        return;
    }

    keys[(row * 4) + col - 1] = pressed;
}
#endif

/* ------------------------------------------------------------------------- */

static int joyport_rushware_keypad_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == rushware_keypad_enabled) {
        return 0;
    }

    if (val) {
        memset(keys, 0, 16);
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(handle_keys);
#endif
    } else {
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(NULL);
#endif
    }

    rushware_keypad_enabled = val;

    return 0;
}

static BYTE rushware_keypad_read(int port)
{
    BYTE retval = 0xff;

    if (keys[KEYPAD_KEY_ENT]) {
        retval = 0xe0;
    }
    if (keys[KEYPAD_KEY_DOT]) {
        retval = 0xe1;
    }
    if (keys[KEYPAD_KEY_MULT]) {
        retval = 0xe2;
    }
    if (keys[KEYPAD_KEY_DIV]) {
        retval = 0xe3;
    }
    if (keys[KEYPAD_KEY_MIN]) {
        retval = 0xe4;
    }
    if (keys[KEYPAD_KEY_PLUS]) {
        retval = 0xe5;
    }
    if (keys[KEYPAD_KEY_9]) {
        retval = 0xe6;
    }
    if (keys[KEYPAD_KEY_8]) {
        retval = 0xe7;
    }
    if (keys[KEYPAD_KEY_7]) {
        retval = 0xe8;
    }
    if (keys[KEYPAD_KEY_6]) {
        retval = 0xe9;
    }
    if (keys[KEYPAD_KEY_5]) {
        retval = 0xea;
    }
    if (keys[KEYPAD_KEY_4]) {
        retval = 0xeb;
    }
    if (keys[KEYPAD_KEY_3]) {
        retval = 0xec;
    }
    if (keys[KEYPAD_KEY_2]) {
        retval = 0xed;
    }
    if (keys[KEYPAD_KEY_1]) {
        retval = 0xee;
    }
    if (keys[KEYPAD_KEY_0]) {
        retval = 0xef;
    }

    joyport_display_joyport(JOYPORT_ID_RUSHWARE_KEYPAD, (BYTE)~retval);

    return retval;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_rushware_keypad_device = {
    "RushWare Keypad",
    IDGS_RUSHWARE_KEYPAD,
    JOYPORT_RES_ID_KEYPAD,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_OPTIONAL,
    joyport_rushware_keypad_enable,
    rushware_keypad_read,
    NULL,               /* no digital store */
    NULL,               /* no pot-x read */
    NULL,               /* no pot-y read */
    NULL,               /* no write snapshot */
    NULL                /* no read snapshot */
};

/* ------------------------------------------------------------------------- */

int joyport_rushware_keypad_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_RUSHWARE_KEYPAD, &joyport_rushware_keypad_device);
}
