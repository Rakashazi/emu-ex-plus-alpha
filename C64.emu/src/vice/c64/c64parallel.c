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
#include "joyport.h"
#include "log.h"
#include "maincpu.h"
#include "types.h"
#include "userport.h"
#include "via.h"

#ifdef C64PAR_DEBUG
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

/*
"standard" (SpeedDOS) cable

VIA#1   User port plug
 2, PA0     C, PB0
 3, PA1     D, PB1
 4, PA2     E, PB2
 5, PA3     F, PB3
 6, PA4     H, PB4
 7, PA5     J, PB5
 8, PA6     K, PB6
 9, PA7     L, PB7

18, CB1     8, PC2      <- this one is NOT connected on the "21sec Backup" cable!
39, CA2     B, FLAG2
*/

#define PC_PORT_STANDARD        0
#define PC_PORT_FORMEL64        1

#define PC_PORT_NUM             2

static uint8_t parallel_cable_drive_value[NUM_DISK_UNITS] = { 0xff, 0xff, 0xff, 0xff };
static uint8_t parallel_cable_cpu_value[PC_PORT_NUM] = { 0xff, 0xff };

static int parallel_cable_enabled = 0;

static const int portmap[DRIVE_PC_NUM] = {
    PC_PORT_STANDARD, /* DRIVE_PC_NONE */
    PC_PORT_STANDARD, /* DRIVE_PC_STANDARD */
    PC_PORT_STANDARD, /* DRIVE_PC_DD3 */
    PC_PORT_FORMEL64, /* DRIVE_PC_FORMEL64 */
    PC_PORT_STANDARD, /* DRIVE_PC_21SEC_BACKUP */
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

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static void userport_par_cable_store_pbx(uint8_t data, int pulse);
static uint8_t userport_par_cable_read_pbx(uint8_t orig);
static int userport_par_cable_enable(int value);

static userport_device_t par_cable_device = {
    "Userport parallel drive cable",      /* device name */
    JOYSTICK_ADAPTER_ID_NONE,             /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_DRIVE_PAR_CABLE, /* device is a parallel drive cable */
    userport_par_cable_enable,            /* enable function */
    userport_par_cable_read_pbx,          /* read pb0-pb7 function */
    userport_par_cable_store_pbx,         /* NO store pb0-pb7 function */
    NULL,                                 /* NO read pa2 pin function */
    NULL,                                 /* NO store pa2 pin function */
    NULL,                                 /* NO read pa3 pin function */
    NULL,                                 /* NO store pa3 pin function */
    0,                                    /* pc pin is NOT needed */
    NULL,                                 /* NO store sp1 pin function */
    NULL,                                 /* NO read sp1 pin function */
    NULL,                                 /* NO store sp2 pin function */
    NULL,                                 /* NO read sp2 pin function */
    NULL,                                 /* NO reset pin function */
    NULL,                                 /* NO power toggle function */
    NULL,                                 /* NO snapshot write function */
    NULL                                  /* NO snapshot read function */
};

static int userport_par_cable_enable(int value)
{
    int val = value ? 1 : 0;

    parallel_cable_enabled = val;

    return 0;
}

static void userport_par_cable_store_pbx(uint8_t data, int pulse)
{
    if (pulse) {
        parallel_cable_cpu_pulse(DRIVE_PC_STANDARD);
    } else {
        parallel_cable_cpu_write(DRIVE_PC_STANDARD, data);
    }
}

static uint8_t userport_par_cable_read_pbx(uint8_t orig)
{
    return parallel_cable_cpu_read(DRIVE_PC_STANDARD, orig);
}

int parallel_cable_cpu_resources_init(void)
{
    return userport_device_register(USERPORT_DEVICE_DRIVE_PAR_CABLE, &par_cable_device);
}

/* ------------------------------------------------------------------------- */

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
                case DRIVE_PC_21SEC_BACKUP:
                    /* do nothing */
                    break;
                default:
                    if (unit->type == DRIVE_TYPE_1570 ||
                        unit->type == DRIVE_TYPE_1571 ||
                        unit->type == DRIVE_TYPE_1571CR) {
                        ciacore_set_flag(unit->cia1571);
                    } else {
                        /* FIXME: don't do this for the 21.sec cable */
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
