/*
 * serial.c - IEEE and serial device implementation.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "machine-bus.h"
#include "printer.h"
#include "serial-trap.h"
#include "serial.h"
#include "traps.h"
#include "types.h"


/* Flag: Have traps been installed?  */
static int traps_installed = 0;

/* Pointer to list of traps we are using.  */
static const trap_t *serial_traps;

/* Logging goes here.  */
static log_t serial_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

int serial_init(const trap_t *trap_list)
{
    serial_log = log_open("Serial");

    /* Remove installed traps, if any.  */
    serial_remove_traps();

    /* Install specified traps.  */
    serial_traps = trap_list;
    serial_install_traps();

    if (printer_serial_late_init() < 0) {
        return -1;
    }

    return 0;
}

void serial_shutdown(void)
{
    unsigned int unit;

    for (unit = 0; unit < SERIAL_MAXDEVICES; unit++) {
        machine_bus_device_detach(unit);
    }
}

int serial_install_traps(void)
{
    if (!traps_installed && serial_traps != NULL) {
        const trap_t *p;

        for (p = serial_traps; p->func != NULL; p++) {
            traps_add(p);
        }
        traps_installed = 1;
    }
    return 0;
}

int serial_remove_traps(void)
{
    if (traps_installed && serial_traps != NULL) {
        const trap_t *p;

        for (p = serial_traps; p->func != NULL; p++) {
            traps_remove(p);
        }
        traps_installed = 0;
    }
    return 0;
}
