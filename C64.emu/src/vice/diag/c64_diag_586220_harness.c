/*
 * c64_diag_586220_harness.c - c64 diagnosis cartridge harness hub emulation.
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
#include <string.h>

#include "c64_diag_586220_harness.h"
#include "datasette.h"
#include "types.h"

static BYTE c64_diag_userport_pax = 0;
static BYTE c64_diag_userport_pbx = 0;
static BYTE c64_diag_userport_sp1 = 0;
static BYTE c64_diag_userport_sp2 = 0;
static BYTE c64_diag_tapeport = 0;
static BYTE c64_diag_joyport0 = 0;
static BYTE c64_diag_joyport1 = 0;
static BYTE c64_diag_keyboard_pax = 0;
static BYTE c64_diag_keyboard_pbx = 0;
static BYTE c64_diag_serial = 0;
static BYTE c64_diag_switches = 0;

void c64_diag_586220_init(void)
{
    c64_diag_userport_pax = 0;
    c64_diag_userport_pbx = 0;
    c64_diag_userport_sp1 = 0;
    c64_diag_userport_sp2 = 0;
    c64_diag_tapeport = 0;
    c64_diag_joyport0 = 0;
    c64_diag_joyport1 = 0;
    c64_diag_keyboard_pax = 0;
    c64_diag_keyboard_pbx = 0;
    c64_diag_serial = 0;
    c64_diag_switches = 0;
}

void c64_diag_586220_store_userport_pax(BYTE val)
{
    c64_diag_userport_pax = val;
}

void c64_diag_586220_store_userport_pbx(BYTE val)
{
    c64_diag_userport_pbx = val;
}

void c64_diag_586220_store_userport_sp(BYTE port, BYTE val)
{
    if (!port) {
        c64_diag_userport_sp1 = val;
    } else {
        c64_diag_userport_sp2 = val;
    }
}

void c64_diag_586220_store_tapeport(BYTE pin, BYTE val)
{
    c64_diag_tapeport &= ~(1 << pin);
    c64_diag_tapeport |= (val << pin);

    switch (pin) {
        case C64_DIAG_TAPEPORT_MOTOR:
            machine_set_tape_write_in(val);
            break;
        case C64_DIAG_TAPEPORT_READ:
            machine_set_tape_sense(val);
            break;
        case C64_DIAG_TAPEPORT_WRITE:
            machine_set_tape_motor_in(val);
            break;
        case C64_DIAG_TAPEPORT_SENSE:
            machine_trigger_flux_change(val);
            break;
    }

    if (c64_diag_tapeport & 5) {
        c64_diag_switches = 1;
    } else {
        c64_diag_switches = 0;
    }
}

void c64_diag_586220_store_joyport_dig(BYTE port, BYTE val)
{
    if (!port) {
        c64_diag_joyport0 = val;
    } else {
        c64_diag_joyport1 = val;
    }
}

void c64_diag_586220_store_keyboard(BYTE port, BYTE val)
{
    if (!port) {
        c64_diag_keyboard_pax = val;
    } else {
        c64_diag_keyboard_pbx = val;
    }
}

void c64_diag_586220_store_serial(BYTE val)
{
    c64_diag_serial = val;
}

BYTE c64_diag_586220_read_userport_pax(void)
{
    BYTE retval;

    retval = (c64_diag_userport_pax & 4) << 1;
    retval |= (c64_diag_userport_pax & 2) >> 1;

    return retval;
}

BYTE c64_diag_586220_read_userport_pbx(void)
{
    BYTE retval;

    retval = c64_diag_userport_pbx >> 4;
    retval |= (c64_diag_userport_pbx & 0xf) << 4;

    return retval;
}

BYTE c64_diag_586220_read_userport_sp(BYTE port)
{
    if (!port) {
        return c64_diag_userport_sp2;
    }
    return c64_diag_userport_sp1;
}

BYTE c64_diag_586220_read_tapeport(BYTE pin)
{
    BYTE retval;

    retval = c64_diag_tapeport & 0xf5;
    retval |= (c64_diag_tapeport & 8) >> 2;
    retval |= (c64_diag_tapeport & 2) << 2;
    retval &= (1 << pin);

    return retval;
}

BYTE c64_diag_586220_read_joyport_dig(BYTE port)
{
    if (c64_diag_switches) {
        if (!port) {
            return c64_diag_joyport1;
        }
        return c64_diag_joyport0;
    }
    return 0;
}

BYTE c64_diag_586220_read_joyport_pot(void)
{
    return 0xff;
}

BYTE c64_diag_586220_read_keyboard(BYTE port)
{
    if (!port) {
        return c64_diag_keyboard_pbx;
    }
    return c64_diag_keyboard_pax;
}

BYTE c64_diag_586220_read_serial(void)
{
    BYTE retval;

    retval = (c64_diag_serial & 8) >> 3;
    retval |= (c64_diag_serial & 4) >> 1;
    retval |= (c64_diag_serial & 2) << 1;
    retval |= (c64_diag_serial & 1) << 3;

    return retval;
}
