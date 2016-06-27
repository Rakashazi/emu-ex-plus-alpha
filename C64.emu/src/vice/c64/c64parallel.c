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

static BYTE parallel_cable_drive_value[DRIVE_NUM] = { 0xff, 0xff, 0xff, 0xff };
static BYTE parallel_cable_cpu_value[PC_PORT_NUM] = { 0xff, 0xff };

static int portmap[DRIVE_PC_NUM] = {
    PC_PORT_STANDARD, /* DRIVE_PC_NONE */
    PC_PORT_STANDARD, /* DRIVE_PC_STANDARD */
    PC_PORT_STANDARD, /* DRIVE_PC_DD3 */
    PC_PORT_FORMEL64, /* DRIVE_PC_FORMEL64 */
};

static BYTE parallel_cable_value(int type)
{
    unsigned int dnr, port;
    BYTE val;

    port = portmap[type];
    val = parallel_cable_cpu_value[port];

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        if (drive_context[dnr]->drive->enable && drive_context[dnr]->drive->parallel_cable) {
            if (portmap[drive_context[dnr]->drive->parallel_cable] == (int)port) {
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
void parallel_cable_drive_write(int type, BYTE data, int handshake, unsigned int dnr)
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

BYTE parallel_cable_drive_read(int type, int handshake)
{
    int port;
    BYTE rc;

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
    drive_t *drive;

    port = portmap[type];

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        if (drive->enable && drive->parallel_cable) {
            if (portmap[drive->parallel_cable] == port) {
                drive_cpu_execute_one(drive_context[dnr], maincpu_clk);
            }
        }
    }
}

void parallel_cable_cpu_write(int type, BYTE data)
{
    int port;

    port = portmap[type];
    parallel_cable_cpu_execute(type);

    parallel_cable_cpu_value[port] = data;

    DBG(("PARCABLE (%d:%d) CPU W DATA %02x", type, port, data));
}

BYTE parallel_cable_cpu_read(int type, BYTE data)
{
    BYTE rc;

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

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive_t *drive;

        drive = drive_context[dnr]->drive;

        if (drive->enable && drive->parallel_cable) {
            switch (drive->parallel_cable) {
                case DRIVE_PC_DD3:
                    dd3_set_signal(drive_context[dnr]);
                    break;
                case DRIVE_PC_FORMEL64:
                    viacore_signal(drive_context[dnr]->via1d1541, VIA_SIG_CB1, VIA_SIG_FALL);
                    break;
                default:
                    if (drive->type == DRIVE_TYPE_1570 ||
                        drive->type == DRIVE_TYPE_1571 ||
                        drive->type == DRIVE_TYPE_1571CR) {
                        ciacore_set_flag(drive_context[dnr]->cia1571);
                    } else {
                        viacore_signal(drive_context[dnr]->via1d1541, VIA_SIG_CB1, VIA_SIG_FALL);
                    }
                    break;
            }
        }
    }
}

void parallel_cable_cpu_undump(int type, BYTE data)
{
    parallel_cable_cpu_value[portmap[type]] = data;
}
