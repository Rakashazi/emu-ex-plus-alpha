/*
 * c64iec.c - IEC bus handling for the C64.
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

#include "c64.h"
#include "c64cart.h"
#include "c64iec.h"
#include "cartridge.h"
#include "drive.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "maincpu.h"
#include "types.h"

/* #define DEBUG_IECBUS_VCD */

#ifdef DEBUG_IECBUS_VCD

# include "machine.h"
# include "log.h"
# include <string.h>


#define IEC_DEBUG_PORTS() iec_debug_ports()

static void iec_debug_ports(void)
{
    unsigned int unit;

    /*! memory for the old state of the iecbus */
    static iecbus_t old_iecbus;

    /*! remember if this is the first call of this function (= 1) or not.
     *  We need this so that in case of the first call, all values are output.
     */
    static int firstcall = 1;

    static unsigned long cycles_per_second = 0;

    unsigned long time_usec;

    if (firstcall) {
        cycles_per_second = machine_get_cycles_per_second();

        if (cycles_per_second == 0) {
            /* make sure to output the cycle count instead. */
            cycles_per_second = 1000000;
        }
    }

    time_usec = (unsigned long) ((1000000. * maincpu_clk) / cycles_per_second );

    if ((old_iecbus.cpu_bus != iecbus.cpu_bus) || firstcall) {
        log_message(LOG_DEFAULT, "#%lu: cpu_bus changed from $%02x to $%02x.", time_usec, old_iecbus.cpu_bus, iecbus.cpu_bus);
        old_iecbus.cpu_bus = iecbus.cpu_bus;
    }

    if ((old_iecbus.cpu_port != iecbus.cpu_port) || firstcall) {
        log_message(LOG_DEFAULT, "#%lu: cpu_port changed from $%02x to $%02x.", time_usec, old_iecbus.cpu_port, iecbus.cpu_port);
        old_iecbus.cpu_port = iecbus.cpu_port;
    }

    if ((old_iecbus.drv_port != iecbus.drv_port) || firstcall) {
        log_message(LOG_DEFAULT, "#%lu: drv_port changed from $%02x to $%02x.", time_usec, old_iecbus.drv_port, iecbus.drv_port);
        old_iecbus.drv_port = iecbus.drv_port;
    }

    for (unit = 0; unit < 8 + DRIVE_NUM; unit++) {
        if ((old_iecbus.drv_bus[unit] != iecbus.drv_bus[unit]) || firstcall) {
            log_message(LOG_DEFAULT, "#%lu: drv_bus[ %2u] changed from $%02x to $%02x.", time_usec, unit, old_iecbus.drv_bus[unit], iecbus.drv_bus[unit]);
            old_iecbus.drv_bus[unit] = iecbus.drv_bus[unit];
        }

        if ((old_iecbus.drv_data[unit] != iecbus.drv_data[unit]) || firstcall) {
            log_message(LOG_DEFAULT, "#%lu: drv_data[%2u] changed from $%02x to $%02x.", time_usec, unit, old_iecbus.drv_data[unit], iecbus.drv_data[unit]);
            old_iecbus.drv_data[unit] = iecbus.drv_data[unit];
        }
    }

    /* remember: we already have been called, no extra processing */

    firstcall = 0;
}

#else

#define IEC_DEBUG_PORTS()

#endif

void iec_update_cpu_bus(BYTE data)
{
    iecbus.cpu_bus = (((data << 2) & 0x80) | ((data << 2) & 0x40) | ((data << 1) & 0x10));
}

void iec_update_ports(void)
{
    unsigned int unit;

    iecbus.cpu_port = iecbus.cpu_bus;
    for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
        iecbus.cpu_port &= iecbus.drv_bus[unit];
    }

    iecbus.drv_port = (((iecbus.cpu_port >> 4) & 0x4) | (iecbus.cpu_port >> 7) | ((iecbus.cpu_bus << 3) & 0x80));

    IEC_DEBUG_PORTS();
}

void iec_update_ports_embedded(void)
{
    iec_update_ports();
}

void iec_drive_write(BYTE data, unsigned int dnr)
{
    iecbus.drv_bus[dnr + 8] = (((data << 3) & 0x40) | ((data << 6) & ((~data ^ iecbus.cpu_bus) << 3) & 0x80));
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
    return IEC_BUS_IEC | (cartridge_type_enabled(CARTRIDGE_IEEE488) ? IEC_BUS_IEEE : 0);
}

void c64iec_init(void)
{
    iecbus_update_ports = iec_update_ports;
}
