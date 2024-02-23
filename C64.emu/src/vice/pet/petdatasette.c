/*
 * petdatasette.c - PET specific CBM cassette implementation.
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
#include "pet.h"
#include "petpia.h"
#include "petvia.h"
#include "tapeport.h"
#include "via.h"

void machine_trigger_flux_change(int port, unsigned int on)
{
    if (port == TAPEPORT_PORT_1) {
        pia1_signal(PIA_SIG_CA1, PIA_SIG_FALL, 0);      /* TODO: offset */
    } else {
        viacore_signal(machine_context.via, VIA_SIG_CB1, VIA_SIG_FALL);
    }
}

void machine_set_tape_sense(int port, int sense)
{
    if (port == TAPEPORT_PORT_1) {
        pia1_set_tape1_sense(sense);
    } else {
        pia1_set_tape2_sense(sense);
    }
}

void machine_set_tape_write_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        pia1_set_tape1_write_in(val);
    } else {
        pia1_set_tape2_write_in(val);
    }
}

void machine_set_tape_motor_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        pia1_set_tape1_motor_in(val);
    } else {
        pia1_set_tape2_motor_in(val);
    }
}
