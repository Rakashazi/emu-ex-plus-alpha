/*
 * c128cpu.c - Emulation of the main 8502 processor.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "vicii.h"
#include "viciitypes.h"
#include "z80.h"
#include "c128mmu.h"

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

/* functions for mmu region swap handling */

/* FIXME: some cases might not be handled correctly yet
          they will be implemented based on the results
          of tests on real hardware. */
static uint8_t c128_cpu_mmu_page_0 = 0;
static uint8_t c128_cpu_mmu_page_1 = 1;

static uint8_t c128_cpu_mmu_page_0_bank = 0;
static uint8_t c128_cpu_mmu_page_1_bank = 0;

static uint8_t c128_cpu_mmu_page_0_target_ram = 1;
static uint8_t c128_cpu_mmu_page_1_target_ram = 1;

static uint8_t c128_cpu_mmu_zp_sp_shared = 0;

void c128_cpu_set_mmu_page_0(uint8_t val)
{
    c128_cpu_mmu_page_0 = val;
}

void c128_cpu_set_mmu_page_1(uint8_t val)
{
    c128_cpu_mmu_page_1 = val;
}

void c128_cpu_set_mmu_page_0_bank(uint8_t val)
{
    c128_cpu_mmu_page_0_bank = val;
}

void c128_cpu_set_mmu_page_1_bank(uint8_t val)
{
    c128_cpu_mmu_page_1_bank = val;
}

void c128_cpu_set_mmu_page_0_target_ram(uint8_t val)
{
    c128_cpu_mmu_page_0_target_ram = val;
}

void c128_cpu_set_mmu_page_1_target_ram(uint8_t val)
{
    c128_cpu_mmu_page_1_target_ram = val;
}

void c128_cpu_set_mmu_zp_sp_shared(uint8_t val)
{
    c128_cpu_mmu_zp_sp_shared = val;
}

static uint8_t c128_cpu_mmu_wrap_read_zero(uint16_t address)
{
    uint8_t defretval = _mem_read_tab_ptr[0]((uint16_t)address);

    uint8_t addr_pos = (address & 0xff);
    uint8_t addr_page = 0;
    uint8_t addr_bank = 0;
    uint16_t addr;
    int use_ram_only = 0;

    /* Make sure the internal cpu port is always used for address 0 and 1 */
    if (address == 0 || address == 1) {
        return defretval;
    }

    /* Check if there is no translation that needs to be done */
    if (c128_cpu_mmu_page_0 == 0 && c128_cpu_mmu_page_0_bank == 0) {
        return defretval;
    }

    /* check if the address page is page 0 and in shared memory then bank does not change */
    if (c128_cpu_mmu_zp_sp_shared && addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        use_ram_only = 1;
    /* check if the address page is page 0 and replace addr with mmu given page and bank */
    } else if (addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    /* check if the address page is page 0 target and if it is current RAM, ifso replace addr with page 0 and bank 0 */
    } else if (addr_page == c128_cpu_mmu_page_0 && c128_cpu_mmu_page_0_target_ram) {
        addr_page = 0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    }

    if (use_ram_only) {
        addr = (addr_page << 8) | addr_pos;
        return mem_ram[addr | (addr_bank << 16)];
    }

    return defretval;
}

static uint8_t c128_cpu_mmu_wrap_read(uint16_t address)
{
    uint8_t addr_pos = (address & 0xff);
    uint8_t addr_page = (address >> 8);
    uint8_t addr_bank = 0;
    uint16_t addr;
    int use_ram_only = 0;

    /* Check if there is no translation that needs to be done */
    if (c128_cpu_mmu_page_0 == 0 && c128_cpu_mmu_page_1 == 1 && c128_cpu_mmu_page_0_bank == 0 && c128_cpu_mmu_page_1_bank == 0) {
        return _mem_read_tab_ptr[address >> 8]((uint16_t)address);
    }

    /* Make sure the internal cpu port is always used for address 0 and 1 */
    if (address == 0 || address == 1) {
        return _mem_read_tab_ptr[addr_page]((uint16_t)address);
    }

    /* check if the address page is page 1 and in shared memory then bank does not change */
    if (c128_cpu_mmu_zp_sp_shared && addr_page == 1) {
        addr_page = c128_cpu_mmu_page_1;
        use_ram_only = 1;
    /* check if the address page is page 0 and in shared memory then bank does not change */
    } else if (c128_cpu_mmu_zp_sp_shared && addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        use_ram_only = 1;
    /* check if the address page is page 1 and replace addr with mmu given page and bank */
    } else if (addr_page == 1) {
        addr_page = c128_cpu_mmu_page_1;
        addr_bank = c128_cpu_mmu_page_1_bank;
        use_ram_only = 1;
    /* check if the address page is page 1 target and if it is current RAM, ifso replace addr with page 1 and bank 0 */
    } else if (addr_page == c128_cpu_mmu_page_1 && c128_cpu_mmu_page_1_target_ram) {
        addr_page = 1;
        addr_bank = c128_cpu_mmu_page_1_bank;
        use_ram_only = 1;
    /* check if the address page is page 0 and replace addr with mmu given page and bank */
    } else if (addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    /* check if the address page is page 0 target and if it is current RAM, ifso replace addr with page 0 and bank 0 */
    } else if (addr_page == c128_cpu_mmu_page_0 && c128_cpu_mmu_page_0_target_ram) {
        addr_page = 0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    }

    addr = (addr_page << 8) | addr_pos;

    if (use_ram_only) {
        return mem_ram[addr | (addr_bank << 16)];
    }

    return _mem_read_tab_ptr[address >> 8]((uint16_t)address);
}

static void c128_cpu_mmu_wrap_store(uint16_t address, uint8_t value)
{
    uint8_t addr_pos = (address & 0xff);
    uint8_t addr_page = (address >> 8);
    uint8_t addr_bank = 0;
    uint16_t addr;
    int use_ram_only = 0;

    /* Check if there is no translation that needs to be done */
    if (c128_cpu_mmu_page_0 == 0 && c128_cpu_mmu_page_1 == 1 && c128_cpu_mmu_page_0_bank == 0 && c128_cpu_mmu_page_1_bank == 0) {
        _mem_write_tab_ptr[addr_page]((uint16_t)address, value);
        return;
    }


    /* Make sure the internal cpu port is always used for address 0 and 1 */
    if (address == 0 || address == 1) {
        _mem_write_tab_ptr[addr_page]((uint16_t)address, value);
        return;
    }

    /* check if the address page is page 1 and in shared memory then bank does not change */
    if (c128_cpu_mmu_zp_sp_shared && addr_page == 1) {
        addr_page = c128_cpu_mmu_page_1;
        use_ram_only = 1;
    /* check if the address page is page 0 and in shared memory then bank does not change */
    } else if (c128_cpu_mmu_zp_sp_shared && addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        use_ram_only = 1;
    /* check if the address page is page 1 and replace addr with mmu given page and bank */
    } else if (addr_page == 1) {
        addr_page = c128_cpu_mmu_page_1;
        addr_bank = c128_cpu_mmu_page_1_bank;
        use_ram_only = 1;
    /* check if the address page is page 1 target and if it is current RAM, ifso replace addr with page 1 and bank 0 */
    } else if (addr_page == c128_cpu_mmu_page_1 && c128_cpu_mmu_page_1_target_ram) {
        addr_page = 1;
        addr_bank = c128_cpu_mmu_page_1_bank;
        use_ram_only = 1;
    /* check if the address page is page 0 and replace addr with mmu given page and bank */
    } else if (addr_page == 0) {
        addr_page = c128_cpu_mmu_page_0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    /* check if the address page is page 0 target and if it is current RAM, ifso replace addr with page 0 and bank 0 */
    } else if (addr_page == c128_cpu_mmu_page_0 && c128_cpu_mmu_page_0_target_ram) {
        addr_page = 0;
        addr_bank = c128_cpu_mmu_page_0_bank;
        use_ram_only = 1;
    }

    addr = (addr_page << 8) | addr_pos;

    if (use_ram_only) {
        mem_ram[addr | (addr_bank << 16)] = value;
    } else {
        _mem_write_tab_ptr[address >> 8]((uint16_t)address, value);
    }
}

/* ------------------------------------------------------------------------- */

static int opcode_cycle[2];

/* 8502 cycle stretch indicator */
int maincpu_stretch = 0;

/* 8502 memory refresh alarm counter */
CLOCK c128cpu_memory_refresh_clk;

#define PAGE_ZERO mem_page_zero

#define PAGE_ONE mem_page_one

#define DMA_FUNC z80_mainloop(CPU_INT_STATUS, ALARM_CONTEXT)

#define LOAD(addr) (c128_cpu_mmu_wrap_read((uint16_t)(addr)))

#define STORE(addr, value) (c128_cpu_mmu_wrap_store((uint16_t)(addr), (uint8_t)(value)))

#define STORE_ZERO(addr, value) (c128_cpu_mmu_wrap_store((uint16_t)(addr), (uint8_t)(value)))

#define LOAD_ZERO(addr) (c128_cpu_mmu_wrap_read_zero((uint16_t)(addr)))

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

#define CLK_ADD(clock, amount) c128cpu_clock_add(&clock, amount)

#define REWIND_FETCH_OPCODE(clock) vicii_clock_add(clock, -(2 + opcode_cycle[0] + opcode_cycle[1]))

#define CPU_DELAY_CLK vicii_delay_clk();

#define CPU_REFRESH_CLK c128cpu_memory_refresh_alarm_handler();

#define CPU_ADDITIONAL_RESET() c128cpu_memory_refresh_clk = 11

#ifdef FEATURE_CPUMEMHISTORY
#if 0
static void memmap_mem_store(unsigned int addr, unsigned int value)
{
    monitor_memmap_store(addr, MEMMAP_RAM_W);
    (*_mem_write_tab_ptr[(addr) >> 8])((uint16_t)(addr), (uint8_t)(value));
}
#endif

static void memmap_mark_read(unsigned int addr)
{
    monitor_memmap_store(addr, (memmap_state & MEMMAP_STATE_OPCODE) ? MEMMAP_RAM_X : (memmap_state & MEMMAP_STATE_INSTR) ? 0 : MEMMAP_RAM_R);
    memmap_state &= ~(MEMMAP_STATE_OPCODE);
}

#if 0
static uint8_t memmap_mem_read(unsigned int addr)
{
    memmap_mark_read(addr);
    return (*_mem_read_tab_ptr[(addr) >> 8])((uint16_t)(addr));
}
#endif

static uint8_t memmap_mem_read_dummy(unsigned int addr)
{
    memmap_mark_read(addr);
    return (*_mem_read_tab_ptr_dummy[(addr) >> 8])((uint16_t)(addr));
}

static void memmap_mem_store_dummy(unsigned int addr, unsigned int value)
{
    monitor_memmap_store(addr, MEMMAP_RAM_W);
    (*_mem_write_tab_ptr_dummy[(addr) >> 8])((uint16_t)(addr), (uint8_t)(value));
}
#endif


/* 8502 in fast mode always uses 0xee */
#define ANE(value, pc_inc)                                              \
    do {                                                                \
        uint8_t tmp;                                                       \
        if (vicii.fastmode != 0) {                                      \
            tmp = ((reg_a_read | 0xee) & reg_x_read & ((uint8_t)(value))); \
        } else {                                                        \
            tmp = ((reg_a_read | 0xff) & reg_x_read & ((uint8_t)(value))); \
        }                                                               \
        reg_a_write(tmp);                                               \
        LOCAL_SET_NZ(tmp);                                              \
        INC_PC(pc_inc);                                                 \
    } while (0)

/* No OR takes place on 8502 */
#define LXA(value, pc_inc)                           \
    do {                                             \
        uint8_t tmp = ((reg_a_read) & ((uint8_t)(value))); \
        reg_x_write(tmp);                            \
        reg_a_write(tmp);                            \
        LOCAL_SET_NZ(tmp);                           \
        INC_PC(pc_inc);                              \
    } while (0)

#include "../maincpu.c"
