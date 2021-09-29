/*
 * c64parallel.c - Parallel cable handling for the C64.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  groepaz <groepaz@gmx.net>
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

/* define for debug messages */
/* #define C64PAR_DEBUG */

#include "vice.h"

#include "c64.h"
#include "c64parallel.h"
#include "cia.h"
#include "dolphindos3.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecdrive.h"
#include "log.h"
#include "maincpu.h"
#include "types.h"
#include "via.h"

#ifdef C64PAR_DEBUG
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#define PC_PORT_STANDARD        0
#define PC_PORT_FORMEL64        1

#define PC_PORT_NUM             2

static uint8_t parallel_cable_drive_value[NUM_DISK_UNITS] = { 0xff, 0xff, 0xff, 0xff };
static uint8_t parallel_cable_cpu_value[PC_PORT_NUM] = { 0xff, 0xff };

static const int portmap[DRIVE_PC_NUM] = {
    PC_PORT_STANDARD, /* DRIVE_PC_NONE */
    PC_PORT_STANDARD, /* DRIVE_PC_STANDARD */
    PC_PORT_STANDARD, /* DRIVE_PC_DD3 */
    PC_PORT_FORMEL64, /* DRIVE_PC_FORMEL64 */
};

static uint8_t parallel_cable_value(int type)
{
    unsigned int dnr, port;
    uint8_t val;

    port = portmap[type];
    val = parallel_cable_cpu_value[port];

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        if (diskunit_context[dnr]->enable && diskunit_context[dnr]->parallel_cable) {
            if (portmap[diskunit_context[dnr]->parallel_cable] == (int)port) {
                val &= parallel_cable_drive_value[dnr];
            }
        }
    }
    DBG(("PARCABLE (%d:%d) CPU %02x DRIVE %02x VAL %02x", type, port, parallel_cable_cpu_value[port], parallel_cable_drive_value[0], val));

    return val;
}

/*
    interface for the drive (read/write)
*/
void parallel_cable_drive_write(int type, uint8_t data, int handshake, unsigned int dnr)
{
    int port;

    DBG(("PARCABLE (%d:%d) DRV (%d) W DATA %02x HS %02x", type, portmap[type], dnr, data, handshake));

    port = portmap[type];

    if (handshake == PARALLEL_WRITE_HS || handshake == PARALLEL_HS) {
        if (port == PC_PORT_STANDARD) {
            ciacore_set_flag(machine_context.cia2);
        }
    }

    if (handshake == PARALLEL_WRITE_HS || handshake == PARALLEL_WRITE) {
        parallel_cable_drive_value[dnr] = data;
    }
}

uint8_t parallel_cable_drive_read(int type, int handshake)
{
    int port;
    uint8_t rc;

    port = portmap[type];

    if (handshake) {
        if (port == PC_PORT_STANDARD) {
            ciacore_set_flag(machine_context.cia2);
        }
    }

    rc = parallel_cable_value(type);

    DBG(("PARCABLE (%d:%d) DRV R DATA %02x HS %02x", type, portmap[type], rc, handshake));

    return rc;
}

/* execute drive cpu for all drives that are connected to the respective port
   on the C64
 */
void parallel_cable_cpu_execute(int type)
{
    unsigned int dnr;
    int port;

    port = portmap[type];

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (unit->enable && unit->parallel_cable) {
            if (portmap[unit->parallel_cable] == port) {
                drive_cpu_execute_one(unit, maincpu_clk);
            }
        }
    }
}

void parallel_cable_cpu_write(int type, uint8_t data)
{
    int port;

    port = portmap[type];
    parallel_cable_cpu_execute(type);

    parallel_cable_cpu_value[port] = data;

    DBG(("PARCABLE (%d:%d) CPU W DATA %02x", type, port, data));
}

uint8_t parallel_cable_cpu_read(int type, uint8_t data)
{
    uint8_t rc;

    parallel_cable_cpu_execute(type);

    rc = parallel_cable_value(type);

    DBG(("PARCABLE (%d:%d) CPU R %02x", type, portmap[type], rc));

    return data & rc;
}

void parallel_cable_cpu_pulse(int type)
{
    unsigned int dnr;

    parallel_cable_cpu_execute(type);

    DBG(("PARCABLE (%d:%d) CPU Pulse", type, portmap[type]));

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (unit->enable && unit->parallel_cable) {
            switch (unit->parallel_cable) {
                case DRIVE_PC_DD3:
                    dd3_set_signal(unit);
                    break;
                case DRIVE_PC_FORMEL64:
                    viacore_signal(unit->via1d1541, VIA_SIG_CB1, VIA_SIG_FALL);
                    break;
                default:
                    if (unit->type == DRIVE_TYPE_1570 ||
                        unit->type == DRIVE_TYPE_1571 ||
                        unit->type == DRIVE_TYPE_1571CR) {
                        ciacore_set_flag(unit->cia1571);
                    } else {
                        viacore_signal(unit->via1d1541, VIA_SIG_CB1, VIA_SIG_FALL);
                    }
                    break;
            }
        }
    }
}

void parallel_cable_cpu_undump(int type, uint8_t data)
{
    parallel_cable_cpu_value[portmap[type]] = data;
}
