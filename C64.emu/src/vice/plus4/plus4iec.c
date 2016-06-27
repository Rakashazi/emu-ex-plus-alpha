/*
 * plus4iec.c - IEC bus handling for the Plus4.
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

#include <stdio.h>
#include <string.h>

#include "drive.h"
#include "maincpu.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "plus4iec.h"
#include "types.h"


void iec_update_cpu_bus(BYTE data)
{
    iecbus.cpu_bus = (((data << 7) & 0x80) | ((data << 5) & 0x40) | ((data << 2) & 0x10));
}

void iec_update_ports(void)
{
    unsigned int unit;

    iecbus.cpu_port = iecbus.cpu_bus;
    for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
        iecbus.cpu_port &= iecbus.drv_bus[unit];
    }

    iecbus.drv_port = (((iecbus.cpu_port >> 4) & 0x4) | (iecbus.cpu_port >> 7) | ((iecbus.cpu_bus << 3) & 0x80));
}

void iec_update_ports_embedded(void)
{
    iec_update_ports();
}

void iec_drive_write(BYTE data, unsigned int dnr)
{
    iecbus.drv_bus[dnr + 8] = (((data << 3) & 0x40)
                               | ((data << 6) & ((~data ^ iecbus.cpu_bus) << 3)
                                  & 0x80));
    iecbus.drv_data[dnr + 8] = data;
    iec_update_ports();
}

BYTE iec_drive_read(unsigned int dnr)
{
    return iecbus.drv_port;
}

iecbus_t *iecbus_drive_port(void)
{
    return &iecbus;
}

/* This function is called from ui_update_menus() */
int iec_available_busses(void)
{
    return IEC_BUS_IEC | IEC_BUS_TCBM;
}

void iec_fast_drive_write(BYTE data, unsigned int dnr)
{
/* The Plus4 does not use fast IEC.  */
}

void iec_fast_drive_direction(int direction, unsigned int dnr)
{
}

void plus4iec_init(void)
{
    iecbus_update_ports = iec_update_ports;
}
