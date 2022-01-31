/*
 * cbm2datasette.c - CBM-II specific CBM cassette implementation.
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

/* #define DEBUG_TAPE */

#include "vice.h"

#include "cbm2.h"
#include "cbm2tpi.h"
#include "cia.h"
#include "datasette.h"
#include "log.h"
#include "tapeport.h"

#ifdef DEBUG_TAPE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#ifdef DEBUG_TAPE
static void logit(int f, int n)
{
    static char *names[4] = {
        "machine_trigger_flux_change",
        "machine_set_tape_sense",
        "machine_set_tape_write_in",
        "machine_set_tape_motor_in"
    };
    static int buf[4];
    buf[f] = n;
    log_debug("%28s flux:%d sense:%d write:%d motor:%d",
              names[f], buf[0], buf[1], buf[2], buf[3]);

}
#else
#define logit(f, n)
#endif

void machine_trigger_flux_change(int port, unsigned int on)
{
    if (port == TAPEPORT_PORT_1) {
        logit(0, on);
        ciacore_set_flag(machine_context.cia1);
    }
}

void machine_set_tape_sense(int port, int sense)
{
    if (port == TAPEPORT_PORT_1) {
        logit(1, sense);
        tpi1_set_tape_sense(sense);
    }
}

void machine_set_tape_write_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        logit(2, val);
        tpi1_set_tape_write_in(val);
    }
}

void machine_set_tape_motor_in(int port, int val)
{
    if (port == TAPEPORT_PORT_1) {
        logit(3, val);
        tpi1_set_tape_motor_in(val);
    }
}
