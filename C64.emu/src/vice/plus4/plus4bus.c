/*
 * plus4bus.c - Generic interface to IO bus functions.
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

#include "iecbus.h"
#include "machine-bus.h"
#include "serial.h"
#include "types.h"


int machine_bus_lib_directory(unsigned int unit, const char *pattern,
                              BYTE **buf)
{
    return serial_iec_lib_directory(unit, pattern, buf);
}

int machine_bus_lib_read_sector(unsigned int unit, unsigned int track,
                                unsigned int sector, BYTE *buf)
{
    return serial_iec_lib_read_sector(unit, track, sector, buf);
}

int machine_bus_lib_write_sector(unsigned int unit, unsigned int track,
                                 unsigned int sector, BYTE *buf)
{
    return serial_iec_lib_write_sector(unit, track, sector, buf);
}

unsigned int machine_bus_device_type_get(unsigned int unit)
{
    return serial_device_type_get(unit);
}

void machine_bus_status_truedrive_set(unsigned int enable)
{
    iecbus_status_set(IECBUS_STATUS_TRUEDRIVE, 0, enable);
    serial_trap_truedrive_set(enable);
}

void machine_bus_status_drivetype_set(unsigned int unit, unsigned int enable)
{
    iecbus_status_set(IECBUS_STATUS_DRIVETYPE, unit, enable);
}

void machine_bus_status_virtualdevices_set(unsigned int enable)
{
    iecbus_status_set(IECBUS_STATUS_VIRTUALDEVICES, 0, enable);
}

void machine_bus_eof_callback_set(void (*func)(void))
{
    serial_trap_eof_callback_set(func);
}

void machine_bus_attention_callback_set(void (*func)(void))
{
    serial_trap_attention_callback_set(func);
}

void machine_bus_init_machine(void)
{
    iecbus_init();
}
