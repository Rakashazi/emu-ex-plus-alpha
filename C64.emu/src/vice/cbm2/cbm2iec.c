/*
 * cbm2iec.c - IEC bus handling for the CBM2.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "cbm2iec.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "types.h"


void cbm2iec_init(void)
{
}

void iec_update_ports(void)
{
    /* Not used for now.  */
}

void iec_update_ports_embedded(void)
{
    iec_update_ports();
}

iecbus_t *iecbus_drive_port(void)
{
    return NULL;
}

BYTE iec_drive_read(unsigned int dnr)
{
    /* FIXME: unused */
    return 0;
}

void iec_drive_write(BYTE data, unsigned int dnr)
{
    /* FIXME: unused */
    iec_update_ports();
}

BYTE parallel_cable_drive_read(int type, int handshake)
{
    return 0;
}

void parallel_cable_drive_write(int type, BYTE data, int handshake, unsigned int dnr)
{
}

int iec_available_busses(void)
{
    return IEC_BUS_IEEE;
}

void iec_fast_drive_direction(int direction, unsigned int dnr)
{
}

void iec_fast_drive_write(BYTE data, unsigned int dnr)
{
}

void debug_iec_drv_read(unsigned int data)
{
    /* FIXME: unused */
}

void debug_iec_drv_write(unsigned int data)
{
    /* FIXME: unused */
}

void debug_iec_bus_read(unsigned int data)
{
    /* FIXME: unused */
}

void debug_iec_bus_write(unsigned int data)
{
    /* FIXME: unused */
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

