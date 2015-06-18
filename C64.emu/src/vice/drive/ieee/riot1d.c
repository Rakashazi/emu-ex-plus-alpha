/*
 * riot1d.c - RIOT1 emulation in the SFD1001, 8050 and 8250 disk drive.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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
#include "drivetypes.h"
#include "lib.h"
#include "parallel.h"
#include "riot.h"
#include "riotd.h"
#include "types.h"


void riot1_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    riotcore_store(ctxptr->riot1, addr, data);
}

BYTE riot1_read(drive_context_t *ctxptr, WORD addr)
{
    return riotcore_read(ctxptr->riot1, addr);
}

static void set_irq(riot_context_t *riot_context, int fl, CLOCK clk)
{
}

static void restore_irq(riot_context_t *riot_context, int fl)
{
}

static void undump_pra(riot_context_t *riot_context, BYTE byte)
{
}

static void store_pra(riot_context_t *riot_context, BYTE byte)
{
}

static void undump_prb(riot_context_t *riot_context, BYTE byte)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(riot_context->context);

    drive_context->func->parallel_set_bus(byte);
}

static void store_prb(riot_context_t *riot_context, BYTE byte)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(riot_context->context);

    drive_context->func->parallel_set_bus((BYTE)(parallel_atn ? 0xff : byte));
}

void riot1_set_pardata(riot_context_t *riot_context)
{
    store_prb(riot_context, riot_context->old_pb);
}

static void reset(riot_context_t *riot_context)
{
    store_prb(riot_context, 0xff);
}

static BYTE read_pra(riot_context_t *riot_context)
{
    return (parallel_bus & ~(riot_context->riot_io)[1])
           | (riot_context->riot_io[0] & riot_context->riot_io[1]);
}

static BYTE read_prb(riot_context_t *riot_context)
{
    return (0xff & ~(riot_context->riot_io)[3])
           | (riot_context->riot_io[2] & riot_context->riot_io[3]);
}

void riot1_init(drive_context_t *ctxptr)
{
    riotcore_init(ctxptr->riot1, ctxptr->cpu->alarm_context,
                  ctxptr->cpu->clk_guard, ctxptr->mynumber);
}

void riot1_setup_context(drive_context_t *ctxptr)
{
    riot_context_t *riot;

    ctxptr->riot1 = lib_malloc(sizeof(riot_context_t));
    riot = ctxptr->riot1;

    riot->prv = NULL;
    riot->context = (void *)ctxptr;

    riot->rmw_flag = &(ctxptr->cpu->rmw_flag);
    riot->clk_ptr = ctxptr->clk_ptr;

    riotcore_setup_context(riot);

    riot->myname = lib_msprintf("RIOT1D%d", ctxptr->mynumber);

    riot->undump_pra = undump_pra;
    riot->undump_prb = undump_prb;
    riot->store_pra = store_pra;
    riot->store_prb = store_prb;
    riot->read_pra = read_pra;
    riot->read_prb = read_prb;
    riot->reset = reset;
    riot->set_irq = set_irq;
    riot->restore_irq = restore_irq;
}
