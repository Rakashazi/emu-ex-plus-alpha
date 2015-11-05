/*
 * scpu64cpu.c - Emulation of the main 65816 processor.
 *
 * Written by
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

/* NULL */
#include <stdio.h>

#include "vice.h"

#include "interrupt.h"
#include "6510core.h"
#include "clkguard.h"
#include "alarm.h"
#include "main65816cpu.h"
#include "mem.h"
#include "scpu64mem.h"
#include "vicii.h"
#include "vicii-cycle.h"
#include "viciitypes.h"
#include "snapshot.h"
#include "reu.h"

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
#define CYCLE_EXACT_ALARM

BYTE scpu64_fastmode = 1;
static CLOCK buffer_finish, buffer_finish_half;
static CLOCK maincpu_diff, maincpu_accu;
int scpu64_emulation_mode;
alarm_context_t *maincpu_alarm_context = NULL;

/* Mask: BA low */
int maincpu_ba_low_flags = 0;
static CLOCK maincpu_ba_low_start = 0;

int scpu64_get_half_cycle(void)
{
    if (scpu64_fastmode) {
        return maincpu_accu / 1000000;
    }
    return -1;
}

void maincpu_steal_cycles(void)
{
    if (maincpu_ba_low_flags & MAINCPU_BA_LOW_VICII) {
        vicii_steal_cycles();
        maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_VICII;
    }

    if (maincpu_ba_low_flags & MAINCPU_BA_LOW_REU) {
        reu_dma_start();
        maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_REU;
    }

    while (maincpu_clk >= alarm_context_next_pending_clk(maincpu_alarm_context)) {
        alarm_context_dispatch(maincpu_alarm_context, maincpu_clk);
    }
}

inline static void check_ba(void)
{
    if (!scpu64_fastmode && maincpu_ba_low_flags) {
        maincpu_steal_cycles();
    }
}

static inline void scpu64_maincpu_inc(void)
{
    while (maincpu_clk >= alarm_context_next_pending_clk(maincpu_alarm_context)) {
        alarm_context_dispatch(maincpu_alarm_context, maincpu_clk);
    }
    maincpu_clk++;
    if (maincpu_ba_low_flags) {
        maincpu_ba_low_flags &= ~MAINCPU_BA_LOW_VICII;
        maincpu_ba_low_flags |= vicii_cycle();
        if (!maincpu_ba_low_flags) maincpu_ba_low_start = CLOCK_MAX;
    } else {
        maincpu_ba_low_flags |= vicii_cycle();
        if (maincpu_ba_low_flags) maincpu_ba_low_start = maincpu_clk + 3;
    }
}

static inline void scpu64_clock_inc(int write)
{
    if (scpu64_fastmode) {
        maincpu_accu += maincpu_diff;
        if (maincpu_accu > 20000000) {
            maincpu_accu -= 20000000;
            scpu64_maincpu_inc();
        }
    } else {
        scpu64_maincpu_inc();
    }
}

static inline void wait_buffer(void)
{
    if (buffer_finish > maincpu_clk || (buffer_finish == maincpu_clk && buffer_finish_half > maincpu_accu)) {
        maincpu_accu = buffer_finish_half;
        while (maincpu_clk < buffer_finish) {
            if (maincpu_clk >= maincpu_ba_low_start) {
                maincpu_steal_cycles();
            }
            scpu64_maincpu_inc();
        }
    }
}

void scpu64_clock_read_stretch_eprom(void)
{
    if (scpu64_fastmode) {
        maincpu_accu += maincpu_diff * 3;
        if (maincpu_accu > 20000000) {
            maincpu_accu -= 20000000;
            scpu64_maincpu_inc();
        }
    } else {
        if (maincpu_ba_low_flags) {
            maincpu_steal_cycles();
        }
    }
}

void scpu64_clock_write_stretch_eprom(void)
{
    if (scpu64_fastmode) {
        maincpu_accu += maincpu_diff * 3;
        if (maincpu_accu > 20000000) {
            maincpu_accu -= 20000000;
            scpu64_maincpu_inc();
        }
    }
}

#define SHIFT -2000000
void scpu64_clock_read_stretch_io(void)
{
    if (maincpu_ba_low_flags) {
        maincpu_steal_cycles();
    }
    if (scpu64_fastmode) {
        wait_buffer();
        if (maincpu_accu >= 0 + SHIFT + 20000000) {
            scpu64_maincpu_inc();
        }
        scpu64_maincpu_inc();
        maincpu_accu = 11500000 + SHIFT; /* measured */
    }
}

void scpu64_clock_write_stretch_io_start(void) /* before write! */
{
    if (scpu64_fastmode) {
        wait_buffer();
        if (maincpu_accu >= 0 + SHIFT + 20000000) {
            scpu64_maincpu_inc();
        }
    }
    if (maincpu_clk >= maincpu_ba_low_start) {
        maincpu_steal_cycles();
    }
}

void scpu64_clock_write_stretch_io_start_cia(void) /* before write! */
{
    if (scpu64_fastmode) {
        wait_buffer();
        if (maincpu_accu >= 0 + SHIFT + 20000000) {
            scpu64_maincpu_inc();
        }
        scpu64_maincpu_inc();
        if (maincpu_ba_low_flags) { /* yes it's not an ordinary write, can't be performed in the BA delay */
            maincpu_steal_cycles();
        }
    }
}

void scpu64_clock_write_stretch_io_cia(void) /* after write! */
{
    if (scpu64_fastmode) {
        maincpu_accu = 16800000 + SHIFT; /* measured */
        scpu64_maincpu_inc();
    }
}

void scpu64_clock_write_stretch_io(void) /* before write! */
{
    if (scpu64_fastmode) {
        maincpu_accu = 11500000 + SHIFT; /* measured */
        scpu64_maincpu_inc();
    }
}

void scpu64_clock_write_stretch_io_long(void) /* before write! */
{
    if (scpu64_fastmode) {
        maincpu_accu = 17600000 + SHIFT; /* measured */
        scpu64_maincpu_inc();
    }
}

void scpu64_clock_write_stretch(void)
{
    if (scpu64_fastmode) {
        wait_buffer();
        buffer_finish = maincpu_clk + 1;
        if (maincpu_accu >= 0 + SHIFT + 20000000) {
            buffer_finish++;
        }
        buffer_finish_half = 11500000 + SHIFT;
    }
}

void scpu64_set_fastmode(int mode)
{
    if (scpu64_fastmode != mode) {
        if (!mode) {
            scpu64_maincpu_inc();
            maincpu_accu = 17700000 + SHIFT; /* measured */
        }
        scpu64_fastmode = mode;
        maincpu_resync_limits();
    }
}

void scpu64_set_fastmode_nosync(int mode)
{
    if (scpu64_fastmode != mode) {
        scpu64_fastmode = mode;
        maincpu_resync_limits();
    }
}

/* TODO: refresh */
static DWORD simm_cell;
static DWORD simm_row_mask = ~(2048 * 4 - 1);

void scpu64_set_simm_row_size(int value)
{
    simm_row_mask = ~((1 << value) - 1);
}

void scpu64_clock_read_stretch_simm(DWORD addr)
{
    if (scpu64_fastmode) {
        if (!((simm_cell ^ addr) & ~3)) {
            return; /* same cell, no delay */
        } else if ((simm_cell ^ addr) & simm_row_mask) {
            /* different row, two and half delay */
            maincpu_accu += maincpu_diff * 2 + (maincpu_diff >> 1);
        } else if (!(((simm_cell + 4) ^ addr) & ~3)) {
            simm_cell = addr;
            return; /* next cell, no delay */
        } else {
            maincpu_accu += maincpu_diff;/* same row, one delay */
        }
        simm_cell = addr;
        if (maincpu_accu > 20000000) {
            maincpu_accu -= 20000000;
            scpu64_maincpu_inc();
        }
    } else {
        if (maincpu_ba_low_flags) {
            maincpu_steal_cycles();
        }
    }
}

void scpu64_clock_write_stretch_simm(DWORD addr)
{
    if (scpu64_fastmode) {
        if ((simm_cell ^ addr) & simm_row_mask) {
            maincpu_accu += maincpu_diff * 2;/* different row, two delay */
        } else {
            maincpu_accu += maincpu_diff;/* same row, one delay */
        }
        simm_cell = addr;
        if (maincpu_accu > 20000000) {
            maincpu_accu -= 20000000;
            scpu64_maincpu_inc();
        }
    } 
}

void scpu64_clock_read_ioram(void) /* scpu64 v1 $d200-$d3ff one extra 20 Mhz cycle */
{
    if (scpu64_fastmode) {
        maincpu_accu += maincpu_diff; /* one delay */
    }
}

static void clk_overflow_callback(CLOCK sub, void *unused_data)
{
    if (buffer_finish > sub) {
        buffer_finish -= sub;
    } else {
        buffer_finish = 0;
    }
}

#define CPU_ADDITIONAL_INIT() clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL)

/* SCPU64 needs external reg_pc */
#define NEED_REG_PC

#define STORE(addr, value) \
    do { \
        DWORD tmpx1 = (addr); \
        BYTE tmpx2 = (value); \
        if (tmpx1 & ~0xffff) { \
            mem_store2(tmpx1, tmpx2); \
        } else { \
            (*_mem_write_tab_ptr[tmpx1 >> 8])((WORD)tmpx1, tmpx2); \
        } \
    } while (0)

#define LOAD(addr) \
    (((addr) & ~0xffff)?mem_read2(addr):(*_mem_read_tab_ptr[(addr) >> 8])((WORD)(addr)))

#define STORE_LONG(addr, value) store_long((DWORD)(addr), (BYTE)(value))

static inline void store_long(DWORD addr, BYTE value)
{
    if (addr & ~0xffff) {
        mem_store2(addr, value);
    } else {
        (*_mem_write_tab_ptr[addr >> 8])((WORD)addr, value);
    }
    scpu64_clock_inc(1);
}

#define LOAD_LONG(addr) load_long(addr)

static inline BYTE load_long(DWORD addr)
{
    BYTE tmp;

    if ((addr) & ~0xffff) {
        tmp = mem_read2(addr);
    } else {
        tmp = (*_mem_read_tab_ptr[(addr) >> 8])((WORD)addr);
    }
    scpu64_clock_inc(0);
    return tmp;
}

int scpu64_snapshot_write_cpu_state(snapshot_module_t *m)
{
    return SMW_B(m, scpu64_fastmode) < 0
        || SMW_DW(m, buffer_finish) < 0
        || SMW_DW(m, buffer_finish_half) < 0
        || SMW_DW(m, maincpu_accu) < 0
        || SMW_DW(m, maincpu_ba_low_flags) < 0
        || SMW_DW(m, maincpu_ba_low_start) < 0;
}

/* XXX: Assumes `CLOCK' is the same size as a `DWORD'.  */
int scpu64_snapshot_read_cpu_state(snapshot_module_t *m)
{
    return SMR_B(m, &scpu64_fastmode) < 0
        || SMR_DW(m, &buffer_finish) < 0
        || SMR_DW(m, &buffer_finish_half) < 0
        || SMR_DW(m, &maincpu_accu) < 0
        || SMR_DW_INT(m, &maincpu_ba_low_flags) < 0
        || SMR_DW(m, &maincpu_ba_low_start) < 0;
}

#define EMULATION_MODE_CHANGED scpu64_emulation_mode = reg_emul

#define CLK_INC(clock) scpu64_clock_inc(0)

#define CPU_ADDITIONAL_RESET() (buffer_finish = maincpu_clk, buffer_finish_half = 0, maincpu_accu = 0, maincpu_diff = machine_get_cycles_per_second())

#define FETCH_PARAM(addr) ((((int)(addr)) < bank_limit) ? (check_ba(), scpu64_clock_inc(0), bank_base[addr]) : LOAD_PBR(addr))
#define FETCH_PARAM_DUMMY(addr) scpu64_clock_inc(0)
#define LOAD_LONG_DUMMY(addr) scpu64_clock_inc(0)

#define LOAD_INT_ADDR(addr)                        \
    if (scpu64_interrupt_reroute()) {              \
        reg_pc = LOAD_LONG(addr + 0xf80000);       \
        reg_pc |= LOAD_LONG(addr + 0xf80001) << 8; \
    } else {                                       \
        reg_pc = LOAD_LONG(addr);                  \
        reg_pc |= LOAD_LONG(addr + 1) << 8;        \
    }
/* ------------------------------------------------------------------------- */

#include "../main65816cpu.c"
