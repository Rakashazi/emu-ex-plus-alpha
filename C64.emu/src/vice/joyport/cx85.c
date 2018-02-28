/*
 * cx85.c - Atari CX85 keypad emulation.
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

/* Control port <--> CX85 keypad connections:

   cport | keypad | I/O
   -------------------------
     1   | KEY0   |  I
     2   | KEY1   |  I
     3   | KEY2   |  I
     4   | KEY3   |  I
     5   | PRESS  |  I
     6   | KEY4   |  I

The keypad has the following layout:

KEYPAD                  KEYMAP KEYS
---------------------   --------------------------
| S | 7 | 8 | 9 | - |   |  0 |  1 |  2 |  3 |  4 |
---------------------   --------------------------
| N | 4 | 5 | 6 | E |   |  5 |  6 |  7 |  8 |    |
----------------| N |   --------------------|    |
| D | 1 | 2 | 3 | T |   | 10 | 11 | 12 | 13 |  9 |
----------------| E |   --------------------|    |
| Y |   0   | . | R |   | 15 |    16   | 18 |    |
---------------------   --------------------------

S = eScape
N = No
D = Delete
Y = Yes

The keypad grounds a line when a key is pressed.

The following logic is used:

KEY4 = !Escape
KEY3 = Minus || Enter || Dot || 0 || 3 || 2 || 1 || Yes || escape
KEY2 = 9 || 8 || 7 || No || Minus || Enter || Dot || 0 || escape
KEY1 = Enter || 2 || 8 || 5 || Minus || 3 || 9 || 6
KEY0 = Dot || 1 || 7 || 4 || Minus || 3 || 9 || 6

which results in:

KEY PRESSED   KEY4 KEY3 KEY2 KEY1 KEY0
-----------   ---- ---- ---- ---- ----
0               1    1    1    0    0
1               1    1    0    0    1
2               1    1    0    1    0
3               1    1    0    1    1
4               1    0    0    0    1
5               1    0    0    1    0
6               1    0    0    1    1
7               1    0    1    0    1
8               1    0    1    1    0
9               1    0    1    1    1
.               1    1    1    0    1
-               1    1    1    1    1
Enter           1    1    1    1    0
escape          0    1    1    0    0
No              1    0    1    0    0
Delete          1    0    0    0    0
Yes             1    1    0    0    0

The PRESS (POT AY) line is used to indicate a key press.
 */

#define KEYPAD_KEY_ESCAPE 0
#define KEYPAD_KEY_7      1
#define KEYPAD_KEY_8      2
#define KEYPAD_KEY_9      3
#define KEYPAD_KEY_MINUS  4

#define KEYPAD_KEY_NO     5
#define KEYPAD_KEY_4      6
#define KEYPAD_KEY_5      7
#define KEYPAD_KEY_6      8
#define KEYPAD_KEY_ENTER  9

#define KEYPAD_KEY_DELETE 10
#define KEYPAD_KEY_1      11
#define KEYPAD_KEY_2      12
#define KEYPAD_KEY_3      13

#define KEYPAD_KEY_YES    15
#define KEYPAD_KEY_0      16
#define KEYPAD_KEY_DOT    18

static int cx85_enabled = 0;

static unsigned int keys[20];

/* ------------------------------------------------------------------------- */

#ifdef COMMON_KBD
static void handle_keys(int row, int col, int pressed)
{
    /* ignore non-existing keys */
    if ((row == 2 && col == 4) || (row == 3 && col == 2) || (row == 3 && col == 4)) {
        return;
    }

    keys[(row * 5) + col] = (unsigned int)pressed;
}
#endif

/* ------------------------------------------------------------------------- */

static int joyport_cx85_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == cx85_enabled) {
        return 0;
    }

    if (val) {
        memset(keys, 0, 20);
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(handle_keys);
#endif
    } else {
#ifdef COMMON_KBD
        keyboard_register_joy_keypad(NULL);
#endif
    }

    cx85_enabled = val;

    return 0;
}

static BYTE cx85_read_dig(int port)
{
    unsigned int retval = 0;
    unsigned int tmp;

    /* KEY4 */
    tmp = !keys[KEYPAD_KEY_ESCAPE] << 4;
    retval |= tmp;

    /* KEY3 */
    tmp = keys[KEYPAD_KEY_MINUS] |
          keys[KEYPAD_KEY_ENTER] |
          keys[KEYPAD_KEY_DOT]   |
          keys[KEYPAD_KEY_0]     |
          keys[KEYPAD_KEY_3]     |
          keys[KEYPAD_KEY_2]     |
          keys[KEYPAD_KEY_1]     |
          keys[KEYPAD_KEY_YES]   |
          keys[KEYPAD_KEY_ESCAPE];
    tmp <<= 3;
    retval |= tmp;

    /* KEY2 */
    tmp = keys[KEYPAD_KEY_9]     |
          keys[KEYPAD_KEY_8]     |
          keys[KEYPAD_KEY_7]     |
          keys[KEYPAD_KEY_NO]    |
          keys[KEYPAD_KEY_MINUS] |
          keys[KEYPAD_KEY_ENTER] |
          keys[KEYPAD_KEY_DOT]   |
          keys[KEYPAD_KEY_0]     |
          keys[KEYPAD_KEY_ESCAPE];
    tmp <<= 2;
    retval |= tmp;

    /* KEY1 */
    tmp = keys[KEYPAD_KEY_ENTER] |
          keys[KEYPAD_KEY_2]     |
          keys[KEYPAD_KEY_8]     |
          keys[KEYPAD_KEY_5]     |
          keys[KEYPAD_KEY_MINUS] |
          keys[KEYPAD_KEY_3]     |
          keys[KEYPAD_KEY_9]     |
          keys[KEYPAD_KEY_6];
    tmp <<= 1;
    retval |= tmp;

    /* KEY0 */
    tmp = keys[KEYPAD_KEY_DOT]   |
          keys[KEYPAD_KEY_1]     |
          keys[KEYPAD_KEY_7]     |
          keys[KEYPAD_KEY_4]     |
          keys[KEYPAD_KEY_MINUS] |
          keys[KEYPAD_KEY_3]     |
          keys[KEYPAD_KEY_9]     |
          keys[KEYPAD_KEY_6];
    retval |= tmp;

    retval |= 0xe0;

    joyport_display_joyport(JOYPORT_ID_CX85_KEYPAD, (BYTE)~retval);

    return (BYTE)retval;
}

static BYTE cx85_read_pot(void)
{
    int i;

    for (i = 0; i < 20; ++i) {
        if (keys[i]) {
            return 0xff;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_cx85_device = {
    "Atari CX85 keypad",
    IDGS_CX85,
    JOYPORT_RES_ID_KEYPAD,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_cx85_enable,
    cx85_read_dig,
    NULL,               /* no digital store */
    NULL,               /* no pot-x read */
    cx85_read_pot,
    NULL,               /* no write snapshot */
    NULL                /* no read snapshot */
};

/* ------------------------------------------------------------------------- */

int joyport_cx85_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_CX85_KEYPAD, &joyport_cx85_device);
}
