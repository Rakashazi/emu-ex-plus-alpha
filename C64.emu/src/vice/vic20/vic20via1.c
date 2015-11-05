/*
 * vic20via1.c - VIA1 emulation in the VIC20.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "datasette.h"
#include "interrupt.h"
#include "keyboard.h"
#include "lib.h"
#include "maincpu.h"
#include "types.h"
#include "via.h"
#include "vic20.h"
#include "vic20iec.h"
#include "vic20via.h"


void via1_store(WORD addr, BYTE data)
{
    viacore_store(machine_context.via1, addr, data);
}

BYTE via1_read(WORD addr)
{
    return viacore_read(machine_context.via1, addr);
}

BYTE via1_peek(WORD addr)
{
    return viacore_peek(machine_context.via1, addr);
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
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE myoldpb,
                      WORD addr)
{
    if ((byte ^ myoldpb) & 8) {
        datasette_toggle_write_bit((~(via_context->via[VIA_DDRB]) | byte) & 0x8);
    }
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
/*iec_pcr_write(0x22);*/
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
    /* FIXME: this should use via_set_ca2() and via_set_cb2() */
    if (byte != via_context->via[VIA_PCR]) {
        register BYTE tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((tmp & 0x0c) != 0x0c) {
            tmp |= 0x02;
        }
        if ((tmp & 0xc0) != 0xc0) {
            tmp |= 0x20;
        }
        iec_pcr_write(tmp);
    }
    return byte;
}

static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    BYTE byte;
    /* FIXME: not 100% sure about this... */
    BYTE val = ~(via_context->via[VIA_DDRA]);
    BYTE msk = via_context->oldpb;
    BYTE m;
    int i;

    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~rev_keyarr[i];
        }
    }

    byte = val | (via_context->via[VIA_PRA] & via_context->via[VIA_DDRA]);
    return byte;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;
    /* FIXME: not 100% sure about this... */
    BYTE val = ~(via_context->via[VIA_DDRB]);
    BYTE msk = via_context->oldpa;
    int m, i;

    for (m = 0x1, i = 0; i < 8; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~keyarr[i];
        }
    }

    /* Bit 7 is mapped to the right direction of the joystick (bit
       3 in `joystick_value[]'). */
    if ((joystick_value[1] | joystick_value[2]) & 0x8) {
        val &= 0x7f;
    }

    byte = val | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]);
    return byte;
}

void via1_init(via_context_t *via_context)
{
    viacore_init(machine_context.via1, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void vic20via1_setup_context(machine_context_t *machine_context)
{
    via_context_t *via;

    machine_context->via1 = lib_malloc(sizeof(via_context_t));
    via = machine_context->via1;

    via->prv = NULL;
    via->context = NULL;

    via->rmw_flag = &maincpu_rmw_flag;
    via->clk_ptr = &maincpu_clk;

    via->myname = lib_msprintf("Via1");
    via->my_module_name = lib_msprintf("VIA1");

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
