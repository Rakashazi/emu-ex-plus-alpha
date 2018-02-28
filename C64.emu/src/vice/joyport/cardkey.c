/*
 * cardkey.c - Cardco Cardkey 1 keypad emulation.
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

/* Control port <--> Cardkey connections:

   cport | keypad | I/O
   -------------------------
     1   | KEY0   |  I
     2   | KEY1   |  I
     3   | KEY2   |  I
     4   | KEY3   |  I
     5   | PRESS  |  I

The keypad has the following layout:

KEYPAD                  KEYMAP KEYS
-----------------   ---------------------
| 7 | 8 | 9 | * |   |  1 |  2 |  3 |  4 |
-----------------   ---------------------
| 4 | 5 | 6 | D |   |  6 |  7 |  8 |  9 |
-----------------   ---------------------
| 1 | 2 | 3 | - |   | 11 | 12 | 13 | 14 |
-----------------   ---------------------
| . | 0 | E | + |   | 16 | 17 | 18 | 19 |
-----------------   ---------------------

E = Enter

The keypad grounds a line when a key is pressed.

The following logic is used:

KEY3 = 0 || 1 || 2 || 3 || 4 || 5 || 6 || 7
KEY2 = 0 || 1 || 2 || 3 || 8 || 9 || Plus || Minus
KEY1 = 0 || 1 || 8 || 9 || 4 || 5 || Div || Mult
KEY0 = 0 || 8 || 4 || Div || 2 || Plus || 6 || Dot

which results in:

KEY PRESSED   KEY3 KEY2 KEY1 KEY0
-----------   ---- ---- ---- ----
0               1    1    1    1
1               1    1    1    0
2               1    1    0    1
3               1    1    0    0
4               1    0    1    1
5               1    0    1    0
6               1    0    0    1
7               1    0    0    0
8               0    1    1    1
9               0    1    1    0
+               0    1    0    1
-               0    1    0    0
/               0    0    1    1
*               0    0    1    0
.               0    0    0    1
Enter           0    0    0    0

The PRESS (POT AY) line is used to indicate a key press.
 */

#define ROW_COL(x, y) ((x * 4) + y)

#define KEYPAD_KEY_7     ROW_COL(0,0)
#define KEYPAD_KEY_8     ROW_COL(0,1)
#define KEYPAD_KEY_9     ROW_COL(0,2)
#define KEYPAD_KEY_MULT  ROW_COL(0,3)

#define KEYPAD_KEY_4     ROW_COL(1,0)
#define KEYPAD_KEY_5     ROW_COL(1,1)
#define KEYPAD_KEY_6     ROW_COL(1,2)
#define KEYPAD_KEY_DIV   ROW_COL(1,3)

#define KEYPAD_KEY_1     ROW_COL(2,0)
#define KEYPAD_KEY_2     ROW_COL(2,1)
#define KEYPAD_KEY_3     ROW_COL(2,2)
#define KEYPAD_KEY_MINUS ROW_COL(2,3)

#define KEYPAD_KEY_DOT   ROW_COL(3,0)
#define KEYPAD_KEY_0     ROW_COL(3,1)
#define KEYPAD_KEY_ENTER ROW_COL(3,2)
#define KEYPAD_KEY_PLUS  ROW_COL(3,3)

static int cardkey_enabled = 0;

static unsigned int keys[16];

/* ------------------------------------------------------------------------- */

#ifdef COMMON_KBD
static void handle_keys(int row, int col, int pressed)
{
    if (row < 0 || row > 3 || col < 1 || col > 4) {
        return;
    }

    keys[(row * 4) + col - 1] = (unsigned int)pressed;
}
#endif

/* ------------------------------------------------------------------------- */

static int joyport_cardkey_enable(int port, int value)
{
    int val = value ? 1 : 0;

    if (val == cardkey_enabled) {
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

    cardkey_enabled = val;

    return 0;
}

static BYTE cardkey_read_dig(int port)
{
    unsigned int retval = 0;
    unsigned int tmp;

    /* KEY3 */
    tmp = keys[KEYPAD_KEY_0] |
          keys[KEYPAD_KEY_1] |
          keys[KEYPAD_KEY_2] |
          keys[KEYPAD_KEY_3] |
          keys[KEYPAD_KEY_4] |
          keys[KEYPAD_KEY_5] |
          keys[KEYPAD_KEY_6] |
          keys[KEYPAD_KEY_7];
    tmp <<= 3;
    retval |= tmp;

    /* KEY2 */
    tmp = keys[KEYPAD_KEY_0] |
          keys[KEYPAD_KEY_1] |
          keys[KEYPAD_KEY_2] |
          keys[KEYPAD_KEY_3] |
          keys[KEYPAD_KEY_8] |
          keys[KEYPAD_KEY_9] |
          keys[KEYPAD_KEY_PLUS] |
          keys[KEYPAD_KEY_MINUS];
    tmp <<= 2;
    retval |= tmp;

    /* KEY1 */
    tmp = keys[KEYPAD_KEY_0] |
          keys[KEYPAD_KEY_1] |
          keys[KEYPAD_KEY_8] |
          keys[KEYPAD_KEY_9] |
          keys[KEYPAD_KEY_4] |
          keys[KEYPAD_KEY_5] |
          keys[KEYPAD_KEY_DIV] |
          keys[KEYPAD_KEY_MULT];
    tmp <<= 1;
    retval |= tmp;

    /* KEY0 */
    tmp = keys[KEYPAD_KEY_0] |
          keys[KEYPAD_KEY_8] |
          keys[KEYPAD_KEY_4] |
          keys[KEYPAD_KEY_DIV] |
          keys[KEYPAD_KEY_2] |
          keys[KEYPAD_KEY_PLUS] |
          keys[KEYPAD_KEY_6] |
          keys[KEYPAD_KEY_DOT];
    retval |= tmp;

    retval |= 0xf0;

    joyport_display_joyport(JOYPORT_ID_CARDCO_KEYPAD, (BYTE)~retval);

    return (BYTE)retval;
}

static BYTE cardkey_read_pot(void)
{
    int i;

    for (i = 0; i < 16; ++i) {
        if (keys[i]) {
            return 0xff;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static joyport_t joyport_cardkey_device = {
    "Cardco Cardkey 1 keypad",
    IDGS_CARDKEY,
    JOYPORT_RES_ID_KEYPAD,
    JOYPORT_IS_NOT_LIGHTPEN,
    JOYPORT_POT_REQUIRED,
    joyport_cardkey_enable,
    cardkey_read_dig,
    NULL,				/* no digital store */
    NULL,				/* no pot-x read */
    cardkey_read_pot,
    NULL,				/* no write snapshot */
    NULL				/* no read snapshot */
};

/* ------------------------------------------------------------------------- */

int joyport_cardkey_resources_init(void)
{
    return joyport_device_register(JOYPORT_ID_CARDCO_KEYPAD, &joyport_cardkey_device);
}
