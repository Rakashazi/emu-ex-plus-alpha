/*
 * via4000.c - VIA emulation in the 4000 disk drive.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *
 * Based on old code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "debug.h"
#include "drive.h"
#include "drivesync.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "interrupt.h"
#include "lib.h"
#include "rotation.h"
#include "types.h"
#include "via.h"
#include "via4000.h"
#include "viad.h"
#include "pc8477.h"


#define iecbus (viap->v_iecbus)

typedef struct drivevia_context_s {
    unsigned int number;
    struct drive_s *drive;
    struct iecbus_s *v_iecbus;
} drivevia_context_t;


void via4000_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    viacore_store(ctxptr->via4000, addr, data);
}

BYTE via4000_read(drive_context_t *ctxptr, WORD addr)
{
    return viacore_read(ctxptr->via4000, addr);
}

BYTE via4000_peek(drive_context_t *ctxptr, WORD addr)
{
    return viacore_peek(ctxptr->via4000, addr);
}

int via4000_dump(drive_context_t *ctxptr, WORD addr)
{
    viacore_dump(ctxptr->via4000);
    return 0;
}

static void set_ca2(via_context_t *via_context, int state)
{
}

static void set_cb2(via_context_t *via_context, int state)
{
}

static void set_int(via_context_t *via_context, unsigned int int_num,
                    int value, CLOCK rclk)
{
    drive_context_t *drive_context = (drive_context_t *)(via_context->context);

    interrupt_set_irq(drive_context->cpu->int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num,
                        int value)
{
    drive_context_t *drive_context = (drive_context_t *)(via_context->context);

    interrupt_restore_irq(drive_context->cpu->int_status, int_num, value);
}

static void undump_pra(via_context_t *via_context, BYTE byte)
{
    drivevia_context_t *viap = (drivevia_context_t *)(via_context->prv);

    if (iecbus != NULL) {
        BYTE *drive_bus, *drive_data;
        unsigned int unit;

        drive_bus = &(iecbus->drv_bus[viap->number + 8]);
        drive_data = &(iecbus->drv_data[viap->number + 8]);

        *drive_data = ~byte;
        *drive_bus = ((((*drive_data) << 3) & 0x40)
                      | (((*drive_data) << 6)
                         & (((*drive_data) | iecbus->cpu_bus) << 3) & 0x80));

        iecbus->cpu_port = iecbus->cpu_bus;
        for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
            iecbus->cpu_port &= iecbus->drv_bus[unit];
        }

        iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                            | (iecbus->cpu_port >> 7)
                            | ((iecbus->cpu_bus << 3) & 0x80));
    } else {
        iec_drive_write((BYTE)(~byte), viap->number);
    }
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE oldpa,
                      WORD addr)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    if (byte != oldpa) {
        DEBUG_IEC_DRV_WRITE(byte);

        if (iecbus != NULL) {
            BYTE *drive_data, *drive_bus;
            unsigned int unit;

            drive_bus = &(iecbus->drv_bus[viap->number + 8]);
            drive_data = &(iecbus->drv_data[viap->number + 8]);

            *drive_data = ~byte;
            *drive_bus = ((((*drive_data) << 3) & 0x40)
                          | (((*drive_data) << 6)
                             & (((*drive_data) | iecbus->cpu_bus) << 3) & 0x80));

            iecbus->cpu_port = iecbus->cpu_bus;
            for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
                iecbus->cpu_port &= iecbus->drv_bus[unit];
            }

            iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                                | (iecbus->cpu_port >> 7)
                                | ((iecbus->cpu_bus << 3) & 0x80));

            DEBUG_IEC_BUS_WRITE(iecbus->drv_port);
        } else {
            iec_drive_write((BYTE)(~byte), viap->number);
            DEBUG_IEC_BUS_WRITE(~byte);
        }

        iec_fast_drive_direction(byte & 0x20, viap->number);
    }
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    viap->drive->led_status = (byte & 0x40) ? 1 : 0;
    viap->drive->led_status |= (byte & 0x20) ? 2 : 0;
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE p_oldpb,
                      WORD addr)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    viap->drive->led_status = (byte & 0x40) ? 1 : 0;
    viap->drive->led_status |= (byte & 0x20) ? 2 : 0;
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
    return byte;
}

static void undump_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_sr(via_context_t *via_context, BYTE byte)
{
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    iec_fast_drive_write((BYTE)(~byte), viap->number);
}

static void store_t2l(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
}

static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    BYTE byte;
    drivevia_context_t *viap;

    viap = (drivevia_context_t *)(via_context->prv);

    if (iecbus != NULL) {
        byte = (((via_context->via[VIA_PRA] & 0x1a)
                 | iecbus->drv_port) ^ 0x85);
    } else {
        byte = (((via_context->via[VIA_PRA] & 0x1a)
                 | iec_drive_read(viap->number)) ^ 0x85);
    }

    DEBUG_IEC_DRV_READ(byte);

    DEBUG_IEC_BUS_READ(byte);

    return byte;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;
    drivevia_context_t *viap;
    drive_context_t *drive;

    viap = (drivevia_context_t *)(via_context->prv);
    drive = (drive_context_t *)(via_context->context);

    byte = (viap->number << 3) | (pc8477_irq(drive->pc8477) ? 0x80 : 0);

    return byte;
}

void via4000_init(drive_context_t *ctxptr)
{
    viacore_init(ctxptr->via4000, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status, ctxptr->cpu->clk_guard);
}

void via4000_setup_context(drive_context_t *ctxptr)
{
    drivevia_context_t *viap;
    via_context_t *via;

    /* Clear struct as snapshot code may write uninitialized values.  */
    ctxptr->via4000 = lib_calloc(1, sizeof(via_context_t));
    via = ctxptr->via4000;

    via->prv = lib_malloc(sizeof(drivevia_context_t));
    viap = (drivevia_context_t *)(via->prv);
    viap->number = ctxptr->mynumber;

    via->context = (void *)ctxptr;

    via->rmw_flag = &(ctxptr->cpu->rmw_flag);
    via->clk_ptr = ctxptr->clk_ptr;

    via->myname = lib_msprintf("4000Drive%dVia1", ctxptr->mynumber);
    via->my_module_name = lib_msprintf("4000VIA1D%d", ctxptr->mynumber);

    viacore_setup_context(via);

    via->my_module_name_alt1 = lib_msprintf("VIA1D%d", ctxptr->mynumber);
    via->my_module_name_alt2 = lib_msprintf("VIA4000");

    via->irq_line = IK_IRQ;

    viap->drive = ctxptr->drive;
    iecbus = iecbus_drive_port();

    via->undump_pra = undump_pra;
    via->undump_prb = undump_prb;
    via->undump_pcr = undump_pcr;
    via->undump_acr = undump_acr;
    via->store_pra = store_pra;
    via->store_prb = store_prb;
    via->store_pcr = store_pcr;
    via->store_acr = store_acr;
    via->store_sr = store_sr;
    via->store_t2l = store_t2l;
    via->read_pra = read_pra;
    via->read_prb = read_prb;
    via->set_int = set_int;
    via->restore_int = restore_int;
    via->set_ca2 = set_ca2;
    via->set_cb2 = set_cb2;
    via->reset = reset;
}
