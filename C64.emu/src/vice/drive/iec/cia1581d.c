/*
 * cia1581d.c - Definitions for the MOS6526 (CIA) chip in the 1581
 * disk drive ($4000).  Notice that the real 1581 uses a 8520 CIA.
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

#include "cia.h"
#include "ciad.h"
#include "debug.h"
#include "drive.h"
#include "drivetypes.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "wd1770.h"


typedef struct drivecia1581_context_s {
    unsigned int number;
    struct drive_s *drive;
    struct iecbus_s *iecbus;
} drivecia1581_context_t;


void cia1581_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    ciacore_store(ctxptr->cia1581, addr, data);
}

BYTE cia1581_read(drive_context_t *ctxptr, WORD addr)
{
    return ciacore_read(ctxptr->cia1581, addr);
}

BYTE cia1581_peek(drive_context_t *ctxptr, WORD addr)
{
    return ciacore_peek(ctxptr->cia1581, addr);
}

int cia1581_dump(drive_context_t *ctxptr, WORD addr)
{
    ciacore_dump(ctxptr->cia1581);
    return 0;
}

static void cia_set_int_clk(cia_context_t *cia_context, int value, CLOCK clk)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(cia_context->context);

    interrupt_set_irq(drive_context->cpu->int_status, cia_context->int_num,
                      value, clk);
}

static void cia_restore_int(cia_context_t *cia_context, int value)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(cia_context->context);

    interrupt_restore_irq(drive_context->cpu->int_status, (int)(cia_context->int_num), value);
}

/*************************************************************************
 * Hardware binding
 */

static void do_reset_cia(cia_context_t *cia_context)
{
    drivecia1581_context_t *cia1581p;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);

    cia1581p->drive->led_status = 1;
}

static void pulse_ciapc(cia_context_t *cia_context, CLOCK rclk)
{
}

#define PRE_STORE_CIA
#define PRE_READ_CIA
#define PRE_PEEK_CIA

static void undump_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE b)
{
    drivecia1581_context_t *cia1581p;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);

    cia1581p->drive->led_status = (b & 0x40) ? 1 : 0;
}

static void undump_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE b)
{
}

static void store_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    drivecia1581_context_t *cia1581p;
    drive_context_t *drive;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);
    drive = (drive_context_t *)(cia_context->context);

    wd1770_set_side(drive->wd1770, (byte & 0x01) ? 0 : 1);
    wd1770_set_motor(drive->wd1770, (byte & 0x04) ? 0 : 1);

    cia1581p->drive->led_status = (byte & 0x40) ? 1 : 0;
    if (cia1581p->drive->led_status) {
        cia1581p->drive->led_active_ticks += *(cia_context->clk_ptr)
                                             - cia1581p->drive->led_last_change_clk;
    }
    cia1581p->drive->led_last_change_clk = *(cia_context->clk_ptr);
}

static void store_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    drivecia1581_context_t *cia1581p;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);

    if (byte != cia_context->old_pb) {
        if (cia1581p->iecbus != NULL) {
            BYTE *drive_bus, *drive_data;
            unsigned int unit;

            drive_bus = &(cia1581p->iecbus->drv_bus[cia1581p->number + 8]);
            drive_data = &(cia1581p->iecbus->drv_data[cia1581p->number + 8]);

            *drive_data = (BYTE)~byte;
            *drive_bus = (BYTE)(((((*drive_data) << 3) & 0x40)
                        | (((*drive_data) << 6)
                            & (((*drive_data)
                                    | cia1581p->iecbus->cpu_bus) << 3) & 0x80)));

            cia1581p->iecbus->cpu_port = cia1581p->iecbus->cpu_bus;
            for (unit = 4; unit < 8 + DRIVE_NUM; unit++) {
                cia1581p->iecbus->cpu_port
                    &= cia1581p->iecbus->drv_bus[unit];
            }

            cia1581p->iecbus->drv_port =
                (BYTE)((((cia1581p->iecbus->cpu_port >> 4) & 0x4)
                            | (cia1581p->iecbus->cpu_port >> 7)
                            | ((cia1581p->iecbus->cpu_bus << 3) & 0x80)));
        } else {
            iec_drive_write((BYTE)(~byte), cia1581p->number);
        }

        iec_fast_drive_direction(byte & 0x20, cia1581p->number);
    }
}

static BYTE read_ciapa(cia_context_t *cia_context)
{
    drive_context_t *drive_context;
    drivecia1581_context_t *cia1581p;
    BYTE tmp;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);
    drive_context = (drive_context_t *)(cia_context->context);

    tmp = (BYTE)(8 * (cia1581p->number));

    if (!wd1770_disk_change(drive_context->wd1770)) {
        tmp |= 0x80;
    }

    return (BYTE)((tmp & ~(cia_context->c_cia[CIA_DDRA]))
            | (cia_context->c_cia[CIA_PRA] & cia_context->c_cia[CIA_DDRA]));
}

static BYTE read_ciapb(cia_context_t *cia_context)
{
    drivecia1581_context_t *cia1581p;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);

    if (cia1581p->iecbus != NULL) {
        BYTE *drive_port;

        drive_port = &(cia1581p->iecbus->drv_port);

        return (BYTE)((((cia_context->c_cia[CIA_PRB] & 0x1a)
                        | (*drive_port)) ^ 0x85)
                | (cia1581p->drive->read_only ? 0 : 0x40));
    } else {
        return (BYTE)((((cia_context->c_cia[CIA_PRB] & 0x1a)
                        | iec_drive_read(cia1581p->number)) ^ 0x85)
                | (cia1581p->drive->read_only ? 0 : 0x40));
    }
}

static void read_ciaicr(cia_context_t *cia_context)
{
}

static void read_sdr(cia_context_t *cia_context)
{
}

static void store_sdr(cia_context_t *cia_context, BYTE byte)
{
    drivecia1581_context_t *cia1581p;

    cia1581p = (drivecia1581_context_t *)(cia_context->prv);

    iec_fast_drive_write(byte, cia1581p->number);
}

void cia1581_init(drive_context_t *ctxptr)
{
    ciacore_init(ctxptr->cia1581, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status, ctxptr->cpu->clk_guard);
}

void cia1581_setup_context(drive_context_t *ctxptr)
{
    drivecia1581_context_t *cia1581p;
    cia_context_t *cia;

    ctxptr->cia1581 = lib_calloc(1, sizeof(cia_context_t));
    cia = ctxptr->cia1581;

    cia->prv = lib_malloc(sizeof(drivecia1581_context_t));
    cia1581p = (drivecia1581_context_t *)(cia->prv);
    cia1581p->number = (unsigned int)(ctxptr->mynumber);

    cia->context = (void *)ctxptr;

    cia->rmw_flag = &(ctxptr->cpu->rmw_flag);
    cia->clk_ptr = ctxptr->clk_ptr;

    /* FIXME: incorrect, J1 closed = 1, J1 open, no ticks at all */
    cia1581_set_timing(cia, 1000000, 50);

    ciacore_setup_context(cia);

    cia->debugFlag = 0;
    cia->irq_line = IK_IRQ;
    cia->myname = lib_msprintf("CIA1581D%d", ctxptr->mynumber);

    cia1581p->drive = ctxptr->drive;
    cia1581p->iecbus = iecbus_drive_port();

    cia->undump_ciapa = undump_ciapa;
    cia->undump_ciapb = undump_ciapb;
    cia->store_ciapa = store_ciapa;
    cia->store_ciapb = store_ciapb;
    cia->store_sdr = store_sdr;
    cia->read_ciapa = read_ciapa;
    cia->read_ciapb = read_ciapb;
    cia->read_ciaicr = read_ciaicr;
    cia->read_sdr = read_sdr;
    cia->cia_set_int_clk = cia_set_int_clk;
    cia->cia_restore_int = cia_restore_int;
    cia->do_reset_cia = do_reset_cia;
    cia->pulse_ciapc = pulse_ciapc;
    cia->pre_store = NULL;
    cia->pre_read = NULL;
    cia->pre_peek = NULL;
}

void cia1581_set_timing(cia_context_t *cia_context, int tickspersec, int powerfreq)
{
    cia_context->power_freq = powerfreq;
    cia_context->ticks_per_sec = (CLOCK)tickspersec;
    cia_context->todticks = tickspersec / powerfreq;
    cia_context->power_tickcounter = 0;
    cia_context->power_ticks = 0;
}
