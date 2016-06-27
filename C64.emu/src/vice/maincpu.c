/*
 * maincpu.c - Emulation of the main 6510 processor.
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

#include <stdio.h>
#include <stdlib.h>

#include "6510core.h"
#include "alarm.h"
#include "clkguard.h"
#include "debug.h"
#include "interrupt.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#ifdef C64DTV
#include "mos6510dtv.h"
#else
#include "mos6510.h"
#endif
#include "h6809regs.h"
#include "snapshot.h"
#include "traps.h"
#include "types.h"

/* MACHINE_STUFF should define/undef

 - NEED_REG_PC

 The following are optional:

 - PAGE_ZERO
 - PAGE_ONE
 - STORE_IND
 - LOAD_IND

*/

/* ------------------------------------------------------------------------- */

#define NEED_REG_PC

/* ------------------------------------------------------------------------- */

/* Implement the hack to make opcode fetches faster.  */
#define JUMP(addr)                                                                         \
    do {                                                                                   \
        reg_pc = (unsigned int)(addr);                                                     \
        if (reg_pc >= (unsigned int)bank_limit || reg_pc < (unsigned int)bank_start) {     \
            mem_mmu_translate((unsigned int)(addr), &bank_base, &bank_start, &bank_limit); \
        }                                                                                  \
    } while (0)

/* ------------------------------------------------------------------------- */

#ifdef FEATURE_CPUMEMHISTORY
#ifndef C64DTV /* FIXME: fix DTV and remove this */

/* map access functions to memmap hooks */
#ifndef STORE
#define STORE(addr, value) \
    memmap_mem_store(addr, value)
#endif

#ifndef LOAD
#define LOAD(addr) \
    memmap_mem_read(addr)
#endif

#ifndef STORE_ZERO
#define STORE_ZERO(addr, value) \
    memmap_mem_store((addr) & 0xff, value)
#endif

#ifndef LOAD_ZERO
#define LOAD_ZERO(addr) \
    memmap_mem_read((addr) & 0xff)
#endif

#endif /* C64DTV */
#endif /* FEATURE_CPUMEMHISTORY */

#ifndef STORE
#define STORE(addr, value) \
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value))
#endif

#ifndef LOAD
#define LOAD(addr) \
    (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr))
#endif

#ifndef STORE_ZERO
#define STORE_ZERO(addr, value) \
    (*_mem_write_tab_ptr[0])((WORD)(addr), (BYTE)(value))
#endif

#ifndef LOAD_ZERO
#define LOAD_ZERO(addr) \
    (*_mem_read_tab_ptr[0])((WORD)(addr))
#endif

#define LOAD_ADDR(addr) \
    ((LOAD((addr) + 1) << 8) | LOAD(addr))

#define LOAD_ZERO_ADDR(addr) \
    ((LOAD_ZERO((addr) + 1) << 8) | LOAD_ZERO(addr))

/* Those may be overridden by the machine stuff.  Probably we want them in
   the .def files, but if most of the machines do not use, we might keep it
   here and only override it where needed.  */
#ifndef PAGE_ZERO
#define PAGE_ZERO mem_ram
#endif

#ifndef PAGE_ONE
#define PAGE_ONE (mem_ram + 0x100)
#endif

#ifndef STORE_IND
#define STORE_IND(addr, value) STORE((addr), (value))
#endif

#ifndef LOAD_IND
#define LOAD_IND(addr) LOAD((addr))
#endif

#ifndef DMA_FUNC
static void maincpu_generic_dma(void)
{
    /* Generic DMA hosts can be implemented here.
       For example a very accurate REU emulation. */
}
#define DMA_FUNC maincpu_generic_dma()
#endif

#ifndef DMA_ON_RESET
#define DMA_ON_RESET
#endif

#ifndef CPU_ADDITIONAL_RESET
#define CPU_ADDITIONAL_RESET()
#endif

#ifndef CPU_ADDITIONAL_INIT
#define CPU_ADDITIONAL_INIT()
#endif

/* ------------------------------------------------------------------------- */

struct interrupt_cpu_status_s *maincpu_int_status = NULL;
#ifndef CYCLE_EXACT_ALARM
alarm_context_t *maincpu_alarm_context = NULL;
#endif
clk_guard_t *maincpu_clk_guard = NULL;
monitor_interface_t *maincpu_monitor_interface = NULL;

/* Global clock counter.  */
CLOCK maincpu_clk = 0L;
/* if != 0, exit when this many cycles have been executed */
CLOCK maincpu_clk_limit = 0L;

/* This is flag is set to 1 each time a Read-Modify-Write instructions that
   accesses memory is executed.  We can emulate the RMW behaviour of the 6510
   this way.  VERY important notice: Always assign 1 for true, 0 for false!
   Some functions depend on this to do some optimization.  */
int maincpu_rmw_flag = 0;

/* Information about the last executed opcode.  This is used to know the
   number of write cycles in the last executed opcode and to delay interrupts
   by one more cycle if necessary, as happens with conditional branch opcodes
   when the branch is taken.  */
unsigned int last_opcode_info;

/* Address of the last executed opcode. This is used by watchpoints. */
unsigned int last_opcode_addr;

/* Number of write cycles for each 6510 opcode.  */
const CLOCK maincpu_opcode_write_cycles[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  3, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $00 */
    /* $10 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $10 */
    /* $20 */  2, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $20 */
    /* $30 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $30 */
    /* $40 */  0, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 2, 2, /* $40 */
    /* $50 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $50 */
    /* $60 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $60 */
    /* $70 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $70 */
    /* $80 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
    /* $90 */  0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
    /* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $A0 */
    /* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* $B0 */
    /* $C0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $C0 */
    /* $D0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, /* $D0 */
    /* $E0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, /* $E0 */
    /* $F0 */  0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2  /* $F0 */
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
};

/* Public copy of the CPU registers.  As putting the registers into the
   function makes it faster, you have to generate a `TRAP' interrupt to have
   the values copied into this struct.  */
#ifdef C64DTV
mos6510dtv_regs_t maincpu_regs;
#else
mos6510_regs_t maincpu_regs;
#endif

/* ------------------------------------------------------------------------- */

monitor_interface_t *maincpu_monitor_interface_get(void)
{
#ifdef C64DTV
    maincpu_monitor_interface->cpu_regs = NULL;
    maincpu_monitor_interface->dtv_cpu_regs = &maincpu_regs;
#else
    maincpu_monitor_interface->cpu_regs = &maincpu_regs;
    maincpu_monitor_interface->cpu_R65C02_regs = NULL;
    maincpu_monitor_interface->dtv_cpu_regs = NULL;
#endif

#ifdef HAVE_Z80_REGS
    maincpu_monitor_interface->z80_cpu_regs = &z80_regs;
#else
    maincpu_monitor_interface->z80_cpu_regs = NULL;
#endif
#ifdef HAVE_6809_REGS
    maincpu_monitor_interface->h6809_cpu_regs = &h6809_regs;
#else
    maincpu_monitor_interface->h6809_cpu_regs = NULL;
#endif

    maincpu_monitor_interface->int_status = maincpu_int_status;

    maincpu_monitor_interface->clk = &maincpu_clk;

    maincpu_monitor_interface->current_bank = 0;
    maincpu_monitor_interface->mem_bank_list = mem_bank_list;
    maincpu_monitor_interface->mem_bank_from_name = mem_bank_from_name;
    maincpu_monitor_interface->mem_bank_read = mem_bank_read;
    maincpu_monitor_interface->mem_bank_peek = mem_bank_peek;
    maincpu_monitor_interface->mem_bank_write = mem_bank_write;

    maincpu_monitor_interface->mem_ioreg_list_get = mem_ioreg_list_get;

    maincpu_monitor_interface->toggle_watchpoints_func = mem_toggle_watchpoints;

    maincpu_monitor_interface->set_bank_base = NULL;
    maincpu_monitor_interface->get_line_cycle = machine_get_line_cycle;

    return maincpu_monitor_interface;
}

/* ------------------------------------------------------------------------- */

void maincpu_early_init(void)
{
    maincpu_int_status = interrupt_cpu_status_new();
}

void maincpu_init(void)
{
    interrupt_cpu_status_init(maincpu_int_status, &last_opcode_info);

    /* cpu specifix additional init routine */
    CPU_ADDITIONAL_INIT();
}

void maincpu_shutdown(void)
{
    interrupt_cpu_status_destroy(maincpu_int_status);
}

static void cpu_reset(void)
{
    int preserve_monitor;

    preserve_monitor = maincpu_int_status->global_pending_int & IK_MONITOR;

    interrupt_cpu_status_reset(maincpu_int_status);

    if (preserve_monitor) {
        interrupt_monitor_trap_on(maincpu_int_status);
    }

    maincpu_clk = 6; /* # of clock cycles needed for RESET.  */

    /* CPU specific extra reset routine, currently only used
       for 8502 fast mode refresh cycle. */
    CPU_ADDITIONAL_RESET();

    /* Do machine-specific initialization.  */
    machine_reset();
}

void maincpu_reset(void)
{
    cpu_reset();
}

/* ------------------------------------------------------------------------- */

/* Return nonzero if a pending NMI should be dispatched now.  This takes
   account for the internal delays of the 6510, but does not actually check
   the status of the NMI line.  */
inline static int interrupt_check_nmi_delay(interrupt_cpu_status_t *cs,
                                            CLOCK cpu_clk)
{
    CLOCK nmi_clk = cs->nmi_clk + INTERRUPT_DELAY;

    /* BRK (0x00) delays the NMI by one opcode.  */
    /* TODO DO_INTERRUPT sets last opcode to 0: can NMI occur right after IRQ? */
    if (OPINFO_NUMBER(*cs->last_opcode_info_ptr) == 0x00) {
        return 0;
    }

    /* Branch instructions delay IRQs and NMI by one cycle if branch
       is taken with no page boundary crossing.  */
    if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr)) {
        nmi_clk++;
    }

    if (cpu_clk >= nmi_clk) {
        return 1;
    }

    return 0;
}

/* Return nonzero if a pending IRQ should be dispatched now.  This takes
   account for the internal delays of the 6510, but does not actually check
   the status of the IRQ line.  */
inline static int interrupt_check_irq_delay(interrupt_cpu_status_t *cs,
                                            CLOCK cpu_clk)
{
    CLOCK irq_clk = cs->irq_clk + INTERRUPT_DELAY;

    /* Branch instructions delay IRQs and NMI by one cycle if branch
       is taken with no page boundary crossing.  */
    if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr)) {
        irq_clk++;
    }

    /* If an opcode changes the I flag from 1 to 0, the 6510 needs
       one more opcode before it triggers the IRQ routine.  */
    if (cpu_clk >= irq_clk) {
        if (!OPINFO_ENABLES_IRQ(*cs->last_opcode_info_ptr)) {
            return 1;
        } else {
            cs->global_pending_int |= IK_IRQPEND;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

#ifdef NEED_REG_PC
unsigned int reg_pc;
#endif

static BYTE **o_bank_base;
static int *o_bank_start;
static int *o_bank_limit;

void maincpu_resync_limits(void)
{
    if (o_bank_base) {
        mem_mmu_translate(reg_pc, o_bank_base, o_bank_start, o_bank_limit);
    }
}

void maincpu_mainloop(void)
{
#ifndef C64DTV
    /* Notice that using a struct for these would make it a lot slower (at
       least, on gcc 2.7.2.x).  */
    BYTE reg_a = 0;
    BYTE reg_x = 0;
    BYTE reg_y = 0;
#else
    int reg_a_read_idx = 0;
    int reg_a_write_idx = 0;
    int reg_x_idx = 2;
    int reg_y_idx = 1;

#define reg_a_write(c)                      \
    do {                                    \
        dtv_registers[reg_a_write_idx] = c; \
        if (reg_a_write_idx >= 3) {         \
            maincpu_resync_limits();        \
        }                                   \
    } while (0);
#define reg_a_read dtv_registers[reg_a_read_idx]
#define reg_x_write(c)                \
    do {                              \
        dtv_registers[reg_x_idx] = c; \
        if (reg_x_idx >= 3) {         \
            maincpu_resync_limits();  \
        }                             \
    } while (0);

#define reg_x_read dtv_registers[reg_x_idx]
#define reg_y_write(c)                \
    do {                              \
        dtv_registers[reg_y_idx] = c; \
        if (reg_y_idx >= 3) {         \
            maincpu_resync_limits();  \
        }                             \
    } while (0);
#define reg_y_read dtv_registers[reg_y_idx]
#endif
    BYTE reg_p = 0;
    BYTE reg_sp = 0;
    BYTE flag_n = 0;
    BYTE flag_z = 0;
#ifndef NEED_REG_PC
    unsigned int reg_pc;
#endif
    BYTE *bank_base;
    int bank_start = 0;
    int bank_limit = 0;

    o_bank_base = &bank_base;
    o_bank_start = &bank_start;
    o_bank_limit = &bank_limit;

    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);

    while (1) {
#define CLK maincpu_clk
#define RMW_FLAG maincpu_rmw_flag
#define LAST_OPCODE_INFO last_opcode_info
#define LAST_OPCODE_ADDR last_opcode_addr
#define TRACEFLG debug.maincpu_traceflg

#define CPU_INT_STATUS maincpu_int_status

#define ALARM_CONTEXT maincpu_alarm_context

#define CHECK_PENDING_ALARM() (clk >= next_alarm_clk(maincpu_int_status))

#define CHECK_PENDING_INTERRUPT() check_pending_interrupt(maincpu_int_status)

#define TRAP(addr) maincpu_int_status->trap_func(addr);

#define ROM_TRAP_HANDLER() traps_handler()

#define JAM()                                                         \
    do {                                                              \
        unsigned int tmp;                                             \
                                                                      \
        EXPORT_REGISTERS();                                           \
        tmp = machine_jam("   " CPU_STR ": JAM at $%04X   ", reg_pc); \
        switch (tmp) {                                                \
            case JAM_RESET:                                           \
                DO_INTERRUPT(IK_RESET);                               \
                break;                                                \
            case JAM_HARD_RESET:                                      \
                mem_powerup();                                        \
                DO_INTERRUPT(IK_RESET);                               \
                break;                                                \
            case JAM_MONITOR:                                         \
                monitor_startup(e_comp_space);                        \
                IMPORT_REGISTERS();                                   \
                break;                                                \
            default:                                                  \
                CLK++;                                                \
        }                                                             \
    } while (0)

#define CALLER e_comp_space

#define ROM_TRAP_ALLOWED() mem_rom_trap_allowed((WORD)reg_pc)

#define GLOBAL_REGS maincpu_regs

#include "6510core.c"

        maincpu_int_status->num_dma_per_opcode = 0;

        if (maincpu_clk_limit && (maincpu_clk > maincpu_clk_limit)) {
            log_error(LOG_DEFAULT, "cycle limit reached.");
            exit(EXIT_FAILURE);
        }
#if 0
        if (CLK > 246171754) {
            debug.maincpu_traceflg = 1;
        }
#endif
    }
}

/* ------------------------------------------------------------------------- */

void maincpu_set_pc(int pc) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_PC(&maincpu_regs, pc);
#else
    MOS6510_REGS_SET_PC(&maincpu_regs, pc);
#endif
}

void maincpu_set_a(int a) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_A(&maincpu_regs, a);
#else
    MOS6510_REGS_SET_A(&maincpu_regs, a);
#endif
}

void maincpu_set_x(int x) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_X(&maincpu_regs, x);
#else
    MOS6510_REGS_SET_X(&maincpu_regs, x);
#endif
}

void maincpu_set_y(int y) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_Y(&maincpu_regs, y);
#else
    MOS6510_REGS_SET_Y(&maincpu_regs, y);
#endif
}

void maincpu_set_sign(int n) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_SIGN(&maincpu_regs, n);
#else
    MOS6510_REGS_SET_SIGN(&maincpu_regs, n);
#endif
}

void maincpu_set_zero(int z) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_ZERO(&maincpu_regs, z);
#else
    MOS6510_REGS_SET_ZERO(&maincpu_regs, z);
#endif
}

void maincpu_set_carry(int c) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_CARRY(&maincpu_regs, c);
#else
    MOS6510_REGS_SET_CARRY(&maincpu_regs, c);
#endif
}

void maincpu_set_interrupt(int i) {
#ifdef C64DTV
    MOS6510DTV_REGS_SET_INTERRUPT(&maincpu_regs, i);
#else
    MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, i);
#endif
}

unsigned int maincpu_get_pc(void) {
#ifdef C64DTV
    return MOS6510DTV_REGS_GET_PC(&maincpu_regs);
#else
    return MOS6510_REGS_GET_PC(&maincpu_regs);
#endif
}

unsigned int maincpu_get_a(void) {
#ifdef C64DTV
    return MOS6510DTV_REGS_GET_A(&maincpu_regs);
#else
    return MOS6510_REGS_GET_A(&maincpu_regs);
#endif
}

unsigned int maincpu_get_x(void) {
#ifdef C64DTV
    return MOS6510DTV_REGS_GET_X(&maincpu_regs);
#else
    return MOS6510_REGS_GET_X(&maincpu_regs);
#endif
}

unsigned int maincpu_get_y(void) {
#ifdef C64DTV
    return MOS6510DTV_REGS_GET_Y(&maincpu_regs);
#else
    return MOS6510_REGS_GET_Y(&maincpu_regs);
#endif
}

unsigned int maincpu_get_sp(void) {
#ifdef C64DTV
    return MOS6510DTV_REGS_GET_SP(&maincpu_regs);
#else
    return MOS6510_REGS_GET_SP(&maincpu_regs);
#endif
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "MAINCPU";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int maincpu_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
                               ((BYTE)SNAP_MINOR));
    if (m == NULL) {
        return -1;
    }

#ifdef C64DTV
    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_A(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_X(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_Y(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_SP(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)MOS6510DTV_REGS_GET_PC(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)MOS6510DTV_REGS_GET_STATUS(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R3(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R4(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R5(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R6(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R7(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R8(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R9(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R10(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R11(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R12(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R13(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R14(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_R15(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_ACM(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510DTV_REGS_GET_YXM(&maincpu_regs)) < 0
        || SMW_BA(m, burst_cache, 4) < 0
        || SMW_W(m, burst_addr) < 0
        || SMW_DW(m, dtvclockneg) < 0
        || SMW_DW(m, (DWORD)last_opcode_info) < 0) {
        goto fail;
    }
#else
    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_B(m, MOS6510_REGS_GET_A(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_X(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_Y(&maincpu_regs)) < 0
        || SMW_B(m, MOS6510_REGS_GET_SP(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)MOS6510_REGS_GET_PC(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)MOS6510_REGS_GET_STATUS(&maincpu_regs)) < 0
        || SMW_DW(m, (DWORD)last_opcode_info) < 0) {
        goto fail;
    }
#endif

    if (interrupt_write_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_write_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int maincpu_snapshot_read_module(snapshot_t *s)
{
    BYTE a, x, y, sp, status;
#ifdef C64DTV
    BYTE r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, acm, yxm;
#endif
    WORD pc;
    BYTE major, minor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);
    if (m == NULL) {
        return -1;
    }

    /* FIXME: This is a mighty kludge to prevent VIC-II from stealing the
       wrong number of cycles.  */
    maincpu_rmw_flag = 0;

    /* XXX: Assumes `CLOCK' is the same size as a `DWORD'.  */
    if (0
        || SMR_DW(m, &maincpu_clk) < 0
        || SMR_B(m, &a) < 0
        || SMR_B(m, &x) < 0
        || SMR_B(m, &y) < 0
        || SMR_B(m, &sp) < 0
        || SMR_W(m, &pc) < 0
        || SMR_B(m, &status) < 0
#ifdef C64DTV
        || SMR_B(m, &r3) < 0
        || SMR_B(m, &r4) < 0
        || SMR_B(m, &r5) < 0
        || SMR_B(m, &r6) < 0
        || SMR_B(m, &r7) < 0
        || SMR_B(m, &r8) < 0
        || SMR_B(m, &r9) < 0
        || SMR_B(m, &r10) < 0
        || SMR_B(m, &r11) < 0
        || SMR_B(m, &r12) < 0
        || SMR_B(m, &r13) < 0
        || SMR_B(m, &r14) < 0
        || SMR_B(m, &r15) < 0
        || SMR_B(m, &acm) < 0
        || SMR_B(m, &yxm) < 0
        || SMR_BA(m, burst_cache, 4) < 0
        || SMR_W(m, &burst_addr) < 0
        || SMR_DW_INT(m, &dtvclockneg) < 0
#endif
        || SMR_DW_UINT(m, &last_opcode_info) < 0) {
        goto fail;
    }

#ifdef C64DTV
    MOS6510DTV_REGS_SET_A(&maincpu_regs, a);
    MOS6510DTV_REGS_SET_X(&maincpu_regs, x);
    MOS6510DTV_REGS_SET_Y(&maincpu_regs, y);
    MOS6510DTV_REGS_SET_SP(&maincpu_regs, sp);
    MOS6510DTV_REGS_SET_PC(&maincpu_regs, pc);
    MOS6510DTV_REGS_SET_STATUS(&maincpu_regs, status);
    MOS6510DTV_REGS_SET_R3(&maincpu_regs, r3);
    MOS6510DTV_REGS_SET_R4(&maincpu_regs, r4);
    MOS6510DTV_REGS_SET_R5(&maincpu_regs, r5);
    MOS6510DTV_REGS_SET_R6(&maincpu_regs, r6);
    MOS6510DTV_REGS_SET_R7(&maincpu_regs, r7);
    MOS6510DTV_REGS_SET_R8(&maincpu_regs, r8);
    MOS6510DTV_REGS_SET_R9(&maincpu_regs, r9);
    MOS6510DTV_REGS_SET_R10(&maincpu_regs, r10);
    MOS6510DTV_REGS_SET_R11(&maincpu_regs, r11);
    MOS6510DTV_REGS_SET_R12(&maincpu_regs, r12);
    MOS6510DTV_REGS_SET_R13(&maincpu_regs, r13);
    MOS6510DTV_REGS_SET_R14(&maincpu_regs, r14);
    MOS6510DTV_REGS_SET_R15(&maincpu_regs, r15);
    MOS6510DTV_REGS_SET_ACM(&maincpu_regs, acm);
    MOS6510DTV_REGS_SET_YXM(&maincpu_regs, yxm);
#else
    MOS6510_REGS_SET_A(&maincpu_regs, a);
    MOS6510_REGS_SET_X(&maincpu_regs, x);
    MOS6510_REGS_SET_Y(&maincpu_regs, y);
    MOS6510_REGS_SET_SP(&maincpu_regs, sp);
    MOS6510_REGS_SET_PC(&maincpu_regs, pc);
    MOS6510_REGS_SET_STATUS(&maincpu_regs, status);
#endif

    if (interrupt_read_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_read_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
