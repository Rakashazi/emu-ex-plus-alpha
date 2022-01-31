/*
 * plus4datasette.c - Plus4 specific CBM cassette implementation.
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

#include "datasette.h"
#include "plus4mem.h"
#include "tapeport.h"

void machine_trigger_flux_change(int port, unsigned int on)
{
    if (port == TAPEPORT_PORT_1) {
        mem_proc_port_trigger_flux_change(on);
    }
}

void machine_set_tape_sense(int port, int sense)
{
    if (port == TAPEPORT_PORT_1) {
        pio1_set_tape_sense(sense);
    }
}

void machine_set_tape_write_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        mem_proc_port_set_write_in(val);
    }
}

void machine_set_tape_motor_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        mem_proc_port_set_motor_in(val);
    }
}
