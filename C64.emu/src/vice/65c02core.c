/*
 * 65C02core.c - 65C02/65SC02 emulation core.
 *
 * Written by
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

/* This file is included by the following CPU definition files:
   - drivecpu65c02.c
 */

/* any CPU definition file that includes this file needs to do the following:
 *
 * - define reg_a as 8bit.
 * - define reg_x as 8bit.
 * - define reg_y as 8bit.
 * - define reg_pc as 16bit.
 * - define reg_sp as 8bit.
 * - define reg_p as 8bit
 * - define the cpu being emulated in a var 'cpu_type' (CPU_WDC65C02, CPU_R65C02, CPU_65SC02).
 * - define a function to handle the WDC65C02 STP opcode (WDC_STP(void)).
 * - define a function to handle the WDC65C02 WAI opcode (WDC_WAI(void)).
 *
 */

/* still to check and possibly fix:
 *
 * - BRK does get interrupted by an IRQ/NMI on the 65(S)C02?
 */

#ifndef CPU_STR
#define CPU_STR "65(S)C02 CPU"
#endif

#include "traps.h"

/* To avoid 'magic' numbers, we will use the following defines. */
#define CYCLES_0   0
#define CYCLES_1   1
#define CYCLES_2   2
#define CYCLES_3   3
#define CYCLES_4   4
#define CYCLES_5   5

#define SIZE_1   1
#define SIZE_2   2
#define SIZE_3   3

#define BIT_0   0
#define BIT_1   1
#define BIT_2   2
#define BIT_3   3
#define BIT_4   4
#define BIT_5   5
#define BIT_6   6
#define BIT_7   7

#define IRQ_CYCLES      7
#define NMI_CYCLES      7
#define RESET_CYCLES    6

/* ------------------------------------------------------------------------- */
/* Backup for non-6509 CPUs.  */

#ifndef LOAD_IND
#define LOAD_IND(a)     LOAD(a)
#endif
#ifndef STORE_IND
#define STORE_IND(a, b)  STORE(a, b)
#endif

/* ------------------------------------------------------------------------- */
/* Backup for non-variable cycle CPUs.  */

#ifndef CLK_ADD
#define CLK_ADD(clock, amount) clock += amount
#endif

#ifndef REWIND_FETCH_OPCODE
#define REWIND_FETCH_OPCODE(clock, amount) clock -= amount
#endif

/* ------------------------------------------------------------------------- */
/* Hook for additional delay.  */

#ifndef CPU_DELAY_CLK
#define CPU_DELAY_CLK
#endif

#ifndef CPU_REFRESH_CLK
#define CPU_REFRESH_CLK
#endif

/* ------------------------------------------------------------------------- */

#ifndef CYCLE_EXACT_ALARM
#define PROCESS_ALARMS                                             \
    while (CLK >= alarm_context_next_pending_clk(ALARM_CONTEXT)) { \
        alarm_context_dispatch(ALARM_CONTEXT, CLK);                \
        CPU_DELAY_CLK                                              \
    }
#else
#define PROCESS_ALARMS
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

#ifndef DRIVE_CPU
/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()       \
    do {                         \
        GLOBAL_REGS.pc = reg_pc; \
        GLOBAL_REGS.a = reg_a;   \
        GLOBAL_REGS.x = reg_x;   \
        GLOBAL_REGS.y = reg_y;   \
        GLOBAL_REGS.sp = reg_sp; \
        GLOBAL_REGS.p = reg_p;   \
        GLOBAL_REGS.n = flag_n;  \
        GLOBAL_REGS.z = flag_z;  \
    } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                                 \
    do {                                                   \
        reg_a = GLOBAL_REGS.a;                             \
        reg_x = GLOBAL_REGS.x;                             \
        reg_y = GLOBAL_REGS.y;                             \
        reg_sp = GLOBAL_REGS.sp;                           \
        reg_p = GLOBAL_REGS.p;                             \
        flag_n = GLOBAL_REGS.n;                            \
        flag_z = GLOBAL_REGS.z;                            \
        bank_start = bank_limit = 0; /* prevent caching */ \
        JUMP(GLOBAL_REGS.pc);                              \
    } while (0)
#else  /* DRIVE_CPU */
#define IMPORT_REGISTERS()
#define EXPORT_REGISTERS()
#endif /* !DRIVE_CPU */

/* Stack operations. */

#ifndef PUSH
#define PUSH(val) ((PAGE_ONE)[(reg_sp--)] = ((BYTE)(val)))
#endif
#ifndef PULL
#define PULL()    ((PAGE_ONE)[(++reg_sp)])
#endif

#ifdef DEBUG
#define TRACE_NMI(clk)                        \
    do {                                      \
        if (TRACEFLG) {                       \
            debug_nmi(CPU_INT_STATUS, (clk)); \
        }                                     \
    } while (0)
#define TRACE_IRQ(clk)                        \
    do {                                      \
        if (TRACEFLG) {                       \
            debug_irq(CPU_INT_STATUS, (clk)); \
        }                                     \
    } while (0)
#define TRACE_BRK()                \
    do {                           \
        if (TRACEFLG) {            \
            debug_text("*** BRK"); \
        }                          \
    } while (0)
#else
#define TRACE_NMI(clk)
#define TRACE_IRQ(clk)
#define TRACE_BRK()
#endif

/* Perform the interrupts in `int_kind'.  If we have both NMI and IRQ,
   execute NMI. NMI can _not_ take over an in progress IRQ. */
/* FIXME: LOCAL_STATUS() should check byte ready first.  */
#define DO_INTERRUPT(int_kind)                                                                                \
    do {                                                                                                      \
        BYTE ik = (int_kind);                                                                                 \
                                                                                                              \
        if (ik & (IK_IRQ | IK_IRQPEND | IK_NMI)) {                                                            \
            if ((ik & IK_NMI)                                                                                 \
                 && interrupt_check_nmi_delay(CPU_INT_STATUS, CLK)) {                                         \
                TRACE_NMI(CLK);                                                                               \
                if (monitor_mask[CALLER] & (MI_STEP)) {                                                       \
                    monitor_check_icount_interrupt();                                                         \
                }                                                                                             \
                interrupt_ack_nmi(CPU_INT_STATUS);                                                            \
                if (NMI_CYCLES == 7) {                                                                        \
                    LOAD(reg_pc);   /* dummy reads */                                                         \
                    CLK_ADD(CLK, 1);                                                                          \
                    LOAD(reg_pc);                                                                             \
                    CLK_ADD(CLK, 1);                                                                          \
                }                                                                                             \
                LOCAL_SET_BREAK(0);                                                                           \
                PUSH(reg_pc >> 8);                                                                            \
                PUSH(reg_pc & 0xff);                                                                          \
                CLK_ADD(CLK, 2);                                                                              \
                PUSH(LOCAL_STATUS());                                                                         \
                CLK_ADD(CLK, 1);                                                                              \
                LOCAL_SET_DECIMAL(0);                                                                         \
                LOCAL_SET_INTERRUPT(1);                                                                       \
                JUMP(LOAD_ADDR(0xfffa));                                                                      \
                SET_LAST_OPCODE(0);                                                                           \
                CLK_ADD(CLK, 2);                                                                              \
            }                                                                                                 \
            if ((ik & (IK_IRQ | IK_IRQPEND)) && (!LOCAL_INTERRUPT() || OPINFO_DISABLES_IRQ(LAST_OPCODE_INFO)) \
                    && interrupt_check_irq_delay(CPU_INT_STATUS, CLK)) {                                      \
                TRACE_IRQ(CLK);                                                                               \
                if (monitor_mask[CALLER] & (MI_STEP)) {                                                       \
                    monitor_check_icount_interrupt();                                                         \
                }                                                                                             \
                interrupt_ack_irq(CPU_INT_STATUS);                                                            \
                if (NMI_CYCLES == 7) {                                                                        \
                    LOAD(reg_pc);   /* dummy reads */                                                         \
                    CLK_ADD(CLK, 1);                                                                          \
                    LOAD(reg_pc);                                                                             \
                    CLK_ADD(CLK, 1);                                                                          \
                }                                                                                             \
                LOCAL_SET_BREAK(0);                                                                           \
                PUSH(reg_pc >> 8);                                                                            \
                PUSH(reg_pc & 0xff);                                                                          \
                CLK_ADD(CLK, 2);                                                                              \
                PUSH(LOCAL_STATUS());                                                                         \
                CLK_ADD(CLK, 1);                                                                              \
                LOCAL_SET_DECIMAL(0);                                                                         \
                LOCAL_SET_INTERRUPT(1);                                                                       \
                JUMP(LOAD_ADDR(0xfffe));                                                                      \
                SET_LAST_OPCODE(0);                                                                           \
                CLK_ADD(CLK, 2);                                                                              \
            }                                                                                                 \
        }                                                                                                     \
        if (ik & (IK_TRAP | IK_RESET)) {                                                                      \
            if (ik & IK_TRAP) {                                                                               \
                EXPORT_REGISTERS();                                                                           \
                interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);                                              \
                IMPORT_REGISTERS();                                                                           \
                if (CPU_INT_STATUS->global_pending_int & IK_RESET) {                                          \
                    ik |= IK_RESET;                                                                           \
                }                                                                                             \
            }                                                                                                 \
            if (ik & IK_RESET) {                                                                              \
                interrupt_ack_reset(CPU_INT_STATUS);                                                          \
                cpu_reset();                                                                                  \
                bank_start = bank_limit = 0; /* prevent caching */                                            \
                JUMP(LOAD_ADDR(0xfffc));                                                                      \
                DMA_ON_RESET;                                                                                 \
            }                                                                                                 \
        }                                                                                                     \
        if (ik & (IK_MONITOR | IK_DMA)) {                                                                     \
            if (ik & IK_MONITOR) {                                                                            \
                if (monitor_force_import(CALLER)) {                                                           \
                    IMPORT_REGISTERS();                                                                       \
                }                                                                                             \
                if (monitor_mask[CALLER]) {                                                                   \
                    EXPORT_REGISTERS();                                                                       \
                }                                                                                             \
                if (monitor_mask[CALLER] & (MI_STEP)) {                                                       \
                    monitor_check_icount((WORD)reg_pc);                                                       \
                    IMPORT_REGISTERS();                                                                       \
                }                                                                                             \
                if (monitor_mask[CALLER] & (MI_BREAK)) {                                                      \
                    if (monitor_check_breakpoints(CALLER, (WORD)reg_pc)) {                                    \
                        monitor_startup(CALLER);                                                              \
                        IMPORT_REGISTERS();                                                                   \
                    }                                                                                         \
                }                                                                                             \
                if (monitor_mask[CALLER] & (MI_WATCH)) {                                                      \
                    monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)reg_pc);                                \
                    IMPORT_REGISTERS();                                                                       \
                }                                                                                             \
            }                                                                                                 \
            if (ik & IK_DMA) {                                                                                \
                EXPORT_REGISTERS();                                                                           \
                DMA_FUNC;                                                                                     \
                interrupt_ack_dma(CPU_INT_STATUS);                                                            \
                IMPORT_REGISTERS();                                                                           \
            }                                                                                                 \
        }                                                                                                     \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Addressing modes.  For convenience, page boundary crossing cycles and
   ``idle'' memory reads are handled here as well. */

#define LOAD_ABS(addr) LOAD(addr)

#define LOAD_ABS_X(addr)              \
    ((((addr) & 0xff) + reg_x) > 0xff \
     ? (LOAD(reg_pc + 2),             \
        CLK_ADD(CLK, CYCLES_1),       \
        LOAD((addr) + reg_x))         \
     : LOAD((addr) + reg_x))

#define LOAD_ABS_X_RMW(addr) \
    (LOAD(reg_pc + 2),       \
     CLK_ADD(CLK, CYCLES_1), \
     LOAD((addr) + reg_x))

#define LOAD_ABS_Y(addr)              \
    ((((addr) & 0xff) + reg_y) > 0xff \
     ? (LOAD(reg_pc + 2),             \
        CLK_ADD(CLK, CYCLES_1),       \
        LOAD((addr) + reg_y))         \
     : LOAD((addr) + reg_y))

#define LOAD_INDIRECT(addr) (CLK_ADD(CLK, CYCLES_2), LOAD(LOAD_ZERO_ADDR((addr))))

#define LOAD_IND_X(addr) (CLK_ADD(CLK, CYCLES_3), LOAD(LOAD_ZERO_ADDR((addr) + reg_x)))

#define LOAD_IND_Y(addr)                                                      \
    (CLK_ADD(CLK, CYCLES_2), ((LOAD_ZERO_ADDR((addr)) & 0xff) + reg_y) > 0xff \
     ? (LOAD(reg_pc + 1),                                                     \
        CLK_ADD(CLK, CYCLES_1),                                               \
        LOAD(LOAD_ZERO_ADDR((addr)) + reg_y))                                 \
     : LOAD(LOAD_ZERO_ADDR((addr)) + reg_y))

#define LOAD_ZERO_X(addr) (LOAD_ZERO((addr) + reg_x))

#define LOAD_ZERO_Y(addr) (LOAD_ZERO((addr) + reg_y))

#define LOAD_IND_Y_BANK(addr)                                                 \
    (CLK_ADD(CLK, CYCLES_2), ((LOAD_ZERO_ADDR((addr)) & 0xff) + reg_y) > 0xff \
     ? (LOAD(reg_pc + 1),                                                     \
        CLK_ADD(CLK, CYCLES_1),                                               \
        LOAD_IND(LOAD_ZERO_ADDR((addr)) + reg_y))                             \
     : LOAD_IND(LOAD_ZERO_ADDR((addr)) + reg_y))

#define STORE_ABS(addr, value)  \
    do {                        \
        STORE((addr), (value)); \
    } while (0)

#define STORE_ABS_RRW(addr, value) \
    do {                           \
        LOAD(addr);                \
        CLK_ADD(CLK, CYCLES_1);    \
        STORE(addr, value);        \
        CLK_ADD(CLK, CYCLES_1);    \
    } while (0)

#define STORE_ABS_X(addr, value)        \
    do {                                \
        LOAD(reg_pc - 1);               \
        CLK_ADD(CLK, CYCLES_1);         \
        STORE((addr) + reg_x, (value)); \
        CLK_ADD(CLK, CYCLES_1);         \
    } while (0)

#define STORE_ABS_X_RRW(addr, value)    \
    do {                                \
        LOAD((addr) + reg_x);           \
        CLK_ADD(CLK, CYCLES_1);         \
        STORE((addr) + reg_x, (value)); \
        CLK_ADD(CLK, CYCLES_1);         \
    } while (0)

#define STORE_ABS_Y(addr, value)        \
    do {                                \
        LOAD(reg_pc - 1);               \
        CLK_ADD(CLK, CYCLES_1);         \
        STORE((addr) + reg_y, (value)); \
        CLK_ADD(CLK, CYCLES_1);         \
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

#define ADC(value, clk_inc, pc_inc)                                                      \
    do {                                                                                 \
        unsigned int tmp_value;                                                          \
        unsigned int tmp, tmp2;                                                          \
                                                                                         \
        tmp_value = (value);                                                             \
        CLK_ADD(CLK, (clk_inc));                                                         \
                                                                                         \
        if (LOCAL_DECIMAL()) {                                                           \
            tmp = (reg_a & 0xf) + (tmp_value & 0xf) + LOCAL_CARRY();                     \
            tmp2 = (reg_a & 0xf0) + (tmp_value & 0xf0);                                  \
            if (tmp > 9) {                                                               \
                tmp2 += 0x10;                                                            \
                tmp += 6;                                                                \
            }                                                                            \
            LOCAL_SET_OVERFLOW(~(reg_a ^ tmp_value) & (reg_a ^ tmp) & 0x80);             \
            if (tmp2 > 0x90) {                                                           \
                tmp2 += 0x60;                                                            \
            }                                                                            \
            LOCAL_SET_CARRY(tmp2 & 0xff00);                                              \
            tmp = (tmp & 0xf) + (tmp2 & 0xf0);                                           \
            LOCAL_SET_NZ(tmp);                                                           \
            LOAD(reg_pc + pc_inc - 1);                                                   \
            CLK_ADD(CLK, CYCLES_1);                                                      \
        } else {                                                                         \
            tmp = tmp_value + reg_a + LOCAL_CARRY();                                     \
            LOCAL_SET_NZ(tmp & 0xff);                                                    \
            LOCAL_SET_OVERFLOW(!((reg_a ^ tmp_value) & 0x80) && ((reg_a ^ tmp) & 0x80)); \
            LOCAL_SET_CARRY(tmp > 0xff);                                                 \
        }                                                                                \
        reg_a = tmp;                                                                     \
        INC_PC(pc_inc);                                                                  \
    } while (0)

#define AND(value, clk_inc, pc_inc)      \
    do {                                 \
        reg_a = (BYTE)(reg_a & (value)); \
        LOCAL_SET_NZ(reg_a);             \
        CLK_ADD(CLK, (clk_inc));         \
        INC_PC(pc_inc);                  \
    } while (0)

#define ASL(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp_value, tmp_addr;                 \
                                                          \
        tmp_addr = (addr);                                \
        tmp_value = load_func(tmp_addr);                  \
        LOCAL_SET_CARRY(tmp_value & 0x80);                \
        tmp_value = (tmp_value << 1) & 0xff;              \
        LOCAL_SET_NZ(tmp_value);                          \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc);                            \
        store_func(tmp_addr, tmp_value);                  \
    } while (0)

#define ASL_A()                        \
    do {                               \
        LOCAL_SET_CARRY(reg_a & 0x80); \
        reg_a = reg_a << 1;            \
        LOCAL_SET_NZ(reg_a);           \
        INC_PC(SIZE_1);                \
    } while (0)

#define BBR(bit)                                           \
    do {                                                   \
        unsigned int tmp, tmp_addr;                        \
        unsigned int dest_addr;                            \
        BYTE value;                                        \
                                                           \
        if (cpu_type == CPU_65SC02) {                      \
            NOOP_IMM(SIZE_1);                              \
        } else {                                           \
            tmp_addr = LOAD(reg_pc + 1);                   \
            CLK_ADD(CLK, CYCLES_1);                        \
            value = LOAD(reg_pc + 2);                      \
            CLK_ADD(CLK, CYCLES_1);                        \
            tmp = LOAD_ZERO(tmp_addr) & (1 << bit);        \
            CLK_ADD(CLK, CYCLES_1);                        \
            INC_PC(SIZE_3);                                \
            LOAD(reg_pc);                                  \
            CLK_ADD(CLK, CYCLES_1);                        \
            if (!tmp) {                                    \
                dest_addr = reg_pc + (signed char)(value); \
                OPCODE_DELAYS_INTERRUPT();                 \
                JUMP(dest_addr & 0xffff);                  \
            }                                              \
        }                                                  \
    } while (0)

#define BBS(bit)                                           \
    do {                                                   \
        unsigned int tmp, tmp_addr;                        \
        unsigned int dest_addr;                            \
        BYTE value;                                        \
                                                           \
        if (cpu_type == CPU_65SC02) {                      \
            NOOP_IMM(SIZE_1);                              \
        } else {                                           \
            tmp_addr = LOAD(reg_pc + 1);                   \
            CLK_ADD(CLK, CYCLES_1);                        \
            value = LOAD(reg_pc + 2);                      \
            CLK_ADD(CLK, CYCLES_1);                        \
            tmp = LOAD_ZERO(tmp_addr) & (1 << bit);        \
            CLK_ADD(CLK, CYCLES_1);                        \
            INC_PC(SIZE_3);                                \
            LOAD(reg_pc);                                  \
            CLK_ADD(CLK, CYCLES_1);                        \
                                                           \
            if (tmp) {                                     \
                dest_addr = reg_pc + (signed char)(value); \
                OPCODE_DELAYS_INTERRUPT();                 \
                JUMP(dest_addr & 0xffff);                  \
            }                                              \
        }                                                  \
    } while (0)

#define BIT(value, clk_inc, pc_inc)     \
    do {                                \
        unsigned int tmp;               \
                                        \
        tmp = (value);                  \
        CLK_ADD(CLK, clk_inc);          \
        LOCAL_SET_SIGN(tmp & 0x80);     \
        LOCAL_SET_OVERFLOW(tmp & 0x40); \
        LOCAL_SET_ZERO(!(tmp & reg_a)); \
        INC_PC(pc_inc);                 \
    } while (0)

#define BIT_IMM(value)                  \
    do {                                \
        unsigned int tmp;               \
                                        \
        tmp = (value);                  \
        LOCAL_SET_ZERO(!(tmp & reg_a)); \
        INC_PC(SIZE_2);                 \
    } while (0)

#define BRANCH(cond, value)                            \
    do {                                               \
        unsigned int dest_addr = 0;                    \
        INC_PC(SIZE_2);                                \
                                                       \
        if (cond) {                                    \
            dest_addr = reg_pc + (signed char)(value); \
                                                       \
            LOAD(reg_pc);                              \
            CLK_ADD(CLK, CYCLES_1);                    \
            if ((reg_pc ^ dest_addr) & 0xff00) {       \
                LOAD(reg_pc);                          \
                CLK_ADD(CLK, CYCLES_1);                \
            } else {                                   \
                OPCODE_DELAYS_INTERRUPT();             \
            }                                          \
            JUMP(dest_addr & 0xffff);                  \
        }                                              \
    } while (0)

#define BRK()                    \
    do {                         \
        EXPORT_REGISTERS();      \
        TRACE_BRK();             \
        INC_PC(SIZE_2);          \
        LOCAL_SET_BREAK(1);      \
        PUSH(reg_pc >> 8);       \
        PUSH(reg_pc & 0xff);     \
        CLK_ADD(CLK, CYCLES_2);  \
        PUSH(LOCAL_STATUS());    \
        CLK_ADD(CLK, CYCLES_1);  \
        LOCAL_SET_DECIMAL(0);    \
        LOCAL_SET_INTERRUPT(1);  \
        JUMP(LOAD_ADDR(0xfffe)); \
        CLK_ADD(CLK, CYCLES_2);  \
    } while (0)

#define CLC()               \
    do {                    \
        INC_PC(SIZE_1);     \
        LOCAL_SET_CARRY(0); \
    } while (0)

#define CLD()                 \
    do {                      \
        INC_PC(SIZE_1);       \
        LOCAL_SET_DECIMAL(0); \
    } while (0)

#define CLI()                     \
    do {                          \
        INC_PC(SIZE_1);           \
        if (LOCAL_INTERRUPT()) {  \
            OPCODE_ENABLES_IRQ(); \
        }                         \
        LOCAL_SET_INTERRUPT(0);   \
    } while (0)

#define CLV()                  \
    do {                       \
        INC_PC(SIZE_1);        \
        LOCAL_SET_OVERFLOW(0); \
    } while (0)

#define CMP(value, clk_inc, pc_inc)   \
    do {                              \
        unsigned int tmp;             \
                                      \
        tmp = reg_a - (value);        \
        LOCAL_SET_CARRY(tmp < 0x100); \
        LOCAL_SET_NZ(tmp & 0xff);     \
        CLK_ADD(CLK, (clk_inc));      \
        INC_PC(pc_inc);               \
    } while (0)

#define CPX(value, clk_inc, pc_inc)   \
    do {                              \
        unsigned int tmp;             \
                                      \
        tmp = reg_x - (value);        \
        LOCAL_SET_CARRY(tmp < 0x100); \
        LOCAL_SET_NZ(tmp & 0xff);     \
        CLK_ADD(CLK, (clk_inc));      \
        INC_PC(pc_inc);               \
    } while (0)

#define CPY(value, clk_inc, pc_inc)   \
    do {                              \
        unsigned int tmp;             \
                                      \
        tmp = reg_y - (value);        \
        LOCAL_SET_CARRY(tmp < 0x100); \
        LOCAL_SET_NZ(tmp & 0xff);     \
        CLK_ADD(CLK, (clk_inc));      \
        INC_PC(pc_inc);               \
    } while (0)

#define DEA()                \
    do {                     \
        reg_a--;             \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define DEC(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp, tmp_addr;                       \
                                                          \
        tmp_addr = (addr);                                \
        tmp = load_func(tmp_addr);                        \
        tmp = (tmp - 1) & 0xff;                           \
        LOCAL_SET_NZ(tmp);                                \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, (clk_inc));                          \
        store_func(tmp_addr, tmp);                        \
    } while (0)

#define DEX()                \
    do {                     \
        reg_x--;             \
        LOCAL_SET_NZ(reg_x); \
        INC_PC(SIZE_1);      \
    } while (0)

#define DEY()                \
    do {                     \
        reg_y--;             \
        LOCAL_SET_NZ(reg_y); \
        INC_PC(SIZE_1);      \
    } while (0)

#define EOR(value, clk_inc, pc_inc)      \
    do {                                 \
        reg_a = (BYTE)(reg_a ^ (value)); \
        LOCAL_SET_NZ(reg_a);             \
        CLK_ADD(CLK, (clk_inc));         \
        INC_PC(pc_inc);                  \
    } while (0)

#define INA()                \
    do {                     \
        reg_a++;             \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define INC(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp, tmp_addr;                       \
                                                          \
        tmp_addr = (addr);                                \
        tmp = (load_func(tmp_addr) + 1) & 0xff;           \
        LOCAL_SET_NZ(tmp);                                \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, (clk_inc));                          \
        store_func(tmp_addr, tmp);                        \
    } while (0)

#define INX()                \
    do {                     \
        reg_x++;             \
        LOCAL_SET_NZ(reg_x); \
        INC_PC(SIZE_1);      \
    } while (0)

#define INY()                \
    do {                     \
        reg_y++;             \
        LOCAL_SET_NZ(reg_y); \
        INC_PC(SIZE_1);      \
    } while (0)

/* The 0x02 NOP opcode is also used to patch the ROM.  The function trap_handler()
   returns nonzero if this is not a patch, but a `real' NOP instruction. */

#define NOP_02()                                                  \
    do {                                                          \
        DWORD trap_result;                                        \
        EXPORT_REGISTERS();                                       \
        if (!ROM_TRAP_ALLOWED()                                   \
            || (trap_result = ROM_TRAP_HANDLER()) == (DWORD)-1) { \
            NOOP_IMM(SIZE_2);                                     \
        } else {                                                  \
            if (trap_result) {                                    \
                REWIND_FETCH_OPCODE(CLK, CYCLES_2);               \
                SET_OPCODE(trap_result);                          \
                IMPORT_REGISTERS();                               \
                goto trap_skipped;                                \
            } else {                                              \
                IMPORT_REGISTERS();                               \
            }                                                     \
        }                                                         \
    } while (0)

#define JMP(addr)   \
    do {            \
        JUMP(addr); \
    } while (0)

#define JMP_IND()                                    \
    do {                                             \
        WORD dest_addr;                              \
        dest_addr = LOAD(p2);                        \
        CLK_ADD(CLK, CYCLES_1);                      \
        LOAD(reg_pc + 2);                            \
        CLK_ADD(CLK, CYCLES_1);                      \
        dest_addr |= (LOAD((p2 + 1) & 0xffff) << 8); \
        CLK_ADD(CLK, CYCLES_1);                      \
        JUMP(dest_addr);                             \
    } while (0)

#define JMP_IND_X()                                          \
    do {                                                     \
        WORD dest_addr;                                      \
        dest_addr = LOAD((p2 + reg_x) & 0xffff);             \
        CLK_ADD(CLK, CYCLES_1);                              \
        LOAD(reg_pc + 2);                                    \
        CLK_ADD(CLK, CYCLES_1);                              \
        dest_addr |= (LOAD((p2 + reg_x + 1) & 0xffff) << 8); \
        CLK_ADD(CLK, CYCLES_1);                              \
        JUMP(dest_addr);                                     \
    } while (0)

#define JSR()                                  \
    do {                                       \
        unsigned int tmp_addr;                 \
                                               \
        CLK_ADD(CLK, CYCLES_1);                \
        INC_PC(SIZE_2);                        \
        CLK_ADD(CLK, CYCLES_2);                \
        PUSH(((reg_pc) >> 8) & 0xff);          \
        PUSH((reg_pc) & 0xff);                 \
        tmp_addr = (p1 | (LOAD(reg_pc) << 8)); \
        CLK_ADD(CLK, CYCLES_1);                \
        JUMP(tmp_addr);                        \
    } while (0)

#define LDA(value, clk_inc, pc_inc) \
    do {                            \
        reg_a = (BYTE)(value);      \
        CLK_ADD(CLK, (clk_inc));    \
        LOCAL_SET_NZ(reg_a);        \
        INC_PC(pc_inc);             \
    } while (0)

#define LDX(value, clk_inc, pc_inc) \
    do {                            \
        reg_x = (BYTE)(value);      \
        LOCAL_SET_NZ(reg_x);        \
        CLK_ADD(CLK, (clk_inc));    \
        INC_PC(pc_inc);             \
    } while (0)

#define LDY(value, clk_inc, pc_inc) \
    do {                            \
        reg_y = (BYTE)(value);      \
        LOCAL_SET_NZ(reg_y);        \
        CLK_ADD(CLK, (clk_inc));    \
        INC_PC(pc_inc);             \
    } while (0)

#define LSR(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp, tmp_addr;                       \
                                                          \
        tmp_addr = (addr);                                \
        tmp = load_func(tmp_addr);                        \
        LOCAL_SET_CARRY(tmp & 0x01);                      \
        tmp >>= 1;                                        \
        LOCAL_SET_NZ(tmp);                                \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc);                            \
        store_func(tmp_addr, tmp);                        \
    } while (0)

#define LSR_A()                        \
    do {                               \
        LOCAL_SET_CARRY(reg_a & 0x01); \
        reg_a = reg_a >> 1;            \
        LOCAL_SET_NZ(reg_a);           \
        INC_PC(SIZE_1);                \
    } while (0)

#define ORA(value, clk_inc, pc_inc)      \
    do {                                 \
        reg_a = (BYTE)(reg_a | (value)); \
        LOCAL_SET_NZ(reg_a);             \
        CLK_ADD(CLK, (clk_inc));         \
        INC_PC(pc_inc);                  \
    } while (0)

#define NOOP(clk_inc, pc_inc) (CLK_ADD(CLK, (clk_inc)), INC_PC(pc_inc))

#define NOOP_IMM(pc_inc) INC_PC(pc_inc)

#define NOOP_ZP()               \
    do {                        \
        LOAD_ZERO(p1);          \
        CLK_ADD(CLK, CYCLES_1); \
        INC_PC(SIZE_2);         \
    } while (0)

#define NOOP_ZP_X()             \
    do {                        \
        LOAD_ZERO(p1);          \
        CLK_ADD(CLK, CYCLES_1); \
        LOAD_ZERO(p1 + reg_x);  \
        CLK_ADD(CLK, CYCLES_1); \
        INC_PC(SIZE_2);         \
    } while (0)

#define NOOP_ABS()              \
    do {                        \
        LOAD(p2);               \
        CLK_ADD(CLK, CYCLES_1); \
        INC_PC(SIZE_3);         \
    } while (0)

#define NOOP_5C()               \
    do {                        \
        LOAD(p2);               \
        CLK_ADD(CLK, CYCLES_1); \
        LOAD(reg_pc + 2);       \
        CLK_ADD(CLK, CYCLES_1); \
        LOAD(reg_pc + 2);       \
        CLK_ADD(CLK, CYCLES_1); \
        LOAD(reg_pc + 2);       \
        CLK_ADD(CLK, CYCLES_1); \
        LOAD(reg_pc + 2);       \
        CLK_ADD(CLK, CYCLES_1); \
        INC_PC(SIZE_3);         \
    } while (0)

#define NOP()  NOOP_IMM(SIZE_1)

#define PHA()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_1); \
        PUSH(reg_a);            \
        INC_PC(SIZE_1);         \
    } while (0)

#define PHP()                           \
    do {                                \
        CLK_ADD(CLK, CYCLES_1);         \
        PUSH(LOCAL_STATUS() | P_BREAK); \
        INC_PC(SIZE_1);                 \
    } while (0)

#define PHX()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_1); \
        PUSH(reg_x);            \
        INC_PC(SIZE_1);         \
    } while (0)

#define PHY()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_1); \
        PUSH(reg_y);            \
        INC_PC(SIZE_1);         \
    } while (0)

#define PLA()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_2); \
        reg_a = PULL();         \
        LOCAL_SET_NZ(reg_a);    \
        INC_PC(SIZE_1);         \
    } while (0)

#define PLP()                                                 \
    do {                                                      \
        BYTE s = PULL();                                      \
                                                              \
        if (!(s & P_INTERRUPT) && LOCAL_INTERRUPT()) {        \
            OPCODE_ENABLES_IRQ();                             \
        } else if ((s & P_INTERRUPT) && !LOCAL_INTERRUPT()) { \
            OPCODE_DISABLES_IRQ();                            \
        }                                                     \
        CLK_ADD(CLK, CYCLES_2);                               \
        LOCAL_SET_STATUS(s);                                  \
        INC_PC(SIZE_1);                                       \
    } while (0)

#define PLX()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_2); \
        reg_x = PULL();         \
        LOCAL_SET_NZ(reg_x);    \
        INC_PC(SIZE_1);         \
    } while (0)

#define PLY()                   \
    do {                        \
        CLK_ADD(CLK, CYCLES_2); \
        reg_y = PULL();         \
        LOCAL_SET_NZ(reg_y);    \
        INC_PC(SIZE_1);         \
    } while (0)

#define RMB(bit)                                     \
    do {                                             \
        unsigned int tmp, tmp_addr;                  \
                                                     \
        if (cpu_type == CPU_65SC02) {                \
            NOOP_IMM(SIZE_1);                        \
        } else {                                     \
            tmp_addr = LOAD(reg_pc + 1);             \
            CLK_ADD(CLK, CYCLES_1);                  \
            LOAD_ZERO(tmp_addr);                     \
            CLK_ADD(CLK, CYCLES_1);                  \
            tmp = LOAD_ZERO(tmp_addr) & ~(1 << bit); \
            CLK_ADD(CLK, CYCLES_1);                  \
            INC_PC(SIZE_2);                          \
            STORE_ZERO(tmp_addr, tmp);               \
            CLK_ADD(CLK, 1);                         \
        }                                            \
    } while (0)

#define ROL(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp, tmp_addr;                       \
                                                          \
        tmp_addr = (addr);                                \
        tmp = load_func(tmp_addr);                        \
        tmp = (tmp << 1) | LOCAL_CARRY();                 \
        LOCAL_SET_CARRY(tmp & 0x100);                     \
        LOCAL_SET_NZ(tmp & 0xff);                         \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc);                            \
        store_func(tmp_addr, tmp);                        \
    } while (0)

#define ROL_A()                        \
    do {                               \
        unsigned int tmp = reg_a << 1; \
                                       \
        reg_a = tmp | LOCAL_CARRY();   \
        LOCAL_SET_CARRY(tmp & 0x100);  \
        LOCAL_SET_NZ(reg_a);           \
        INC_PC(SIZE_1);                \
    } while (0)

#define ROR(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int src, tmp_addr;                       \
                                                          \
        tmp_addr = (addr);                                \
        src = load_func(tmp_addr);                        \
        if (LOCAL_CARRY()) {                              \
            src |= 0x100;                                 \
        }                                                 \
        LOCAL_SET_CARRY(src & 0x01);                      \
        src >>= 1;                                        \
        LOCAL_SET_NZ(src);                                \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, (clk_inc));                          \
        store_func(tmp_addr, src);                        \
    } while (0)

#define ROR_A()                              \
    do {                                     \
        BYTE tmp = reg_a;                    \
                                             \
        reg_a = (reg_a >> 1) | (reg_p << 7); \
        LOCAL_SET_CARRY(tmp & 0x01);         \
        LOCAL_SET_NZ(reg_a);                 \
        INC_PC(SIZE_1);                      \
    } while (0)

/* RTI does must not use `OPCODE_ENABLES_IRQ()' even if the I flag changes
   from 1 to 0 because the value of I is set 3 cycles before the end of the
   opcode, and thus the 6510 has enough time to call the interrupt routine as
   soon as the opcode ends, if necessary.  */
#define RTI()                        \
    do {                             \
        WORD tmp;                    \
                                     \
        CLK_ADD(CLK, CYCLES_4);      \
        tmp = (WORD)PULL();          \
        LOCAL_SET_STATUS((BYTE)tmp); \
        tmp = (WORD)PULL();          \
        tmp |= (WORD)PULL() << 8;    \
        JUMP(tmp);                   \
    } while (0)

#define RTS()                      \
    do {                           \
        WORD tmp;                  \
                                   \
        CLK_ADD(CLK, CYCLES_3);    \
        tmp = PULL();              \
        tmp = tmp | (PULL() << 8); \
        LOAD(tmp);                 \
        CLK_ADD(CLK, CYCLES_1);    \
        tmp++;                     \
        JUMP(tmp);                 \
    } while (0)

#define SBC(value, clk_inc, pc_inc)                                               \
    do {                                                                          \
        WORD src, tmp;                                                            \
                                                                                  \
        src = (WORD)(value);                                                      \
        CLK_ADD(CLK, (clk_inc));                                                  \
        if (LOCAL_DECIMAL()) {                                                    \
            tmp = reg_a - (src & 0xf) + LOCAL_CARRY() - 1;                        \
            if ((tmp & 0xf) > (reg_a & 0xf)) {                                    \
                tmp -= 6;                                                         \
            }                                                                     \
            tmp -= (src & 0xf0);                                                  \
            if ((tmp & 0xf0) > (reg_a & 0xf0)) {                                  \
                tmp -= 0x60;                                                      \
            }                                                                     \
            LOCAL_SET_OVERFLOW(!(tmp > reg_a));                                   \
            LOCAL_SET_CARRY(!(tmp > reg_a));                                      \
            LOCAL_SET_NZ(tmp & 0xff);                                             \
            LOAD(reg_pc + pc_inc - 1);                                            \
            CLK_ADD(CLK, CYCLES_1);                                               \
        } else {                                                                  \
            tmp = reg_a - src - ((LOCAL_CARRY()) ? 0 : 1);                        \
            LOCAL_SET_NZ(tmp & 0xff);                                             \
            LOCAL_SET_CARRY(tmp < 0x100);                                         \
            LOCAL_SET_OVERFLOW(((reg_a ^ tmp) & 0x80) && ((reg_a ^ src) & 0x80)); \
        }                                                                         \
        reg_a = (BYTE)tmp;                                                        \
        INC_PC(pc_inc);                                                           \
    } while (0)

#undef SEC    /* defined in time.h on SunOS. */
#define SEC()               \
    do {                    \
        LOCAL_SET_CARRY(1); \
        INC_PC(SIZE_1);     \
    } while (0)

#define SED()                 \
    do {                      \
        LOCAL_SET_DECIMAL(1); \
        INC_PC(SIZE_1);       \
    } while (0)

#define SEI()                      \
    do {                           \
        if (!LOCAL_INTERRUPT()) {  \
            OPCODE_DISABLES_IRQ(); \
        }                          \
        LOCAL_SET_INTERRUPT(1);    \
        INC_PC(SIZE_1);            \
    } while (0)

#define SMB(bit)                                    \
    do {                                            \
        unsigned tmp, tmp_addr;                     \
                                                    \
        if (cpu_type == CPU_65SC02) {               \
            NOOP_IMM(SIZE_1);                       \
        } else {                                    \
            tmp_addr = LOAD(reg_pc + 1);            \
            CLK_ADD(CLK, CYCLES_1);                 \
            LOAD(tmp_addr);                         \
            CLK_ADD(CLK, CYCLES_1);                 \
            tmp = LOAD_ZERO(tmp_addr) | (1 << bit); \
            CLK_ADD(CLK, CYCLES_1);                 \
            INC_PC(SIZE_2);                         \
            STORE_ZERO(tmp_addr, tmp);              \
            CLK_ADD(CLK, CYCLES_1);                 \
        }                                           \
    } while (0)

#define STA(addr, clk_inc1, clk_inc2, pc_inc, store_func) \
    do {                                                  \
        unsigned int tmp;                                 \
                                                          \
        CLK_ADD(CLK, (clk_inc1));                         \
        tmp = (addr);                                     \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc2 - 1);                       \
        store_func(tmp, reg_a);                           \
        CLK_ADD(CLK, CYCLES_1);                           \
    } while (0)

#define STA_ZERO(addr, clk_inc, pc_inc) \
    do {                                \
        CLK_ADD(CLK, (clk_inc));        \
        STORE_ZERO((addr), reg_a);      \
        INC_PC(pc_inc);                 \
    } while (0)

#define STA_IND_Y(addr)                \
    do {                               \
        unsigned int tmp;              \
                                       \
        CLK_ADD(CLK, CYCLES_2);        \
        tmp = LOAD_ZERO_ADDR(addr);    \
        LOAD(reg_pc + 1);              \
        CLK_ADD(CLK, CYCLES_1);        \
        INC_PC(SIZE_2);                \
        STORE_IND(tmp + reg_y, reg_a); \
        CLK_ADD(CLK, CYCLES_1);        \
    } while (0)

#define STP()                            \
    do {                                 \
        if (cpu_type == CPU_WDC65C02) {  \
            WDC_STP();                   \
        } else {                         \
            REWIND_FETCH_OPCODE(CLK, 2); \
            NOOP_IMM(SIZE_1);            \
        }                                \
    } while (0)

#define STX(addr)               \
    do {                        \
        unsigned int tmp;       \
                                \
        tmp = (addr);           \
        INC_PC(SIZE_3);         \
        STORE(tmp, reg_x);      \
        CLK_ADD(CLK, CYCLES_1); \
    } while (0)

#define STX_ZERO(addr, clk_inc, pc_inc) \
    do {                                \
        CLK_ADD(CLK, (clk_inc));        \
        STORE_ZERO((addr), reg_x);      \
        INC_PC(pc_inc);                 \
    } while (0)

#define STY(addr)               \
    do {                        \
        unsigned int tmp;       \
                                \
        tmp = (addr);           \
        INC_PC(SIZE_3);         \
        STORE(tmp, reg_y);      \
        CLK_ADD(CLK, CYCLES_1); \
    } while (0)

#define STY_ZERO(addr, clk_inc, pc_inc) \
    do {                                \
        CLK_ADD(CLK, (clk_inc));        \
        STORE_ZERO((addr), reg_y);      \
        INC_PC(pc_inc);                 \
    } while (0)

#define STZ(addr, clk_inc, pc_inc, store_func) \
    do {                                       \
        unsigned int tmp;                      \
                                               \
        tmp = (addr);                          \
        INC_PC(pc_inc);                        \
        CLK_ADD(CLK, clk_inc);                 \
        store_func(tmp, 0);                    \
    } while (0)

#define STZ_ZERO(addr, clk_inc, pc_inc) \
    do {                                \
        CLK_ADD(CLK, (clk_inc));        \
        STORE_ZERO((addr), 0);          \
        INC_PC(pc_inc);                 \
    } while (0)

#define TAX()                \
    do {                     \
        reg_x = reg_a;       \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define TAY()                \
    do {                     \
        reg_y = reg_a;       \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define TRB(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp_value, tmp_addr;                 \
                                                          \
        tmp_addr = (addr);                                \
        tmp_value = load_func(tmp_addr);                  \
        LOCAL_SET_ZERO(!(tmp_value & reg_a));             \
        tmp_value &= (~reg_a);                            \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc);                            \
        store_func(tmp_addr, tmp_value);                  \
    } while (0)

#define TSB(addr, clk_inc, pc_inc, load_func, store_func) \
    do {                                                  \
        unsigned int tmp_value, tmp_addr;                 \
                                                          \
        tmp_addr = (addr);                                \
        tmp_value = load_func(tmp_addr);                  \
        LOCAL_SET_ZERO(!(tmp_value & reg_a));             \
        tmp_value |= reg_a;                               \
        INC_PC(pc_inc);                                   \
        CLK_ADD(CLK, clk_inc);                            \
        store_func(tmp_addr, tmp_value);                  \
    } while (0)

#define TSX()                 \
    do {                      \
        reg_x = reg_sp;       \
        LOCAL_SET_NZ(reg_sp); \
        INC_PC(SIZE_1);       \
    } while (0)

#define TXA()                \
    do {                     \
        reg_a = reg_x;       \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define TXS()           \
    do {                \
        reg_sp = reg_x; \
        INC_PC(SIZE_1); \
    } while (0)

#define TYA()                \
    do {                     \
        reg_a = reg_y;       \
        LOCAL_SET_NZ(reg_a); \
        INC_PC(SIZE_1);      \
    } while (0)

#define WAI()                            \
    do {                                 \
        if (cpu_type == CPU_WDC65C02) {  \
            WDC_WAI();                   \
        } else {                         \
            REWIND_FETCH_OPCODE(CLK, 2); \
            NOOP_IMM(SIZE_1);            \
        }                                \
    } while (0)

/* ------------------------------------------------------------------------- */

/* These tables have a different meaning than for the 6502, it represents
   the amount of extra fetches to the opcode fetch.
 */
 static const BYTE fetch_tab[] = {
            /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
    /* $00 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $00 */
    /* $10 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $10 */
    /* $20 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $20 */
    /* $30 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $30 */
    /* $40 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $40 */
    /* $50 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $50 */
    /* $60 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $60 */
    /* $70 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $70 */
    /* $80 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $80 */
    /* $90 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $90 */ 
    /* $A0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $A0 */
    /* $B0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0, /* $B0 */
    /* $C0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 2, 2, 2, 2, 0, /* $C0 */
    /* $D0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 2, 2, 2, 2, 0, /* $D0 */
    /* $E0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, /* $E0 */
    /* $F0 */  1, 1, 1, 0, 1, 1, 1, 0, 1, 2, 1, 0, 2, 2, 2, 0  /* $F0 */
};

#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS

#define opcode_t DWORD

#define FETCH_OPCODE(o)                                        \
    do {                                                       \
        if (((int)reg_pc) < bank_limit) {                      \
            o = (*((DWORD *)(bank_base + reg_pc)) & 0xffffff); \
            CLK_ADD(CLK, CYCLES_1);                            \
            if (fetch_tab[o & 0xff]) {                         \
                CLK_ADD(CLK, fetch_tab[o & 0xff]);             \
            }                                                  \
        } else {                                               \
            o = LOAD(reg_pc);                                  \
            CLK_ADD(CLK, CYCLES_1);                            \
            if (fetch_tab[o & 0xff]) {                         \
                o |= LOAD(reg_pc + 1) << 8;                    \
                CLK_ADD(CLK, CYCLES_1);                        \
                if (fetch_tab[o & 0xff] - 1) {                 \
                    o |= (LOAD(reg_pc + 2) << 16);             \
                    CLK_ADD(CLK, CYCLES_1);                    \
                }                                              \
            }                                                  \
        }                                                      \
    } while (0)

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 (opcode >> 8)

#else /* WORDS_BIGENDIAN || !ALLOW_UNALIGNED_ACCESS */

#define opcode_t         \
    struct {             \
        BYTE ins;        \
        union {          \
            BYTE op8[2]; \
            WORD op16;   \
        } op;            \
    }

#define FETCH_OPCODE(o)                                                                   \
    do {                                                                                  \
        if (((int)reg_pc) < bank_limit) {                                                 \
            (o).ins = *(bank_base + reg_pc);                                              \
            (o).op.op16 = (*(bank_base + reg_pc + 1) | (*(bank_base + reg_pc + 2) << 8)); \
            CLK_ADD(CLK, CYCLES_1);                                                       \
            if (fetch_tab[(o).ins]) {                                                     \
                CLK_ADD(CLK, fetch_tab[(o).ins]);                                         \
            }                                                                             \
        } else {                                                                          \
            (o).ins = LOAD(reg_pc);                                                       \
            CLK_ADD(CLK, CYCLES_1);                                                       \
            if (fetch_tab[(o).ins]) {                                                     \
                (o).op.op16 = LOAD(reg_pc + 1);                                           \
                CLK_ADD(CLK, CYCLES_1);                                                   \
                if (fetch_tab[(o).ins - 1]) {                                             \
                    (o).op.op16 |= (LOAD(reg_pc + 2) << 8);                               \
                    CLK_ADD(CLK, CYCLES_1);                                               \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
    } while (0)

#define p0 (opcode.ins)
#define p2 (opcode.op.op16)

#ifdef WORDS_BIGENDIAN
#  define p1 (opcode.op.op8[1])
#else
#  define p1 (opcode.op.op8[0])
#endif

#endif /* !WORDS_BIGENDIAN */

/*  SET_OPCODE for traps */
#if !defined WORDS_BIGENDIAN && defined ALLOW_UNALIGNED_ACCESS
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

/* ------------------------------------------------------------------------ */

/* Here, the CPU is emulated. */

{
    CPU_DELAY_CLK;

    PROCESS_ALARMS;

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
            CPU_DELAY_CLK

                PROCESS_ALARMS                                        \
        }
    }

    {
        opcode_t opcode;
#ifdef DEBUG
        CLOCK debug_clk;
#ifdef DRIVE_CPU
        debug_clk = CLK;
#else
        debug_clk = maincpu_clk;
#endif
#endif

#ifdef FEATURE_CPUMEMHISTORY
#ifndef DRIVE_CPU
        memmap_state |= (MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif
#endif
        SET_LAST_ADDR(reg_pc);
        FETCH_OPCODE(opcode);

#ifdef FEATURE_CPUMEMHISTORY
#ifndef DRIVE_CPU
        /* HACK to cope with FETCH_OPCODE optimization in x64 */
        if (((int)reg_pc) < bank_limit) {
            memmap_mem_read(reg_pc);
        }
#endif
        if (p0 == 0x20) {
            monitor_cpuhistory_store(reg_pc, p0, p1, LOAD(reg_pc + 2), reg_a, reg_x, reg_y, reg_sp, LOCAL_STATUS());
        } else {
            monitor_cpuhistory_store(reg_pc, p0, p1, p2 >> 8, reg_a, reg_x, reg_y, reg_sp, LOCAL_STATUS());
        }
        memmap_state &= ~(MEMMAP_STATE_INSTR | MEMMAP_STATE_OPCODE);
#endif

#ifdef DEBUG
#ifdef DRIVE_CPU
        if (TRACEFLG) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8);

            debug_drive((DWORD)(reg_pc), debug_clk,
                        mon_disassemble_to_string(e_disk8_space,
                                                  reg_pc, op,
                                                  lo, hi, 0, 1, "R65(SC)02"),
                        reg_a, reg_x, reg_y, reg_sp, drv->mynumber + 8);
        }
#else
        if (TRACEFLG) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8);

            if (op == 0x20) {
                hi = LOAD(reg_pc + 2);
            }

            debug_maincpu((DWORD)(reg_pc), debug_clk,
                          mon_disassemble_to_string(e_comp_space,
                                                    reg_pc, op,
                                                    lo, hi, 0, 1, "65(SC)02"),
                          reg_a, reg_x, reg_y, reg_sp);
        }
        if (debug.perform_break_into_monitor) {
            monitor_startup_trap();
            debug.perform_break_into_monitor = 0;
        }
#endif
#endif

trap_skipped:
        SET_LAST_OPCODE(p0);

        switch (p0) {
            default:            /* 1 byte, 1 cycle NOP */
                NOOP_IMM(SIZE_1);
                break;

            case 0x22:          /* NOP #$nn */
            case 0x42:          /* NOP #$nn */
            case 0x62:          /* NOP #$nn */
            case 0x82:          /* NOP #$nn */
            case 0xc2:          /* NOP #$nn */
            case 0xe2:          /* NOP #$nn */
                NOOP_IMM(SIZE_2);
                break;

            case 0x44:          /* NOP $nn */
                NOOP_ZP();
                break;

            case 0x54:          /* NOP $nn,X */
            case 0xd4:          /* NOP $nn,X */
            case 0xf4:          /* NOP $nn,X */
                NOOP_ZP_X();
                break;

            case 0xdc:          /* NOP $nnnn */
            case 0xfc:          /* NOP $nnnn */
                NOOP_ABS();
                break;

            case 0x5c:          /* NOP $nnnn + 4 (FIXME: correct ??) */
                NOOP_5C();
                break;

            case 0x00:          /* BRK */
                BRK();
                break;

            case 0x01:          /* ORA ($nn,X) */
                ORA(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0x02:          /* NOP #$nn - also used for traps */
                STATIC_ASSERT(TRAP_OPCODE == 0x02);
                NOP_02();
                break;

            case 0x04:          /* TSB $nn */
                TSB(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x05:          /* ORA $nn */
                ORA(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0x06:          /* ASL $nn */
                ASL(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x07:          /* RMB0 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_0);
                break;

            case 0x08:          /* PHP */
                PHP();
                break;

            case 0x09:          /* ORA #$nn */
                ORA(p1, CYCLES_0, SIZE_2);
                break;

            case 0x0a:          /* ASL A */
                ASL_A();
                break;

            case 0x0c:          /* TSB $nnnn */
                TSB(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x0d:          /* ORA $nnnn */
                ORA(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0x0e:          /* ASL $nnnn */
                ASL(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x0f:          /* BBR0 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_0);
                break;

            case 0x10:          /* BPL $nnnn */
                BRANCH(!LOCAL_SIGN(), p1);
                break;

            case 0x11:          /* ORA ($nn),Y */
                ORA(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0x12:          /* ORA ($nn) */
                ORA(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0x14:          /* TRB $nn */
                TRB(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x15:          /* ORA $nn,X */
                ORA(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0x16:          /* ASL $nn,X */
                ASL((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x17:          /* RMB1 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_1);
                break;

            case 0x18:          /* CLC */
                CLC();
                break;

            case 0x19:          /* ORA $nnnn,Y */
                ORA(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0x1a:          /* INA */
                INA();
                break;

            case 0x1c:          /* TRB $nnnn */
                TRB(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x1d:          /* ORA $nnnn,X */
                ORA(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0x1e:          /* ASL $nnnn,X */
                ASL(p2, CYCLES_1, SIZE_3, LOAD_ABS_X, STORE_ABS_X_RRW);
                break;

            case 0x1f:          /* BBR1 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_1);
                break;

            case 0x20:          /* JSR $nnnn */
                JSR();
                break;

            case 0x21:          /* AND ($nn,X) */
                AND(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0x24:          /* BIT $nn */
                BIT(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0x25:          /* AND $nn */
                AND(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0x26:          /* ROL $nn */
                ROL(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x27:          /* RMB2 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_2);
                break;

            case 0x28:          /* PLP */
                PLP();
                break;

            case 0x29:          /* AND #$nn */
                AND(p1, CYCLES_0, SIZE_2);
                break;

            case 0x2a:          /* ROL A */
                ROL_A();
                break;

            case 0x2c:          /* BIT $nnnn */
                BIT(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0x2d:          /* AND $nnnn */
                AND(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0x2e:          /* ROL $nnnn */
                ROL(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x2f:          /* BBR2 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_2);
                break;

            case 0x30:          /* BMI $nnnn */
                BRANCH(LOCAL_SIGN(), p1);
                break;

            case 0x31:          /* AND ($nn),Y */
                AND(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0x32:          /* AND ($nn) */
                AND(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0x34:          /* BIT $nn,X */
                BIT(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0x35:          /* AND $nn,X */
                AND(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0x36:          /* ROL $nn,X */
                ROL((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x37:          /* RMB3 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_3);
                break;

            case 0x38:          /* SEC */
                SEC();
                break;

            case 0x39:          /* AND $nnnn,Y */
                AND(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0x3a:          /* DEA */
                DEA();
                break;

            case 0x3c:          /* BIT $nnnn,X */
                BIT(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0x3d:          /* AND $nnnn,X */
                AND(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0x3e:          /* ROL $nnnn,X */
                ROL(p2, CYCLES_1, SIZE_3, LOAD_ABS_X, STORE_ABS_X_RRW);
                break;

            case 0x3f:          /* BBR3 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_3);
                break;

            case 0x40:          /* RTI */
                RTI();
                break;

            case 0x41:          /* EOR ($nn,X) */
                EOR(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0x45:          /* EOR $nn */
                EOR(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0x46:          /* LSR $nn */
                LSR(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x47:          /* RMB4 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_4);
                break;

            case 0x48:          /* PHA */
                PHA();
                break;

            case 0x49:          /* EOR #$nn */
                EOR(p1, CYCLES_0, SIZE_2);
                break;

            case 0x4a:          /* LSR A */
                LSR_A();
                break;

            case 0x4c:          /* JMP $nnnn */
                JMP(p2);
                break;

            case 0x4d:          /* EOR $nnnn */
                EOR(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0x4e:          /* LSR $nnnn */
                LSR(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x4f:          /* BBR4 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_4);
                break;

            case 0x50:          /* BVC $nnnn */
                BRANCH(!LOCAL_OVERFLOW(), p1);
                break;

            case 0x51:          /* EOR ($nn),Y */
                EOR(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0x52:          /* EOR ($nn) */
                EOR(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0x55:          /* EOR $nn,X */
                EOR(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0x56:          /* LSR $nn,X */
                LSR((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x57:          /* RMB5 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_5);
                break;

            case 0x58:          /* CLI */
                CLI();
                break;

            case 0x59:          /* EOR $nnnn,Y */
                EOR(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0x5a:          /* PHY */
                PHY();
                break;

            case 0x5d:          /* EOR $nnnn,X */
                EOR(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0x5e:          /* LSR $nnnn,X */
                LSR(p2, CYCLES_1, SIZE_3, LOAD_ABS_X, STORE_ABS_X_RRW);
                break;

            case 0x5f:          /* BBR5 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_5);
                break;

            case 0x60:          /* RTS */
                RTS();
                break;

            case 0x61:          /* ADC ($nn,X) */
                ADC(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0x64:          /* STZ $nn */
                STZ_ZERO(p1, CYCLES_1, SIZE_2);
                break;

            case 0x65:          /* ADC $nn */
                ADC(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0x66:          /* ROR $nn */
                ROR(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x67:          /* RMB6 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_6);
                break;

            case 0x68:          /* PLA */
                PLA();
                break;

            case 0x69:          /* ADC #$nn */
                ADC(p1, CYCLES_0, SIZE_2);
                break;

            case 0x6a:          /* ROR A */
                ROR_A();
                break;

            case 0x6c:          /* JMP ($nnnn) */
                JMP_IND();
                break;

            case 0x6d:          /* ADC $nnnn */
                ADC(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0x6e:          /* ROR $nnnn */
                ROR(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0x6f:          /* BBR6 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_6);
                break;

            case 0x70:          /* BVS $nnnn */
                BRANCH(LOCAL_OVERFLOW(), p1);
                break;

            case 0x71:          /* ADC ($nn),Y */
                ADC(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0x72:          /* ADC ($nn) */
                ADC(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0x74:          /* STZ $nn,X */
                STZ_ZERO(p1 + reg_x, CYCLES_2, SIZE_2);
                break;

            case 0x75:          /* ADC $nn,X */
                ADC(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0x76:          /* ROR $nn,X */
                ROR((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0x77:          /* RMB7 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                RMB(BIT_7);
                break;

            case 0x78:          /* SEI */
                SEI();
                break;

            case 0x79:          /* ADC $nnnn,Y */
                ADC(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0x7a:          /* PLY */
                PLY();
                break;

            case 0x7c:          /* JMP ($nnnn,X) */
                JMP_IND_X();
                break;

            case 0x7d:          /* ADC $nnnn,X */
                ADC(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0x7e:          /* ROR $nnnn,X */
                ROR(p2, CYCLES_1, SIZE_3, LOAD_ABS_X, STORE_ABS_X_RRW);
                break;

            case 0x7f:          /* BBR7 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBR(BIT_7);
                break;

            case 0x80:          /* BRA $nnnn */
                BRANCH(1, p1);
                break;

            case 0x81:          /* STA ($nn,X) */
                STA(LOAD_ZERO_ADDR(p1 + reg_x), CYCLES_3, CYCLES_1, SIZE_2, STORE_ABS);
                break;

            case 0x84:          /* STY $nn */
                STY_ZERO(p1, CYCLES_1, SIZE_2);
                break;

            case 0x85:          /* STA $nn */
                STA_ZERO(p1, CYCLES_1, SIZE_2);
                break;

            case 0x86:          /* STX $nn */
                STX_ZERO(p1, CYCLES_1, SIZE_2);
                break;

            case 0x87:          /* SMB0 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_0);
                break;

            case 0x88:          /* DEY */
                DEY();
                break;

            case 0x89:          /* BIT #$nn */
                BIT_IMM(p1);
                break;

            case 0x8a:          /* TXA */
                TXA();
                break;

            case 0x8c:          /* STY $nnnn */
                STY(p2);
                break;

            case 0x8d:          /* STA $nnnn */
                STA(p2, CYCLES_0, CYCLES_1, SIZE_3, STORE_ABS);
                break;

            case 0x8e:          /* STX $nnnn */
                STX(p2);
                break;

            case 0x8f:          /* BBS0 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_0);
                break;

            case 0x90:          /* BCC $nnnn */
                BRANCH(!LOCAL_CARRY(), p1);
                break;

            case 0x91:          /* STA ($nn),Y */
                STA_IND_Y(p1);
                break;

            case 0x92:          /* STA ($nn) */
                STA(LOAD_ZERO_ADDR(p1), CYCLES_2, CYCLES_1, SIZE_2, STORE_ABS);
                break;

            case 0x94:          /* STY $nn,X */
                STY_ZERO(p1 + reg_x, CYCLES_2, SIZE_2);
                break;

            case 0x95:          /* STA $nn,X */
                STA_ZERO(p1 + reg_x, CYCLES_2, SIZE_2);
                break;

            case 0x96:          /* STX $nn,Y */
                STX_ZERO(p1 + reg_y, CYCLES_2, SIZE_2);
                break;

            case 0x97:          /* SMB1 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_1);
                break;

            case 0x98:          /* TYA */
                TYA();
                break;

            case 0x99:          /* STA $nnnn,Y */
                STA(p2, CYCLES_0, CYCLES_0, SIZE_3, STORE_ABS_Y);
                break;

            case 0x9a:          /* TXS */
                TXS();
                break;

            case 0x9c:          /* STZ $nnnn */
                STZ(p2, CYCLES_1, SIZE_3, STORE_ABS);
                break;

            case 0x9d:          /* STA $nnnn,X */
                STA(p2, CYCLES_0, CYCLES_0, SIZE_3, STORE_ABS_X);
                break;

            case 0x9e:          /* STZ $nnnn,X */
                STZ(p2, CYCLES_0, SIZE_3, STORE_ABS_X);
                break;

            case 0x9f:          /* BBS1 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_1);
                break;

            case 0xa0:          /* LDY #$nn */
                LDY(p1, CYCLES_0, SIZE_2);
                break;

            case 0xa1:          /* LDA ($nn,X) */
                LDA(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0xa2:          /* LDX #$nn */
                LDX(p1, CYCLES_0, SIZE_2);
                break;

            case 0xa4:          /* LDY $nn */
                LDY(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xa5:          /* LDA $nn */
                LDA(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xa6:          /* LDX $nn */
                LDX(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xa7:          /* SMB2 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_2);
                break;

            case 0xa8:          /* TAY */
                TAY();
                break;

            case 0xa9:          /* LDA #$nn */
                LDA(p1, CYCLES_0, SIZE_2);
                break;

            case 0xaa:          /* TAX */
                TAX();
                break;

            case 0xac:          /* LDY $nnnn */
                LDY(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xad:          /* LDA $nnnn */
                LDA(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xae:          /* LDX $nnnn */
                LDX(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xaf:          /* BBS2 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_2);
                break;

            case 0xb0:          /* BCS $nnnn */
                BRANCH(LOCAL_CARRY(), p1);
                break;

            case 0xb1:          /* LDA ($nn),Y */
                LDA(LOAD_IND_Y_BANK(p1), CYCLES_1, SIZE_2);
                break;

            case 0xb2:          /* LDA ($nn) */
                LDA(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0xb4:          /* LDY $nn,X */
                LDY(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0xb5:          /* LDA $nn,X */
                LDA(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0xb6:          /* LDX $nn,Y */
                LDX(LOAD_ZERO_Y(p1), CYCLES_2, SIZE_2);
                break;

            case 0xb7:          /* SMB3 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_3);
                break;

            case 0xb8:          /* CLV */
                CLV();
                break;

            case 0xb9:          /* LDA $nnnn,Y */
                LDA(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0xba:          /* TSX */
                TSX();
                break;

            case 0xbc:          /* LDY $nnnn,X */
                LDY(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0xbd:          /* LDA $nnnn,X */
                LDA(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0xbe:          /* LDX $nnnn,Y */
                LDX(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0xbf:          /* BBS3 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_3);
                break;

            case 0xc0:          /* CPY #$nn */
                CPY(p1, CYCLES_0, SIZE_2);
                break;

            case 0xc1:          /* CMP ($nn,X) */
                CMP(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0xc4:          /* CPY $nn */
                CPY(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xc5:          /* CMP $nn */
                CMP(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xc6:          /* DEC $nn */
                DEC(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0xc7:          /* SMB4 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_4);
                break;

            case 0xc8:          /* INY */
                INY();
                break;

            case 0xc9:          /* CMP #$nn */
                CMP(p1, CYCLES_0, SIZE_2);
                break;

            case 0xca:          /* DEX */
                DEX();
                break;

            case 0xcb:          /* WAI (WDC65C02) / single byte, single cycle NOP (R65C02/65SC02) */
                WAI();
                break;

            case 0xcc:          /* CPY $nnnn */
                CPY(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xcd:          /* CMP $nnnn */
                CMP(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xce:          /* DEC $nnnn */
                DEC(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0xcf:          /* BBS4 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_4);
                break;

            case 0xd0:          /* BNE $nnnn */
                BRANCH(!LOCAL_ZERO(), p1);
                break;

            case 0xd1:          /* CMP ($nn),Y */
                CMP(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0xd2:          /* CMP ($nn) */
                CMP(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0xd5:          /* CMP $nn,X */
                CMP(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0xd6:          /* DEC $nn,X */
                DEC((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0xd7:          /* SMB5 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_5);
                break;

            case 0xd8:          /* CLD */
                CLD();
                break;

            case 0xd9:          /* CMP $nnnn,Y */
                CMP(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0xda:          /* PHX */
                PHX();
                break;

            case 0xdb:          /* STP (WDC65C02) / single byte, single cycle NOP (R65C02/65SC02) */
                STP();
                break;

            case 0xdd:          /* CMP $nnnn,X */
                CMP(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0xde:          /* DEC $nnnn,X */
                DEC(p2, CYCLES_1, SIZE_3, LOAD_ABS_X_RMW, STORE_ABS_X_RRW);
                break;

            case 0xdf:          /* BBS5 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_5);
                break;

            case 0xe0:          /* CPX #$nn */
                CPX(p1, CYCLES_0, SIZE_2);
                break;

            case 0xe1:          /* SBC ($nn,X) */
                SBC(LOAD_IND_X(p1), CYCLES_1, SIZE_2);
                break;

            case 0xe4:          /* CPX $nn */
                CPX(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xe5:          /* SBC $nn */
                SBC(LOAD_ZERO(p1), CYCLES_1, SIZE_2);
                break;

            case 0xe6:          /* INC $nn */
                INC(p1, CYCLES_3, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0xe7:          /* SMB6 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_6);
                break;

            case 0xe8:          /* INX */
                INX();
                break;

            case 0xe9:          /* SBC #$nn */
                SBC(p1, CYCLES_0, SIZE_2);
                break;

            case 0xea:          /* NOP */
                NOP();
                break;

            case 0xec:          /* CPX $nnnn */
                CPX(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xed:          /* SBC $nnnn */
                SBC(LOAD(p2), CYCLES_1, SIZE_3);
                break;

            case 0xee:          /* INC $nnnn */
                INC(p2, CYCLES_1, SIZE_3, LOAD_ABS, STORE_ABS_RRW);
                break;

            case 0xef:          /* BBS6 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_6);
                break;

            case 0xf0:          /* BEQ $nnnn */
                BRANCH(LOCAL_ZERO(), p1);
                break;

            case 0xf1:          /* SBC ($nn),Y */
                SBC(LOAD_IND_Y(p1), CYCLES_1, SIZE_2);
                break;

            case 0xf2:          /* SBC ($nn) */
                SBC(LOAD_INDIRECT(p1), CYCLES_1, SIZE_2);
                break;

            case 0xf5:          /* SBC $nn,X */
                SBC(LOAD_ZERO_X(p1), CYCLES_2, SIZE_2);
                break;

            case 0xf6:          /* INC $nn,X */
                INC((p1 + reg_x) & 0xff, CYCLES_4, SIZE_2, LOAD_ZERO, STORE_ZERO);
                break;

            case 0xf7:          /* SMB7 $nn (65C02) / single byte, single cycle NOP (65SC02) */
                SMB(BIT_7);
                break;

            case 0xf8:          /* SED */
                SED();
                break;

            case 0xf9:          /* SBC $nnnn,Y */
                SBC(LOAD_ABS_Y(p2), CYCLES_1, SIZE_3);
                break;

            case 0xfa:          /* PLX */
                PLX();
                break;

            case 0xfd:          /* SBC $nnnn,X */
                SBC(LOAD_ABS_X(p2), CYCLES_1, SIZE_3);
                break;

            case 0xfe:          /* INC $nnnn,X */
                INC(p2, CYCLES_1, SIZE_3, LOAD_ABS_X_RMW, STORE_ABS_X_RRW);
                break;

            case 0xff:          /* BBS7 $nn,$nnnn (65C02) / single byte, single cycle NOP (65SC02) */
                BBS(BIT_7);
                break;
        }
    }
}
