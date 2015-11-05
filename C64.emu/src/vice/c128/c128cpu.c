/*
 * c128cpu.c - Emulation of the main 8502 processor.
 *
 * Written by
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

#include "clkguard.h"
#include "maincpu.h"
#include "mem.h"
#include "vicii.h"
#include "viciitypes.h"
#include "z80.h"

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

/* ------------------------------------------------------------------------- */

/* C128 needs external reg_pc */
#define NEED_REG_PC

/* Put Z80 registers into monitor sturct.  */
#define HAVE_Z80_REGS

/* ------------------------------------------------------------------------- */

static int opcode_cycle[2];

/* 8502 cycle stretch indicator */
int maincpu_stretch = 0;

/* 8502 memory refresh alarm counter */
CLOCK c128cpu_memory_refresh_clk;

#define PAGE_ZERO mem_page_zero

#define PAGE_ONE mem_page_one

#define DMA_FUNC z80_mainloop(CPU_INT_STATUS, ALARM_CONTEXT)

#define DMA_ON_RESET                   \
    EXPORT_REGISTERS();                \
    DMA_FUNC;                          \
    interrupt_ack_dma(CPU_INT_STATUS); \
    IMPORT_REGISTERS();                \
    JUMP(LOAD_ADDR(0xfffc));

inline static void c128cpu_clock_add(CLOCK *clock, int amount)
{
    if (amount) {
        *clock = vicii_clock_add(*clock, amount);
    }
}

inline static void c128cpu_memory_refresh_alarm_handler(void)
{
    if (maincpu_clk >= c128cpu_memory_refresh_clk) {
        vicii_memory_refresh_alarm_handler();
    }
}

static void clk_overflow_callback(CLOCK sub, void *unused_data)
{
    c128cpu_memory_refresh_clk -= sub;
}

#define CLK_ADD(clock, amount) c128cpu_clock_add(&clock, amount)

#define REWIND_FETCH_OPCODE(clock) vicii_clock_add(clock, -(2 + opcode_cycle[0] + opcode_cycle[1]))

#define CPU_DELAY_CLK vicii_delay_clk();

#define CPU_REFRESH_CLK c128cpu_memory_refresh_alarm_handler();

#define CPU_ADDITIONAL_RESET() c128cpu_memory_refresh_clk = 11

#define CPU_ADDITIONAL_INIT() clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL)

#ifdef FEATURE_CPUMEMHISTORY
#warning "CPUMEMHISTORY implementation for x128 is incomplete"
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


/* 8502 in fast mode always uses 0xee */
#define ANE(value, pc_inc)                                              \
    do {                                                                \
        BYTE tmp;                                                       \
        if (vicii.fastmode != 0) {                                      \
            tmp = ((reg_a_read | 0xee) & reg_x_read & ((BYTE)(value))); \
        } else {                                                        \
            tmp = ((reg_a_read | 0xff) & reg_x_read & ((BYTE)(value))); \
        }                                                               \
        reg_a_write(tmp);                                               \
        LOCAL_SET_NZ(tmp);                                              \
        INC_PC(pc_inc);                                                 \
    } while (0)

/* No OR takes place on 8502 */
#define LXA(value, pc_inc)                           \
    do {                                             \
        BYTE tmp = ((reg_a_read) & ((BYTE)(value))); \
        reg_x_write(tmp);                            \
        reg_a_write(tmp);                            \
        LOCAL_SET_NZ(tmp);                           \
        INC_PC(pc_inc);                              \
    } while (0)

#include "../maincpu.c"
