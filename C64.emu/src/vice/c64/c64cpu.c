/*
 * c64cpu.c - Emulation of the main 6510 processor used for x64
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

#include "cpmcart.h"

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
 - CHECK_AND_RUN_ALTERNATE_CPU

*/

#ifdef FEATURE_CPUMEMHISTORY
/* FIXME do proper ROM/RAM/IO tests */

inline static void memmap_mem_update(unsigned int addr, int write)
{
    unsigned int type = MEMMAP_RAM_R;

    if (write) {
        if ((addr >= 0xd000) && (addr <= 0xdfff)) {
            type = MEMMAP_I_O_W;
        } else {
            type = MEMMAP_RAM_W;
        }
    } else {
        switch (addr >> 12) {
            case 0xa:
            case 0xb:
            case 0xe:
            case 0xf:
                if (pport.data_read & (1 << ((addr >> 14) & 1))) {
                    type = MEMMAP_ROM_R;
                } else {
                    type = MEMMAP_RAM_R;
                }
                break;
            case 0xd:
                type = MEMMAP_I_O_R;
                break;
            default:
                type = MEMMAP_RAM_R;
                break;
        }
        if (memmap_state & MEMMAP_STATE_OPCODE) {
            /* HACK: transform R to X */
            type >>= 2;
            memmap_state &= ~(MEMMAP_STATE_OPCODE);
        } else if (memmap_state & MEMMAP_STATE_INSTR) {
            /* ignore operand reads */
            type = 0;
        }
    }
    monitor_memmap_store(addr, type);
}

static void memmap_mem_store(unsigned int addr, unsigned int value)
{
    memmap_mem_update(addr, 1);
    (*_mem_write_tab_ptr[(addr) >> 8])((uint16_t)(addr), (uint8_t)(value));
}

static uint8_t memmap_mem_read(unsigned int addr)
{
    memmap_mem_update(addr, 0);
    return (*_mem_read_tab_ptr[(addr) >> 8])((uint16_t)(addr));
}

static void memmap_mark_read(unsigned int addr)
{
    memmap_mem_update(addr, 0);
}
#endif

static void check_and_run_alternate_cpu(void)
{
    cpmcart_check_and_run_z80();
}

#define CHECK_AND_RUN_ALTERNATE_CPU check_and_run_alternate_cpu();

#define HAVE_Z80_REGS

#include "../maincpu.c"
