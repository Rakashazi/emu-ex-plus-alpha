/*
 * main65816cpu.c - Emulation of the main 65816 processor.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Kajtar Zsolt <soci@c64.rulez.org>
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
#include "main65816cpu.h"
#include "mem.h"
#include "monitor.h"
#include "snapshot.h"
#include "traps.h"
#include "types.h"
#include "wdc65816.h"

/* MACHINE_STUFF should define/undef

 - NEED_REG_PC

*/

/* ------------------------------------------------------------------------- */

#define NEED_REG_PC

/* ------------------------------------------------------------------------- */

/* Implement the hack to make opcode fetches faster.  */
#define JUMP(addr)                            \
    do {                                      \
        reg_pc = (unsigned int)(addr);        \
        if (reg_pbr != bank_bank || reg_pc >= (unsigned int)bank_limit || reg_pc < (unsigned int)bank_start) { \
            mem_mmu_translate((unsigned int)(addr) | (reg_pbr << 16), &bank_base, &bank_start, &bank_limit); \
            bank_bank = reg_pbr;              \
        }                                     \
    } while (0)

/* ------------------------------------------------------------------------- */

#ifndef STORE
#define STORE(addr, value) \
    (*_mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value))
#endif

#ifndef LOAD
#define LOAD(addr) \
    (*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr))
#endif

/* Those may be overridden by the machine stuff.  Probably we want them in
   the .def files, but if most of the machines do not use, we might keep it
   here and only override it where needed.  */

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

/* This flag is an obsolete optimization. It's always 0 for the 65816 CPU,
   but has to be kept for the common code. */
int maincpu_rmw_flag = 0;

/* Global clock counter.  */
CLOCK maincpu_clk = 0L;
/* if != 0, exit when this many cycles have been executed */
CLOCK maincpu_clk_limit = 0L;

/* Information about the last executed opcode.  This is used to know the
   number of write cycles in the last executed opcode and to delay interrupts
   by one more cycle if necessary, as happens with conditional branch opcodes
   when the branch is taken.  */
unsigned int last_opcode_info;

/* Address of the last executed opcode. This is used by watchpoints. */
unsigned int last_opcode_addr;

/* Public copy of the CPU registers.  As putting the registers into the
   function makes it faster, you have to generate a `TRAP' interrupt to have
   the values copied into this struct.  */
WDC65816_regs_t maincpu_regs;

/* ------------------------------------------------------------------------- */

monitor_interface_t *maincpu_monitor_interface_get(void)
{
    maincpu_monitor_interface->cpu_regs = NULL;
    maincpu_monitor_interface->cpu_R65C02_regs = NULL;
    maincpu_monitor_interface->cpu_65816_regs = &maincpu_regs;
    maincpu_monitor_interface->dtv_cpu_regs = NULL;
    maincpu_monitor_interface->z80_cpu_regs = NULL;
    maincpu_monitor_interface->h6809_cpu_regs = NULL;

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

    /* cpu specific additional init routine */
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

#ifdef NEED_REG_PC
unsigned int reg_pc;
#endif

static BYTE **o_bank_base;
static int *o_bank_start;
static int *o_bank_limit;
static BYTE *o_bank_bank;

void maincpu_resync_limits(void) {
    if (o_bank_base) {
        mem_mmu_translate(reg_pc | (*o_bank_bank << 16), o_bank_base, o_bank_start, o_bank_limit);
    }
}

void maincpu_mainloop(void)
{
    /* Notice that using a struct for these would make it a lot slower (at
       least, on gcc 2.7.2.x).  */
 union regs {
     WORD reg_s;
     BYTE reg_q[2];
 } regs65802;

#define reg_c regs65802.reg_s
#ifndef WORDS_BIGENDIAN
#define reg_a regs65802.reg_q[0]
#define reg_b regs65802.reg_q[1]
#else
#define reg_a regs65802.reg_q[1]
#define reg_b regs65802.reg_q[0]
#endif

    WORD reg_x = 0;
    WORD reg_y = 0;
    BYTE reg_pbr = 0;
    BYTE reg_dbr = 0;
    WORD reg_dpr = 0;
    BYTE reg_p = 0;
    WORD reg_sp = 0x100;
    BYTE flag_n = 0;
    BYTE flag_z = 0;
    BYTE reg_emul = 1;
    int interrupt65816 = IK_RESET;
#ifndef NEED_REG_PC
    unsigned int reg_pc;
#endif
    BYTE *bank_base;
    int bank_start = 0;
    int bank_limit = 0;
    BYTE bank_bank = 0;

    o_bank_base = &bank_base;
    o_bank_start = &bank_start;
    o_bank_limit = &bank_limit;
    o_bank_bank = &bank_bank;

    reg_c = 0;

    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);

    while (1) {

#define CLK maincpu_clk
#define LAST_OPCODE_INFO last_opcode_info
#define LAST_OPCODE_ADDR last_opcode_addr
#define TRACEFLG debug.maincpu_traceflg

#define CPU_INT_STATUS maincpu_int_status

#define ALARM_CONTEXT maincpu_alarm_context

#define CHECK_PENDING_ALARM() \
   (clk >= next_alarm_clk(maincpu_int_status))

#define CHECK_PENDING_INTERRUPT() \
   check_pending_interrupt(maincpu_int_status)

#define TRAP(addr) \
   maincpu_int_status->trap_func(addr);

#define ROM_TRAP_HANDLER() \
   traps_handler()

#define JAM()                                                         \
    do {                                                              \
        unsigned int tmp;                                             \
                                                                      \
        EXPORT_REGISTERS();                                           \
        tmp = machine_jam("   " CPU_STR ": JAM at $%02x%04X   ", reg_pbr, reg_pc); \
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
                STP();                                                \
        }                                                             \
    } while (0)

#define STP_65816() JAM()
#define WAI_65816() WAI()
#define COP_65816(value) COP()

#define CALLER e_comp_space

#define ROM_TRAP_ALLOWED() mem_rom_trap_allowed((WORD)reg_pc)

#define GLOBAL_REGS maincpu_regs

#include "65816core.c"

        maincpu_int_status->num_dma_per_opcode = 0;

        if (maincpu_clk_limit && (maincpu_clk > maincpu_clk_limit)) {
            log_error(LOG_DEFAULT, "cycle limit reached.");
            exit(EXIT_FAILURE);
        }
#if 0
        if (CLK > 246171754)
            debug.maincpu_traceflg = 1;
#endif
    }
}

/* ------------------------------------------------------------------------- */

void maincpu_set_pc(int pc) {
    WDC65816_REGS_SET_PC(&maincpu_regs, pc);
}

void maincpu_set_a(int a) {
    WDC65816_REGS_SET_A(&maincpu_regs, a);
}

void maincpu_set_x(int x) {
    WDC65816_REGS_SET_X(&maincpu_regs, x);
}

void maincpu_set_y(int y) {
    WDC65816_REGS_SET_Y(&maincpu_regs, y);
}

void maincpu_set_sign(int n) {
    WDC65816_REGS_SET_SIGN(&maincpu_regs, n);
}

void maincpu_set_zero(int z) {
    WDC65816_REGS_SET_ZERO(&maincpu_regs, z);
}

void maincpu_set_carry(int c) {
    WDC65816_REGS_SET_CARRY(&maincpu_regs, c);
}

void maincpu_set_interrupt(int i) {
    WDC65816_REGS_SET_INTERRUPT(&maincpu_regs, i);
}

unsigned int maincpu_get_pc(void) {
    return WDC65816_REGS_GET_PC(&maincpu_regs);
}

unsigned int maincpu_get_a(void) {
    return WDC65816_REGS_GET_A(&maincpu_regs);
}

unsigned int maincpu_get_x(void) {
    return WDC65816_REGS_GET_X(&maincpu_regs);
}

unsigned int maincpu_get_y(void) {
    return WDC65816_REGS_GET_Y(&maincpu_regs);
}

unsigned int maincpu_get_sp(void) {
    return WDC65816_REGS_GET_SP(&maincpu_regs);
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "MAIN6565802CPU";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int maincpu_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
                               ((BYTE)SNAP_MINOR));
    if (m == NULL)
        return -1;

    if (0
        || SMW_DW(m, maincpu_clk) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_A(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_B(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)WDC65816_REGS_GET_X(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)WDC65816_REGS_GET_Y(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)WDC65816_REGS_GET_SP(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)WDC65816_REGS_GET_DPR(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_PBR(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_DBR(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_EMUL(&maincpu_regs)) < 0
        || SMW_W(m, (WORD)WDC65816_REGS_GET_PC(&maincpu_regs)) < 0
        || SMW_B(m, (BYTE)WDC65816_REGS_GET_STATUS(&maincpu_regs)) < 0
        || SMW_DW(m, (DWORD)last_opcode_info) < 0)
        goto fail;

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
    BYTE a, b, pbr, dbr, emul, status;
    WORD x, y, sp, pc, dpr;
    BYTE major, minor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);
    if (m == NULL) {
        return -1;
    }

    /* XXX: Assumes `CLOCK' is the same size as a `DWORD'.  */
    if (0
        || SMR_DW(m, &maincpu_clk) < 0
        || SMR_B(m, &a) < 0
        || SMR_B(m, &b) < 0
        || SMR_W(m, &x) < 0
        || SMR_W(m, &y) < 0
        || SMR_W(m, &sp) < 0
        || SMR_W(m, &dpr) < 0
        || SMR_B(m, &pbr) < 0
        || SMR_B(m, &dbr) < 0
        || SMR_B(m, &emul) < 0
        || SMR_W(m, &pc) < 0
        || SMR_B(m, &status) < 0
        || SMR_DW_UINT(m, &last_opcode_info) < 0)
        goto fail;

    WDC65816_REGS_SET_A(&maincpu_regs, a);
    WDC65816_REGS_SET_B(&maincpu_regs, b);
    WDC65816_REGS_SET_X(&maincpu_regs, x);
    WDC65816_REGS_SET_Y(&maincpu_regs, y);
    WDC65816_REGS_SET_SP(&maincpu_regs, sp);
    WDC65816_REGS_SET_DPR(&maincpu_regs, dpr);
    WDC65816_REGS_SET_PBR(&maincpu_regs, pbr);
    WDC65816_REGS_SET_DBR(&maincpu_regs, dbr);
    WDC65816_REGS_SET_EMUL(&maincpu_regs, emul);
    WDC65816_REGS_SET_PC(&maincpu_regs, pc);
    WDC65816_REGS_SET_STATUS(&maincpu_regs, status);

    if (interrupt_read_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    if (interrupt_read_new_snapshot(maincpu_int_status, m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL)
        snapshot_module_close(m);
    return -1;
}
