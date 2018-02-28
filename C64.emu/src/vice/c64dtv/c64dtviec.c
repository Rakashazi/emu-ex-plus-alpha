/*
 * c64dtviec.c - IEC bus handling for the C64DTV.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "c64dtv.h"
#include "c64iec.h"
#include "drive.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "maincpu.h"
#include "types.h"


void iec_update_cpu_bus(BYTE data)
{
    iecbus.cpu_bus = (((data << 2) & 0x80)
                      | ((data << 2) & 0x40)
                      | ((data << 1) & 0x10));
}

void iec_update_ports(void)
{
    unsigned int unit;

    iecbus.cpu_port = iecbus.cpu_bus;
    for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
        iecbus.cpu_port &= iecbus.drv_bus[unit];
    }

    iecbus.drv_port = (((iecbus.cpu_port >> 4) & 0x4)
                       | (iecbus.cpu_port >> 7)
                       | ((iecbus.cpu_bus << 3) & 0x80));
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
    return IEC_BUS_IEC;
}

void c64iec_init(void)
{
    iecbus_update_ports = iec_update_ports;
}

/* KLUDGES: dummy to satisfy linker, unused */
BYTE plus4tcbm_outputa[2], plus4tcbm_outputb[2], plus4tcbm_outputc[2];
void plus4tcbm_update_pa(BYTE byte, unsigned int dnr)
{
}
void plus4tcbm_update_pb(BYTE byte, unsigned int dnr)
{
}
void plus4tcbm_update_pc(BYTE byte, unsigned int dnr)
{
}

