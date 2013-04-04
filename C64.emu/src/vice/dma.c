/*
 * dma.c - Utility functions for DMA hosts.
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

#include "6510core.h"
#include "debug.h"
#include "dma.h"
#include "interrupt.h"
#include "log.h"
#include "types.h"


/* Asynchronously steal `num' cycles from the CPU, starting from cycle
   `start_clk'.  */
void dma_maincpu_steal_cycles(CLOCK start_clk, int num, CLOCK sub)
{
    CLOCK irq_sub = 0;
    CLOCK nmi_sub = 0;
    unsigned int cycles_left_to_trigger_irq;
    interrupt_cpu_status_t *cs = maincpu_int_status;
    CLOCK dma_start;

    if (num == 0) {
        return;
    }

    dma_start = start_clk + sub;

    if (start_clk == cs->last_stolen_cycles_clk) {
        cs->num_last_stolen_cycles += num;
    } else {
        cs->num_last_stolen_cycles = num;
    }

#ifdef DEBUG
    if (debug.maincpu_traceflg) {
        log_debug("START %i NUM %i SUB %i MAIN %i DMAST %i",
                  start_clk, num, sub, maincpu_clk, dma_start);
    }
#endif

    cs->num_cycles_left[cs->num_dma_per_opcode] = maincpu_clk - dma_start;
    cs->dma_start_clk[cs->num_dma_per_opcode] = dma_start;
    (cs->num_dma_per_opcode)++;

#ifdef DEBUG
    if (debug.maincpu_traceflg) {
        debug_dma("VICII", start_clk, num);
    }
#endif

    cycles_left_to_trigger_irq =
        (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr) ? 2 : 1);
    if (cs->irq_clk >= start_clk
        && dma_start == (maincpu_clk - cycles_left_to_trigger_irq)
        && cs->num_dma_per_opcode == 1) {
#ifdef DEBUGIRQDMA
        if (debug.maincpu_traceflg) {
            log_debug("DECR");
        }
#endif
        irq_sub = 1;
    }
    if (cs->nmi_clk >= start_clk
        && dma_start == (maincpu_clk - cycles_left_to_trigger_irq)
        && cs->num_dma_per_opcode == 1) {
#ifdef DEBUGIRQDMA
        if (debug.maincpu_traceflg) {
            log_debug("DECR");
        }
#endif
        nmi_sub = 1;
    }

    maincpu_clk += num;

    cs->last_stolen_cycles_clk = dma_start + num;
#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        log_debug("IRQCLK %i LASTSTOLEN %i",
                  cs->irq_clk, cs->last_stolen_cycles_clk);
    }
#endif

    if (cs->irq_clk > dma_start) {
        cs->irq_clk = cs->last_stolen_cycles_clk;
    } else {
        cs->irq_clk += num;
    }

    if (cs->nmi_clk > dma_start) {
        cs->nmi_clk = cs->last_stolen_cycles_clk;
    } else {
        cs->nmi_clk += num;
    }

    cs->irq_clk -= irq_sub;
    cs->nmi_clk -= nmi_sub;

#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        log_debug("NEWIRQCLK %i", cs->irq_clk);
    }
#endif
}
