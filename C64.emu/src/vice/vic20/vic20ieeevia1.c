/*
 * vic20ieeevia1.c - IEEE488 interface VIA1 emulation in the VIC-1112.
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

#include "drive.h"
#include "interrupt.h"
#include "lib.h"
#include "maincpu.h"
#include "parallel.h"
#include "types.h"
#include "via.h"
#include "vic20.h"
#include "vic20ieeevia.h"


void ieeevia1_store(WORD addr, BYTE data)
{
    viacore_store(machine_context.ieeevia1, addr, data);
}

BYTE ieeevia1_read(WORD addr)
{
    return viacore_read(machine_context.ieeevia1, addr);
}

BYTE ieeevia1_peek(WORD addr)
{
    return viacore_peek(machine_context.ieeevia1, addr);
}

static void set_ca2(via_context_t *via_context, int state)
{
}

static void set_cb2(via_context_t *via_context, int state)
{
}

static void set_int(via_context_t *via_context, unsigned int int_num, int value, CLOCK rclk)
{
    interrupt_set_irq(maincpu_int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num, int value)
{
    interrupt_restore_irq(maincpu_int_status, int_num, value);
}

static void undump_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_sr(via_context_t *via_context, BYTE byte)
{
}

static void store_t2l(via_context_t *via_context, BYTE byte)
{
}

static void undump_pra(via_context_t *via_context, BYTE byte)
{
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE myoldpa,
                      WORD addr)
{
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
    parallel_cpu_set_dav((BYTE)(!(byte & 0x01)));
    parallel_cpu_set_nrfd((BYTE)(!(byte & 0x02)));
    parallel_cpu_set_ndac((BYTE)(!(byte & 0x04)));
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE myoldpb,
                      WORD addr)
{
    parallel_cpu_set_dav((BYTE)(!(byte & 0x01)));
    parallel_cpu_set_nrfd((BYTE)(!(byte & 0x02)));
    parallel_cpu_set_ndac((BYTE)(!(byte & 0x04)));
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
    parallel_cpu_set_dav(0);
    parallel_cpu_set_nrfd(0);
    parallel_cpu_set_ndac(0);
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
    return byte;
}

static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    return 0xff;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = 255
           - (parallel_atn ? 0x80 : 0)
           - (parallel_ndac ? 0x40 : 0)
           - (parallel_nrfd ? 0x20 : 0)
           - (parallel_dav ? 0x10 : 0)
           - (parallel_eoi ? 0x08 : 0);

    /* none of the load changes output register value -> std. masking */
    byte = ((byte & ~(via_context->via[VIA_DDRB]))
            | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]));
    return byte;
}

void ieeevia1_init(via_context_t *via_context)
{
    viacore_init(machine_context.ieeevia1, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void vic20ieeevia1_setup_context(machine_context_t *machine_context)
{
    via_context_t *via;

    machine_context->ieeevia1 = lib_malloc(sizeof(via_context_t));
    via = machine_context->ieeevia1;

    via->prv = NULL;
    via->context = NULL;

    via->rmw_flag = &maincpu_rmw_flag;
    via->clk_ptr = &maincpu_clk;

    via->myname = lib_msprintf("IeeeVia1");
    via->my_module_name = lib_msprintf("IeeeVia1");

    viacore_setup_context(via);

    via->write_offset = 0;

    via->irq_line = IK_IRQ;

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
