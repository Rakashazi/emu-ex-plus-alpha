/*
 * vic20.c - IEC bus handling for the VIC20.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "cia.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "maincpu.h"
#include "resources.h"
#include "types.h"
#include "via.h"
#include "vic20iec.h"


#define NOT(x) ((x) ^ 1)


static uint8_t cpu_data, cpu_clock, cpu_atn;
static uint8_t drive_data[NUM_DISK_UNITS], drive_clock[NUM_DISK_UNITS];
static uint8_t drive_atna[NUM_DISK_UNITS], drive_data_modifier[NUM_DISK_UNITS];
static uint8_t bus_data, bus_clock, bus_atn;
static uint8_t cpu_bus_val;

static inline void resolve_bus_signals(void)
{
    unsigned int i;

    bus_atn = NOT(cpu_atn);
    bus_clock = NOT(cpu_clock);
    bus_data = NOT(cpu_data);

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        diskunit_context_t *unit = diskunit_context[i];

        bus_clock &= unit->enable ? NOT(drive_clock[i]) : 0x01;
        bus_data &= unit->enable ? NOT(drive_data[i])
                    & NOT(drive_data_modifier[i]) : 0x01;
    }
}

void iec_update_ports(void)
{
    /* Not used for now.  */
}

void vic20iec_init(void)
{
    iecbus_update_ports = iec_update_ports;
    cpu_clock = 1;
}

void iec_update_cpu_bus(uint8_t data)
{
}

void iec_update_ports_embedded(void)
{
    iec_update_ports();
}

static void iec_calculate_data_modifier(unsigned int dnr)
{
    switch (diskunit_context[dnr]->type) {
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            drive_data_modifier[dnr] = (cpu_atn & drive_atna[dnr]);
            break;
        default:
            drive_data_modifier[dnr] = (NOT(cpu_atn) ^ NOT(drive_atna[dnr]));
    }
}

void iec_drive_write(uint8_t data, unsigned int dnr)
{
    data = ~data;
    drive_data[dnr] = ((data & 2) >> 1);
    drive_clock[dnr] = ((data & 8) >> 3);
    drive_atna[dnr] = ((data & 16) >> 4);
    iec_calculate_data_modifier(dnr);
    resolve_bus_signals();
}

uint8_t iec_drive_read(unsigned int dnr)
{
    uint8_t drive_bus_val;

    drive_bus_val = bus_data | (bus_clock << 2) | (bus_atn << 7);

    return drive_bus_val;
}

/*
   The VIC20 has a strange bus layout for the serial IEC bus.

     VIA1 CA2 CLK out
     VIA1 CB1 SRQ in
     VIA1 CB2 DATA out
     VIA2 PA0 CLK in
     VIA2 PA1 DATA in
     VIA2 PA7 ATN out

 */

/* These two routines are called for VIA2 Port A. */

uint8_t iec_pa_read(void)
{
    drive_cpu_execute_all(maincpu_clk);

    cpu_bus_val = (bus_data << 1) | bus_clock | (NOT(bus_atn) << 7);

    return cpu_bus_val;
}

void iec_pa_write(uint8_t data)
{
    unsigned int i;

    drive_cpu_execute_all(maincpu_clk);

    /* Signal ATN interrupt to the drives.  */
    if ((cpu_atn == 0) && (data & 128)) {
        for (i = 0; i < NUM_DISK_UNITS; i++) {
            diskunit_context_t *unit = diskunit_context[i];

            if (unit->enable) {
                switch (unit->type) {
                    case DRIVE_TYPE_1581:
                        ciacore_set_flag(unit->cia1581);
                        break;
                    case DRIVE_TYPE_2000:
                    case DRIVE_TYPE_4000:
                        viacore_signal(unit->via4000, VIA_SIG_CA2, VIA_SIG_RISE);
                        break;
                    default:
                        viacore_signal(unit->via1d1541, VIA_SIG_CA1, VIA_SIG_RISE);
                }
            }
        }
    }

    /* Release ATN signal.  */
    if (!(data & 128)) {
        for (i = 0; i < NUM_DISK_UNITS; i++) {
            diskunit_context_t *unit = diskunit_context[i];

            if (unit->enable) {
                switch (unit->type) {
                    case DRIVE_TYPE_1581:
                        break;
                    case DRIVE_TYPE_2000:
                    case DRIVE_TYPE_4000:
                        viacore_signal(unit->via4000, VIA_SIG_CA2, 0);
                        break;
                    default:
                        viacore_signal(unit->via1d1541, VIA_SIG_CA1, 0);
                }
            }
        }
    }

    cpu_atn = ((data & 128) >> 7);

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        iec_calculate_data_modifier(i);
    }

    resolve_bus_signals();
}


/* This routine is called for VIA1 PCR (= CA2 and CB2).
   Although Cx2 uses three bits for control, we assume the calling routine has
   set bit 5 and bit 1 to the real output value for CB2 (DATA out) and CA2
   (CLK out) resp. (25apr1997 AF) */

void iec_pcr_write(uint8_t data)
{
    unsigned int i;

    drive_cpu_execute_all(maincpu_clk);

    cpu_data = ((data & 32) >> 5);
    cpu_clock = ((data & 2) >> 1);

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        iec_calculate_data_modifier(i);
    }

    resolve_bus_signals();
}

void iec_fast_drive_write(uint8_t data, unsigned int dnr)
{
/* The VIC20 does not use fast IEC.  */
}

void iec_fast_drive_direction(int direction, unsigned int dnr)
{
}

iecbus_t *iecbus_drive_port(void)
{
    return NULL;
}

void parallel_cable_drive_write(int port, uint8_t data, int handshake, unsigned int dnr)
{
}

uint8_t parallel_cable_drive_read(int port, int handshake)
{
    return 0;
}

int iec_available_busses(void)
{
    int ieee488_enabled;

    resources_get_int("IEEE488", &ieee488_enabled);

    return IEC_BUS_IEC | (ieee488_enabled ? IEC_BUS_IEEE : 0);
}

/* KLUDGES: dummy to satisfy linker, unused */
uint8_t plus4tcbm_outputa[2], plus4tcbm_outputb[2], plus4tcbm_outputc[2];

void plus4tcbm_update_pa(uint8_t byte, unsigned int dnr)
{
}

void plus4tcbm_update_pb(uint8_t byte, unsigned int dnr)
{
}

void plus4tcbm_update_pc(uint8_t byte, unsigned int dnr)
{
}
