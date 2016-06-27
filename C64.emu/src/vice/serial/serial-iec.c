/*
 * serial-iec.c
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

#include "serial-iec-bus.h"
#include "serial-iec.h"
#include "types.h"


static int serial_iec_st = 0;
static unsigned int listen = 0;
static unsigned int talk = 0;


static void serial_iec_set_st(BYTE st)
{
    serial_iec_st = (int)st;
}

int serial_iec_open(unsigned int unit, unsigned int secondary,
                    const char *name, unsigned int length)
{
    unsigned int i;

    serial_iec_bus_open(unit, (BYTE)secondary, serial_iec_set_st);

    for (i = 0; i < length; i++) {
        serial_iec_bus_write(unit, (BYTE)secondary, name[i], serial_iec_set_st);
    }

    serial_iec_bus_unlisten(unit, (BYTE)secondary, serial_iec_set_st);

    return 0;
}

int serial_iec_close(unsigned int unit, unsigned int secondary)
{
    if (listen) {
        serial_iec_bus_unlisten(unit, (BYTE)secondary, serial_iec_set_st);
        listen = 0;
    }

    if (talk) {
        serial_iec_bus_untalk(unit, (BYTE)secondary, serial_iec_set_st);
        talk = 0;
    }

    serial_iec_bus_close(unit, (BYTE)secondary, serial_iec_set_st);

    return 0;
}

int serial_iec_read(unsigned int unit, unsigned int secondary, BYTE *data)
{
    if (listen) {
        serial_iec_bus_unlisten(unit, (BYTE)secondary, serial_iec_set_st);
        listen = 0;
    }

    if (!talk) {
        serial_iec_bus_talk(unit | 0x40, (BYTE)secondary, serial_iec_set_st);
        talk = 1;
    }

    *data = serial_iec_bus_read(unit, (BYTE)secondary, serial_iec_set_st);

    return serial_iec_st;
}

int serial_iec_write(unsigned int unit, unsigned int secondary, BYTE data)
{
    if (talk) {
        serial_iec_bus_untalk(unit, (BYTE)secondary, serial_iec_set_st);
        talk = 0;
    }

    if (!listen) {
        serial_iec_bus_listen(unit | 0x20, (BYTE)secondary, serial_iec_set_st);
        listen = 1;
    }

    serial_iec_bus_write(unit, (BYTE)secondary, data, serial_iec_set_st);

    return serial_iec_st;
}

int serial_iec_flush(unsigned int unit, unsigned int secondary)
{
    return 0;
}
