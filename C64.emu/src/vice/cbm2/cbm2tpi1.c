/*
 * cbm2tpi1.c - TPI 1 for CBM-II
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include "cbm2.h"
#include "cbm2cia.h"
#include "cbm2mem.h"
#include "cbm2tpi.h"
#include "cia.h"
#include "datasette.h"
#include "drive.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "parallel.h"
#include "tpi.h"
#include "types.h"


void tpi1_store(WORD addr, BYTE data)
{
    tpicore_store(machine_context.tpi1, addr, data);
}

BYTE tpi1_read(WORD addr)
{
    return tpicore_read(machine_context.tpi1, addr);
}

BYTE tpi1_peek(WORD addr)
{
    return tpicore_peek(machine_context.tpi1, addr);
}

static void set_int(unsigned int int_num, int value)
{
    maincpu_set_irq(int_num, value);
}

static void restore_int(unsigned int int_num, int value)
{
    interrupt_restore_irq(maincpu_int_status, int_num, value);
}

/*----------------------------------------------------------------------*/
/* TPI resources. */

static int tape1_sense = 0;

void tpi1_set_tape_sense(int v)
{
    tape1_sense = v;
}

/*----------------------------------------------------------------------*/
/* I/O */

static void set_ca(tpi_context_t *tpi_context, int a)
{
    cbm2_set_tpi1ca(a);
}

static void set_cb(tpi_context_t *tpi_context, int a)
{
    cbm2_set_tpi1cb(a);
}

static void reset(tpi_context_t *tpi_context)
{
    /* assuming input after reset */
    parallel_cpu_set_atn(0);
    parallel_cpu_set_ndac(0);
    parallel_cpu_set_nrfd(0);
    parallel_cpu_set_dav(0);
    parallel_cpu_set_eoi(0);
    parallel_cpu_set_bus(0xff);
    cia1_set_ieee_dir(machine_context.cia1, 0);
}

static void store_pa(tpi_context_t *tpi_context, BYTE byte)
{
    if (byte != tpi_context->oldpa) {
        BYTE tmp = ~byte;
        cia1_set_ieee_dir(machine_context.cia1, byte & 2);
        if (byte & 2) {
            parallel_cpu_set_ndac(0);
            parallel_cpu_set_nrfd(0);
            parallel_cpu_set_atn((BYTE)(tmp & 0x08));
            parallel_cpu_set_dav((BYTE)(tmp & 0x10));
            parallel_cpu_set_eoi((BYTE)(tmp & 0x20));
        } else {
            /* order is important */
            parallel_cpu_set_nrfd((BYTE)(tmp & 0x80));
            parallel_cpu_set_ndac((BYTE)(tmp & 0x40));
            parallel_cpu_set_atn(0);
            parallel_cpu_set_dav(0);
            parallel_cpu_set_eoi(0);
        }
    }
}

static void undump_pa(tpi_context_t *tpi_context, BYTE byte)
{
    BYTE tmp = ~byte;
    cia1_set_ieee_dir(machine_context.cia1, byte & 2);
    if (byte & 2) {
        parallel_cpu_set_ndac(0);
        parallel_cpu_set_nrfd(0);
        parallel_cpu_restore_atn((BYTE)(tmp & 0x08));
        parallel_cpu_set_dav((BYTE)(tmp & 0x10));
        parallel_cpu_set_eoi((BYTE)(tmp & 0x20));
    } else {
        /* order is important */
        parallel_cpu_set_nrfd((BYTE)(tmp & 0x80));
        parallel_cpu_set_ndac((BYTE)(tmp & 0x40));
        parallel_cpu_restore_atn(0);
        parallel_cpu_set_dav(0);
        parallel_cpu_set_eoi(0);
    }
}

static void store_pb(tpi_context_t *tpi_context, BYTE byte)
{
    if ((byte ^ tpi_context->oldpb) & 0x40) {
        datasette_set_motor(!(byte & 0x40));
    }
    if ((byte ^ tpi_context->oldpb) & 0x20) {
        datasette_toggle_write_bit(byte & 0x20);
    }
}

static void store_pc(tpi_context_t *tpi_context, BYTE byte)
{
}

static void undump_pb(tpi_context_t *tpi_context, BYTE byte)
{
}

static void undump_pc(tpi_context_t *tpi_context, BYTE byte)
{
}

static BYTE read_pa(tpi_context_t *tpi_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = 0x07;
    byte += parallel_atn ? 0 : 8;
    byte += parallel_dav ? 0 : 16;
    byte += parallel_eoi ? 0 : 32;
    byte += parallel_ndac ? 0 : 64;
    byte += parallel_nrfd ? 0 : 128;

    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPA])
           | (tpi_context->c_tpi[TPI_PA] & tpi_context->c_tpi[TPI_DDPA]);

    return byte;
}

static BYTE read_pb(tpi_context_t *tpi_context)
{
    BYTE byte;

    byte = 0x7f;
    byte += tape1_sense ? 0x80 : 0;

    byte = (byte & ~(tpi_context->c_tpi)[TPI_DDPB])
           | (tpi_context->c_tpi[TPI_PB] & tpi_context->c_tpi[TPI_DDPB]);
    return byte;
}

static BYTE read_pc(tpi_context_t *tpi_context)
{
    BYTE byte;
    byte = (0xff & ~(tpi_context->c_tpi)[TPI_DDPC])
           | (tpi_context->c_tpi[TPI_PC] & tpi_context->c_tpi[TPI_DDPC]);
    return byte;
}

void tpi1_init(tpi_context_t *tpi_context)
{
    tpi_context->log = log_open(tpi_context->myname);

    tpi_context->tpi_int_num
        = interrupt_cpu_status_int_new(maincpu_int_status, tpi_context->myname);
}

void tpi1_setup_context(machine_context_t *machine_context)
{
    tpi_context_t *tpi_context;

    machine_context->tpi1 = lib_malloc(sizeof(tpi_context_t));
    tpi_context = machine_context->tpi1;

    tpi_context->prv = NULL;

    tpi_context->context = (void *)machine_context;

    tpi_context->rmw_flag = &maincpu_rmw_flag;
    tpi_context->clk_ptr = &maincpu_clk;

    tpi_context->myname = lib_msprintf("TPI1");

    tpicore_setup_context(tpi_context);

    tpi_context->irq_line = IK_IRQ;

    tpi_context->store_pa = store_pa;
    tpi_context->store_pb = store_pb;
    tpi_context->store_pc = store_pc;
    tpi_context->read_pa = read_pa;
    tpi_context->read_pb = read_pb;
    tpi_context->read_pc = read_pc;
    tpi_context->undump_pa = undump_pa;
    tpi_context->undump_pb = undump_pb;
    tpi_context->undump_pc = undump_pc;
    tpi_context->reset = reset;
    tpi_context->set_ca = set_ca;
    tpi_context->set_cb = set_cb;
    tpi_context->set_int = set_int;
    tpi_context->restore_int = restore_int;
}
