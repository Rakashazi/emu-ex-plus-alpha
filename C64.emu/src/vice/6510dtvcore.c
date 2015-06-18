/*
 * 6510dtvcore.c - Cycle based 6510 emulation core.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * DTV sections written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Cycle based rewrite by
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

/* This file is included by (some) CPU definition files */
/* (mainc64cpu.c, mainviccpu.c) */

#define CPU_STR "Main CPU"

#include "traps.h"

#ifndef C64DTV
/* The C64DTV can use different shadow registers for accu read/write. */
/* For standard 6510, this is not the case. */
#define reg_a_write reg_a
#define reg_a_read  reg_a
#endif

/* ------------------------------------------------------------------------- */

#define LOCAL_SET_NZ(val)        (flag_z = flag_n = (val))

#define LOCAL_SET_OVERFLOW(val)   \
    do {                          \
        if (val) {                \
            reg_p |= P_OVERFLOW;  \
        } else {                  \
            reg_p &= ~P_OVERFLOW; \
        }                         \
    } while (0)

#define LOCAL_SET_BREAK(val)   \
    do {                       \
        if (val) {             \
            reg_p |= P_BREAK;  \
        } else {               \
            reg_p &= ~P_BREAK; \
        }                      \
    } while (0)

#define LOCAL_SET_DECIMAL(val)   \
    do {                         \
        if (val) {               \
            reg_p |= P_DECIMAL;  \
        } else {                 \
            reg_p &= ~P_DECIMAL; \
        }                        \
    } while (0)

#define LOCAL_SET_INTERRUPT(val)   \
    do {                           \
        if (val) {                 \
            reg_p |= P_INTERRUPT;  \
        } else {                   \
            reg_p &= ~P_INTERRUPT; \
        }                          \
    } while (0)

#define LOCAL_SET_CARRY(val)   \
    do {                       \
        if (val) {             \
            reg_p |= P_CARRY;  \
        } else {               \
            reg_p &= ~P_CARRY; \
        }                      \
    } while (0)

#define LOCAL_SET_SIGN(val)      (flag_n = (val) ? 0x80 : 0)
#define LOCAL_SET_ZERO(val)      (flag_z = !(val))
#define LOCAL_SET_STATUS(val)    (reg_p = ((val) & ~(P_ZERO | P_SIGN)), \
                                  LOCAL_SET_ZERO((val) & P_ZERO),       \
                                  flag_n = (val))

#define LOCAL_OVERFLOW()         (reg_p & P_OVERFLOW)
#define LOCAL_BREAK()            (reg_p & P_BREAK)
#define LOCAL_DECIMAL()          (reg_p & P_DECIMAL)
#define LOCAL_INTERRUPT()        (reg_p & P_INTERRUPT)
#define LOCAL_CARRY()            (reg_p & P_CARRY)
#define LOCAL_SIGN()             (flag_n & 0x80)
#define LOCAL_ZERO()             (!flag_z)
#define LOCAL_STATUS()           (reg_p | (flag_n & 0x80) | P_UNUSED    \
                                  | (LOCAL_ZERO() ? P_ZERO : 0))

#ifdef LAST_OPCODE_INFO

/* If requested, gather some info about the last executed opcode for timing
   purposes.  */

/* Remember the number of the last opcode.  By default, the opcode does not
   delay interrupt and does not change the I flag.  */
#define SET_LAST_OPCODE(x) OPINFO_SET(LAST_OPCODE_INFO, (x), 0, 0, 0)

/* Remember that the last opcode delayed a pending IRQ or NMI by one cycle.  */
#define OPCODE_DELAYS_INTERRUPT() OPINFO_SET_DELAYS_INTERRUPT(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 0 to 1, so we have
   to dispatch an IRQ even if the I flag is 0 when we check it.  */
#define OPCODE_DISABLES_IRQ() OPINFO_SET_DISABLES_IRQ(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 1 to 0, so we must
   not dispatch an IRQ even if the I flag is 1 when we check it.  */
#define OPCODE_ENABLES_IRQ() OPINFO_SET_ENABLES_IRQ(LAST_OPCODE_INFO, 1)

#else

/* Info about the last opcode is not needed.  */
#define SET_LAST_OPCODE(x)
#define OPCODE_DELAYS_INTERRUPT()
#define OPCODE_DISABLES_IRQ()
#define OPCODE_ENABLES_IRQ()

#endif

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

#ifndef C64DTV
/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()          \
    do {                            \
        GLOBAL_REGS.pc = reg_pc ;   \
        GLOBAL_REGS.a = reg_a_read; \
        GLOBAL_REGS.x = reg_x;      \
        GLOBAL_REGS.y = reg_y;      \
        GLOBAL_REGS.sp = reg_sp;    \
        GLOBAL_REGS.p = reg_p;      \
        GLOBAL_REGS.n = flag_n;     \
        GLOBAL_REGS.z = flag_z;     \
    } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                                 \
    do {                                                   \
        reg_a_write = GLOBAL_REGS.a;                       \
        reg_x = GLOBAL_REGS.x;                             \
        reg_y = GLOBAL_REGS.y;                             \
        reg_sp = GLOBAL_REGS.sp;                           \
        reg_p = GLOBAL_REGS.p;                             \
        flag_n = GLOBAL_REGS.n;                            \
        flag_z = GLOBAL_REGS.z;                            \
        bank_start = bank_limit = 0; /* prevent caching */ \
        JUMP(GLOBAL_REGS.pc);                              \
    } while (0)

#else  /* C64DTV */

/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()                                           \
    do {                                                             \
        GLOBAL_REGS.pc = reg_pc;                                     \
        GLOBAL_REGS.a = dtv_registers[0];                            \
        GLOBAL_REGS.x = dtv_registers[2];                            \
        GLOBAL_REGS.y = dtv_registers[1];                            \
        GLOBAL_REGS.sp = reg_sp;                                     \
        GLOBAL_REGS.p = reg_p;                                       \
        GLOBAL_REGS.n = flag_n;                                      \
        GLOBAL_REGS.z = flag_z;                                      \
        GLOBAL_REGS.r3 = dtv_registers[3];                           \
        GLOBAL_REGS.r4 = dtv_registers[4];                           \
        GLOBAL_REGS.r5 = dtv_registers[5];                           \
        GLOBAL_REGS.r6 = dtv_registers[6];                           \
        GLOBAL_REGS.r7 = dtv_registers[7];                           \
        GLOBAL_REGS.r8 = dtv_registers[8];                           \
        GLOBAL_REGS.r9 = dtv_registers[9];                           \
        GLOBAL_REGS.r10 = dtv_registers[10];                         \
        GLOBAL_REGS.r11 = dtv_registers[11];                         \
        GLOBAL_REGS.r12 = dtv_registers[12];                         \
        GLOBAL_REGS.r13 = dtv_registers[13];                         \
        GLOBAL_REGS.r14 = dtv_registers[14];                         \
        GLOBAL_REGS.r15 = dtv_registers[15];                         \
        GLOBAL_REGS.acm = (reg_a_write_idx << 4) | (reg_a_read_idx); \
        GLOBAL_REGS.yxm = (reg_y_idx << 4) | (reg_x_idx);            \
    } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                                 \
    do {                                                   \
        dtv_registers[0] = GLOBAL_REGS.a;                  \
        dtv_registers[2] = GLOBAL_REGS.x;                  \
        dtv_registers[1] = GLOBAL_REGS.y;                  \
        reg_sp = GLOBAL_REGS.sp;                           \
        reg_p = GLOBAL_REGS.p;                             \
        flag_n = GLOBAL_REGS.n;                            \
        flag_z = GLOBAL_REGS.z;                            \
        dtv_registers[3] = GLOBAL_REGS.r3;                 \
        dtv_registers[4] = GLOBAL_REGS.r4;                 \
        dtv_registers[5] = GLOBAL_REGS.r5;                 \
        dtv_registers[6] = GLOBAL_REGS.r6;                 \
        dtv_registers[7] = GLOBAL_REGS.r7;                 \
        dtv_registers[8] = GLOBAL_REGS.r8;                 \
        dtv_registers[9] = GLOBAL_REGS.r9;                 \
        dtv_registers[10] = GLOBAL_REGS.r10;               \
        dtv_registers[11] = GLOBAL_REGS.r11;               \
        dtv_registers[12] = GLOBAL_REGS.r12;               \
        dtv_registers[13] = GLOBAL_REGS.r13;               \
        dtv_registers[14] = GLOBAL_REGS.r14;               \
        dtv_registers[15] = GLOBAL_REGS.r15;               \
        reg_a_write_idx = GLOBAL_REGS.acm >> 4;            \
        reg_a_read_idx = GLOBAL_REGS.acm & 0xf;            \
        reg_y_idx = GLOBAL_REGS.yxm >> 4;                  \
        reg_x_idx = GLOBAL_REGS.yxm & 0xf;                 \
        bank_start = bank_limit = 0; /* prevent caching */ \
        JUMP(GLOBAL_REGS.pc);                              \
    } while (0)

#endif /* C64DTV */

#ifdef DEBUG
#define TRACE_NMI()                         \
    do {                                    \
        if (TRACEFLG) {                     \
            debug_nmi(CPU_INT_STATUS, CLK); \
        }                                   \
    } while (0)

#define TRACE_IRQ()                         \
    do {                                    \
        if (TRACEFLG) {                     \
            debug_irq(CPU_INT_STATUS, CLK); \
        }                                   \
    } while (0)

#define TRACE_BRK()                \
    do {                           \
        if (TRACEFLG) {            \
            debug_text("*** BRK"); \
        }                          \
    } while (0)
#else
#define TRACE_NMI()
#define TRACE_IRQ()
#define TRACE_BRK()
#endif

/* Do the IRQ/BRK sequence, including NMI transformation. */
#define DO_IRQBRK()                                                                                                   \
    do {                                                                                                              \
        /* Interrupt vector to use. Assume regular IRQ/BRK. */                                                        \
        WORD handler_vector = 0xfffe;                                                                                 \
                                                                                                                      \
        PUSH(reg_pc >> 8);                                                                                            \
        CLK_INC();                                                                                                    \
        PUSH(reg_pc & 0xff);                                                                                          \
        CLK_INC();                                                                                                    \
        PUSH(LOCAL_STATUS());                                                                                         \
        CLK_INC();                                                                                                    \
                                                                                                                      \
        /* Process alarms up to this point to get nmi_clk updated. */                                                 \
        while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {                                                \
            alarm_context_dispatch(ALARM_CONTEXT, CLK);                                                               \
        }                                                                                                             \
                                                                                                                      \
        /* If an NMI would occur at this cycle... */                                                                  \
        if ((CPU_INT_STATUS->global_pending_int & IK_NMI) && (CLK >= (CPU_INT_STATUS->nmi_clk + INTERRUPT_DELAY))) {  \
            /* Transform the IRQ/BRK into an NMI. */                                                                  \
            handler_vector = 0xfffa;                                                                                  \
            TRACE_NMI();                                                                                              \
            if (monitor_mask[CALLER] & (MI_STEP)) {                                                                   \
                monitor_check_icount_interrupt();                                                                     \
            }                                                                                                         \
            interrupt_ack_nmi(CPU_INT_STATUS);                                                                        \
        }                                                                                                             \
                                                                                                                      \
        LOCAL_SET_INTERRUPT(1);                                                                                       \
        addr = LOAD(handler_vector);                                                                                  \
        CLK_INC();                                                                                                    \
        addr |= (LOAD(handler_vector + 1) << 8);                                                                      \
        CLK_INC();                                                                                                    \
        JUMP(addr);                                                                                                   \
    } while (0)

/* Perform the interrupts in `int_kind'.  If we have both NMI and IRQ,
   execute NMI.  */
/* FIXME: LOCAL_STATUS() should check byte ready first.  */
#define DO_INTERRUPT(int_kind)                                                 \
    do {                                                                       \
        BYTE ik = (int_kind);                                                  \
        WORD addr;                                                             \
                                                                               \
        if (ik & (IK_IRQ | IK_IRQPEND | IK_NMI)) {                             \
            if ((ik & IK_NMI)                                                  \
                && interrupt_check_nmi_delay(CPU_INT_STATUS, CLK)) {           \
                TRACE_NMI();                                                   \
                if (monitor_mask[CALLER] & (MI_STEP)) {                        \
                    monitor_check_icount_interrupt();                          \
                }                                                              \
                interrupt_ack_nmi(CPU_INT_STATUS);                             \
                if (!SKIP_CYCLE) {                                             \
                    LOAD(reg_pc);                                              \
                    CLK_INC();                                                 \
                    LOAD(reg_pc);                                              \
                    CLK_INC();                                                 \
                }                                                              \
                LOCAL_SET_BREAK(0);                                            \
                PUSH(reg_pc >> 8);                                             \
                CLK_INC();                                                     \
                PUSH(reg_pc & 0xff);                                           \
                CLK_INC();                                                     \
                PUSH(LOCAL_STATUS());                                          \
                CLK_INC();                                                     \
                addr = LOAD(0xfffa);                                           \
                CLK_INC();                                                     \
                addr |= (LOAD(0xfffb) << 8);                                   \
                CLK_INC();                                                     \
                LOCAL_SET_INTERRUPT(1);                                        \
                JUMP(addr);                                                    \
                SET_LAST_OPCODE(0);                                            \
            } else if ((ik & (IK_IRQ | IK_IRQPEND))                            \
                     && (!LOCAL_INTERRUPT()                                    \
                         || OPINFO_DISABLES_IRQ(LAST_OPCODE_INFO))             \
                     && interrupt_check_irq_delay(CPU_INT_STATUS, CLK)) {      \
                TRACE_IRQ();                                                   \
                if (monitor_mask[CALLER] & (MI_STEP)) {                        \
                    monitor_check_icount_interrupt();                          \
                }                                                              \
                interrupt_ack_irq(CPU_INT_STATUS);                             \
                if (!SKIP_CYCLE) {                                             \
                    LOAD(reg_pc);                                              \
                    CLK_INC();                                                 \
                    LOAD(reg_pc);                                              \
                    CLK_INC();                                                 \
                }                                                              \
                LOCAL_SET_BREAK(0);                                            \
                DO_IRQBRK();                                                   \
                SET_LAST_OPCODE(0);                                            \
            }                                                                  \
        }                                                                      \
        if (ik & (IK_TRAP | IK_RESET)) {                                       \
            if (ik & IK_TRAP) {                                                \
                EXPORT_REGISTERS();                                            \
                interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);               \
                IMPORT_REGISTERS();                                            \
                if (CPU_INT_STATUS->global_pending_int & IK_RESET) {           \
                    ik |= IK_RESET;                                            \
                }                                                              \
            }                                                                  \
            if (ik & IK_RESET) {                                               \
                interrupt_ack_reset(CPU_INT_STATUS);                           \
                cpu_reset();                                                   \
                addr = LOAD(0xfffc);                                           \
                addr |= (LOAD(0xfffd) << 8);                                   \
                bank_start = bank_limit = 0; /* prevent caching */             \
                JUMP(addr);                                                    \
                DMA_ON_RESET;                                                  \
            }                                                                  \
        }                                                                      \
        if (ik & (IK_MONITOR | IK_DMA)) {                                      \
            if (ik & IK_MONITOR) {                                             \
                if (monitor_force_import(CALLER)) {                            \
                    IMPORT_REGISTERS();                                        \
                }                                                              \
                if (monitor_mask[CALLER]) {                                    \
                    EXPORT_REGISTERS();                                        \
                }                                                              \
                if (monitor_mask[CALLER] & (MI_STEP)) {                        \
                    monitor_check_icount((WORD)reg_pc);                        \
                    IMPORT_REGISTERS();                                        \
                }                                                              \
                if (monitor_mask[CALLER] & (MI_BREAK)) {                       \
                    if (monitor_check_breakpoints(CALLER, (WORD)reg_pc)) {     \
                        monitor_startup(CALLER);                               \
                        IMPORT_REGISTERS();                                    \
                    }                                                          \
                }                                                              \
                if (monitor_mask[CALLER] & (MI_WATCH)) {                       \
                    monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)reg_pc); \
                    IMPORT_REGISTERS();                                        \
                }                                                              \
            }                                                                  \
            if (ik & IK_DMA) {                                                 \
                EXPORT_REGISTERS();                                            \
                DMA_FUNC;                                                      \
                interrupt_ack_dma(CPU_INT_STATUS);                             \
                IMPORT_REGISTERS();                                            \
            }                                                                  \
        }                                                                      \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Addressing modes.  For convenience, page boundary crossing cycles and
   ``idle'' memory reads are handled here as well. */

#define GET_TEMP(dest) dest = new_value;

#define GET_IMM(dest) dest = (BYTE)(p1);
/* same as above, for NOOP */
#define GET_IMM_DUMMY()

#define GET_ABS(dest)        \
    dest = (BYTE)(LOAD(p2)); \
    CLK_INC();

/* same as above, for NOOP */
#define GET_ABS_DUMMY()      \
    LOAD(p2);                \
    CLK_INC();

#define SET_ABS(value) \
    STORE(p2, value);  \
    CLK_INC();

#define SET_ABS_RMW(old_value, new_value) \
    if (!SKIP_CYCLE) {                    \
        STORE(p2, old_value);             \
        CLK_INC();                        \
    }                                     \
    STORE(p2, new_value);                 \
    CLK_INC();

#define INT_ABS_I_R(reg_i)                                 \
    if (!SKIP_CYCLE && ((((p2) & 0xff) + reg_i) > 0xff)) { \
        LOAD((((p2) + reg_i) & 0xff) | ((p2) & 0xff00));   \
        CLK_INC();                                         \
    }

#define INT_ABS_I_W(reg_i)                               \
    if (!SKIP_CYCLE) {                                   \
        LOAD((((p2) + reg_i) & 0xff) | ((p2) & 0xff00)); \
        CLK_INC();                                       \
    }

#define GET_ABS_X(dest)        \
    INT_ABS_I_R(reg_x)         \
    dest = LOAD((p2) + reg_x); \
    CLK_INC();
/* same as above, for NOOP */
#define GET_ABS_X_DUMMY()      \
    INT_ABS_I_R(reg_x)         \
    LOAD((p2) + reg_x);        \
    CLK_INC();

#define GET_ABS_Y(dest)        \
    INT_ABS_I_R(reg_y)         \
    dest = LOAD((p2) + reg_y); \
    CLK_INC();

#define GET_ABS_X_RMW(dest)    \
    INT_ABS_I_W(reg_x)         \
    dest = LOAD((p2) + reg_x); \
    CLK_INC();

#define GET_ABS_Y_RMW(dest)    \
    INT_ABS_I_W(reg_y)         \
    dest = LOAD((p2) + reg_y); \
    CLK_INC();

#define SET_ABS_X(value)      \
    INT_ABS_I_W(reg_x)        \
    STORE(p2 + reg_x, value); \
    CLK_INC();

#define SET_ABS_Y(value)      \
    INT_ABS_I_W(reg_y)        \
    STORE(p2 + reg_y, value); \
    CLK_INC();

#define SET_ABS_I_RMW(reg_i, old_value, new_value) \
    if (!SKIP_CYCLE) {                             \
        STORE(p2 + reg_i, old_value);              \
        CLK_INC();                                 \
    }                                              \
    STORE(p2 + reg_i, new_value);                  \
    CLK_INC();

#define SET_ABS_X_RMW(old_value, new_value) SET_ABS_I_RMW(reg_x, old_value, new_value)

#define SET_ABS_Y_RMW(old_value, new_value) SET_ABS_I_RMW(reg_y, old_value, new_value)

#define GET_ZERO(dest)    \
    dest = LOAD_ZERO(p1); \
    CLK_INC();

/* same as above, for NOOP */
#define GET_ZERO_DUMMY()    \
    LOAD_ZERO(p1); \
    CLK_INC();

#define SET_ZERO(value)    \
    STORE_ZERO(p1, value); \
    CLK_INC();

#define SET_ZERO_RMW(old_value, new_value) \
    if (!SKIP_CYCLE) {                     \
        STORE_ZERO(p1, old_value);         \
        CLK_INC();                         \
    }                                      \
    STORE_ZERO(p1, new_value);             \
    CLK_INC();

#define INT_ZERO_I      \
    if (!SKIP_CYCLE) {  \
        LOAD_ZERO(p1);  \
        CLK_INC();      \
    }

#define GET_ZERO_X(dest)          \
    INT_ZERO_I                    \
    dest = LOAD_ZERO(p1 + reg_x); \
    CLK_INC();
/* same as above, for NOOP */
#define GET_ZERO_X_DUMMY()        \
    INT_ZERO_I                    \
    LOAD_ZERO(p1 + reg_x);        \
    CLK_INC();

#define GET_ZERO_Y(dest)          \
    INT_ZERO_I                    \
    dest = LOAD_ZERO(p1 + reg_y); \
    CLK_INC();

#define SET_ZERO_X(value)          \
    INT_ZERO_I                     \
    STORE_ZERO(p1 + reg_x, value); \
    CLK_INC();

#define SET_ZERO_Y(value)          \
    INT_ZERO_I                     \
    STORE_ZERO(p1 + reg_y, value); \
    CLK_INC();

#define SET_ZERO_I_RMW(reg_i, old_value, new_value) \
    if (!SKIP_CYCLE) {                              \
        STORE_ZERO(p1 + reg_i, old_value);          \
        CLK_INC();                                  \
    }                                               \
    STORE_ZERO(p1 + reg_i, new_value);              \
    CLK_INC();

#define SET_ZERO_X_RMW(old_value, new_value) SET_ZERO_I_RMW(reg_x, old_value, new_value)

#define SET_ZERO_Y_RMW(old_value, new_value) SET_ZERO_I_RMW(reg_y, old_value, new_value)

#define INT_IND_X                   \
    unsigned int tmpa, addr;        \
    LOAD_ZERO(p1);                  \
    CLK_INC();                      \
    tmpa = (p1 + reg_x) & 0xff;     \
    addr = LOAD_ZERO(tmpa);         \
    CLK_INC();                      \
    tmpa = (tmpa + 1) & 0xff;       \
    addr |= (LOAD_ZERO(tmpa) << 8); \
    CLK_INC();

#define GET_IND_X(dest) \
    INT_IND_X           \
    dest = LOAD(addr);  \
    CLK_INC();

#define SET_IND_X(value)    \
    {                       \
        INT_IND_X           \
        STORE(addr, value); \
        CLK_INC();          \
    }

#define INT_IND_Y_R()                                        \
    unsigned int tmpa, addr;                                 \
    tmpa = LOAD_ZERO(p1);                                    \
    CLK_INC();                                               \
    tmpa |= (LOAD_ZERO(p1 + 1) << 8);                        \
    CLK_INC();                                               \
    if (!SKIP_CYCLE && ((((tmpa) & 0xff) + reg_y) > 0xff)) { \
        LOAD((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff));     \
        CLK_INC();                                           \
    }                                                        \
    addr = (tmpa + reg_y) & 0xffff;                          \

#define INT_IND_Y_W()                                    \
    unsigned int tmpa, addr;                             \
    tmpa = LOAD_ZERO(p1);                                \
    CLK_INC();                                           \
    tmpa |= (LOAD_ZERO(p1 + 1) << 8);                    \
    CLK_INC();                                           \
    if (!SKIP_CYCLE) {                                   \
        LOAD((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff)); \
        CLK_INC();                                       \
    }                                                    \
    addr = (tmpa + reg_y) & 0xffff;
/* like above, for SHA_IND_Y */
#define INT_IND_Y_W_NOADDR()                             \
    unsigned int tmpa;                                   \
    tmpa = LOAD_ZERO(p1);                                \
    CLK_INC();                                           \
    tmpa |= (LOAD_ZERO(p1 + 1) << 8);                    \
    CLK_INC();                                           \
    if (!SKIP_CYCLE) {                                   \
        LOAD((tmpa & 0xff00) | ((tmpa + reg_y) & 0xff)); \
        CLK_INC();                                       \
    }

#define GET_IND_Y(dest) \
    INT_IND_Y_R()       \
    dest = LOAD(addr);  \
    CLK_INC();

#define GET_IND_Y_RMW(dest) \
    INT_IND_Y_W()           \
    dest = LOAD(addr);      \
    CLK_INC();

#define SET_IND_Y(value)    \
    {                       \
        INT_IND_Y_W()       \
        STORE(addr, value); \
        CLK_INC();          \
    }

#define SET_IND_RMW(old_value, new_value) \
    if (!SKIP_CYCLE) {                    \
        STORE(addr, old_value);           \
        CLK_INC();                        \
    }                                     \
    STORE(addr, new_value);               \
    CLK_INC();

#define SET_ABS_SH_I(addr, reg_and, reg_i)                      \
    do {                                                        \
        unsigned int tmp2, tmp3, value;                         \
                                                                \
        tmp3 = reg_and & (((addr) >> 8) + 1);                   \
        /* Set by main cpu to signal steal after above fetch */ \
        if (OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {             \
            /* Remove the signal */                             \
            LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;        \
            /* Value is not ANDed */                            \
            value = reg_and;                                    \
        } else {                                                \
            value = tmp3;                                       \
        }                                                       \
        tmp2 = addr + reg_i;                                    \
        if ((((addr) & 0xff) + reg_i) > 0xff) {                 \
            tmp2 = (tmp2 & 0xff) | ((tmp3) << 8);               \
        }                                                       \
        STORE(tmp2, (value));                                   \
        CLK_INC();                                              \
    } while (0)

#define INC_PC(value) (reg_pc += (value))

/* ------------------------------------------------------------------------- */

/* Opcodes.  */

/*
   A couple of caveats about PC:

   - the VIC-II emulation requires PC to be incremented before the first
     write access (this is not (very) important when writing to the zero
     page);

   - `p0', `p1' and `p2' can only be used *before* incrementing PC: some
     machines (eg. the C128) might depend on this.
*/

#define ADC(get_func, pc_inc)                                                                      \
    do {                                                                                           \
        unsigned int tmp_value;                                                                    \
        unsigned int tmp;                                                                          \
                                                                                                   \
        get_func(tmp_value);                                                                       \
                                                                                                   \
        if (LOCAL_DECIMAL()) {                                                                     \
            tmp = (reg_a_read & 0xf) + (tmp_value & 0xf) + (reg_p & 0x1);                          \
            if (tmp > 0x9) {                                                                       \
                tmp += 0x6;                                                                        \
            }                                                                                      \
            if (tmp <= 0x0f) {                                                                     \
                tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0);                      \
            } else {                                                                               \
                tmp = (tmp & 0xf) + (reg_a_read & 0xf0) + (tmp_value & 0xf0) + 0x10;               \
            }                                                                                      \
            LOCAL_SET_ZERO(!((reg_a_read + tmp_value + (reg_p & 0x1)) & 0xff));                    \
            LOCAL_SET_SIGN(tmp & 0x80);                                                            \
            LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && !((reg_a_read ^ tmp_value) & 0x80)); \
            if ((tmp & 0x1f0) > 0x90) {                                                            \
                tmp += 0x60;                                                                       \
            }                                                                                      \
            LOCAL_SET_CARRY((tmp & 0xff0) > 0xf0);                                                 \
        } else {                                                                                   \
            tmp = tmp_value + reg_a_read + (reg_p & P_CARRY);                                      \
            LOCAL_SET_NZ(tmp & 0xff);                                                              \
            LOCAL_SET_OVERFLOW(!((reg_a_read ^ tmp_value) & 0x80) && ((reg_a_read ^ tmp) & 0x80)); \
            LOCAL_SET_CARRY(tmp > 0xff);                                                           \
        }                                                                                          \
        reg_a_write = tmp;                                                                         \
        INC_PC(pc_inc);                                                                            \
    } while (0)

#define ANC()                                  \
    do {                                       \
        reg_a_write = (BYTE)(reg_a_read & p1); \
        LOCAL_SET_NZ(reg_a_read);              \
        LOCAL_SET_CARRY(LOCAL_SIGN());         \
        INC_PC(2);                             \
    } while (0)

#define AND(get_func, pc_inc)                     \
    do {                                          \
        unsigned int value;                       \
        get_func(value)                           \
        reg_a_write = (BYTE)(reg_a_read & value); \
        LOCAL_SET_NZ(reg_a_read);                 \
        INC_PC(pc_inc);                           \
    } while (0)

#define ANE()                                                       \
    do {                                                            \
        /* Set by main-cpu to signal steal after first fetch */     \
        if (OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {                 \
            /* Remove the signal */                                 \
            LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;            \
            /* TODO emulate the different behaviour */              \
            reg_a_write = (BYTE)((reg_a_read | 0xee) & reg_x & p1); \
        } else {                                                    \
            reg_a_write = (BYTE)((reg_a_read | 0xee) & reg_x & p1); \
        }                                                           \
        LOCAL_SET_NZ(reg_a_read);                                   \
        INC_PC(2);                                                  \
        /* Pretend to be NOP #$nn to not trigger the special case   \
           when cycles are stolen after the second fetch */         \
        SET_LAST_OPCODE(0x80);                                      \
    } while (0)

/* The fanciest opcode ever... ARR! */
#define ARR()                                                       \
    do {                                                            \
        unsigned int tmp;                                           \
                                                                    \
        tmp = reg_a_read & (p1);                                    \
        if (LOCAL_DECIMAL()) {                                      \
            int tmp_2 = tmp;                                        \
            tmp_2 |= (reg_p & P_CARRY) << 8;                        \
            tmp_2 >>= 1;                                            \
            LOCAL_SET_SIGN(LOCAL_CARRY());                          \
            LOCAL_SET_ZERO(!tmp_2);                                 \
            LOCAL_SET_OVERFLOW((tmp_2 ^ tmp) & 0x40);               \
            if (((tmp & 0xf) + (tmp & 0x1)) > 0x5) {                \
                tmp_2 = (tmp_2 & 0xf0) | ((tmp_2 + 0x6) & 0xf);     \
            }                                                       \
            if (((tmp & 0xf0) + (tmp & 0x10)) > 0x50) {             \
                tmp_2 = (tmp_2 & 0x0f) | ((tmp_2 + 0x60) & 0xf0);   \
                LOCAL_SET_CARRY(1);                                 \
            } else {                                                \
                LOCAL_SET_CARRY(0);                                 \
            }                                                       \
            reg_a_write = tmp_2;                                    \
        } else {                                                    \
            tmp |= (reg_p & P_CARRY) << 8;                          \
            tmp >>= 1;                                              \
            LOCAL_SET_NZ(tmp);                                      \
            LOCAL_SET_CARRY(tmp & 0x40);                            \
            LOCAL_SET_OVERFLOW((tmp & 0x40) ^ ((tmp & 0x20) << 1)); \
            reg_a_write = tmp;                                      \
        }                                                           \
        INC_PC(2);                                                  \
    } while (0)

#define ASL(pc_inc, get_func, set_func)      \
    do {                                     \
        unsigned int old_value, new_value;   \
        get_func(old_value)                  \
        LOCAL_SET_CARRY(old_value & 0x80);   \
        new_value = (old_value << 1) & 0xff; \
        LOCAL_SET_NZ(new_value);             \
        INC_PC(pc_inc);                      \
        set_func(old_value, new_value);      \
    } while (0)

#define ASL_A()                             \
    do {                                    \
        LOCAL_SET_CARRY(reg_a_read & 0x80); \
        reg_a_write = reg_a_read << 1;      \
        LOCAL_SET_NZ(reg_a_read);           \
        INC_PC(1);                          \
    } while (0)

#define ASR()                        \
    do {                             \
        unsigned int tmp;            \
                                     \
        tmp = reg_a_read & (p1);     \
        LOCAL_SET_CARRY(tmp & 0x01); \
        reg_a_write = tmp >> 1;      \
        LOCAL_SET_NZ(reg_a_read);    \
        INC_PC(2);                   \
    } while (0)

#define BIT(get_func, pc_inc)                \
    do {                                     \
        unsigned int tmp;                    \
        get_func(tmp)                        \
        LOCAL_SET_SIGN(tmp & 0x80);          \
        LOCAL_SET_OVERFLOW(tmp & 0x40);      \
        LOCAL_SET_ZERO(!(tmp & reg_a_read)); \
        INC_PC(pc_inc);                      \
    } while (0)

#ifdef C64DTV

#define BRANCH(cond)                                              \
    do {                                                          \
        INC_PC(2);                                                \
                                                                  \
        if (cond) {                                               \
            unsigned int dest_addr;                               \
                                                                  \
            burst_broken = 1;                                     \
            dest_addr = reg_pc + (signed char)(p1);               \
                                                                  \
            if (!SKIP_CYCLE) {                                    \
                LOAD(reg_pc);                                     \
                CLK_INC();                                        \
                if ((reg_pc ^ dest_addr) & 0xff00) {              \
                    LOAD((reg_pc & 0xff00) | (dest_addr & 0xff)); \
                    CLK_INC();                                    \
                } else {                                          \
                    OPCODE_DELAYS_INTERRUPT();                    \
                }                                                 \
            }                                                     \
            JUMP(dest_addr & 0xffff);                             \
        }                                                         \
    } while (0)

#else /* !C64DTV */

#define BRANCH(cond)                                          \
    do {                                                      \
        INC_PC(2);                                            \
                                                              \
        if (cond) {                                           \
            unsigned int dest_addr;                           \
                                                              \
            dest_addr = reg_pc + (signed char)(p1);           \
                                                              \
            LOAD(reg_pc);                                     \
            CLK_INC();                                        \
            if ((reg_pc ^ dest_addr) & 0xff00) {              \
                LOAD((reg_pc & 0xff00) | (dest_addr & 0xff)); \
                CLK_INC();                                    \
            } else {                                          \
                OPCODE_DELAYS_INTERRUPT();                    \
            }                                                 \
            JUMP(dest_addr & 0xffff);                         \
        }                                                     \
    } while (0)

#endif

#define BRK() \
    do { \
        WORD addr;          \
        EXPORT_REGISTERS(); \
        TRACE_BRK();        \
        INC_PC(2);          \
        LOCAL_SET_BREAK(1); \
        DO_IRQBRK();        \
    } while (0)

/* The JAM (0x02) opcode is also used to patch the ROM.  The function trap_handler()
   returns nonzero if this is not a patch, but a `real' JAM instruction. */

#define JAM_02()                                                                      \
    do {                                                                              \
        DWORD trap_result;                                                            \
        EXPORT_REGISTERS();                                                           \
        if (!ROM_TRAP_ALLOWED() || (trap_result = ROM_TRAP_HANDLER()) == (DWORD)-1) { \
            REWIND_FETCH_OPCODE(CLK);                                                 \
            JAM();                                                                    \
        } else {                                                                      \
            if (trap_result) {                                                        \
                REWIND_FETCH_OPCODE(CLK);                                             \
                SET_OPCODE(trap_result);                                              \
                IMPORT_REGISTERS();                                                   \
                goto trap_skipped;                                                    \
            } else {                                                                  \
                IMPORT_REGISTERS();                                                   \
            }                                                                         \
        }                                                                             \
    } while (0)

#define CLC()               \
    do {                    \
        INC_PC(1);          \
        LOCAL_SET_CARRY(0); \
    } while (0)

#define CLD()                 \
    do {                      \
        INC_PC(1);            \
        LOCAL_SET_DECIMAL(0); \
    } while (0)

#ifdef OPCODE_UPDATE_IN_FETCH

#define CLI()                                             \
    do {                                                  \
        INC_PC(1);                                        \
        /* Set by main-cpu to signal steal during CLI */  \
        if (!OPINFO_ENABLES_IRQ(LAST_OPCODE_INFO)) {      \
            if (LOCAL_INTERRUPT()) {                      \
                OPCODE_ENABLES_IRQ();                     \
            }                                             \
        } else {                                          \
            /* Remove the signal and the related delay */ \
            LAST_OPCODE_INFO &= ~OPINFO_ENABLES_IRQ_MSK;  \
        }                                                 \
        LOCAL_SET_INTERRUPT(0);                           \
    } while (0)

#else /* !OPCODE_UPDATE_IN_FETCH */

#define CLI()                     \
    do {                          \
        INC_PC(1);                \
        if (LOCAL_INTERRUPT()) {  \
            OPCODE_ENABLES_IRQ(); \
        }                         \
        LOCAL_SET_INTERRUPT(0);   \
    } while (0)

#endif

#define CLV()                  \
    do {                       \
        INC_PC(1);             \
        LOCAL_SET_OVERFLOW(0); \
    } while (0)

#define CP(reg, get_func, pc_inc)     \
    do {                              \
        unsigned int tmp;             \
        BYTE value;                   \
        get_func(value)               \
        tmp = reg - value;            \
        LOCAL_SET_CARRY(tmp < 0x100); \
        LOCAL_SET_NZ(tmp & 0xff);     \
        INC_PC(pc_inc);               \
    } while (0)

#define DCP(pc_inc, get_func, set_func)           \
    do {                                          \
        unsigned int old_value, new_value;        \
        get_func(old_value)                       \
        new_value = (old_value - 1) & 0xff;       \
        LOCAL_SET_CARRY(reg_a_read >= new_value); \
        LOCAL_SET_NZ((reg_a_read - new_value));   \
        INC_PC(pc_inc);                           \
        set_func(old_value, new_value)            \
    } while (0)

#define DEC(pc_inc, get_func, set_func)     \
    do {                                    \
        unsigned int old_value, new_value;  \
        get_func(old_value)                 \
        new_value = (old_value - 1) & 0xff; \
        LOCAL_SET_NZ(new_value);            \
        INC_PC(pc_inc);                     \
        set_func(old_value, new_value)      \
    } while (0)

#define DEX()                \
    do {                     \
        reg_x--;             \
        LOCAL_SET_NZ(reg_x); \
        INC_PC(1);           \
    } while (0)

#define DEY()                \
    do {                     \
        reg_y--;             \
        LOCAL_SET_NZ(reg_y); \
        INC_PC(1);           \
    } while (0)

#define EOR(get_func, pc_inc)                       \
    do {                                            \
        unsigned int value;                         \
        get_func(value)                             \
        reg_a_write = (BYTE)(reg_a_read ^ (value)); \
        LOCAL_SET_NZ(reg_a_read);                   \
        INC_PC(pc_inc);                             \
    } while (0)

#define INC(pc_inc, get_func, set_func)     \
    do {                                    \
        unsigned int old_value, new_value;  \
        get_func(old_value)                 \
        new_value = (old_value + 1) & 0xff; \
        LOCAL_SET_NZ(new_value);            \
        INC_PC(pc_inc);                     \
        set_func(old_value, new_value)      \
    } while (0)

#define INX()                \
    do {                     \
        reg_x++;             \
        LOCAL_SET_NZ(reg_x); \
        INC_PC(1);           \
    } while (0)

#define INY()                \
    do {                     \
        reg_y++;             \
        LOCAL_SET_NZ(reg_y); \
        INC_PC(1);           \
    } while (0)

#define ISB(pc_inc, get_func, set_func)     \
    do {                                    \
        unsigned int old_value, new_value;  \
        get_func(old_value)                 \
        new_value = (old_value + 1) & 0xff; \
        SBC(GET_TEMP, 0);                   \
        INC_PC(pc_inc);                     \
        set_func(old_value, new_value)      \
    } while (0)

#define JMP(addr)   \
    do {            \
        JUMP(addr); \
    } while (0)

#define JMP_IND()                                                    \
    do {                                                             \
        WORD dest_addr;                                              \
        dest_addr = LOAD(p2);                                        \
        CLK_INC();                                                   \
        dest_addr |= (LOAD((p2 & 0xff00) | ((p2 + 1) & 0xff)) << 8); \
        CLK_INC();                                                   \
        JUMP(dest_addr);                                             \
    } while (0)

#define JSR()                                  \
    do {                                       \
        unsigned int tmp_addr;                 \
        if (!SKIP_CYCLE) {                     \
            STACK_PEEK();                      \
            CLK_INC();                         \
        }                                      \
        INC_PC(2);                             \
        PUSH(((reg_pc) >> 8) & 0xff);          \
        CLK_INC();                             \
        PUSH((reg_pc) & 0xff);                 \
        CLK_INC();                             \
        tmp_addr = (p1 | (LOAD(reg_pc) << 8)); \
        CLK_INC();                             \
        JUMP(tmp_addr);                        \
    } while (0)

#define LAS()                                          \
    do {                                               \
        unsigned int value;                            \
        GET_ABS_Y(value)                               \
        reg_a_write = reg_x = reg_sp = reg_sp & value; \
        LOCAL_SET_NZ(reg_a_read);                      \
        INC_PC(3);                                     \
    } while (0)

#define LAX(get_func, pc_inc)     \
    do {                          \
        get_func(reg_x);          \
        reg_a_write = reg_x;      \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(pc_inc);           \
    } while (0)

#define LD(dest, get_func, pc_inc) \
    do {                           \
        get_func(dest);            \
        LOCAL_SET_NZ(dest);        \
        INC_PC(pc_inc);            \
    } while (0)

#define LSR(pc_inc, get_func, set_func)    \
    do {                                   \
        unsigned int old_value, new_value; \
        get_func(old_value)                \
        LOCAL_SET_CARRY(old_value & 0x01); \
        new_value = old_value >> 1;        \
        LOCAL_SET_NZ(new_value);           \
        INC_PC(pc_inc);                    \
        set_func(old_value, new_value)     \
    } while (0)

#define LSR_A()                             \
    do {                                    \
        LOCAL_SET_CARRY(reg_a_read & 0x01); \
        reg_a_write = reg_a_read >> 1;      \
        LOCAL_SET_NZ(reg_a_read);           \
        INC_PC(1);                          \
    } while (0)

/* Note: this is not always exact, as this opcode can be quite unstable!
   Moreover, the behavior is different from the one described in 64doc. */
#define LXA(value, pc_inc)                                             \
    do {                                                               \
        reg_a_write = reg_x = ((reg_a_read | 0xee) & ((BYTE)(value))); \
        LOCAL_SET_NZ(reg_a_read);                                      \
        INC_PC(pc_inc);                                                \
    } while (0)

#define ORA(get_func, pc_inc)                       \
    do {                                            \
        unsigned int value;                         \
        get_func(value)                             \
        reg_a_write = (BYTE)(reg_a_read | (value)); \
        LOCAL_SET_NZ(reg_a_write);                  \
        INC_PC(pc_inc);                             \
    } while (0)

#define NOOP(get_func, pc_inc) \
    do {                       \
        get_func()             \
        INC_PC(pc_inc);        \
    } while (0)

#define PHA()             \
    do {                  \
        PUSH(reg_a_read); \
        CLK_INC();        \
        INC_PC(1);        \
    } while (0)

#define PHP()                           \
    do {                                \
        PUSH(LOCAL_STATUS() | P_BREAK); \
        CLK_INC();                      \
        INC_PC(1);                      \
    } while (0)

#define PLA()                     \
    do {                          \
        if (!SKIP_CYCLE) {        \
            STACK_PEEK();         \
            CLK_INC();            \
        }                         \
        reg_a_write = PULL();     \
        CLK_INC();                \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(1);                \
    } while (0)

#define PLP()                                                 \
    do {                                                      \
        BYTE s;                                               \
        if (!SKIP_CYCLE) {                                    \
            STACK_PEEK();                                     \
            CLK_INC();                                        \
        }                                                     \
        s = PULL();                                           \
        CLK_INC();                                            \
        if (!(s & P_INTERRUPT) && LOCAL_INTERRUPT()) {        \
            OPCODE_ENABLES_IRQ();                             \
        } else if ((s & P_INTERRUPT) && !LOCAL_INTERRUPT()) { \
            OPCODE_DISABLES_IRQ();                            \
        }                                                     \
        LOCAL_SET_STATUS(s);                                  \
        INC_PC(1);                                            \
    } while (0)

#define RLA(pc_inc, get_func, set_func)                   \
    do {                                                  \
        unsigned int old_value, new_value;                \
        get_func(old_value)                               \
        new_value = (old_value << 1) | (reg_p & P_CARRY); \
        LOCAL_SET_CARRY(new_value & 0x100);               \
        reg_a_write = reg_a_read & new_value;             \
        LOCAL_SET_NZ(reg_a_read);                         \
        INC_PC(pc_inc);                                   \
        set_func(old_value, new_value);                   \
    } while (0)

#define ROL(pc_inc, get_func, set_func)                   \
    do {                                                  \
        unsigned int old_value, new_value;                \
        get_func(old_value)                               \
        new_value = (old_value << 1) | (reg_p & P_CARRY); \
        LOCAL_SET_CARRY(new_value & 0x100);               \
        LOCAL_SET_NZ(new_value & 0xff);                   \
        INC_PC(pc_inc);                                   \
        set_func(old_value, new_value);                   \
    } while (0)

#define ROL_A()                                \
    do {                                       \
        unsigned int tmp = reg_a_read << 1;    \
                                               \
        reg_a_write = tmp | (reg_p & P_CARRY); \
        LOCAL_SET_CARRY(tmp & 0x100);          \
        LOCAL_SET_NZ(reg_a_read);              \
        INC_PC(1);                             \
    } while (0)

#define ROR(pc_inc, get_func, set_func)    \
    do {                                   \
        unsigned int old_value, new_value; \
        get_func(old_value)                \
        new_value = old_value;             \
        if (reg_p & P_CARRY) {             \
            new_value |= 0x100;            \
        }                                  \
        LOCAL_SET_CARRY(new_value & 0x01); \
        new_value >>= 1;                   \
        LOCAL_SET_NZ(new_value);           \
        INC_PC(pc_inc);                    \
        set_func(old_value, new_value)     \
    } while (0)

#define ROR_A()                                         \
    do {                                                \
        BYTE tmp = reg_a_read;                          \
                                                        \
        reg_a_write = (reg_a_read >> 1) | (reg_p << 7); \
        LOCAL_SET_CARRY(tmp & 0x01);                    \
        LOCAL_SET_NZ(reg_a_read);                       \
        INC_PC(1);                                      \
    } while (0)

#define RRA(pc_inc, get_func, set_func)    \
    do {                                   \
        unsigned int old_value, new_value; \
        get_func(old_value)                \
        new_value = old_value;             \
        if (reg_p & P_CARRY) {             \
            new_value |= 0x100;            \
        }                                  \
        LOCAL_SET_CARRY(new_value & 0x01); \
        new_value >>= 1;                   \
        LOCAL_SET_NZ(new_value);           \
        INC_PC(pc_inc);                    \
        ADC(GET_TEMP, 0);                  \
        set_func(old_value, new_value)     \
    } while (0)

/* RTI does must not use `OPCODE_ENABLES_IRQ()' even if the I flag changes
   from 1 to 0 because the value of I is set 3 cycles before the end of the
   opcode, and thus the 6510 has enough time to call the interrupt routine as
   soon as the opcode ends, if necessary.  */
#define RTI()                        \
    do {                             \
        WORD tmp;                    \
        if (!SKIP_CYCLE) {           \
            STACK_PEEK();            \
            CLK_INC();               \
        }                            \
        tmp = (WORD)PULL();          \
        CLK_INC();                   \
        LOCAL_SET_STATUS((BYTE)tmp); \
        tmp = (WORD)PULL();          \
        CLK_INC();                   \
        tmp |= (WORD)PULL() << 8;    \
        CLK_INC();                   \
        JUMP(tmp);                   \
    } while (0)

#define RTS()                 \
    do {                      \
        WORD tmp;             \
        if (!SKIP_CYCLE) {    \
            STACK_PEEK();     \
            CLK_INC();        \
        }                     \
        tmp = PULL();         \
        CLK_INC();            \
        tmp |= (PULL() << 8); \
        CLK_INC();            \
        LOAD(tmp);            \
        CLK_INC();            \
        tmp++;                \
        JUMP(tmp);            \
    } while (0)

#define SAC()                       \
    do {                            \
        reg_a_write_idx = p1 >> 4;  \
        reg_a_read_idx = p1 & 0x0f; \
        INC_PC(2);                  \
    } while (0)

#define SBC(get_func, pc_inc)                                                               \
    do {                                                                                    \
        WORD src, tmp;                                                                      \
                                                                                            \
        get_func(src)                                                                       \
        tmp = reg_a_read - src - ((reg_p & P_CARRY) ? 0 : 1);                               \
        if (reg_p & P_DECIMAL) {                                                            \
            unsigned int tmp_a;                                                             \
            tmp_a = (reg_a_read & 0xf) - (src & 0xf) - ((reg_p & P_CARRY) ? 0 : 1);         \
            if (tmp_a & 0x10) {                                                             \
                tmp_a = ((tmp_a - 6) & 0xf) | ((reg_a_read & 0xf0) - (src & 0xf0) - 0x10);  \
            } else {                                                                        \
                tmp_a = (tmp_a & 0xf) | ((reg_a_read & 0xf0) - (src & 0xf0));               \
            }                                                                               \
            if (tmp_a & 0x100) {                                                            \
                tmp_a -= 0x60;                                                              \
            }                                                                               \
            LOCAL_SET_CARRY(tmp < 0x100);                                                   \
            LOCAL_SET_NZ(tmp & 0xff);                                                       \
            LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && ((reg_a_read ^ src) & 0x80)); \
            reg_a_write = (BYTE) tmp_a;                                                     \
        } else {                                                                            \
            LOCAL_SET_NZ(tmp & 0xff);                                                       \
            LOCAL_SET_CARRY(tmp < 0x100);                                                   \
            LOCAL_SET_OVERFLOW(((reg_a_read ^ tmp) & 0x80) && ((reg_a_read ^ src) & 0x80)); \
            reg_a_write = (BYTE) tmp;                                                       \
        }                                                                                   \
        INC_PC(pc_inc);                                                                     \
    } while (0)

#define SBX()                            \
    do {                                 \
        unsigned int tmp;                \
        INC_PC(2);                       \
        tmp = (reg_a_read & reg_x) - p1; \
        LOCAL_SET_CARRY(tmp < 0x100);    \
        reg_x = tmp & 0xff;              \
        LOCAL_SET_NZ(reg_x);             \
    } while (0)

#undef SEC    /* defined in time.h on SunOS. */
#define SEC()               \
    do {                    \
        LOCAL_SET_CARRY(1); \
        INC_PC(1);          \
    } while (0)

#define SED()                 \
    do {                      \
        LOCAL_SET_DECIMAL(1); \
        INC_PC(1);            \
    } while (0)

#define SEI()                      \
    do {                           \
        if (!LOCAL_INTERRUPT()) {  \
            OPCODE_DISABLES_IRQ(); \
        }                          \
        LOCAL_SET_INTERRUPT(1);    \
        INC_PC(1);                 \
    } while (0)

#define SHA_IND_Y()                                    \
    do {                                               \
        INT_IND_Y_W_NOADDR();                          \
        SET_ABS_SH_I(tmpa, reg_a_read & reg_x, reg_y); \
        INC_PC(2);                                     \
    } while (0)

#define SH_ABS_I(reg_and, reg_i)                           \
    do {                                                   \
        if (!SKIP_CYCLE) {                                 \
            LOAD(((p2 + reg_i) & 0xff) | ((p2) & 0xff00)); \
            CLK_INC();                                     \
        }                                                  \
        SET_ABS_SH_I(p2, reg_and, reg_i);                  \
        INC_PC(3);                                         \
    } while (0)

#define SHS_ABS_Y()                          \
    do {                                     \
        SH_ABS_I(reg_a_read & reg_x, reg_y); \
        reg_sp = reg_a_read & reg_x;         \
    } while (0)

#define SIR()                  \
    do {                       \
        reg_y_idx = p1 >> 4;   \
        reg_x_idx = p1 & 0x0f; \
        INC_PC(2);             \
    } while (0)

#define SLO(pc_inc, get_func, set_func)       \
    do {                                      \
        unsigned int old_value, new_value;    \
        get_func(old_value)                   \
        new_value = old_value;                \
        LOCAL_SET_CARRY(old_value & 0x80);    \
        new_value <<= 1;                      \
        reg_a_write = reg_a_read | new_value; \
        LOCAL_SET_NZ(reg_a_read);             \
        INC_PC(pc_inc);                       \
        set_func(old_value, new_value)        \
    } while (0)

#define SRE(pc_inc, get_func, set_func)       \
    do {                                      \
        unsigned int old_value, new_value;    \
        get_func(old_value)                   \
        new_value = old_value;                \
        LOCAL_SET_CARRY(old_value & 0x01);    \
        new_value >>= 1;                      \
        reg_a_write = reg_a_read ^ new_value; \
        LOCAL_SET_NZ(reg_a_read);             \
        INC_PC(pc_inc);                       \
        set_func(old_value, new_value)        \
    } while (0)

#define ST(value, set_func, pc_inc) \
    do {                            \
        INC_PC(pc_inc);             \
        set_func(value);            \
    } while (0)

#define TAX()                     \
    do {                          \
        reg_x = reg_a_read;       \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(1);                \
    } while (0)

#define TAY()                     \
    do {                          \
        reg_y = reg_a_read;       \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(1);                \
    } while (0)

#define TSX()                 \
    do {                      \
        reg_x = reg_sp;       \
        LOCAL_SET_NZ(reg_sp); \
        INC_PC(1);            \
    } while (0)

#define TXA()                     \
    do {                          \
        reg_a_write = reg_x;      \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(1);                \
    } while (0)

#define TXS()           \
    do {                \
        reg_sp = reg_x; \
        INC_PC(1);      \
    } while (0)

#define TYA()                     \
    do {                          \
        reg_a_write = reg_y;      \
        LOCAL_SET_NZ(reg_a_read); \
        INC_PC(1);                \
    } while (0)


/* ------------------------------------------------------------------------- */

static const BYTE fetch_tab[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $00 */
    /* $10 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $10 */
    /* $20 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $20 */
    /* $30 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $30 */
    /* $40 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $40 */
    /* $50 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $50 */
    /* $60 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $60 */
    /* $70 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $70 */
    /* $80 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $80 */
    /* $90 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $90 */
    /* $A0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $A0 */
    /* $B0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $B0 */
    /* $C0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $C0 */
    /* $D0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, /* $D0 */
    /* $E0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, /* $E0 */
    /* $F0 */  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1  /* $F0 */
};


/* ------------------------------------------------------------------------ */

/* Here, the CPU is emulated. */

{
    while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
        alarm_context_dispatch(ALARM_CONTEXT, CLK);
    }

    {
        enum cpu_int pending_interrupt;

        if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
            && (CPU_INT_STATUS->global_pending_int & IK_IRQPEND)
            && CPU_INT_STATUS->irq_pending_clk <= CLK) {
            interrupt_ack_irq(CPU_INT_STATUS);
        }

        pending_interrupt = CPU_INT_STATUS->global_pending_int;
        if (pending_interrupt != IK_NONE) {
            DO_INTERRUPT(pending_interrupt);
            if (!(CPU_INT_STATUS->global_pending_int & IK_IRQ)
                && CPU_INT_STATUS->global_pending_int & IK_IRQPEND) {
                CPU_INT_STATUS->global_pending_int &= ~IK_IRQPEND;
            }
            while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) {
                alarm_context_dispatch(ALARM_CONTEXT, CLK);
            }
        }
    }

    {
        opcode_t opcode;
#ifdef DEBUG
        debug_clk = maincpu_clk;
#endif

#ifdef FEATURE_CPUMEMHISTORY
        memmap_state |= (MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif

        SET_LAST_ADDR(reg_pc);
        FETCH_OPCODE(opcode);

#ifdef FEATURE_CPUMEMHISTORY
        memmap_mark_read(reg_pc);
        /* FIXME JSR (0x20) hasn't load p2 yet. The earlier LOAD(reg_pc+2) hack can break stealing badly on x64sc. */
        monitor_cpuhistory_store(reg_pc, p0, p1, p2 >> 8, reg_a_read, reg_x, reg_y, reg_sp, LOCAL_STATUS());
        memmap_state &= ~(MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif

#ifdef DEBUG
        if (TRACEFLG) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8);

            debug_maincpu((DWORD)(reg_pc), debug_clk,
                          mon_disassemble_to_string(e_comp_space,
                                                    (WORD) reg_pc, op,
                                                    lo, hi, 0, 1, "6502"),
                          reg_a_read, reg_x, reg_y, reg_sp);
        }
        if (debug.perform_break_into_monitor) {
            monitor_startup_trap();
            debug.perform_break_into_monitor = 0;
        }
#endif

trap_skipped:
#ifndef OPCODE_UPDATE_IN_FETCH
        SET_LAST_OPCODE(p0);
#endif

        switch (p0) {
            case 0x00:          /* BRK */
                BRK();
                break;

            case 0x01:          /* ORA ($nn,X) */
                ORA(GET_IND_X, 2);
                break;

            case 0x02:          /* JAM - also used for traps */
                STATIC_ASSERT(TRAP_OPCODE == 0x02);
                JAM_02();
                break;

            case 0x22:          /* JAM */
            case 0x52:          /* JAM */
            case 0x62:          /* JAM */
            case 0x72:          /* JAM */
            case 0x92:          /* JAM */
            case 0xb2:          /* JAM */
            case 0xd2:          /* JAM */
            case 0xf2:          /* JAM */
#ifndef C64DTV
            case 0x12:          /* JAM */
            case 0x32:          /* JAM */
            case 0x42:          /* JAM */
#endif
                REWIND_FETCH_OPCODE(CLK);
                JAM();
                break;

#ifdef C64DTV
            case 0x12:          /* BRA $nnnn */
                BRANCH(1);
                break;

            case 0x32:          /* SAC #$nn */
                SAC();
                break;

            case 0x42:          /* SIR #$nn */
                SIR();
                break;
#endif

            case 0x03:          /* SLO ($nn,X) */
                SLO(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0x04:          /* NOOP $nn */
            case 0x44:          /* NOOP $nn */
            case 0x64:          /* NOOP $nn */
                NOOP(GET_ZERO_DUMMY, 2);
                break;

            case 0x05:          /* ORA $nn */
                ORA(GET_ZERO, 2);
                break;

            case 0x06:          /* ASL $nn */
                ASL(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x07:          /* SLO $nn */
                SLO(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x08:          /* PHP */
                PHP();
                break;

            case 0x09:          /* ORA #$nn */
                ORA(GET_IMM, 2);
                break;

            case 0x0a:          /* ASL A */
                ASL_A();
                break;

            case 0x0b:          /* ANC #$nn */
            case 0x2b:          /* ANC #$nn */
                ANC();
                break;

            case 0x0c:          /* NOOP $nnnn */
                NOOP(GET_ABS_DUMMY, 3);
                break;

            case 0x0d:          /* ORA $nnnn */
                ORA(GET_ABS, 3);
                break;

            case 0x0e:          /* ASL $nnnn */
                ASL(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x0f:          /* SLO $nnnn */
                SLO(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x10:          /* BPL $nnnn */
                BRANCH(!LOCAL_SIGN());
                break;

            case 0x11:          /* ORA ($nn),Y */
                ORA(GET_IND_Y, 2);
                break;

            case 0x13:          /* SLO ($nn),Y */
                SLO(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0x14:          /* NOOP $nn,X */
            case 0x34:          /* NOOP $nn,X */
            case 0x54:          /* NOOP $nn,X */
            case 0x74:          /* NOOP $nn,X */
            case 0xd4:          /* NOOP $nn,X */
            case 0xf4:          /* NOOP $nn,X */
                NOOP(GET_ZERO_X_DUMMY, 2);
                break;

            case 0x15:          /* ORA $nn,X */
                ORA(GET_ZERO_X, 2);
                break;

            case 0x16:          /* ASL $nn,X */
                ASL(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x17:          /* SLO $nn,X */
                SLO(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x18:          /* CLC */
                CLC();
                break;

            case 0x19:          /* ORA $nnnn,Y */
                ORA(GET_ABS_Y, 3);
                break;

            case 0x1a:          /* NOOP */
            case 0x3a:          /* NOOP */
            case 0x5a:          /* NOOP */
            case 0x7a:          /* NOOP */
            case 0xda:          /* NOOP */
            case 0xfa:          /* NOOP */
            case 0xea:          /* NOP */
                NOOP(GET_IMM_DUMMY, 1);
                break;

            case 0x1b:          /* SLO $nnnn,Y */
                SLO(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0x1c:          /* NOOP $nnnn,X */
            case 0x3c:          /* NOOP $nnnn,X */
            case 0x5c:          /* NOOP $nnnn,X */
            case 0x7c:          /* NOOP $nnnn,X */
            case 0xdc:          /* NOOP $nnnn,X */
            case 0xfc:          /* NOOP $nnnn,X */
                NOOP(GET_ABS_X_DUMMY, 3);
                break;

            case 0x1d:          /* ORA $nnnn,X */
                ORA(GET_ABS_X, 3);
                break;

            case 0x1e:          /* ASL $nnnn,X */
                ASL(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x1f:          /* SLO $nnnn,X */
                SLO(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x20:          /* JSR $nnnn */
                JSR();
                break;

            case 0x21:          /* AND ($nn,X) */
                AND(GET_IND_X, 2);
                break;

            case 0x23:          /* RLA ($nn,X) */
                RLA(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0x24:          /* BIT $nn */
                BIT(GET_ZERO, 2);
                break;

            case 0x25:          /* AND $nn */
                AND(GET_ZERO, 2);
                break;

            case 0x26:          /* ROL $nn */
                ROL(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x27:          /* RLA $nn */
                RLA(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x28:          /* PLP */
                PLP();
                break;

            case 0x29:          /* AND #$nn */
                AND(GET_IMM, 2);
                break;

            case 0x2a:          /* ROL A */
                ROL_A();
                break;

            case 0x2c:          /* BIT $nnnn */
                BIT(GET_ABS, 3);
                break;

            case 0x2d:          /* AND $nnnn */
                AND(GET_ABS, 3);
                break;

            case 0x2e:          /* ROL $nnnn */
                ROL(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x2f:          /* RLA $nnnn */
                RLA(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x30:          /* BMI $nnnn */
                BRANCH(LOCAL_SIGN());
                break;

            case 0x31:          /* AND ($nn),Y */
                AND(GET_IND_Y, 2);
                break;

            case 0x33:          /* RLA ($nn),Y */
                RLA(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0x35:          /* AND $nn,X */
                AND(GET_ZERO_X, 2);
                break;

            case 0x36:          /* ROL $nn,X */
                ROL(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x37:          /* RLA $nn,X */
                RLA(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x38:          /* SEC */
                SEC();
                break;

            case 0x39:          /* AND $nnnn,Y */
                AND(GET_ABS_Y, 3);
                break;

            case 0x3b:          /* RLA $nnnn,Y */
                RLA(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0x3d:          /* AND $nnnn,X */
                AND(GET_ABS_X, 3);
                break;

            case 0x3e:          /* ROL $nnnn,X */
                ROL(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x3f:          /* RLA $nnnn,X */
                RLA(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x40:          /* RTI */
                RTI();
                break;

            case 0x41:          /* EOR ($nn,X) */
                EOR(GET_IND_X, 2);
                break;

            case 0x43:          /* SRE ($nn,X) */
                SRE(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0x45:          /* EOR $nn */
                EOR(GET_ZERO, 2);
                break;

            case 0x46:          /* LSR $nn */
                LSR(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x47:          /* SRE $nn */
                SRE(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x48:          /* PHA */
                PHA();
                break;

            case 0x49:          /* EOR #$nn */
                EOR(GET_IMM, 2);
                break;

            case 0x4a:          /* LSR A */
                LSR_A();
                break;

            case 0x4b:          /* ASR #$nn */
                ASR();
                break;

            case 0x4c:          /* JMP $nnnn */
                JMP(p2);
                break;

            case 0x4d:          /* EOR $nnnn */
                EOR(GET_ABS, 3);
                break;

            case 0x4e:          /* LSR $nnnn */
                LSR(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x4f:          /* SRE $nnnn */
                SRE(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x50:          /* BVC $nnnn */
                BRANCH(!LOCAL_OVERFLOW());
                break;

            case 0x51:          /* EOR ($nn),Y */
                EOR(GET_IND_Y, 2);
                break;

            case 0x53:          /* SRE ($nn),Y */
                SRE(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0x55:          /* EOR $nn,X */
                EOR(GET_ZERO_X, 2);
                break;

            case 0x56:          /* LSR $nn,X */
                LSR(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x57:          /* SRE $nn,X */
                SRE(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x58:          /* CLI */
                CLI();
                break;

            case 0x59:          /* EOR $nnnn,Y */
                EOR(GET_ABS_Y, 3);
                break;

            case 0x5b:          /* SRE $nnnn,Y */
                SRE(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0x5d:          /* EOR $nnnn,X */
                EOR(GET_ABS_X, 3);
                break;

            case 0x5e:          /* LSR $nnnn,X */
                LSR(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x5f:          /* SRE $nnnn,X */
                SRE(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x60:          /* RTS */
                RTS();
                break;

            case 0x61:          /* ADC ($nn,X) */
                ADC(GET_IND_X, 2);
                break;

            case 0x63:          /* RRA ($nn,X) */
                RRA(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0x65:          /* ADC $nn */
                ADC(GET_ZERO, 2);
                break;

            case 0x66:          /* ROR $nn */
                ROR(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x67:          /* RRA $nn */
                RRA(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0x68:          /* PLA */
                PLA();
                break;

            case 0x69:          /* ADC #$nn */
                ADC(GET_IMM, 2);
                break;

            case 0x6a:          /* ROR A */
                ROR_A();
                break;

            case 0x6b:          /* ARR #$nn */
                ARR();
                break;

            case 0x6c:          /* JMP ($nnnn) */
                JMP_IND();
                break;

            case 0x6d:          /* ADC $nnnn */
                ADC(GET_ABS, 3);
                break;

            case 0x6e:          /* ROR $nnnn */
                ROR(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x6f:          /* RRA $nnnn */
                RRA(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0x70:          /* BVS $nnnn */
                BRANCH(LOCAL_OVERFLOW());
                break;

            case 0x71:          /* ADC ($nn),Y */
                ADC(GET_IND_Y, 2);
                break;

            case 0x73:          /* RRA ($nn),Y */
                RRA(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0x75:          /* ADC $nn,X */
                ADC(GET_ZERO_X, 2);
                break;

            case 0x76:          /* ROR $nn,X */
                ROR(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x77:          /* RRA $nn,X */
                RRA(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0x78:          /* SEI */
                SEI();
                break;

            case 0x79:          /* ADC $nnnn,Y */
                ADC(GET_ABS_Y, 3);
                break;

            case 0x7b:          /* RRA $nnnn,Y */
                RRA(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0x7d:          /* ADC $nnnn,X */
                ADC(GET_ABS_X, 3);
                break;

            case 0x7e:          /* ROR $nnnn,X */
                ROR(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x7f:          /* RRA $nnnn,X */
                RRA(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0x80:          /* NOOP #$nn */
            case 0x82:          /* NOOP #$nn */
            case 0x89:          /* NOOP #$nn */
            case 0xc2:          /* NOOP #$nn */
            case 0xe2:          /* NOOP #$nn */
                NOOP(GET_IMM_DUMMY, 2);
                break;

            case 0x81:          /* STA ($nn,X) */
                ST(reg_a_read, SET_IND_X, 2);
                break;

            case 0x83:          /* SAX ($nn,X) */
                ST(reg_a_read & reg_x, SET_IND_X, 2);
                break;

            case 0x84:          /* STY $nn */
                ST(reg_y, SET_ZERO, 2);
                break;

            case 0x85:          /* STA $nn */
                ST(reg_a_read, SET_ZERO, 2);
                break;

            case 0x86:          /* STX $nn */
                ST(reg_x, SET_ZERO, 2);
                break;

            case 0x87:          /* SAX $nn */
                ST(reg_a_read & reg_x, SET_ZERO, 2);
                break;

            case 0x88:          /* DEY */
                DEY();
                break;

            case 0x8a:          /* TXA */
                TXA();
                break;

            case 0x8b:          /* ANE #$nn */
                ANE();
                break;

            case 0x8c:          /* STY $nnnn */
                ST(reg_y, SET_ABS, 3);
                break;

            case 0x8d:          /* STA $nnnn */
                ST(reg_a_read, SET_ABS, 3);
                break;

            case 0x8e:          /* STX $nnnn */
                ST(reg_x, SET_ABS, 3);
                break;

            case 0x8f:          /* SAX $nnnn */
                ST(reg_a_read & reg_x, SET_ABS, 3);
                break;

            case 0x90:          /* BCC $nnnn */
                BRANCH(!LOCAL_CARRY());
                break;

            case 0x91:          /* STA ($nn),Y */
                ST(reg_a_read, SET_IND_Y, 2);
                break;

            case 0x93:          /* SHA ($nn),Y */
                SHA_IND_Y();
                break;

            case 0x94:          /* STY $nn,X */
                ST(reg_y, SET_ZERO_X, 2);
                break;

            case 0x95:          /* STA $nn,X */
                ST(reg_a_read, SET_ZERO_X, 2);
                break;

            case 0x96:          /* STX $nn,Y */
                ST(reg_x, SET_ZERO_Y, 2);
                break;

            case 0x97:          /* SAX $nn,Y */
                ST(reg_a_read & reg_x, SET_ZERO_Y, 2);
                break;

            case 0x98:          /* TYA */
                TYA();
                break;

            case 0x99:          /* STA $nnnn,Y */
                ST(reg_a_read, SET_ABS_Y, 3);
                break;

            case 0x9a:          /* TXS */
                TXS();
                break;

            case 0x9b:          /* NOP (SHS) $nnnn,Y */
#ifdef C64DTV
                NOOP(GET_ABS_Y_DUMMY, 3);
#else
                SHS_ABS_Y();
#endif
                break;

            case 0x9c:          /* SHY $nnnn,X */
                SH_ABS_I(reg_y, reg_x);
                break;

            case 0x9d:          /* STA $nnnn,X */
                ST(reg_a_read, SET_ABS_X, 3);
                break;

            case 0x9e:          /* SHX $nnnn,Y */
                SH_ABS_I(reg_x, reg_y);
                break;

            case 0x9f:          /* SHA $nnnn,Y */
                SH_ABS_I(reg_a_read & reg_x, reg_y);
                break;

            case 0xa0:          /* LDY #$nn */
                LD(reg_y, GET_IMM, 2);
                break;

            case 0xa1:          /* LDA ($nn,X) */
                LD(reg_a_write, GET_IND_X, 2);
                break;

            case 0xa2:          /* LDX #$nn */
                LD(reg_x, GET_IMM, 2);
                break;

            case 0xa3:          /* LAX ($nn,X) */
                LAX(GET_IND_X, 2);
                break;

            case 0xa4:          /* LDY $nn */
                LD(reg_y, GET_ZERO, 2);
                break;

            case 0xa5:          /* LDA $nn */
                LD(reg_a_write, GET_ZERO, 2);
                break;

            case 0xa6:          /* LDX $nn */
                LD(reg_x, GET_ZERO, 2);
                break;

            case 0xa7:          /* LAX $nn */
                LAX(GET_ZERO, 2);
                break;

            case 0xa8:          /* TAY */
                TAY();
                break;

            case 0xa9:          /* LDA #$nn */
                LD(reg_a_write, GET_IMM, 2);
                break;

            case 0xaa:          /* TAX */
                TAX();
                break;

            case 0xab:          /* LXA #$nn */
                LXA(p1, 2);
                break;

            case 0xac:          /* LDY $nnnn */
                LD(reg_y, GET_ABS, 3);
                break;

            case 0xad:          /* LDA $nnnn */
                LD(reg_a_write, GET_ABS, 3);
                break;

            case 0xae:          /* LDX $nnnn */
                LD(reg_x, GET_ABS, 3);
                break;

            case 0xaf:          /* LAX $nnnn */
                LAX(GET_ABS, 3);
                break;

            case 0xb0:          /* BCS $nnnn */
                BRANCH(LOCAL_CARRY());
                break;

            case 0xb1:          /* LDA ($nn),Y */
                LD(reg_a_write, GET_IND_Y, 2);
                break;

            case 0xb3:          /* LAX ($nn),Y */
                LAX(GET_IND_Y, 2);
                break;

            case 0xb4:          /* LDY $nn,X */
                LD(reg_y, GET_ZERO_X, 2);
                break;

            case 0xb5:          /* LDA $nn,X */
                LD(reg_a_write, GET_ZERO_X, 2);
                break;

            case 0xb6:          /* LDX $nn,Y */
                LD(reg_x, GET_ZERO_Y, 2);
                break;

            case 0xb7:          /* LAX $nn,Y */
                LAX(GET_ZERO_Y, 2);
                break;

            case 0xb8:          /* CLV */
                CLV();
                break;

            case 0xb9:          /* LDA $nnnn,Y */
                LD(reg_a_write, GET_ABS_Y, 3);
                break;

            case 0xba:          /* TSX */
                TSX();
                break;

            case 0xbb:          /* LAS $nnnn,Y */
                LAS();
                break;

            case 0xbc:          /* LDY $nnnn,X */
                LD(reg_y, GET_ABS_X, 3);
                break;

            case 0xbd:          /* LDA $nnnn,X */
                LD(reg_a_write, GET_ABS_X, 3);
                break;

            case 0xbe:          /* LDX $nnnn,Y */
                LD(reg_x, GET_ABS_Y, 3);
                break;

            case 0xbf:          /* LAX $nnnn,Y */
                LAX(GET_ABS_Y, 3);
                break;

            case 0xc0:          /* CPY #$nn */
                CP(reg_y, GET_IMM, 2);
                break;

            case 0xc1:          /* CMP ($nn,X) */
                CP(reg_a_read, GET_IND_X, 2);
                break;

            case 0xc3:          /* DCP ($nn,X) */
                DCP(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0xc4:          /* CPY $nn */
                CP(reg_y, GET_ZERO, 2);
                break;

            case 0xc5:          /* CMP $nn */
                CP(reg_a_read, GET_ZERO, 2);
                break;

            case 0xc6:          /* DEC $nn */
                DEC(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0xc7:          /* DCP $nn */
                DCP(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0xc8:          /* INY */
                INY();
                break;

            case 0xc9:          /* CMP #$nn */
                CP(reg_a_read, GET_IMM, 2);
                break;

            case 0xca:          /* DEX */
                DEX();
                break;

            case 0xcb:          /* SBX #$nn */
                SBX();
                break;

            case 0xcc:          /* CPY $nnnn */
                CP(reg_y, GET_ABS, 3);
                break;

            case 0xcd:          /* CMP $nnnn */
                CP(reg_a_read, GET_ABS, 3);
                break;

            case 0xce:          /* DEC $nnnn */
                DEC(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0xcf:          /* DCP $nnnn */
                DCP(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0xd0:          /* BNE $nnnn */
                BRANCH(!LOCAL_ZERO());
                break;

            case 0xd1:          /* CMP ($nn),Y */
                CP(reg_a_read, GET_IND_Y, 2);
                break;

            case 0xd3:          /* DCP ($nn),Y */
                DCP(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0xd5:          /* CMP $nn,X */
                CP(reg_a_read, GET_ZERO_X, 2);
                break;

            case 0xd6:          /* DEC $nn,X */
                DEC(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0xd7:          /* DCP $nn,X */
                DCP(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0xd8:          /* CLD */
                CLD();
                break;

            case 0xd9:          /* CMP $nnnn,Y */
                CP(reg_a_read, GET_ABS_Y, 3);
                break;

            case 0xdb:          /* DCP $nnnn,Y */
                DCP(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0xdd:          /* CMP $nnnn,X */
                CP(reg_a_read, GET_ABS_X, 3);
                break;

            case 0xde:          /* DEC $nnnn,X */
                DEC(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0xdf:          /* DCP $nnnn,X */
                DCP(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0xe0:          /* CPX #$nn */
                CP(reg_x, GET_IMM, 2);
                break;

            case 0xe1:          /* SBC ($nn,X) */
                SBC(GET_IND_X, 2);
                break;

            case 0xe3:          /* ISB ($nn,X) */
                ISB(2, GET_IND_X, SET_IND_RMW);
                break;

            case 0xe4:          /* CPX $nn */
                CP(reg_x, GET_ZERO, 2);
                break;

            case 0xe5:          /* SBC $nn */
                SBC(GET_ZERO, 2);
                break;

            case 0xe6:          /* INC $nn */
                INC(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0xe7:          /* ISB $nn */
                ISB(2, GET_ZERO, SET_ZERO_RMW);
                break;

            case 0xe8:          /* INX */
                INX();
                break;

            case 0xe9:          /* SBC #$nn */
            case 0xeb:          /* USBC #$nn (same as SBC) */
                SBC(GET_IMM, 2);
                break;

            case 0xec:          /* CPX $nnnn */
                CP(reg_x, GET_ABS, 3);
                break;

            case 0xed:          /* SBC $nnnn */
                SBC(GET_ABS, 3);
                break;

            case 0xee:          /* INC $nnnn */
                INC(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0xef:          /* ISB $nnnn */
                ISB(3, GET_ABS, SET_ABS_RMW);
                break;

            case 0xf0:          /* BEQ $nnnn */
                BRANCH(LOCAL_ZERO());
                break;

            case 0xf1:          /* SBC ($nn),Y */
                SBC(GET_IND_Y, 2);
                break;

            case 0xf3:          /* ISB ($nn),Y */
                ISB(2, GET_IND_Y_RMW, SET_IND_RMW);
                break;

            case 0xf5:          /* SBC $nn,X */
                SBC(GET_ZERO_X, 2);
                break;

            case 0xf6:          /* INC $nn,X */
                INC(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0xf7:          /* ISB $nn,X */
                ISB(2, GET_ZERO_X, SET_ZERO_X_RMW);
                break;

            case 0xf8:          /* SED */
                SED();
                break;

            case 0xf9:          /* SBC $nnnn,Y */
                SBC(GET_ABS_Y, 3);
                break;

            case 0xfb:          /* ISB $nnnn,Y */
                ISB(3, GET_ABS_Y_RMW, SET_ABS_Y_RMW);
                break;

            case 0xfd:          /* SBC $nnnn,X */
                SBC(GET_ABS_X, 3);
                break;

            case 0xfe:          /* INC $nnnn,X */
                INC(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;

            case 0xff:          /* ISB $nnnn,X */
                ISB(3, GET_ABS_X_RMW, SET_ABS_X_RMW);
                break;
        }
    }
}
