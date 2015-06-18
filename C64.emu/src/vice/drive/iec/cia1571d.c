/*
 * cia1571d.c - Definitions for the MOS6526 (CIA) chip in the 1571
 * disk drive ($4000).
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
#include "drivetypes.h"
#include "iecdrive.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "types.h"


typedef struct drivecia1571_context_s {
    unsigned int number;
    struct drive_s *drive;
} drivecia1571_context_t;


void cia1571_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    ciacore_store(ctxptr->cia1571, addr, data);
}

BYTE cia1571_read(drive_context_t *ctxptr, WORD addr)
{
    return ciacore_read(ctxptr->cia1571, addr);
}

BYTE cia1571_peek(drive_context_t *ctxptr, WORD addr)
{
    return ciacore_peek(ctxptr->cia1571, addr);
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

    interrupt_restore_irq(drive_context->cpu->int_status, cia_context->int_num, value);
}


/*************************************************************************
 * Hardware binding
 */

static void do_reset_cia(cia_context_t *cia_context)
{
}

static void pulse_ciapc(cia_context_t *cia_context, CLOCK rclk)
{
    drivecia1571_context_t *ciap;

    ciap = (drivecia1571_context_t *)(cia_context->prv);

    if (ciap->drive->parallel_cable == DRIVE_PC_STANDARD) {
        parallel_cable_drive_write(DRIVE_PC_STANDARD, 0, PARALLEL_HS, ciap->number);
    }
}

static void undump_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
}

static void undump_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    drivecia1571_context_t *ciap;

    ciap = (drivecia1571_context_t *)(cia_context->prv);

    if (ciap->drive->parallel_cable == DRIVE_PC_STANDARD) {
        parallel_cable_drive_write(DRIVE_PC_STANDARD, byte, PARALLEL_WRITE, ciap->number);
    }
}

static void store_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
}

static void store_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    drivecia1571_context_t *ciap;

    ciap = (drivecia1571_context_t *)(cia_context->prv);

    if (ciap->drive->parallel_cable == DRIVE_PC_STANDARD) {
        parallel_cable_drive_write(DRIVE_PC_STANDARD, byte, PARALLEL_WRITE, ciap->number);
    }
}

static BYTE read_ciapa(cia_context_t *cia_context)
{
    return (0xff & ~(cia_context->c_cia[CIA_DDRA]))
           | (cia_context->c_cia[CIA_PRA] & cia_context->c_cia[CIA_DDRA]);
}

static BYTE read_ciapb(cia_context_t *cia_context)
{
    drivecia1571_context_t *ciap;
    BYTE byte = 0xff;

    ciap = (drivecia1571_context_t *)(cia_context->prv);

    if (ciap->drive->parallel_cable == DRIVE_PC_STANDARD) {
        byte = parallel_cable_drive_read(ciap->drive->parallel_cable, 1);
    }

    return (byte & ~(cia_context->c_cia[CIA_DDRB]))
           | (cia_context->c_cia[CIA_PRB] & cia_context->c_cia[CIA_DDRB]);
}

static void read_ciaicr(cia_context_t *cia_context)
{
}

static void read_sdr(cia_context_t *cia_context)
{
}

static void store_sdr(cia_context_t *cia_context, BYTE byte)
{
    drivecia1571_context_t *cia1571p;

    cia1571p = (drivecia1571_context_t *)(cia_context->prv);

    iec_fast_drive_write((BYTE)byte, cia1571p->number);
}

void cia1571_init(drive_context_t *ctxptr)
{
    ciacore_init(ctxptr->cia1571, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status, ctxptr->cpu->clk_guard);
}

void cia1571_setup_context(drive_context_t *ctxptr)
{
    drivecia1571_context_t *cia1571p;
    cia_context_t *cia;

    ctxptr->cia1571 = lib_calloc(1, sizeof(cia_context_t));
    cia = ctxptr->cia1571;

    cia->prv = lib_malloc(sizeof(drivecia1571_context_t));
    cia1571p = (drivecia1571_context_t *)(cia->prv);
    cia1571p->number = ctxptr->mynumber;

    cia->context = (void *)ctxptr;

    cia->rmw_flag = &(ctxptr->cpu->rmw_flag);
    cia->clk_ptr = ctxptr->clk_ptr;

    cia1571_set_timing(cia, 1000000, 50);

    ciacore_setup_context(cia);

    cia->debugFlag = 0;
    cia->irq_line = IK_IRQ;
    cia->myname = lib_msprintf("CIA1571D%d", ctxptr->mynumber);

    cia1571p->drive = ctxptr->drive;

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

void cia1571_set_timing(cia_context_t *cia_context, int tickspersec, int powerfreq)
{
    cia_context->power_freq = powerfreq;
    cia_context->ticks_per_sec = tickspersec;
    cia_context->todticks = tickspersec / powerfreq;
    cia_context->power_tickcounter = 0;
    cia_context->power_ticks = 0;
}
