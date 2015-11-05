/*
 * petcpu.c - Emulation of the main 6502 processor.
 *
 * Written by Olaf Seibert.
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

#include "maincpu.h"
#include "mem.h"
#include "pets.h"
#include "6809.h"

#ifdef FEATURE_CPUMEMHISTORY
#include "monitor.h"
#endif

/* ------------------------------------------------------------------------- */

/* MACHINE_STUFF should define/undef

 - NEED_REG_PC
 - TRACE

 The following are optional:

 - PAGE_ZERO
 - PAGE_ONE
 - STORE_IND
 - LOAD_IND
 - DMA_FUNC
 - DMA_ON_RESET

*/

#define DMA_FUNC h6809_mainloop(CPU_INT_STATUS, ALARM_CONTEXT)

#define DMA_ON_RESET                                                             \
    while (petres.superpet && petres.superpet_cpu_switch == SUPERPET_CPU_6809) { \
        EXPORT_REGISTERS();                                                      \
        cpu6809_reset();                                                         \
        DMA_FUNC;                                                                \
        interrupt_ack_dma(CPU_INT_STATUS);                                       \
        IMPORT_REGISTERS();                                                      \
    }

#define HAVE_6809_REGS

#ifdef FEATURE_CPUMEMHISTORY
#warning "CPUMEMHISTORY implementation for xpet is incomplete"
void memmap_mem_store(unsigned int addr, unsigned int value)
{
    monitor_memmap_store(addr, MEMMAP_RAM_W);
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

void memmap_mark_read(unsigned int addr)
{
    monitor_memmap_store(addr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
    memmap_state &= ~(MEMMAP_STATE_OPCODE);
}

BYTE memmap_mem_read(unsigned int addr)
{
    memmap_mark_read(addr);
    return (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr));
}
#endif

#include "../maincpu.c"
