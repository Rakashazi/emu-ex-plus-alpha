/*
 * vsidcpu.c - Emulation of the main 6510 processor used for vsid and x64
 *
 * Written by groepaz <groepaz@gmx.net>
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

#ifdef FEATURE_CPUMEMHISTORY
#include "monitor.h"
#include "c64pla.h"
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

#ifdef FEATURE_CPUMEMHISTORY
void memmap_mem_store(unsigned int addr, unsigned int value)
{
    if ((addr >= 0xd000) && (addr <= 0xdfff)) {
        monitor_memmap_store(addr, MEMMAP_I_O_W);
    } else {
        monitor_memmap_store(addr, MEMMAP_RAM_W);
    }
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value));
}

void memmap_mark_read(unsigned int addr)
{
    switch (addr >> 12) {
        case 0xa:
        case 0xb:
        case 0xe:
        case 0xf:
            memmap_state |= MEMMAP_STATE_IGNORE;
            if (pport.data_read & (1 << ((addr >> 14) & 1))) {
                monitor_memmap_store(addr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_ROM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_ROM_R);
            } else {
                monitor_memmap_store(addr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
            }
            memmap_state &= ~(MEMMAP_STATE_IGNORE);
            break;
        case 0xd:
            monitor_memmap_store(addr, MEMMAP_I_O_R);
            break;
        default:
            monitor_memmap_store(addr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
            break;
    }
    memmap_state &= ~(MEMMAP_STATE_OPCODE);
}

BYTE memmap_mem_read(unsigned int addr)
{
    memmap_mark_read(addr);
    return (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr));
}
#endif

#include "../maincpu.c"
