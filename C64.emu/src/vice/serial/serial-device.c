/*
 * serial-device.c
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

#include "serial.h"


/* Initialized serial devices.  */
static serial_t serialdevices[SERIAL_MAXDEVICES];


serial_t *serial_device_get(unsigned int unit)
{
    return &serialdevices[unit];
}

unsigned int serial_device_type_get(unsigned int unit)
{
    serial_t *p;

    p = serial_device_get(unit);

    return p->device;
}

void serial_device_type_set(unsigned int type, unsigned int unit)
{
    serial_t *p;

    p = serial_device_get(unit);

    p->device = type;
}
