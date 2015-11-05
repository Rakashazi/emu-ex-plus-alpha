/*
 * vic20ieeevia2.c - IEEE488 interface VIA2 emulation in the VIC-1112.
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


void ieeevia2_store(WORD addr, BYTE data)
{
    viacore_store(machine_context.ieeevia2, addr, data);
}

BYTE ieeevia2_read(WORD addr)
{
    return viacore_read(machine_context.ieeevia2, addr);
}

BYTE ieeevia2_peek(WORD addr)
{
    return viacore_peek(machine_context.ieeevia2, addr);
}

static void set_ca2(via_context_t *via_context, int state)
{
    parallel_cpu_set_atn((char)(state ? 0 : 1));
}

static void set_cb2(via_context_t *via_context, int state)
{
    parallel_cpu_set_eoi((BYTE)(state ? 0 : 1));
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
    parallel_cpu_set_bus(byte);
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE myoldpa,
                      WORD addr)
{
    parallel_cpu_set_bus(byte);
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE myoldpb,
                      WORD addr)
{
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
    parallel_cpu_set_bus(0xff); /* all data lines high, because of input mode */
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
#if 0
    if (byte != via_context->via[VIA_PCR]) {
        register BYTE tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((tmp & 0x0c) != 0x0c) {
            tmp |= 0x02;
        }
        if ((tmp & 0xc0) != 0xc0) {
            tmp |= 0x20;
        }
        parallel_cpu_set_atn((byte & 2) ? 0 : 1);
        parallel_cpu_set_eoi((byte & 0x20) ? 0 : 1);
    }
#endif
    return byte;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = (parallel_bus & ~(via_context->via[VIA_DDRB]))
           | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]);
    return byte;
}

static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    return 0xff;
}

void ieeevia2_init(via_context_t *via_context)
{
    viacore_init(machine_context.ieeevia2, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void vic20ieeevia2_setup_context(machine_context_t *machine_context)
{
    via_context_t *via;

    machine_context->ieeevia2 = lib_malloc(sizeof(via_context_t));
    via = machine_context->ieeevia2;

    via->prv = NULL;
    via->context = NULL;

    via->rmw_flag = &maincpu_rmw_flag;
    via->clk_ptr = &maincpu_clk;

    via->myname = lib_msprintf("IeeeVia2");
    via->my_module_name = lib_msprintf("IeeeVia2");

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
