/*
 * cbm2tpi2.c - TPI 2 for CBM-II
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
#include "cbm2mem.h"
#include "cbm2tpi.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "tpi.h"
#include "types.h"


void tpi2_store(WORD addr, BYTE data)
{
    tpicore_store(machine_context.tpi2, addr, data);
}

BYTE tpi2_read(WORD addr)
{
    return tpicore_read(machine_context.tpi2, addr);
}

BYTE tpi2_peek(WORD addr)
{
    return tpicore_peek(machine_context.tpi2, addr);
}

static void set_int(unsigned int int_num, int value)
{
}

static void restore_int(unsigned int int_num, int value)
{
}

static BYTE cbm2_model_port_mask = 0xc0;

void set_cbm2_model_port_mask(BYTE val)
{
    cbm2_model_port_mask = val & 0xc0;
}

static void set_ca(tpi_context_t *tpi_context, int a)
{
}

static void set_cb(tpi_context_t *tpi_context, int a)
{
}

static void reset(tpi_context_t *tpi_context)
{
}

static void store_pa(tpi_context_t *tpi_context, BYTE byte)
{
}

static void store_pb(tpi_context_t *tpi_context, BYTE byte)
{
}

static void store_pc(tpi_context_t *tpi_context, BYTE byte)
{
    cbm2_set_tpi2pc(byte);
}

static void undump_pa(tpi_context_t *tpi_context, BYTE byte)
{
}

static void undump_pb(tpi_context_t *tpi_context, BYTE byte)
{
}

static void undump_pc(tpi_context_t *tpi_context, BYTE byte)
{
    cbm2_set_tpi2pc(byte);
}

static BYTE read_pa(tpi_context_t *tpi_context)
{
    BYTE byte;
    byte = (0xff & ~(tpi_context->c_tpi)[TPI_DDPA])
           | (tpi_context->c_tpi[TPI_PA] & tpi_context->c_tpi[TPI_DDPA]);
    return byte;
}

static BYTE read_pb(tpi_context_t *tpi_context)
{
    BYTE byte;
    byte = (0xff & ~(tpi_context->c_tpi)[TPI_DDPB])
           | (tpi_context->c_tpi[TPI_PB] & tpi_context->c_tpi[TPI_DDPB]);
    return byte;
}

static BYTE read_pc(tpi_context_t *tpi_context)
{
    BYTE byte;
    BYTE val = ~(tpi_context->c_tpi)[TPI_DDPC] | 0xc0;
    int msk = (tpi_context->oldpa & 0xff)
              | ((tpi_context->oldpb << 8) & 0xff00);
    int m;
    int i;

    for (m = 0x1, i = 0; i < 16; m <<= 1, i++) {
        if (!(msk & m)) {
            val &= ~keyarr[i];
        }
    }
    byte = (val & 0x3f) | cbm2_model_port_mask;

    return byte;
}

void tpi2_init(tpi_context_t *tpi_context)
{
    tpi_context->log = log_open(tpi_context->myname);
}

void tpi2_setup_context(machine_context_t *machine_context)
{
    tpi_context_t *tpi_context;

    machine_context->tpi2 = lib_malloc(sizeof(tpi_context_t));
    tpi_context = machine_context->tpi2;

    tpi_context->prv = NULL;

    tpi_context->context = (void *)machine_context;

    tpi_context->rmw_flag = &maincpu_rmw_flag;
    tpi_context->clk_ptr = &maincpu_clk;

    tpi_context->myname = lib_msprintf("TPI2");

    tpicore_setup_context(tpi_context);

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
