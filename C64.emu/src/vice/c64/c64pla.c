/*
 * c64pla.c -- C64 PLA handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "c64pla.h"
#include "datasette.h"
#include "mem.h"
#include "tapeport.h"

/* Processor port.  */
pport_t pport;

/* Tape motor status.  */
static BYTE old_port_data_out = 0xff;

/* Tape write line status.  */
static BYTE old_port_write_bit = 0xff;

/* Tape sense line out status. */
static BYTE old_port_sense_out = 0xff;

void c64pla_config_changed(int tape_sense, int write_in, int motor_in, int caps_sense, BYTE pullup)
{
    pport.data_out = (pport.data_out & ~pport.dir) | (pport.data & pport.dir);

    pport.data_read = (pport.data | ~pport.dir) & (pport.data_out | pullup);

    if ((pullup & 0x40) && !caps_sense) {
        pport.data_read &= 0xbf;
    }

    if (!(pport.dir & 0x20)) {
        pport.data_read &= 0xdf;
    }

    if (tape_sense && !(pport.dir & 0x10)) {
        pport.data_read &= 0xef;
    }

    if (write_in && !(pport.dir & 0x08)) {
        pport.data_read &= 0xf7;
    }

    if (motor_in && !(pport.dir & 0x20)) {
        pport.data_read &= 0xdf;
    }

    if (((pport.dir & pport.data) & 0x20) != old_port_data_out) {
        old_port_data_out = (pport.dir & pport.data) & 0x20;
        tapeport_set_motor(!old_port_data_out);
    }

    if (((~pport.dir | pport.data) & 0x8) != old_port_write_bit) {
        old_port_write_bit = (~pport.dir | pport.data) & 0x8;
        tapeport_toggle_write_bit((~pport.dir | pport.data) & 0x8);
    }

    if (((pport.dir & pport.data) & 0x10) != old_port_sense_out) {
        old_port_sense_out = (pport.dir & pport.data) & 0x10;
        tapeport_set_sense_out(!old_port_sense_out);
    }

    pport.dir_read = pport.dir;
}

void c64pla_pport_reset(void)
{
    pport.data = 0x3f;
    pport.data_out = 0x3f;
    pport.data_read = 0x3f;
    pport.dir = 0;
    pport.dir_read = 0;
    pport.data_set_bit6 = 0;
    pport.data_set_bit7 = 0;
    pport.data_falloff_bit6 = 0;
    pport.data_falloff_bit7 = 0;
}
