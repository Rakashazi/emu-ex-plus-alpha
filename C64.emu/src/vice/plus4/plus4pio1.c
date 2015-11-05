/*
 * plus4pio1.c -- Plus4 PIO1 handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "drive.h"
#include "drivetypes.h"
#include "maincpu.h"
#include "plus4parallel.h"
#include "plus4pio1.h"
#include "ted.h"
#include "types.h"


/* FIXME: C16 doesn't have 6529, writes can't mask off the tape_sense line */
/* FIXME: line 2 is used in RS232 IRQ as well at $EA62 in ROM */

static BYTE pio1_data = 0xff;

/* Tape sense line: 1 = some button pressed, 0 = no buttons pressed, or datasette not connected.  */
static int tape_sense = 0;

BYTE pio1_read(WORD addr)
{
    BYTE pio1_value;

    /*  Correct clock */
    ted_handle_pending_alarms(0);

    if (drive_context[0]->drive->parallel_cable
        || drive_context[1]->drive->parallel_cable) {
        pio1_value = parallel_cable_cpu_read(DRIVE_PC_STANDARD);
    } else {
        pio1_value = pio1_data;
    }

    if (tape_sense) {
        pio1_value &= ~4;
    }

    return pio1_value;
}

void pio1_store(WORD addr, BYTE value)
{
    BYTE pio1_outline;

    /*  Correct clock */
    ted_handle_pending_alarms(maincpu_rmw_flag + 1);

    pio1_data = value;

    pio1_outline = value;

    if (tape_sense) {
        pio1_outline &= ~4;
    }

    if (drive_context[0]->drive->parallel_cable
        || drive_context[1]->drive->parallel_cable) {
        parallel_cable_cpu_write(DRIVE_PC_STANDARD, pio1_outline);
    }
}

void pio1_set_tape_sense(int sense)
{
    BYTE pio1_outline;

    tape_sense = sense;

    pio1_outline = pio1_data;

    if (tape_sense) {
        pio1_outline &= ~4;
    }

    if (drive_context[0]->drive->parallel_cable
        || drive_context[1]->drive->parallel_cable) {
        parallel_cable_cpu_write(DRIVE_PC_STANDARD, pio1_outline);
    }
}

/*
    FIXME: snapshot support
    parallel_cable_cpu_undump(DRIVE_PC_STANDARD, (BYTE)data);
 */
