/*
 * c64dtvcpu.c - Emulation of the C64DTV processor.
 *
 * Written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#include "alarm.h"
#include "c64mem.h"
#include "c64dtvblitter.h"
#include "c64dtvcpu.h"
#include "c64dtvdma.h"
#include "monitor.h"
#include <string.h>

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

#ifndef C64DTV
#define C64DTV
#endif

/* TODO this is a hack */
#undef C64_RAM_SIZE
#define C64_RAM_SIZE 0x200000

BYTE dtv_registers[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
BYTE dtvrewind;

int dtvclockneg = 0;

/* Experimental cycle exact alarm handling */
/* #define CYCLE_EXACT_ALARM */

#ifdef CYCLE_EXACT_ALARM
alarm_context_t *maincpu_alarm_context = NULL;
#endif

#define REWIND_FETCH_OPCODE(clock) clock -= dtvrewind; dtvclockneg += dtvrewind

/* Burst mode implementation */

BYTE burst_status, burst_diff, burst_idx, burst_fetch, burst_broken;
BYTE burst_cache[] = {0, 0, 0, 0};
WORD burst_addr, burst_last_addr;


inline static void c64dtvcpu_clock_add(CLOCK *clock, int amount)
{
    if (burst_diff && (amount > 0)) {
        if (burst_diff >= amount) {
            burst_diff -= amount;
            return;
        }
        amount -= burst_diff;
        burst_diff = 0;
    }

    if (amount >= 0) {
        while (amount) {
#ifdef CYCLE_EXACT_ALARM
            while ((*clock) >= alarm_context_next_pending_clk(maincpu_alarm_context)) {
                alarm_context_dispatch(maincpu_alarm_context, (*clock));
            }
#endif
            (*clock)++;
            --amount;
            if (dtvclockneg == 0) {
                if (blitter_active) {
                    c64dtvblitter_perform_blitter();
                } else if (dma_active) {
                    c64dtvdma_perform_dma();
                }
            } else {
                --dtvclockneg;
            }
        }
    } else {
        dtvclockneg -= amount;
        *clock += amount;
    }
}

#define CLK_ADD(clock, amount) c64dtvcpu_clock_add(&clock, amount)

/* This is an optimization making x64dtv consume less host cycles in burst mode. */
inline static void mem_burst_read(const WORD addr, BYTE *burst_cache)
{
    read_func_ptr_t mrtf;
    int paddr = ((((int) dtv_registers[12 + (addr >> 14)]) << 14) + (addr & 0x3fff)) & (C64_RAM_SIZE - 1);

    if (paddr <= 0xffff) {
        mrtf = _mem_read_tab_ptr[paddr >> 8];
        if (mrtf != ram_read) {
            burst_cache[0] = mrtf((WORD)(paddr + 0));
            burst_cache[1] = mrtf((WORD)(paddr + 1));
            burst_cache[2] = mrtf((WORD)(paddr + 2));
            burst_cache[3] = mrtf((WORD)(paddr + 3));
            return;
        }
    }
    /* this memcpy is optimized to a simple dword copy */
    memcpy(burst_cache, &mem_ram[paddr], 4);
}

/* Burst mode & skip cycle helper table */
/* format: SBDDDFFF */
/*  FFF = "fetch", opcode length-1 */
/*  DDD = "diff", opcode execution time difference */
/*    B = "break", breaks burst execution (memory access, jump, ...) */
/*    S = "skip", skip loading operand (for implied<>immediate in skip cycle) */

static const BYTE burst_status_tab[] = {
            /* 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F */
    /* $00 */  0110, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0110, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $00 */
    /* $10 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $10 */
    /* $20 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0110, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $20 */
    /* $30 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $30 */
    /* $40 */  0110, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0110, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $40 */
    /* $50 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $50 */
    /* $60 */  0110, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0110, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $60 */
    /* $70 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $70 */
    /* $80 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $80 */
    /* $90 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $90 */
    /* $A0 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $A0 */
    /* $B0 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $B0 */
    /* $C0 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $C0 */
    /* $D0 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132, /* $D0 */
    /* $E0 */  0021, 0121, 0021, 0121, 0121, 0121, 0121, 0121, 0210, 0021, 0210, 0021, 0132, 0132, 0132, 0132, /* $E0 */
    /* $F0 */  0021, 0121, 0100, 0121, 0121, 0121, 0121, 0121, 0210, 0132, 0210, 0132, 0132, 0132, 0132, 0132  /* $F0 */
};


/* Skip cycle implementation */

#define CLK_RTS (3 - (dtv_registers[9] & 1))
#define CLK_RTI (4 - 2 * (dtv_registers[9] & 1))
#define CLK_BRK (5 - (dtv_registers[9] & 1))
#define CLK_ABS_I_STORE2 (2 - (dtv_registers[9] & 1))
#define CLK_STACK_PUSH (1 - (dtv_registers[9] & 1))
#define CLK_STACK_PULL (2 - 2 * (dtv_registers[9] & 1))
#define CLK_ABS_RMW2 (3 - (dtv_registers[9] & 1))
#define CLK_ABS_I_RMW2 (3 - (dtv_registers[9] & 1))
#define CLK_ZERO_I_STORE (2 - (dtv_registers[9] & 1))
#define CLK_ZERO_I2 (2 - (dtv_registers[9] & 1))
#define CLK_ZERO_RMW (3 - (dtv_registers[9] & 1))
#define CLK_ZERO_I_RMW (4 - 2 * (dtv_registers[9] & 1))
#define CLK_IND_X_RMW (3 - (dtv_registers[9] & 1))
#define CLK_IND_Y_RMW1 (1 - (dtv_registers[9] & 1))
#define CLK_IND_Y_RMW2 (3 - (dtv_registers[9] & 1))
#define CLK_BRANCH2 (1 - (dtv_registers[9] & 1))
#define CLK_INT_CYCLE (1 - (dtv_registers[9] & 1))
#define CLK_JSR_INT_CYCLE (1 - (dtv_registers[9] & 1) + ((dtv_registers[9] & 2) && (reg_pc == 0)) ? 1 : 0)
#define CLK_IND_Y_W (2 - (dtv_registers[9] & 1))
#define CLK_NOOP_ZERO_X (2 - (dtv_registers[9] & 1))

#define IRQ_CYCLES (7 - 2 * (dtv_registers[9] & 1))
#define NMI_CYCLES (7 - 2 * (dtv_registers[9] & 1))


/* New opcodes */

#define SAC(op)                     \
    do {                            \
        reg_a_write_idx = op >> 4;  \
        reg_a_read_idx = op & 0x0f; \
        INC_PC(2);                  \
    } while (0)

#define SIR(op)                \
    do {                       \
        reg_y_idx = op >> 4;   \
        reg_x_idx = op & 0x0f; \
        INC_PC(2);             \
    } while (0)

#define NOOP_ABS_Y()     \
    do {                 \
        LOAD_ABS_Y(p2);  \
        CLK_ADD(CLK, 1); \
        INC_PC(3);       \
    } while (0)

#define BRANCH(cond, value)                                       \
    do {                                                          \
        INC_PC(2);                                                \
                                                                  \
        if (cond) {                                               \
            unsigned int dest_addr;                               \
                                                                  \
            burst_broken = 1;                                     \
            dest_addr = reg_pc + (signed char)(value);            \
                                                                  \
            if (CLK_BRANCH2) {                                    \
                FETCH_PARAM(reg_pc);                              \
                CLK_ADD(CLK, 1);                                  \
                if ((reg_pc ^ dest_addr) & 0xff00) {              \
                    LOAD((reg_pc & 0xff00) | (dest_addr & 0xff)); \
                    CLK_ADD(CLK, 1);                              \
                } else {                                          \
                    OPCODE_DELAYS_INTERRUPT();                    \
                }                                                 \
            }                                                     \
            JUMP(dest_addr & 0xffff);                             \
        }                                                         \
    } while (0)

/* Override optimizations in maincpu.c that directly access mem_ram[] */
/* We need to channel everything through mem_read/mem_store to */
/* let the DTV segment mapper (register 12-15) do its work */

#define STORE(addr, value) \
    mem_store((WORD)(addr), (BYTE)(value))

#define LOAD(addr) \
    mem_read((WORD)(addr))

/* Route zero page operations through register 10 (zeropage mapper) */

#define STORE_ZERO(addr, value) \
    mem_store((WORD)((((WORD) dtv_registers[10]) << 8) + ((WORD)(addr) & 0xff)), (BYTE)(value))

#define LOAD_ZERO(addr) \
    mem_read((WORD)((((WORD) dtv_registers[10]) << 8) + ((WORD)((addr) & 0xff))))

/* Route stack operations through register 11 (stack mapper) */

#define PUSH(val) (STORE((((WORD) dtv_registers[11]) << 8) + ((reg_sp--) & 0xff), val))

#define PULL()    (LOAD((((WORD) dtv_registers[11]) << 8) + ((++reg_sp) & 0xff)))


/* opcode_t etc */

#if defined ALLOW_UNALIGNED_ACCESS

#define opcode_t DWORD

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 (opcode >> 8)

#else /* !ALLOW_UNALIGNED_ACCESS */

#define opcode_t         \
    struct {             \
        BYTE ins;        \
        union {          \
            BYTE op8[2]; \
            WORD op16;   \
        } op;            \
    }

#define p0 (opcode.ins)
#define p2 (opcode.op.op16)

#ifdef WORDS_BIGENDIAN
#  define p1 (opcode.op.op8[1])
#else
#  define p1 (opcode.op.op8[0])
#endif

#endif /* !ALLOW_UNALIGNED_ACCESS */

/*  SET_OPCODE for traps */
#if defined ALLOW_UNALIGNED_ACCESS
#define SET_OPCODE(o) (opcode) = o;
#else
#if !defined WORDS_BIGENDIAN
#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[0] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[1] = ((o) >> 16) & 0xff; \
    } while (0)
#else
#define SET_OPCODE(o)                          \
    do {                                       \
        opcode.ins = (o) & 0xff;               \
        opcode.op.op8[1] = ((o) >> 8) & 0xff;  \
        opcode.op.op8[0] = ((o) >> 16) & 0xff; \
    } while (0)
#endif
#endif


/* FETCH_OPCODE_DTV implementation(s) */

#if defined ALLOW_UNALIGNED_ACCESS
#define FETCH_OPCODE(o)                                                                                \
    do {                                                                                               \
        dtvrewind = 0;                                                                                 \
        if ((dtv_registers[9] & 2) && (((dtv_registers[8] >> ((reg_pc >> 13) & 6)) & 0x03) == 0x01)) { \
            burst_last_addr = burst_addr;                                                              \
            burst_addr = reg_pc & 0xfffc;                                                              \
            if ((burst_addr != burst_last_addr) || burst_broken) {                                     \
                if (burst_addr < bank_limit) {                                                         \
                    memcpy(burst_cache, bank_base + burst_addr, 4);                                    \
                } else {                                                                               \
                    mem_burst_read(burst_addr, burst_cache);                                           \
                }                                                                                      \
            }                                                                                          \
            burst_idx = reg_pc & 3;                                                                    \
            o = burst_cache[burst_idx++];                                                              \
            burst_status = burst_status_tab[o];                                                        \
            burst_fetch = burst_status & 7;                                                            \
            burst_diff = (burst_status >> 3) & 7;                                                      \
            if (burst_fetch--) {                                                                       \
                if (burst_idx > 3) {                                                                   \
                    burst_addr += 4;                                                                   \
                    burst_addr &= 0xfffc;                                                              \
                    burst_last_addr = burst_addr;                                                      \
                    burst_diff--;                                                                      \
                    burst_idx = 0;                                                                     \
                    if (burst_addr < bank_limit && burst_addr) {                                       \
                        memcpy(burst_cache, bank_base + burst_addr, 4);                                \
                    } else {                                                                           \
                        mem_burst_read(burst_addr, burst_cache);                                       \
                    }                                                                                  \
                }                                                                                      \
                o |= (burst_cache[burst_idx++] << 8);                                                  \
                if (burst_fetch--) {                                                                   \
                    if (burst_idx > 3) {                                                               \
                        burst_addr += 4;                                                               \
                        burst_addr &= 0xfffc;                                                          \
                        burst_last_addr = burst_addr;                                                  \
                        burst_diff--;                                                                  \
                        burst_idx = 0;                                                                 \
                        if (burst_addr < bank_limit && burst_addr) {                                   \
                            memcpy(burst_cache, bank_base + burst_addr, 4);                            \
                        } else {                                                                       \
                            mem_burst_read(burst_addr, burst_cache);                                   \
                        }                                                                              \
                    }                                                                                  \
                    o |= (burst_cache[burst_idx] << 16);                                               \
                }                                                                                      \
            }                                                                                          \
            if ((burst_last_addr != burst_addr) || burst_broken) {                                     \
                burst_diff--;                                                                          \
                dtvrewind++;                                                                           \
            }                                                                                          \
            burst_broken = (burst_status >> 6) & 1;                                                    \
            CLK_ADD(CLK, 1);                                                                           \
            if (!(((burst_status_tab[o & 0xff] & 0x80)) && (dtv_registers[9] & 1))) {                  \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
            }                                                                                          \
            if (fetch_tab[o & 0xff]) {                                                                 \
                CLK_ADD(CLK, 1);                                                                       \
            }                                                                                          \
        } else {                                                                                       \
            burst_broken = 1;                                                                          \
            burst_diff = 0;                                                                            \
            if (((int)reg_pc) < bank_limit) {                                                          \
                o = (*((DWORD *)(bank_base + reg_pc)) & 0xffffff);                                     \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
                if (!(((burst_status_tab[o & 0xff] & 0x80)) && (dtv_registers[9] & 1))) {              \
                    CLK_ADD(CLK, 1);                                                                   \
                    dtvrewind++;                                                                       \
                }                                                                                      \
                if (fetch_tab[o & 0xff]) {                                                             \
                    CLK_ADD(CLK, 1);                                                                   \
                }                                                                                      \
            } else {                                                                                   \
                o = LOAD(reg_pc);                                                                      \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
                o |= LOAD(reg_pc + 1) << 8;                                                            \
                if (!(((burst_status_tab[o & 0xff] & 0x80)) && (dtv_registers[9] & 1))) {              \
                    CLK_ADD(CLK, 1);                                                                   \
                    dtvrewind++;                                                                       \
                }                                                                                      \
                if (fetch_tab[o & 0xff]) {                                                             \
                    o |= (LOAD(reg_pc + 2) << 16);                                                     \
                    CLK_ADD(CLK, 1);                                                                   \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
    } while (0)

#else /* !ALLOW_UNALIGNED_ACCESS */
/* TODO optimize */
#define FETCH_OPCODE(o)                                                                                \
    do {                                                                                               \
        dtvrewind = 0;                                                                                 \
        if ((dtv_registers[9] & 2) && (((dtv_registers[8] >> ((reg_pc >> 13) & 6)) & 0x03) == 0x01)) { \
            burst_last_addr = burst_addr;                                                              \
            burst_addr = reg_pc & 0xfffc;                                                              \
            if ((burst_addr != burst_last_addr) || burst_broken) {                                     \
                if (burst_addr < bank_limit) {                                                         \
                    memcpy(burst_cache, bank_base + burst_addr, 4);                                    \
                } else {                                                                               \
                    burst_cache[0] = LOAD(burst_addr + 0);                                             \
                    burst_cache[1] = LOAD(burst_addr + 1);                                             \
                    burst_cache[2] = LOAD(burst_addr + 2);                                             \
                    burst_cache[3] = LOAD(burst_addr + 3);                                             \
                }                                                                                      \
            }                                                                                          \
            burst_idx = reg_pc & 3;                                                                    \
            (o).ins = burst_cache[burst_idx++];                                                        \
            burst_status = burst_status_tab[(o).ins];                                                  \
            burst_fetch = burst_status & 7;                                                            \
            burst_diff = (burst_status >> 3) & 7;                                                      \
            if (burst_fetch--) {                                                                       \
                if (burst_idx > 3) {                                                                   \
                    burst_addr += 4;                                                                   \
                    burst_addr &= 0xfffc;                                                              \
                    burst_last_addr = burst_addr;                                                      \
                    burst_diff--;                                                                      \
                    burst_idx = 0;                                                                     \
                    if (burst_addr < bank_limit && burst_addr) {                                       \
                        memcpy(burst_cache, bank_base + burst_addr, 4);                                \
                    } else {                                                                           \
                        burst_cache[0] = LOAD(burst_addr + 0);                                         \
                        burst_cache[1] = LOAD(burst_addr + 1);                                         \
                        burst_cache[2] = LOAD(burst_addr + 2);                                         \
                        burst_cache[3] = LOAD(burst_addr + 3);                                         \
                    }                                                                                  \
                }                                                                                      \
                (o).op.op16 = burst_cache[burst_idx++];                                                \
                if (burst_fetch--) {                                                                   \
                    if (burst_idx > 3) {                                                               \
                        burst_addr += 4;                                                               \
                        burst_addr &= 0xfffc;                                                          \
                        burst_last_addr = burst_addr;                                                  \
                        burst_diff--;                                                                  \
                        burst_idx = 0;                                                                 \
                        if (burst_addr < bank_limit && burst_addr) {                                   \
                            memcpy(burst_cache, bank_base + burst_addr, 4);                            \
                        } else {                                                                       \
                            burst_cache[0] = LOAD(burst_addr + 0);                                     \
                            burst_cache[1] = LOAD(burst_addr + 1);                                     \
                            burst_cache[2] = LOAD(burst_addr + 2);                                     \
                            burst_cache[3] = LOAD(burst_addr + 3);                                     \
                        }                                                                              \
                    }                                                                                  \
                    (o).op.op16 |= (burst_cache[burst_idx] << 8);                                      \
                }                                                                                      \
            }                                                                                          \
            if ((burst_last_addr != burst_addr) || burst_broken) {                                     \
                burst_diff--;                                                                          \
                dtvrewind++;                                                                           \
            }                                                                                          \
            burst_broken = (burst_status >> 6) & 1;                                                    \
            CLK_ADD(CLK, 1);                                                                           \
            if (!(((burst_status_tab[(o).ins] & 0x80)) && (dtv_registers[9] & 1))) {                   \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
            }                                                                                          \
            if (fetch_tab[(o).ins]) {                                                                  \
                CLK_ADD(CLK, 1);                                                                       \
            }                                                                                          \
        } else {                                                                                       \
            burst_broken = 1;                                                                          \
            burst_diff = 0;                                                                            \
            if (((int)reg_pc) < bank_limit) {                                                          \
                (o).ins = *(bank_base + reg_pc);                                                       \
                (o).op.op16 = (*(bank_base + reg_pc + 1) | (*(bank_base + reg_pc + 2) << 8));          \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
                if (!(((burst_status_tab[(o).ins] & 0x80)) && (dtv_registers[9] & 1))) {               \
                    CLK_ADD(CLK, 1);                                                                   \
                    dtvrewind++;                                                                       \
                }                                                                                      \
                if (fetch_tab[(o).ins]) {                                                              \
                    CLK_ADD(CLK, 1);                                                                   \
                }                                                                                      \
            } else {                                                                                   \
                (o).ins = LOAD(reg_pc);                                                                \
                CLK_ADD(CLK, 1);                                                                       \
                dtvrewind++;                                                                           \
                (o).op.op16 = LOAD(reg_pc + 1);                                                        \
                if (!(((burst_status_tab[(o).ins] & 0x80)) && (dtv_registers[9] & 1))) {               \
                    CLK_ADD(CLK, 1);                                                                   \
                    dtvrewind++;                                                                       \
                }                                                                                      \
                if (fetch_tab[(o).ins]) {                                                              \
                    (o).op.op16 |= (LOAD(reg_pc + 2) << 8);                                            \
                    CLK_ADD(CLK, 1);                                                                   \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
    } while (0)

#endif /* !ALLOW_UNALIGNED_ACCESS */

#ifdef FEATURE_CPUMEMHISTORY
#warning "CPUMEMHISTORY implementation for x64dtv is incomplete"
#if 0
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
#endif

#include "../maincpu.c"
