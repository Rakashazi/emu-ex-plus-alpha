/*
 * c64_diag_586220_harness.h - c64 diagnosis (586220) cartridge harness hub emulation.
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

#ifndef VICE_C64_DIAG_586220_HARNESS_H
#define VICE_C64_DIAG_586220_HARNESS_H

#include "types.h"

#define C64_DIAG_USERPORT_PB0     0
#define C64_DIAG_USERPORT_PB1     1
#define C64_DIAG_USERPORT_PB2     2
#define C64_DIAG_USERPORT_PB3     3
#define C64_DIAG_USERPORT_PB4     4
#define C64_DIAG_USERPORT_PB5     5
#define C64_DIAG_USERPORT_PB6     6
#define C64_DIAG_USERPORT_PB7     7

#define C64_DIAG_USERPORT_PA2     2
#define C64_DIAG_USERPORT_PA3     3

#define C64_DIAG_USERPORT_SP1     0
#define C64_DIAG_USERPORT_SP2     1

#define C64_DIAG_USERPORT_CNT1    0
#define C64_DIAG_USERPORT_CNT2    1
#define C64_DIAG_USERPORT_PC2     2
#define C64_DIAG_USERPORT_FLAG2   3

#define C64_DIAG_TAPEPORT_MOTOR   0
#define C64_DIAG_TAPEPORT_READ    1
#define C64_DIAG_TAPEPORT_WRITE   2
#define C64_DIAG_TAPEPORT_SENSE   3

#define C64_DIAG_JOYPORT_UP       0
#define C64_DIAG_JOYPORT_DOWN     1
#define C64_DIAG_JOYPORT_LEFT     2
#define C64_DIAG_JOYPORT_RIGHT    3
#define C64_DIAG_JOYPORT_POTY     4
#define C64_DIAG_JOYPORT_BUTTON   5
#define C64_DIAG_JOYPORT_POTX     6

#define C64_DIAG_KEYBOARD_PA0     0
#define C64_DIAG_KEYBOARD_PA1     1
#define C64_DIAG_KEYBOARD_PA2     2
#define C64_DIAG_KEYBOARD_PA3     3
#define C64_DIAG_KEYBOARD_PA4     4
#define C64_DIAG_KEYBOARD_PA5     5
#define C64_DIAG_KEYBOARD_PA6     6
#define C64_DIAG_KEYBOARD_PA7     7
#define C64_DIAG_KEYBOARD_PB0     8
#define C64_DIAG_KEYBOARD_PB1     9
#define C64_DIAG_KEYBOARD_PB2     10
#define C64_DIAG_KEYBOARD_PB3     11
#define C64_DIAG_KEYBOARD_PB4     12
#define C64_DIAG_KEYBOARD_PB5     13
#define C64_DIAG_KEYBOARD_PB6     14
#define C64_DIAG_KEYBOARD_PB7     15

#define C64_DIAG_SERIAL_SRQ       0
#define C64_DIAG_SERIAL_ATN       1
#define C64_DIAG_SERIAL_CLK       2
#define C64_DIAG_SERIAL_DATA      3

extern void c64_diag_586220_init(void);

extern void c64_diag_586220_store_userport_pax(BYTE val);
extern BYTE c64_diag_586220_read_userport_pax(void);
extern void c64_diag_586220_store_userport_pbx(BYTE val);
extern BYTE c64_diag_586220_read_userport_pbx(void);

extern void c64_diag_586220_store_userport_sp(BYTE port, BYTE val);
extern BYTE c64_diag_586220_read_userport_sp(BYTE port);

extern void c64_diag_586220_store_tapeport(BYTE pin, BYTE val);
extern BYTE c64_diag_586220_read_tapeport(BYTE pin);

extern void c64_diag_586220_store_joyport_dig(BYTE port, BYTE val);
extern BYTE c64_diag_586220_read_joyport_dig(BYTE port);

extern BYTE c64_diag_586220_read_joyport_pot(void);

extern void c64_diag_586220_store_keyboard(BYTE port, BYTE val);
extern BYTE c64_diag_586220_read_keyboard(BYTE port);

extern void c64_diag_586220_store_serial(BYTE val);
extern BYTE c64_diag_586220_read_serial(void);

#endif
