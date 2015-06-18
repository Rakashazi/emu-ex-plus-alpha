/*
 * 65816core.c - 65816/65802 emulation core.
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

/* This file is included by the following CPU definition files:
 * - main65816cpu.c
 */

/* any CPU definition file that includes this file needs to do the following:
 *
 * - define reg_c as 16bit (8bit on 6502/65C02).
 * - define reg_x as 16bit (8bit on 6502/65C02).
 * - define reg_y as 16bit (8bit on 6502/65C02).
 * - define reg_pbr (Program Bank Register) as 8bit.
 * - define reg_dbr (Data Bank Register) as 8bit.
 * - define reg_dpr (Direct Page Register) as 16bit.
 * - define reg_emul (65C02 Emulation) as int.
 * - define reg_sp as 16bit (8bit on 6502/65C02).
 * - define reg_p as 8bit.
 * - define reg_pc as 16bit.
 * - define a function to handle the STP opcode (STP_65816(void)).
 * - define a function to handle the WAI opcode (WAI_65816(void)).
 * - define a function to handle the COP opcode (COP_65816(BYTE value)).
 *
 * reg_a and reg_b combined is reg_c.
 *
 * the way to define the a, b and c regs is:
 * union regs {
 *     WORD reg_s;
 *     BYTE reg_c[2];
 * } regs65816;
 *
 * #define reg_c regs65816.reg_s
 * #ifndef WORDS_BIGENDIAN
 * #define reg_a regs65816.reg_c[0]
 * #define reg_b regs65816.reg_c[1]
 * #else
 * #define reg_a regs65816.reg_c[1]
 * #define reg_b regs65816.reg_c[0]
 * #endif
 */

#ifndef CPU_STR
#define CPU_STR "65816/65802 CPU"
#endif

#include "traps.h"

/* To avoid 'magic' numbers, the following defines are used. */

#define CYCLES_0   0
#define CYCLES_1   1
#define CYCLES_2   2
#define CYCLES_3   3
#define CYCLES_4   4
#define CYCLES_5   5
#define CYCLES_6   6
#define CYCLES_7   7

#define SIZE_1   1
#define SIZE_2   2
#define SIZE_3   3
#define SIZE_4   4

#define BITS8    1
#define BITS16   0

#define RESET_CYCLES    6

#define SET_OPCODE(o) p0 = o
/* ------------------------------------------------------------------------- */
/* Backup for non-variable cycle CPUs.  */

#ifndef CLK_INC
#define CLK_INC(clock) clock++
#endif

/* ------------------------------------------------------------------------- */
/* Hook for additional delay.  */

#ifndef EMULATION_MODE_CHANGED
#define EMULATION_MODE_CHANGED
#endif

#ifndef WAI_65816
#define WAI_65816() NOP()
#endif

#ifndef STP_65816
#define STP_65816() NOP()
#endif

#ifndef COP_65816
#define COP_65816(value) NOP()
#endif


#ifndef FETCH_PARAM_DUMMY
#define FETCH_PARAM_DUMMY(addr) FETCH_PARAM(addr)
#endif

#ifndef LOAD_LONG_DUMMY
#define LOAD_LONG_DUMMY(addr) LOAD_LONG(addr)
#endif

#define CHECK_INTERRUPT() (interrupt65816 = CPU_INT_STATUS->global_pending_int & (LOCAL_INTERRUPT() ? ~(IK_IRQPEND | IK_IRQ) : ~IK_IRQPEND))
/* ------------------------------------------------------------------------- */
/* Hook for interrupt address manipulation.  */

#ifndef LOAD_INT_ADDR
#define LOAD_INT_ADDR(addr)                 \
    do {                                    \
        reg_pc = LOAD_LONG(addr);           \
        reg_pc |= LOAD_LONG(addr + 1) << 8; \
    } while (0)
#endif

/* ------------------------------------------------------------------------- */

#define LOCAL_SET_NZ(val, bits8)           \
    do {                                   \
        if (!bits8) {                      \
            flag_n = (val >> 8);           \
            flag_z = (val) | flag_n;       \
        } else {                           \
            flag_z = flag_n = ((BYTE)val); \
        }                                  \
    } while (0)

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

#define LOCAL_SET_65816_M(val)   \
    do {                         \
        if (val) {               \
            reg_p |= P_65816_M;  \
        } else {                 \
            reg_p &= ~P_65816_M; \
        }                        \
    } while (0)

#define LOCAL_SET_65816_X(val)   \
    do {                         \
        if (val) {               \
            reg_p |= P_65816_X;  \
        } else {                 \
            reg_p &= ~P_65816_X; \
        }                        \
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

#define LOCAL_65816_M()          (reg_emul || (reg_p & P_65816_M))
#define LOCAL_65816_X()          (reg_emul || (reg_p & P_65816_X))

#define LOCAL_65816_STATUS()     (reg_p | (flag_n & 0x80) | (LOCAL_ZERO() ? P_ZERO : 0))

#ifdef LAST_OPCODE_INFO

/* If requested, gather some info about the last executed opcode for timing
   purposes.  */

/* Remember the number of the last opcode.  By default, the opcode does not
   delay interrupt and does not change the I flag.  */
#define SET_LAST_OPCODE(x) \
    OPINFO_SET(LAST_OPCODE_INFO, (x), 0, 0, 0)

/* Remember that the last opcode changed the I flag from 0 to 1, so we have
   to dispatch an IRQ even if the I flag is 0 when we check it.  */
#define OPCODE_DISABLES_IRQ() \
    OPINFO_SET_DISABLES_IRQ(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 1 to 0, so we must
   not dispatch an IRQ even if the I flag is 1 when we check it.  */
#define OPCODE_ENABLES_IRQ() \
    OPINFO_SET_ENABLES_IRQ(LAST_OPCODE_INFO, 1)

#else

/* Info about the last opcode is not needed.  */
#define SET_LAST_OPCODE(x)
#define OPCODE_DISABLES_IRQ()
#define OPCODE_ENABLES_IRQ()

#endif

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

/* Export the local version of the registers.  */
#define EXPORT_REGISTERS()         \
  do {                             \
      GLOBAL_REGS.pc = reg_pc;     \
      GLOBAL_REGS.a = reg_a;       \
      GLOBAL_REGS.b = reg_b;       \
      GLOBAL_REGS.x = reg_x;       \
      GLOBAL_REGS.y = reg_y;       \
      GLOBAL_REGS.emul = reg_emul; \
      GLOBAL_REGS.dpr = reg_dpr;   \
      GLOBAL_REGS.pbr = reg_pbr;   \
      GLOBAL_REGS.dbr = reg_dbr;   \
      GLOBAL_REGS.sp = reg_sp;     \
      GLOBAL_REGS.p = reg_p;       \
      GLOBAL_REGS.n = flag_n;      \
      GLOBAL_REGS.z = flag_z;      \
  } while (0)

/* Import the public version of the registers.  */
#define IMPORT_REGISTERS()                               \
  do {                                                   \
      reg_a = GLOBAL_REGS.a;                             \
      reg_b = GLOBAL_REGS.b;                             \
      reg_x = GLOBAL_REGS.x;                             \
      reg_y = GLOBAL_REGS.y;                             \
      reg_emul = GLOBAL_REGS.emul;                       \
      reg_dpr = GLOBAL_REGS.dpr;                         \
      reg_pbr = GLOBAL_REGS.pbr;                         \
      reg_dbr = GLOBAL_REGS.dbr;                         \
      reg_sp = GLOBAL_REGS.sp;                           \
      reg_p = GLOBAL_REGS.p;                             \
      flag_n = GLOBAL_REGS.n;                            \
      flag_z = GLOBAL_REGS.z;                            \
      if (reg_emul) { /* fixup emulation mode */         \
          reg_x &= 0xff;                                 \
          reg_y &= 0xff;                                 \
          reg_sp = 0x100 | (reg_sp & 0xff);              \
      }                                                  \
      bank_start = bank_limit = 0; /* prevent caching */ \
      EMULATION_MODE_CHANGED;                            \
      JUMP(GLOBAL_REGS.pc);                              \
  } while (0)

/* Stack operations. */

#ifndef PUSH
#define PUSH(val)                                 \
  do {                                            \
      STORE_LONG(reg_sp, val);                    \
      if (reg_emul) {                             \
          reg_sp = 0x100 | ((reg_sp - 1) & 0xff); \
      } else {                                    \
          reg_sp--;                               \
      }                                           \
  } while (0)
#endif

#ifndef PULL
#define PULL() ((reg_sp = (reg_emul) ? 0x100 | ((reg_sp + 1) & 0xff) : reg_sp + 1), LOAD_LONG(reg_sp))
#endif

#ifdef DEBUG
#define TRACE_NMI() \
    do { if (TRACEFLG) debug_nmi(CPU_INT_STATUS, CLK); } while (0)
#define TRACE_IRQ() \
    do { if (TRACEFLG) debug_irq(CPU_INT_STATUS, CLK); } while (0)
#define TRACE_BRK() do { if (TRACEFLG) debug_text("*** BRK"); } while (0)
#define TRACE_COP() do { if (TRACEFLG) debug_text("*** COP"); } while (0)
#else
#define TRACE_NMI()
#define TRACE_IRQ()
#define TRACE_BRK()
#define TRACE_COP()
#endif

/* Perform the interrupts in `int_kind'.  If we have both NMI and IRQ,
   execute NMI.  */
#define DO_INTERRUPT(ik)                                                       \
    do {                                                                       \
        if (ik & (IK_TRAP | IK_MONITOR | IK_DMA)) {                            \
            if (ik & IK_TRAP) {                                                \
                EXPORT_REGISTERS();                                            \
                interrupt_do_trap(CPU_INT_STATUS, (WORD)reg_pc);               \
                IMPORT_REGISTERS();                                            \
                interrupt65816 &= ~IK_TRAP;                                    \
            }                                                                  \
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
                interrupt65816 &= ~IK_MONITOR;                                 \
            }                                                                  \
            if (ik & IK_DMA) {                                                 \
                EXPORT_REGISTERS();                                            \
                DMA_FUNC;                                                      \
                interrupt_ack_dma(CPU_INT_STATUS);                             \
                IMPORT_REGISTERS();                                            \
                JUMP(reg_pc);                                                  \
                interrupt65816 &= ~IK_DMA;                                     \
            }                                                                  \
        }                                                                      \
    } while (0)
/* ------------------------------------------------------------------------- */

/* Addressing modes.  For convenience, page boundary crossing cycles and
   ``idle'' memory reads are handled here as well. */

#define LOAD_PBR(addr) \
    LOAD_LONG((addr) + (reg_pbr << 16))

#define LOAD_BANK0(addr) \
    LOAD_LONG((addr) & 0xffff)

#define DPR_DELAY \
      if (reg_dpr & 0xff) {                            \
          FETCH_PARAM_DUMMY(reg_pc);                   \
      }                                                \

#ifndef FETCH_PARAM
#define FETCH_PARAM(addr) ((((int)(addr)) < bank_limit) ? (CLK_INC(CLK), bank_base[addr]) : LOAD_PBR(addr))
#endif

/* s */
#define LOAD_STACK(var, bits8)     \
    INC_PC(SIZE_1);                \
    FETCH_PARAM_DUMMY(reg_pc);     \
    if (bits8) {                   \
        CHECK_INTERRUPT();         \
        FETCH_PARAM_DUMMY(reg_pc); \
        var = PULL();              \
    } else {                       \
        FETCH_PARAM_DUMMY(reg_pc); \
        CHECK_INTERRUPT();         \
        var = PULL();              \
        var |= PULL() << 8;        \
    }

/* a */
#define LOAD_ACCU_RRW(var, bits8) \
    INC_PC(SIZE_1);               \
    FETCH_PARAM_DUMMY(reg_pc);    \
    if (bits8) {                  \
        var = reg_a;              \
    } else {                      \
        var = reg_c;              \
    }

/* # */
#define LOAD_IMMEDIATE_FUNC(var, bits8) \
    INC_PC(SIZE_1);                     \
    if (bits8) {                        \
        p1 = FETCH_PARAM(reg_pc);       \
        var = p1;                       \
    } else {                            \
        CHECK_INTERRUPT();              \
        p1 = FETCH_PARAM(reg_pc);       \
        INC_PC(SIZE_1);                 \
        p2 = FETCH_PARAM(reg_pc) << 8;  \
        var = p1 + p2;                  \
    }                                   \
    INC_PC(SIZE_1);

/* $ff wrapping */
#define DIRECT_PAGE_FUNC(var, value, bits8, write) \
  do {                                             \
      unsigned int ea;                             \
                                                   \
      INC_PC(SIZE_1);                              \
      if (bits8) {                                 \
          if (reg_dpr & 0xff) {                    \
              p1 = FETCH_PARAM(reg_pc);            \
              CHECK_INTERRUPT();                   \
              FETCH_PARAM_DUMMY(reg_pc);           \
          } else {                                 \
              CHECK_INTERRUPT();                   \
              p1 = FETCH_PARAM(reg_pc);            \
          }                                        \
          ea = p1 + reg_dpr;                       \
          if (write) {                             \
              STORE_BANK0(ea, value);              \
          } else {                                 \
              var = LOAD_BANK0(ea);                \
          }                                        \
      } else {                                     \
          p1 = FETCH_PARAM(reg_pc);                \
          if (reg_dpr & 0xff) {                    \
              FETCH_PARAM_DUMMY(reg_pc);           \
          }                                        \
          CHECK_INTERRUPT();                       \
          ea = p1 + reg_dpr;                       \
          if (write) {                             \
              STORE_BANK0(ea, value);              \
              STORE_BANK0(ea + 1, value >> 8);     \
          } else {                                 \
              var = LOAD_BANK0(ea);                \
              var |= LOAD_BANK0(ea + 1) << 8;      \
          }                                        \
      }                                            \
      INC_PC(SIZE_1);                              \
  } while (0)

/* $ff wrapping */
#define LOAD_DIRECT_PAGE_FUNC(var, bits8) DIRECT_PAGE_FUNC(var, 0, bits8, 0)

/* $ff wrapping */
#define LOAD_DIRECT_PAGE_FUNC_RRW(var, bits8) \
      unsigned int ea;                        \
                                              \
      INC_PC(SIZE_1);                         \
      p1 = FETCH_PARAM(reg_pc);               \
      DPR_DELAY                               \
      INC_PC(SIZE_1);                         \
      ea = (p1 + reg_dpr) & 0xffff;           \
      var = LOAD_LONG(ea);                    \
      if (reg_emul) {                         \
          CHECK_INTERRUPT();                  \
          STORE_LONG(ea, var);                \
      } else {                                \
          if (bits8) {                        \
              CHECK_INTERRUPT();              \
              LOAD_LONG_DUMMY(ea);            \
          } else {                            \
              ea = (ea + 1) & 0xffff;         \
              var |= LOAD_LONG(ea) << 8;      \
              LOAD_LONG_DUMMY(ea);            \
              CHECK_INTERRUPT();              \
          }                                   \
      }

/* $ff,r wrapping */
#define DIRECT_PAGE_R_FUNC(var, value, bits8, reg_r, write) \
  do {                                                      \
      unsigned int ea;                                      \
                                                            \
      INC_PC(SIZE_1);                                       \
      p1 = FETCH_PARAM(reg_pc);                             \
      DPR_DELAY                                             \
      if (reg_emul) {                                       \
          CHECK_INTERRUPT();                                \
          FETCH_PARAM_DUMMY(reg_pc);                        \
          if (reg_dpr & 0xff) {                             \
              ea = p1 + reg_r + reg_dpr;                    \
          } else {                                          \
              ea = ((p1 + reg_r) & 0xff) + reg_dpr;         \
          }                                                 \
          if (write) {                                      \
              STORE_BANK0(ea, value);                       \
          } else {                                          \
              var = LOAD_BANK0(ea);                         \
          }                                                 \
      } else {                                              \
          ea = p1 + reg_dpr + reg_r;                        \
          if (bits8) {                                      \
              CHECK_INTERRUPT();                            \
              FETCH_PARAM_DUMMY(reg_pc);                    \
              if (write) {                                  \
                  STORE_BANK0(ea, value);                   \
              } else {                                      \
                  var = LOAD_BANK0(ea);                     \
              }                                             \
          } else {                                          \
              FETCH_PARAM_DUMMY(reg_pc);                    \
              CHECK_INTERRUPT();                            \
              if (write) {                                  \
                  STORE_BANK0(ea, value);                   \
                  STORE_BANK0(ea + 1, value >> 8);          \
              } else {                                      \
                  var = LOAD_BANK0(ea);                     \
                  var |= LOAD_BANK0(ea + 1) << 8;           \
              }                                             \
          }                                                 \
      }                                                     \
      INC_PC(SIZE_1);                                       \
  } while (0)

/* $ff,x */
#define LOAD_DIRECT_PAGE_X_FUNC(var, bits8) DIRECT_PAGE_R_FUNC(var, 0, bits8, reg_x, 0)

/* $ff,y */
#define LOAD_DIRECT_PAGE_Y_FUNC(var, bits8) DIRECT_PAGE_R_FUNC(var, 0, bits8, reg_y, 0)

/* $ff,x wrapping */
#define LOAD_DIRECT_PAGE_X_FUNC_RRW(var, bits8)     \
      unsigned int ea;                              \
                                                    \
      INC_PC(SIZE_1);                               \
      p1 = FETCH_PARAM(reg_pc);                     \
      DPR_DELAY                                     \
      FETCH_PARAM_DUMMY(reg_pc);                    \
      INC_PC(SIZE_1);                               \
      if (reg_emul) {                               \
          if (reg_dpr & 0xff) {                     \
              ea = (p1 + reg_x + reg_dpr) & 0xffff; \
          } else {                                  \
              ea = ((p1 + reg_x) & 0xff) + reg_dpr; \
          }                                         \
          var = LOAD_LONG(ea);                      \
          CHECK_INTERRUPT();                        \
          STORE_LONG(ea, var);                      \
      } else {                                      \
          ea = (p1 + reg_dpr + reg_x) & 0xffff;     \
          var = LOAD_LONG(ea);                      \
          if (!bits8) {                             \
              ea = (ea + 1) & 0xffff;               \
              var |= LOAD_LONG(ea) << 8;            \
          }                                         \
          CHECK_INTERRUPT();                        \
          LOAD_LONG_DUMMY(ea);                      \
      }

/* ($ff) no wrapping */
#define INDIRECT_FUNC(var, bits8, write)                      \
  do {                                                        \
      unsigned int ea, ea2;                                   \
                                                              \
      INC_PC(SIZE_1);                                         \
      p1 = FETCH_PARAM(reg_pc);                               \
      DPR_DELAY                                               \
      INC_PC(SIZE_1);                                         \
      ea2 = p1 + reg_dpr;                                     \
      ea = LOAD_BANK0(ea2) | (reg_dbr << 16);                 \
      if (!reg_emul || (reg_dpr & 0xff) || (p1 != 0xff)) {    \
          if (bits8) {                                        \
              CHECK_INTERRUPT();                              \
              ea |= LOAD_BANK0(ea2 + 1) << 8;                 \
              if (write) {                                    \
                  STORE_LONG(ea, var);                        \
              } else {                                        \
                  var = LOAD_LONG(ea);                        \
              }                                               \
          } else {                                            \
              ea |= LOAD_BANK0(ea2 + 1) << 8;                 \
              CHECK_INTERRUPT();                              \
              if (write) {                                    \
                  STORE_LONG(ea, var);                        \
                  STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
              } else {                                        \
                  var = LOAD_LONG(ea);                        \
                  var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
              }                                               \
          }                                                   \
      } else {                                                \
          CHECK_INTERRUPT();                                  \
          ea |= LOAD_LONG(reg_dpr) << 8;                      \
          if (write) {                                        \
              STORE_LONG(ea, var);                            \
          } else {                                            \
              var = LOAD_LONG(ea);                            \
          }                                                   \
      }                                                       \
  } while (0)

/* ($ff) no wrapping */
#define LOAD_INDIRECT_FUNC(var, bits8) INDIRECT_FUNC(var, bits8, 0)

/* ($ff,x) no wrapping */
#define INDIRECT_X_FUNC(var, bits8, write)                    \
  do {                                                        \
      unsigned int ea, ea2;                                   \
                                                              \
      INC_PC(SIZE_1);                                         \
      p1 = FETCH_PARAM(reg_pc);                               \
      DPR_DELAY                                               \
      FETCH_PARAM_DUMMY(reg_pc);                              \
      INC_PC(SIZE_1);                                         \
      if (!reg_emul || (reg_dpr & 0xff)) {                    \
          ea2 = p1 + reg_x + reg_dpr;                         \
          ea = LOAD_BANK0(ea2) | (reg_dbr << 16);             \
          if (bits8) {                                        \
              CHECK_INTERRUPT();                              \
              ea |= LOAD_BANK0(ea2 + 1) << 8;                 \
              if (write) {                                    \
                  STORE_LONG(ea, var);                        \
              } else {                                        \
                  var = LOAD_LONG(ea);                        \
              }                                               \
          } else {                                            \
              ea |= LOAD_BANK0(ea2 + 1) << 8;                 \
              CHECK_INTERRUPT();                              \
              if (write) {                                    \
                  STORE_LONG(ea, var);                        \
                  STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
              } else {                                        \
                  var = LOAD_LONG(ea);                        \
                  var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
              }                                               \
          }                                                   \
      } else {                                                \
          ea2 = ((p1 + reg_x) & 0xff) + reg_dpr;              \
          ea = LOAD_LONG(ea2) | (reg_dbr << 16);              \
          CHECK_INTERRUPT();                                  \
          ea2 = ((p1 + reg_x + 1) & 0xff) + reg_dpr;          \
          ea |= LOAD_LONG(ea2) << 8;                          \
          if (write) {                                        \
              STORE_LONG(ea, var);                            \
          } else {                                            \
              var = LOAD_LONG(ea);                            \
          }                                                   \
      }                                                       \
  } while (0)

/* ($ff,x) no wrapping */
#define LOAD_INDIRECT_X_FUNC(var, bits8) INDIRECT_X_FUNC(var, bits8, 0)

/* ($ff),y no wrapping */
#define INDIRECT_Y_FUNC(var, bits8, write)                            \
  do {                                                                \
      unsigned int ea, ea2;                                           \
                                                                      \
      INC_PC(SIZE_1);                                                 \
      p1 = FETCH_PARAM(reg_pc);                                       \
      DPR_DELAY                                                       \
      INC_PC(SIZE_1);                                                 \
      ea2 = p1 + reg_dpr;                                             \
      ea = LOAD_BANK0(ea2);                                           \
      if (!reg_emul || (reg_dpr & 0xff) || (p1 != 0xff)) {            \
          if (bits8) {                                                \
              if (write) {                                            \
                  ea2 = (LOAD_BANK0(ea2 + 1) << 8) + (reg_dbr << 16); \
                  CHECK_INTERRUPT();                                  \
                  LOAD_LONG_DUMMY(((ea + reg_y) & 0xff) + ea2);       \
                  ea = (ea + ea2 + reg_y) & 0xffffff;                 \
                  STORE_LONG(ea, var);                                \
              } else if (reg_y + ea > 0xff) {                         \
                  ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
                  ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;     \
                  CHECK_INTERRUPT();                                  \
                  LOAD_LONG_DUMMY((ea - 0x100) & 0xffffff);           \
                  var = LOAD_LONG(ea);                                \
              } else {                                                \
                  CHECK_INTERRUPT();                                  \
                  ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
                  ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;     \
                  var = LOAD_LONG(ea);                                \
              }                                                       \
          } else {                                                    \
              if (write) {                                            \
                  ea2 = (LOAD_BANK0(ea2 + 1) << 8) + (reg_dbr << 16); \
                  LOAD_LONG_DUMMY(((ea + reg_y) & 0xff) + ea2);       \
                  ea = (ea + ea2 + reg_y) & 0xffffff;                 \
              } else if (reg_y + ea > 0xff) {                         \
                  ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
                  ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;     \
                  LOAD_LONG_DUMMY((ea - 0x100) & 0xffffff);           \
              } else {                                                \
                  ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
                  ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;     \
              }                                                       \
              CHECK_INTERRUPT();                                      \
              if (write) {                                            \
                  STORE_LONG(ea, var);                                \
                  STORE_LONG((ea + 1) & 0xffffff, var >> 8);          \
              } else {                                                \
                  var = LOAD_LONG(ea);                                \
                  var |= LOAD_LONG((ea + 1) & 0xffffff) << 8;         \
              }                                                       \
          }                                                           \
      } else {                                                        \
          if (write) {                                                \
              ea2 = (LOAD_LONG(reg_dpr) << 8) + (reg_dbr << 16);      \
              CHECK_INTERRUPT();                                      \
              LOAD_LONG_DUMMY(((ea + reg_y) & 0xff) + ea2);           \
              ea = (ea + ea2 + reg_y) & 0xffffff;                     \
              STORE_LONG(ea, var);                                    \
          } else if (reg_y + ea > 0xff) {                             \
              ea |= LOAD_LONG(reg_dpr) << 8;                          \
              CHECK_INTERRUPT();                                      \
              ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;         \
              LOAD_LONG_DUMMY((ea - 0x100) & 0xffffff);               \
              var = LOAD_LONG(ea);                                    \
          } else {                                                    \
              CHECK_INTERRUPT();                                      \
              ea |= LOAD_LONG(reg_dpr) << 8;                          \
              ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;         \
              var = LOAD_LONG(ea);                                    \
          }                                                           \
      }                                                               \
  } while (0)

/* ($ff),y no wrapping */
#define LOAD_INDIRECT_Y_FUNC(var, bits8) INDIRECT_Y_FUNC(var, bits8, 0)

/* [$ff] no wrapping */
#define INDIRECT_LONG_FUNC(var, bits8, write)             \
  do {                                                    \
      unsigned int ea, ea2;                               \
                                                          \
      INC_PC(SIZE_1);                                     \
      p1 = FETCH_PARAM(reg_pc);                           \
      DPR_DELAY                                           \
      ea2 = p1 + reg_dpr;                                 \
      INC_PC(SIZE_1);                                     \
      ea = LOAD_BANK0(ea2);                               \
      ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
      if (bits8) {                                        \
          CHECK_INTERRUPT();                              \
          ea |= LOAD_BANK0(ea2 + 2) << 16;                \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
          }                                               \
      } else {                                            \
          ea |= LOAD_BANK0(ea2 + 2) << 16;                \
          CHECK_INTERRUPT();                              \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
              STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
          }                                               \
      }                                                   \
  } while (0)

/* [$ff] no wrapping */
#define LOAD_INDIRECT_LONG_FUNC(var, bits8) INDIRECT_LONG_FUNC(var, bits8, 0)

/* [$ff],y no wrapping */
#define INDIRECT_LONG_Y_FUNC(var, bits8, write)           \
  do {                                                    \
      unsigned int ea, ea2;                               \
                                                          \
      INC_PC(SIZE_1);                                     \
      p1 = FETCH_PARAM(reg_pc);                           \
      DPR_DELAY                                           \
      ea2 = p1 + reg_dpr;                                 \
      INC_PC(SIZE_1);                                     \
      ea = LOAD_BANK0(ea2);                               \
      ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
      if (bits8) {                                        \
          CHECK_INTERRUPT();                              \
          ea |= LOAD_BANK0(ea2 + 2) << 16;                \
          ea = (ea + reg_y) & 0xffffff;                   \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
          }                                               \
      } else {                                            \
          ea |= LOAD_BANK0(ea2 + 2) << 16;                \
          CHECK_INTERRUPT();                              \
          ea = (ea + reg_y) & 0xffffff;                   \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
              STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
          }                                               \
      }                                                   \
  } while (0)

/* [$ff],y no wrapping */
#define LOAD_INDIRECT_LONG_Y_FUNC(var, bits8) INDIRECT_LONG_Y_FUNC(var, bits8, 0)

/* $ffff no wrapping */
#define ABS_FUNC(var, value, bits8, write)                 \
  do {                                                     \
      unsigned int ea;                                     \
                                                           \
      INC_PC(SIZE_1);                                      \
      p1 = FETCH_PARAM(reg_pc);                            \
      INC_PC(SIZE_1);                                      \
      if (bits8) {                                         \
          CHECK_INTERRUPT();                               \
          p2 = FETCH_PARAM(reg_pc) << 8;                   \
          ea = p1 + p2 + (reg_dbr << 16);                  \
          if (write) {                                     \
              STORE_LONG(ea, value);                       \
          } else {                                         \
              var = LOAD_LONG(ea);                         \
          }                                                \
      } else {                                             \
          p2 = FETCH_PARAM(reg_pc) << 8;                   \
          CHECK_INTERRUPT();                               \
          ea = p1 + p2 + (reg_dbr << 16);                  \
          if (write) {                                     \
              STORE_LONG(ea, value);                       \
              STORE_LONG((ea + 1) & 0xffffff, value >> 8); \
          } else {                                         \
              var = LOAD_LONG(ea);                         \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8;  \
          }                                                \
      }                                                    \
      INC_PC(SIZE_1);                                      \
  } while (0)

/* $ffff no wrapping */
#define LOAD_ABS_FUNC(var, bits8) ABS_FUNC(var, 0, bits8, 0)

/* $ffff no wrapping */
#define LOAD_ABS_FUNC_RRW(var, bits8)    \
      unsigned int ea;                   \
                                         \
      INC_PC(SIZE_1);                    \
      p1 = FETCH_PARAM(reg_pc);          \
      INC_PC(SIZE_1);                    \
      p2 = FETCH_PARAM(reg_pc) << 8;     \
      INC_PC(SIZE_1);                    \
      ea = p1 + p2 + (reg_dbr << 16);    \
      var = LOAD_LONG(ea);               \
      if (reg_emul) {                    \
          CHECK_INTERRUPT();             \
          STORE_LONG(ea, var);           \
      } else {                           \
          if (!bits8) {                  \
              ea = (ea + 1) & 0xffffff;  \
              var |= LOAD_LONG(ea) << 8; \
          }                              \
          CHECK_INTERRUPT();             \
          LOAD_LONG_DUMMY(ea);           \
      }

/* $ffff wrapping */
#define ABS2_FUNC(var, bits8, write)                                        \
  do {                                                                      \
      unsigned int ea;                                                      \
                                                                            \
      INC_PC(SIZE_1);                                                       \
      p1 = FETCH_PARAM(reg_pc);                                             \
      INC_PC(SIZE_1);                                                       \
      if (bits8) {                                                          \
          CHECK_INTERRUPT();                                                \
          p2 = FETCH_PARAM(reg_pc) << 8;                                    \
          ea = p1 + p2 + (reg_dbr << 16);                                   \
          if (write) {                                                      \
              STORE_LONG(ea, var);                                          \
          } else {                                                          \
              var = LOAD_LONG(ea);                                          \
          }                                                                 \
      } else {                                                              \
          p2 = FETCH_PARAM(reg_pc) << 8;                                    \
          ea = p1 + p2 + (reg_dbr << 16);                                   \
          CHECK_INTERRUPT();                                                \
          if (write) {                                                      \
              STORE_LONG(ea, var);                                          \
              STORE_LONG(((ea + 1) & 0xffff) + (reg_dbr << 16), var >> 8);  \
          } else {                                                          \
              var = LOAD_LONG(ea);                                          \
              var |= LOAD_LONG(((ea + 1) & 0xffff) + (reg_dbr << 16)) << 8; \
          }                                                                 \
      }                                                                     \
      INC_PC(SIZE_1);                                                       \
  } while (0)

/* $ffff wrapping */
#define LOAD_ABS2_FUNC(var, bits8) ABS2_FUNC(var, bits8, 0)

/* $ffff wrapping */
#define LOAD_ABS2_FUNC_RRW(var, bits8)                    \
      unsigned int ea;                                    \
                                                          \
      INC_PC(SIZE_1);                                     \
      p1 = FETCH_PARAM(reg_pc);                           \
      INC_PC(SIZE_1);                                     \
      p2 = FETCH_PARAM(reg_pc) << 8;                      \
      INC_PC(SIZE_1);                                     \
      ea = p1 + p2 + (reg_dbr << 16);                     \
      var = LOAD_LONG(ea);                                \
      if (reg_emul) {                                     \
          CHECK_INTERRUPT();                              \
          STORE_LONG(ea, var);                            \
      } else {                                            \
          if (!bits8) {                                   \
              ea = ((ea + 1) & 0xffff) + (reg_dbr << 16); \
              var |= LOAD_LONG(ea) << 8;                  \
          }                                               \
          CHECK_INTERRUPT();                              \
          LOAD_LONG_DUMMY(ea);                            \
      }

/* $ffff,r no wrapping */
#define ABS_R_FUNC(var, value, bits8, reg_r, write)                \
  do {                                                             \
      unsigned int ea;                                             \
                                                                   \
      INC_PC(SIZE_1);                                              \
      p1 = FETCH_PARAM(reg_pc);                                    \
      INC_PC(SIZE_1);                                              \
      if (bits8) {                                                 \
          if (write) {                                             \
              p2 = FETCH_PARAM(reg_pc) << 8;                       \
              ea = p2 + (reg_dbr << 16);                           \
              CHECK_INTERRUPT();                                   \
              LOAD_LONG_DUMMY(((p1 + reg_r) & 0xff) + ea);         \
              ea = (p1 + reg_r + ea) & 0xffffff;                   \
              STORE_LONG(ea, value);                               \
          } else if (!LOCAL_65816_X() || (p1 + reg_r > 0xff)) {    \
              p2 = FETCH_PARAM(reg_pc) << 8;                       \
              ea = p2 + (reg_dbr << 16);                           \
              CHECK_INTERRUPT();                                   \
              LOAD_LONG_DUMMY(((p1 + reg_r) & 0xff) + ea);         \
              ea = (p1 + reg_r + ea) & 0xffffff;                   \
              var = LOAD_LONG(ea);                                 \
          } else {                                                 \
              CHECK_INTERRUPT();                                   \
              p2 = FETCH_PARAM(reg_pc) << 8;                       \
              ea = (p1 + p2 + reg_r + (reg_dbr << 16)) & 0xffffff; \
              var = LOAD_LONG(ea);                                 \
          }                                                        \
      } else {                                                     \
          p2 = FETCH_PARAM(reg_pc) << 8;                           \
          ea = p2 + (reg_dbr << 16);                               \
          if (write) {                                             \
              LOAD_LONG_DUMMY(((p1 + reg_r) & 0xff) + ea);         \
              CHECK_INTERRUPT();                                   \
              ea = (p1 + reg_r + ea) & 0xffffff;                   \
              STORE_LONG(ea, value);                               \
              STORE_LONG((ea + 1) & 0xffffff, value >> 8);         \
          } else {                                                 \
              if (!LOCAL_65816_X() || (p1 + reg_r > 0xff)) {       \
                  LOAD_LONG_DUMMY(((p1 + reg_r) & 0xff) + ea);     \
              }                                                    \
              CHECK_INTERRUPT();                                   \
              ea = (p1 + reg_r + ea) & 0xffffff;                   \
              var = LOAD_LONG(ea);                                 \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8;          \
          }                                                        \
      }                                                            \
      INC_PC(SIZE_1);                                              \
  } while (0)

/* $ffff,r no wrapping */
#define LOAD_ABS_R_FUNC(var, bits8, reg_r) ABS_R_FUNC(var, 0, bits8, reg_r, 0)

/* $ffff,x */
#define LOAD_ABS_X_FUNC(var, bits8) \
    LOAD_ABS_R_FUNC(var, bits8, reg_x)

/* $ffff,x */
#define LOAD_ABS_Y_FUNC(var, bits8) \
    LOAD_ABS_R_FUNC(var, bits8, reg_y)

/* $ffff,x no wrapping */
#define LOAD_ABS_X_FUNC_RRW(var, bits8)                    \
      unsigned int ea;                                     \
                                                           \
      INC_PC(SIZE_1);                                      \
      p1 = FETCH_PARAM(reg_pc);                            \
      INC_PC(SIZE_1);                                      \
      p2 = FETCH_PARAM(reg_pc) << 8;                       \
      INC_PC(SIZE_1);                                      \
      ea = (p1 + p2 + reg_x + (reg_dbr << 16)) & 0xffffff; \
      LOAD_LONG_DUMMY(ea - ((p1 + reg_x) & 0x100));        \
      var = LOAD_LONG(ea);                                 \
      if (reg_emul) {                                      \
          CHECK_INTERRUPT();                               \
          STORE_LONG(ea, var);                             \
      } else {                                             \
          if (bits8) {                                     \
              CHECK_INTERRUPT();                           \
              LOAD_LONG_DUMMY(ea);                         \
          } else {                                         \
              ea = (ea + 1) & 0xffffff;                    \
              var |= LOAD_LONG(ea) << 8;                   \
              CHECK_INTERRUPT();                           \
              LOAD_LONG_DUMMY(ea);                         \
          }                                                                         \
      }

/* $ffff,r wrapping */
#define LOAD_ABS2_R_FUNC(var, bits8, reg_r)                                  \
  do {                                                                       \
      unsigned int ea;                                                       \
                                                                             \
      INC_PC(SIZE_1);                                                        \
      p1 = FETCH_PARAM(reg_pc);                                              \
      INC_PC(SIZE_1);                                                        \
      if (bits8) {                                                           \
          if (!LOCAL_65816_X() || ((p1 + reg_r) > 0xff)) {                   \
              p2 = FETCH_PARAM(reg_pc) << 8;                                 \
              CHECK_INTERRUPT();                                             \
              LOAD_LONG_DUMMY(((p1 + reg_r) & 0xff) + p2 + (reg_dbr << 16)); \
          } else {                                                           \
              CHECK_INTERRUPT();                                             \
              p2 = FETCH_PARAM(reg_pc) << 8;                                 \
          }                                                                  \
          ea = ((p1 + p2 + reg_r) & 0xffff) + (reg_dbr << 16);               \
          var = LOAD_LONG(ea);                                               \
      } else {                                                               \
          p2 = FETCH_PARAM(reg_pc) << 8;                                     \
          ea = ((p1 + p2 + reg_r) & 0xffff) + (reg_dbr << 16);               \
          if (!LOCAL_65816_X() || ((p1 + reg_r) > 0xff)) {                   \
              LOAD_LONG_DUMMY(ea - ((p1 + reg_r) & 0x100));                  \
          }                                                                  \
          CHECK_INTERRUPT();                                                 \
          var = LOAD_LONG(ea);                                               \
          var |= LOAD_LONG(((ea + 1) & 0xffff) + (reg_dbr << 16)) << 8;      \
      }                                                                      \
      INC_PC(SIZE_1);                                                        \
  } while (0)

/* $ffff,x */
#define LOAD_ABS2_X_FUNC(var, bits8) LOAD_ABS2_R_FUNC(var, bits8, reg_x)

/* $ffff,y */
#define LOAD_ABS2_Y_FUNC(var, bits8) LOAD_ABS2_R_FUNC(var, bits8, reg_y)

/* $ffffff no wrapping */
#define ABS_LONG_FUNC(var, bits8, write)                 \
  do {                                                   \
      unsigned int ea;                                   \
                                                         \
      INC_PC(SIZE_1);                                    \
      p1 = FETCH_PARAM(reg_pc);                          \
      INC_PC(SIZE_1);                                    \
      p2 = FETCH_PARAM(reg_pc) << 8;                     \
      INC_PC(SIZE_1);                                    \
      if (bits8) {                                       \
          CHECK_INTERRUPT();                             \
          p3 = FETCH_PARAM(reg_pc) << 16;                \
          ea = p1 + p2 + p3;                             \
          if (write) {                                   \
              STORE_LONG(ea, var);                       \
          } else {                                       \
              var = LOAD_LONG(ea);                       \
          }                                              \
      } else {                                           \
          p3 = FETCH_PARAM(reg_pc) << 16;                \
          CHECK_INTERRUPT();                             \
          ea = p1 + p2 + p3;                             \
          if (write) {                                   \
              STORE_LONG(ea, var);                       \
              STORE_LONG((ea + 1) & 0xffffff, var >> 8); \
          } else {                                       \
              var = LOAD_LONG(ea);                       \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8;\
          }                                              \
      }                                                  \
      INC_PC(SIZE_1);                                    \
  } while (0)

/* $ffffff no wrapping */
#define LOAD_ABS_LONG_FUNC(var, bits8) ABS_LONG_FUNC(var, bits8, 0)

/* $ffffff,x no wrapping */
#define ABS_LONG_X_FUNC(var, bits8, write)                \
  do {                                                    \
      unsigned int ea;                                    \
                                                          \
      INC_PC(SIZE_1);                                     \
      p1 = FETCH_PARAM(reg_pc);                           \
      INC_PC(SIZE_1);                                     \
      p2 = FETCH_PARAM(reg_pc) << 8;                      \
      INC_PC(SIZE_1);                                     \
      if (bits8) {                                        \
          CHECK_INTERRUPT();                              \
          p3 = FETCH_PARAM(reg_pc) << 16;                 \
          ea = (p1 + p2 + p3 + reg_x) & 0xffffff;         \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
          }                                               \
      } else {                                            \
          p3 = FETCH_PARAM(reg_pc) << 16;                 \
          CHECK_INTERRUPT();                              \
          ea = (p1 + p2 + p3 + reg_x) & 0xffffff;         \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
              STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
          }                                               \
      }                                                   \
      INC_PC(SIZE_1);                                     \
  } while (0)

/* $ffffff no wrapping */
#define LOAD_ABS_LONG_X_FUNC(var, bits8) ABS_LONG_X_FUNC(var, bits8, 0)

/* $ff,s no wrapping */
#define STACK_REL_FUNC(var, bits8, write)     \
  do {                                        \
      unsigned int ea;                        \
                                              \
      INC_PC(SIZE_1);                         \
      p1 = FETCH_PARAM(reg_pc);               \
      if (bits8) {                            \
          CHECK_INTERRUPT();                  \
          FETCH_PARAM_DUMMY(reg_pc);          \
          ea = p1 + reg_sp;                   \
          if (write) {                        \
              STORE_BANK0(ea, var);           \
          } else {                            \
              var = LOAD_BANK0(ea);           \
          }                                   \
      } else {                                \
          FETCH_PARAM_DUMMY(reg_pc);          \
          CHECK_INTERRUPT();                  \
          ea = p1 + reg_sp;                   \
          if (write) {                        \
              STORE_BANK0(ea, var);           \
              STORE_BANK0(ea + 1, var >> 8);  \
          } else {                            \
              var = LOAD_BANK0(ea);           \
              var |= LOAD_BANK0(ea + 1) << 8; \
          }                                   \
      }                                       \
      INC_PC(SIZE_1);                         \
  } while (0)

#define LOAD_STACK_REL_FUNC(var, bits8) STACK_REL_FUNC(var, bits8, 0)

/* ($ff,s),y no wrapping */
#define STACK_REL_Y_FUNC(var, bits8, write)               \
  do {                                                    \
      unsigned int ea, ea2;                               \
                                                          \
      INC_PC(SIZE_1);                                     \
      p1 = FETCH_PARAM(reg_pc);                           \
      FETCH_PARAM_DUMMY(reg_pc);                          \
      INC_PC(SIZE_1);                                     \
      ea2 = p1 + reg_sp;                                  \
      ea = LOAD_BANK0(ea2);                               \
      ea |= LOAD_BANK0(ea2 + 1) << 8;                     \
      ea = (ea + reg_y + (reg_dbr << 16)) & 0xffffff;     \
      if (bits8) {                                        \
          CHECK_INTERRUPT();                              \
          LOAD_LONG_DUMMY((ea2 + 1) & 0xffff);            \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
          }                                               \
      } else {                                            \
          LOAD_LONG_DUMMY((ea2 + 1) & 0xffff);            \
          CHECK_INTERRUPT();                              \
          if (write) {                                    \
              STORE_LONG(ea, var);                        \
              STORE_LONG((ea + 1) & 0xffffff, var >> 8);  \
          } else {                                        \
              var = LOAD_LONG(ea);                        \
              var |= LOAD_LONG((ea + 1) & 0xffffff) << 8; \
          }                                               \
      }                                                   \
  } while (0)

/* ($ff,s),y no wrapping */
#define LOAD_STACK_REL_Y_FUNC(var, bits8) STACK_REL_Y_FUNC(var, bits8, 0)

#define STORE_BANK0(addr, value) \
    STORE_LONG((addr) & 0xffff, value);

/* s */
#define STORE_STACK(value, bits8)    \
  do {                               \
      INC_PC(SIZE_1);                \
      if (bits8) {                   \
          CHECK_INTERRUPT();         \
          FETCH_PARAM_DUMMY(reg_pc); \
          PUSH(value);               \
      } else {                       \
          FETCH_PARAM_DUMMY(reg_pc); \
          CHECK_INTERRUPT();         \
          PUSH(value >> 8);          \
          PUSH(value);               \
      }                              \
  } while (0)

/* a */
#define STORE_ACCU_RRW(value, bits8) \
  do {                               \
      if (bits8) {                   \
          reg_a = value;             \
      } else {                       \
          reg_c = value;             \
      }                              \
  } while (0)

/* $ff wrapping */
#define STORE_DIRECT_PAGE(value, bits8) DIRECT_PAGE_FUNC(ea, value, bits8, 1)

/* $ff wrapping */
#define STORE_DIRECT_PAGE_RRW(value, bits8) \
  do {                                      \
      if (bits8) {                          \
          STORE_LONG(ea, value);            \
      } else {                              \
          STORE_LONG(ea, value >> 8);       \
          STORE_BANK0(ea - 1, value);       \
      }                                     \
  } while (0)

/* $ff,x */
#define STORE_DIRECT_PAGE_X(value, bits8) DIRECT_PAGE_R_FUNC(ea, value, bits8, reg_x, 1)

/* $ff,y */
#define STORE_DIRECT_PAGE_Y(value, bits8) DIRECT_PAGE_R_FUNC(ea, value, bits8, reg_y, 1)

/* $ff,x wrapping */
#define STORE_DIRECT_PAGE_X_RRW(value, bits8) STORE_DIRECT_PAGE_RRW(value, bits8)

/* ($ff) no wrapping */
#define STORE_INDIRECT(value, bits8) INDIRECT_FUNC(value, bits8, 1)

/* ($ff,x) no wrapping */
#define STORE_INDIRECT_X(value, bits8) INDIRECT_X_FUNC(value, bits8, 1)

/* ($ff),y no wrapping */
#define STORE_INDIRECT_Y(value, bits8) INDIRECT_Y_FUNC(value, bits8, 1)

/* [$ff] no wrapping */
#define STORE_INDIRECT_LONG(value, bits8) INDIRECT_LONG_FUNC(value, bits8, 1)

/* [$ff],y no wrapping */
#define STORE_INDIRECT_LONG_Y(value, bits8) INDIRECT_LONG_Y_FUNC(value, bits8, 1)

/* $ffff no wrapping */
#define STORE_ABS(value, bits8) ABS_FUNC(ea, value, bits8, 1)

/* $ffff wrapping */
#define STORE_ABS2(value, bits8) ABS2_FUNC(value, bits8, 1)

/* $ffff no wrapping */
#define STORE_ABS_RRW(value, bits8)               \
  do {                                            \
      if (bits8) {                                \
          STORE_LONG(ea, value);                  \
      } else {                                    \
          STORE_LONG(ea, value >> 8);             \
          STORE_LONG((ea - 1) & 0xffffff, value); \
      }                                           \
  } while (0)

/* $ffff wrapping */
#define STORE_ABS2_RRW(value, bits8)                                \
  do {                                                              \
      if (bits8) {                                                  \
          STORE_LONG(ea, value);                                    \
      } else {                                                      \
          STORE_LONG(ea, value >> 8);                               \
          STORE_LONG(((ea - 1) & 0xffff) + (reg_dbr << 16), value); \
      }                                                             \
  } while (0)

/* $ffff,x no wrapping */
#define STORE_ABS_X_RRW(value, bits8) STORE_ABS_RRW(value, bits8)

/* $ffff,x */
#define STORE_ABS_X(value, bits8) ABS_R_FUNC(ea, value, bits8, reg_x, 1)

/* $ffff,y */
#define STORE_ABS_Y(value, bits8) ABS_R_FUNC(ea, value, bits8, reg_y, 1)

/* $ffffff no wrapping */
#define STORE_ABS_LONG(value, bits8) ABS_LONG_FUNC(value, bits8, 1)

/* $ffffff,x no wrapping */
#define STORE_ABS_LONG_X(value, bits8) ABS_LONG_X_FUNC(value, bits8, 1)

/* $ff,s no wrapping */
#define STORE_STACK_REL(value, bits8) STACK_REL_FUNC(value, bits8, 1)

/* ($ff,s),y no wrapping*/
#define STORE_STACK_REL_Y(value, bits8) STACK_REL_Y_FUNC(value, bits8, 1)

#define INC_PC(value)   (reg_pc = (reg_pc + (value)) & 0xffff)

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

#define ADC(load_func)                                                       \
  do {                                                                       \
      unsigned int tmp_value;                                                \
      unsigned int tmp, tmp2;                                                \
                                                                             \
      tmp = LOCAL_CARRY();                                                   \
      if (LOCAL_65816_M()) {                                                 \
          load_func(tmp_value, 1);                                           \
          if (LOCAL_DECIMAL()) {                                             \
              tmp2  = (reg_a & 0x0f) + (tmp_value & 0x0f) + tmp;             \
              tmp = (reg_a & 0xf0) + (tmp_value & 0xf0) + tmp2;              \
              if (tmp2 > 0x9) {                                              \
                  tmp += 0x6;                                                \
              }                                                              \
              if (tmp > 0x99) {                                              \
                  tmp += 0x60;                                               \
              }                                                              \
          } else {                                                           \
              tmp += tmp_value + reg_a;                                      \
          }                                                                  \
          LOCAL_SET_CARRY(tmp & 0x100);                                      \
          LOCAL_SET_OVERFLOW(~(reg_a ^ tmp_value) & (reg_a ^ tmp) & 0x80);   \
          LOCAL_SET_NZ(tmp, 1);                                              \
          reg_a = tmp;                                                       \
      } else {                                                               \
          load_func(tmp_value, 0);                                           \
          if (LOCAL_DECIMAL()) {                                             \
              tmp2 = (reg_c & 0x000f) + (tmp_value & 0x000f) + tmp;          \
              tmp = (reg_c & 0x00f0) + (tmp_value & 0x00f0) + tmp2;          \
              if (tmp2 > 0x9) {                                              \
                  tmp += 0x6;                                                \
              }                                                              \
              tmp2 = (reg_c & 0x0f00) + (tmp_value & 0x0f00) + tmp;          \
              if (tmp > 0x99) {                                              \
                  tmp2 += 0x60;                                              \
              }                                                              \
              tmp = (reg_c & 0xf000) + (tmp_value & 0xf000) + tmp2;          \
              if (tmp2 > 0x999) {                                            \
                  tmp += 0x600;                                              \
              }                                                              \
              if (tmp > 0x9999) {                                            \
                  tmp += 0x6000;                                             \
              }                                                              \
          } else {                                                           \
              tmp += tmp_value + reg_c;                                      \
          }                                                                  \
          LOCAL_SET_CARRY(tmp & 0x10000);                                    \
          LOCAL_SET_OVERFLOW(~(reg_c ^ tmp_value) & (reg_c ^ tmp) & 0x8000); \
          LOCAL_SET_NZ(tmp, 0);                                              \
          reg_c = tmp;                                                       \
      }                                                                      \
  } while (0)

#define LOGICAL(load_func, logic) \
  do {                            \
      unsigned int tmp;           \
                                  \
      if (LOCAL_65816_M()) {      \
          load_func(tmp, 1);      \
          reg_a logic (BYTE)tmp;  \
          LOCAL_SET_NZ(reg_a, 1); \
      } else {                    \
          load_func(tmp, 0);      \
          reg_c logic (WORD)tmp;  \
          LOCAL_SET_NZ(reg_c, 0); \
      }                           \
  } while (0)

#define AND(load_func) LOGICAL(load_func, &=)

#define ASLROL(load_func, store_func, carry) \
  do {                                       \
      unsigned int tmp;                      \
                                             \
      if (LOCAL_65816_M()) {                 \
          load_func(tmp, 1);                 \
          tmp = (tmp << 1) | carry;          \
          LOCAL_SET_CARRY(tmp & 0x100);      \
          LOCAL_SET_NZ(tmp, 1);              \
          store_func(tmp, 1);                \
      } else {                               \
          load_func(tmp, 0);                 \
          tmp = (tmp << 1) | carry;          \
          LOCAL_SET_CARRY(tmp & 0x10000);    \
          LOCAL_SET_NZ(tmp, 0);              \
          store_func(tmp, 0);                \
      }                                      \
  } while (0)

#define ASL(load_func, store_func) ASLROL(load_func, store_func, 0)

#define BIT_IMM(load_func)                      \
  do {                                          \
      unsigned int tmp_value;                   \
                                                \
      if (LOCAL_65816_M()) {                    \
          load_func(tmp_value, 1);              \
          LOCAL_SET_ZERO(!(tmp_value & reg_a)); \
      } else {                                  \
          load_func(tmp_value, 0);              \
          LOCAL_SET_ZERO(!(tmp_value & reg_c)); \
      }                                         \
  } while (0)

#define BIT(load_func)                      \
  do {                                      \
      unsigned int tmp;                     \
                                            \
      if (LOCAL_65816_M()) {                \
          load_func(tmp, 1);                \
          LOCAL_SET_SIGN(tmp & 0x80);       \
          LOCAL_SET_OVERFLOW(tmp & 0x40);   \
          LOCAL_SET_ZERO(!(tmp & reg_a));   \
      } else {                              \
          load_func(tmp, 0);                \
          LOCAL_SET_SIGN(tmp & 0x8000);     \
          LOCAL_SET_OVERFLOW(tmp & 0x4000); \
          LOCAL_SET_ZERO(!(tmp & reg_c));   \
      }                                     \
  } while (0)

#define BRANCH(cond)                                         \
  do {                                                       \
      unsigned int dest_addr;                                \
      INC_PC(SIZE_1);                                        \
                                                             \
      if (cond) {                                            \
          CHECK_INTERRUPT();                                 \
          p1 = FETCH_PARAM(reg_pc);                          \
          INC_PC(SIZE_1);                                    \
          dest_addr = (reg_pc + (signed char)(p1)) & 0xffff; \
                                                             \
          if (((reg_pc ^ dest_addr) & ~0xff) && reg_emul) {  \
              CHECK_INTERRUPT();                             \
              FETCH_PARAM_DUMMY(reg_pc);                     \
              FETCH_PARAM_DUMMY(dest_addr);                  \
          } else {                                           \
              FETCH_PARAM_DUMMY(reg_pc);                     \
          }                                                  \
          reg_pc = dest_addr;                                \
          JUMP(reg_pc);                                      \
      } else {                                               \
          p1 = FETCH_PARAM(reg_pc);                          \
          INC_PC(SIZE_1);                                    \
      }                                                      \
  } while (0)

#define BRANCH_LONG()                \
  do {                               \
      INC_PC(SIZE_1);                \
      p1 = FETCH_PARAM(reg_pc);      \
      INC_PC(SIZE_1);                \
      CHECK_INTERRUPT();             \
      p2 = FETCH_PARAM(reg_pc) << 8; \
      FETCH_PARAM_DUMMY(reg_pc);     \
      INC_PC(p1 + p2 + 1);           \
      JUMP(reg_pc);                  \
  } while (0)

#define BRK()                         \
  do {                                \
      EXPORT_REGISTERS();             \
      TRACE_BRK();                    \
      INC_PC(SIZE_1);                 \
      p1 = FETCH_PARAM(reg_pc);       \
      INC_PC(SIZE_1);                 \
      if (reg_emul) {                 \
          LOCAL_SET_BREAK(1);         \
          PUSH(reg_pc >> 8);          \
          PUSH(reg_pc);               \
          PUSH(LOCAL_STATUS());       \
          LOCAL_SET_DECIMAL(0);       \
          LOCAL_SET_INTERRUPT(1);     \
          CHECK_INTERRUPT();          \
          LOAD_INT_ADDR(0xfffe);      \
      } else {                        \
          PUSH(reg_pbr);              \
          PUSH(reg_pc >> 8);          \
          PUSH(reg_pc);               \
          PUSH(LOCAL_65816_STATUS()); \
          LOCAL_SET_DECIMAL(0);       \
          LOCAL_SET_INTERRUPT(1);     \
          CHECK_INTERRUPT();          \
          LOAD_INT_ADDR(0xffe6);      \
      }                               \
      reg_pbr = 0;                    \
      JUMP(reg_pc);                   \
  } while (0)

#define CLC()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_CARRY(0);        \
  } while (0)

#define CLD()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_DECIMAL(0);      \
  } while (0)

#define CLI()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_INTERRUPT(0);    \
  } while (0)

#define CLV()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_OVERFLOW(0);     \
  } while (0)

#define CMP(load_func)                    \
  do {                                    \
      unsigned int tmp;                   \
      unsigned int value;                 \
                                          \
      if (LOCAL_65816_M()) {              \
          load_func(value, 1);            \
          tmp = reg_a - value;            \
          LOCAL_SET_CARRY(tmp < 0x100);   \
          LOCAL_SET_NZ(tmp, 1);           \
      } else {                            \
          load_func(value, 0);            \
          tmp = reg_c - value;            \
          LOCAL_SET_CARRY(tmp < 0x10000); \
          LOCAL_SET_NZ(tmp, 0);           \
      }                                   \
  } while (0)

#define CMPI(load_func, reg_r)            \
  do {                                    \
      unsigned int tmp;                   \
      unsigned int value;                 \
                                          \
      if (LOCAL_65816_X()) {              \
          load_func(value, 1);            \
          tmp = reg_r - value;            \
          LOCAL_SET_CARRY(tmp < 0x100);   \
          LOCAL_SET_NZ(tmp, 1);           \
      } else {                            \
          load_func(value, 0);            \
          tmp = reg_r - value;            \
          LOCAL_SET_CARRY(tmp < 0x10000); \
          LOCAL_SET_NZ(tmp, 0);           \
      }                                   \
  } while (0)

#define COP()                         \
  do {                                \
      EXPORT_REGISTERS();             \
      TRACE_COP();                    \
      INC_PC(SIZE_1);                 \
      p1 = FETCH_PARAM(reg_pc);       \
      INC_PC(SIZE_1);                 \
      if (reg_emul) {                 \
          LOCAL_SET_BREAK(1);         \
          PUSH(reg_pc >> 8);          \
          PUSH(reg_pc);               \
          PUSH(LOCAL_STATUS());       \
          LOCAL_SET_DECIMAL(0);       \
          LOCAL_SET_INTERRUPT(1);     \
          CHECK_INTERRUPT();          \
          LOAD_INT_ADDR(0xfff4);      \
      } else {                        \
          PUSH(reg_pbr);              \
          PUSH(reg_pc >> 8);          \
          PUSH(reg_pc);               \
          PUSH(LOCAL_65816_STATUS()); \
          LOCAL_SET_DECIMAL(0);       \
          LOCAL_SET_INTERRUPT(1);     \
          CHECK_INTERRUPT();          \
          LOAD_INT_ADDR(0xffe4);      \
      }                               \
      reg_pbr = 0;                    \
      JUMP(reg_pc);                   \
  } while (0)

#define CPX(load_func) CMPI(load_func, reg_x)

#define CPY(load_func) CMPI(load_func, reg_y)

#define INCDEC(load_func, store_func, logic) \
  do {                                       \
      unsigned int tmp;                      \
                                             \
      if (LOCAL_65816_M()) {                 \
          load_func(tmp, 1);                 \
          tmp logic;                         \
          LOCAL_SET_NZ(tmp, 1);              \
          store_func(tmp, 1);                \
      } else {                               \
          load_func(tmp, 0);                 \
          tmp logic;                         \
          LOCAL_SET_NZ(tmp, 0);              \
          store_func(tmp, 0);                \
      }                                      \
  } while (0)

#define DEC(load_func, store_func) INCDEC(load_func, store_func, --)

#define INCDECI(reg_r, logic)     \
  do {                            \
      INC_PC(SIZE_1);             \
      FETCH_PARAM_DUMMY(reg_pc);  \
      reg_r logic;                \
      if (LOCAL_65816_X()) {      \
          reg_r &= 0xff;          \
          LOCAL_SET_NZ(reg_r, 1); \
      } else {                    \
          LOCAL_SET_NZ(reg_r, 0); \
      }                           \
  } while (0)

#define DEX() INCDECI(reg_x, --)

#define DEY() INCDECI(reg_y, --)

#define EOR(load_func) LOGICAL(load_func, ^=)

#define INC(load_func, store_func) INCDEC(load_func, store_func, ++)

#define INX() INCDECI(reg_x, ++)

#define INY() INCDECI(reg_y, ++)

/* The 0x02 COP opcode is also used to patch the ROM.  The function trap_handler()
   returns nonzero if this is not a patch, but a `real' NOP instruction. */

#define COP_02()                                                \
  do {                                                          \
      DWORD trap_result;                                        \
      EXPORT_REGISTERS();                                       \
      if (!ROM_TRAP_ALLOWED()                                   \
          || (trap_result = ROM_TRAP_HANDLER()) == (DWORD)-1) { \
          COP_65816(p1);                                        \
      } else {                                                  \
          if (trap_result) {                                    \
             SET_OPCODE(trap_result & 0xff);                    \
             IMPORT_REGISTERS();                                \
             goto trap_skipped;                                 \
          } else {                                              \
             IMPORT_REGISTERS();                                \
          }                                                     \
      }                                                         \
  } while (0)

#define IRQ()                                 \
  do {                                        \
      TRACE_IRQ();                            \
      if (monitor_mask[CALLER] & (MI_STEP)) { \
          monitor_check_icount_interrupt();   \
      }                                       \
      interrupt_ack_irq(CPU_INT_STATUS);      \
      FETCH_PARAM(reg_pc);                    \
      FETCH_PARAM_DUMMY(reg_pc);              \
      if (reg_emul) {                         \
          LOCAL_SET_BREAK(0);                 \
          PUSH(reg_pc >> 8);                  \
          PUSH(reg_pc);                       \
          PUSH(LOCAL_STATUS());               \
          LOCAL_SET_INTERRUPT(1);             \
          LOCAL_SET_DECIMAL(0);               \
          CHECK_INTERRUPT();                  \
          LOAD_INT_ADDR(0xfffe);              \
      } else {                                \
          PUSH(reg_pbr);                      \
          PUSH(reg_pc >> 8);                  \
          PUSH(reg_pc);                       \
          PUSH(LOCAL_65816_STATUS());         \
          LOCAL_SET_INTERRUPT(1);             \
          LOCAL_SET_DECIMAL(0);               \
          CHECK_INTERRUPT();                  \
          LOAD_INT_ADDR(0xffee);              \
      }                                       \
      reg_pbr = 0;                            \
      JUMP(reg_pc);                           \
  } while (0)

#define JMP()                        \
  do {                               \
      INC_PC(SIZE_1);                \
      CHECK_INTERRUPT();             \
      p1 = FETCH_PARAM(reg_pc);      \
      INC_PC(SIZE_1);                \
      p2 = FETCH_PARAM(reg_pc) << 8; \
      reg_pc = p1 + p2;              \
      JUMP(reg_pc);                  \
  } while (0)

#define JMP_IND()                        \
  do {                                   \
      unsigned int ea;                   \
                                         \
      INC_PC(SIZE_1);                    \
      p1 = FETCH_PARAM(reg_pc);          \
      INC_PC(SIZE_1);                    \
      p2 = FETCH_PARAM(reg_pc) << 8;     \
      ea = p1 + p2;                      \
      CHECK_INTERRUPT();                 \
      reg_pc = LOAD_LONG(ea);            \
      reg_pc |= LOAD_BANK0(ea + 1) << 8; \
      JUMP(reg_pc);                      \
  } while (0)

#define JMP_IND_LONG()                   \
  do {                                   \
      unsigned int ea;                   \
                                         \
      INC_PC(SIZE_1);                    \
      p1 = FETCH_PARAM(reg_pc);          \
      INC_PC(SIZE_1);                    \
      p2 = FETCH_PARAM(reg_pc) << 8;     \
      ea = p1 + p2;                      \
      reg_pc = LOAD_LONG(ea);            \
      CHECK_INTERRUPT();                 \
      reg_pc |= LOAD_BANK0(ea + 1) << 8; \
      reg_pbr = LOAD_BANK0(ea + 2);      \
      JUMP(reg_pc);                      \
  } while (0)

#define JMP_IND_X()                                  \
  do {                                               \
      unsigned int ea;                               \
                                                     \
      INC_PC(SIZE_1);                                \
      p1 = FETCH_PARAM(reg_pc);                      \
      INC_PC(SIZE_1);                                \
      p2 = FETCH_PARAM(reg_pc) << 8;                 \
      ea = (p1 + p2 + reg_x) & 0xffff;               \
      FETCH_PARAM_DUMMY(reg_pc);                     \
      CHECK_INTERRUPT();                             \
      reg_pc = FETCH_PARAM(ea);                      \
      reg_pc |= FETCH_PARAM((ea + 1) & 0xffff) << 8; \
      JUMP(reg_pc);                                  \
  } while (0)

#define JMP_LONG()                   \
  do {                               \
      INC_PC(SIZE_1);                \
      p1 = FETCH_PARAM(reg_pc);      \
      INC_PC(SIZE_1);                \
      CHECK_INTERRUPT();             \
      p2 = FETCH_PARAM(reg_pc) << 8; \
      INC_PC(SIZE_1);                \
      reg_pbr = FETCH_PARAM(reg_pc); \
      p3 = reg_pbr << 16;            \
      reg_pc = p1 + p2;              \
      JUMP(reg_pc);                  \
  } while (0)

#define JSR()                        \
  do {                               \
      INC_PC(SIZE_1);                \
      p1 = FETCH_PARAM(reg_pc);      \
      INC_PC(SIZE_1);                \
      p2 = FETCH_PARAM(reg_pc) << 8; \
      FETCH_PARAM_DUMMY(reg_pc);     \
      CHECK_INTERRUPT();             \
      PUSH(reg_pc >> 8);             \
      PUSH(reg_pc);                  \
      reg_pc = p1 + p2;              \
      JUMP(reg_pc);                  \
  } while (0)

#define JSR_IND_X()                                  \
  do {                                               \
      unsigned int ea;                               \
                                                     \
      INC_PC(SIZE_1);                                \
      p1 = FETCH_PARAM(reg_pc);                      \
      INC_PC(SIZE_1);                                \
      STORE_LONG(reg_sp, reg_pc >> 8);               \
      reg_sp--;                                      \
      PUSH(reg_pc);                                  \
      p2 = FETCH_PARAM(reg_pc) << 8;                 \
      ea = (p1 + p2 + reg_x) & 0xffff;               \
      FETCH_PARAM_DUMMY(reg_pc);                     \
      CHECK_INTERRUPT();                             \
      reg_pc = FETCH_PARAM(ea);                      \
      reg_pc |= FETCH_PARAM((ea + 1) & 0xffff) << 8; \
      JUMP(reg_pc);                                  \
  } while (0)

#define JSR_LONG()                     \
  do {                                 \
      INC_PC(SIZE_1);                  \
      p1 = FETCH_PARAM(reg_pc);        \
      INC_PC(SIZE_1);                  \
      p2 = FETCH_PARAM(reg_pc) << 8;   \
      INC_PC(SIZE_1);                  \
      STORE_LONG(reg_sp, reg_pbr);     \
      LOAD_LONG_DUMMY(reg_sp);         \
      reg_sp--;                        \
      reg_pbr = FETCH_PARAM(reg_pc);   \
      p3 = reg_pbr << 16;              \
      CHECK_INTERRUPT();               \
      STORE_LONG(reg_sp, reg_pc >> 8); \
      reg_sp--;                        \
      PUSH(reg_pc);                    \
      reg_pc = p1 + p2;                \
      JUMP(reg_pc);                    \
  } while (0)

#define LDA(load_func) LOGICAL(load_func, =)

#define LDI(load_func, reg_r)     \
  do {                            \
      unsigned int value;         \
                                  \
      if (LOCAL_65816_X()) {      \
          load_func(value, 1);    \
          reg_r = value;          \
          LOCAL_SET_NZ(reg_r, 1); \
      } else {                    \
          load_func(value, 0);    \
          reg_r = value;          \
          LOCAL_SET_NZ(reg_r, 0); \
      }                           \
  } while (0)

#define LDX(load_func) LDI(load_func, reg_x)

#define LDY(load_func) LDI(load_func, reg_y)

#define LSRROR(load_func, store_func, carry) \
  do {                                       \
      unsigned int tmp;                      \
                                             \
      if (LOCAL_65816_M()) {                 \
          load_func(tmp, 1);                 \
          tmp |= carry ? 0x100 : 0;          \
          LOCAL_SET_CARRY(tmp & 1);          \
          tmp >>= 1;                         \
          LOCAL_SET_NZ(tmp, 1);              \
          store_func(tmp, 1);                \
      } else {                               \
          load_func(tmp, 0);                 \
          tmp |= carry ? 0x10000 : 0;        \
          LOCAL_SET_CARRY(tmp & 1);          \
          tmp >>= 1;                         \
          LOCAL_SET_NZ(tmp, 0);              \
          store_func(tmp, 0);                \
      }                                      \
  } while (0)

#define LSR(load_func, store_func) LSRROR(load_func, store_func, 0)

#define MOVE(logic)                             \
  do {                                          \
      unsigned int tmp;                         \
                                                \
      if (reg_c == 0) {                         \
          INC_PC(SIZE_1);                       \
          p1 = FETCH_PARAM(reg_pc);             \
          INC_PC(SIZE_1);                       \
          reg_dbr = FETCH_PARAM(reg_pc);        \
          INC_PC(SIZE_1);                       \
      } else {                                  \
          p1 = FETCH_PARAM(reg_pc + 1);         \
          reg_dbr = FETCH_PARAM(reg_pc + 2);    \
      }                                         \
      p2 = reg_dbr << 8;                        \
      tmp = LOAD_LONG(reg_x + (reg_dbr << 16)); \
      reg_dbr = p1;                             \
      STORE_LONG(reg_y + (reg_dbr << 16), tmp); \
      LOAD_LONG_DUMMY(reg_y + (reg_dbr << 16)); \
      LOAD_LONG_DUMMY(reg_y + (reg_dbr << 16)); \
      reg_x logic;                              \
      reg_y logic;                              \
      if (LOCAL_65816_X()) {                    \
          reg_x &= 0xff;                        \
          reg_y &= 0xff;                        \
      }                                         \
      reg_c--;                                  \
  } while (0)

#define MVN() MOVE(++)

#define MVP() MOVE(--)

#define NMI()                                 \
  do {                                        \
      TRACE_NMI();                            \
      if (monitor_mask[CALLER] & (MI_STEP)) { \
          monitor_check_icount_interrupt();   \
      }                                       \
      interrupt_ack_nmi(CPU_INT_STATUS);      \
      FETCH_PARAM(reg_pc);                    \
      FETCH_PARAM_DUMMY(reg_pc);              \
      if (reg_emul) {                         \
          LOCAL_SET_BREAK(0);                 \
          PUSH(reg_pc >> 8);                  \
          PUSH(reg_pc);                       \
          PUSH(LOCAL_STATUS());               \
          LOCAL_SET_INTERRUPT(1);             \
          LOCAL_SET_DECIMAL(0);               \
          CHECK_INTERRUPT();                  \
          LOAD_INT_ADDR(0xfffa);              \
      } else {                                \
          PUSH(reg_pbr);                      \
          PUSH(reg_pc >> 8);                  \
          PUSH(reg_pc);                       \
          PUSH(LOCAL_65816_STATUS());         \
          LOCAL_SET_INTERRUPT(1);             \
          LOCAL_SET_DECIMAL(0);               \
          CHECK_INTERRUPT();                  \
          LOAD_INT_ADDR(0xffea);              \
      }                                       \
      reg_pbr = 0;                            \
      JUMP(reg_pc);                           \
  } while (0)

#define NOP()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
  } while (0)

#define ORA(load_func) LOGICAL(load_func, |=)

#define PEA()                        \
  do {                               \
      INC_PC(SIZE_1);                \
      p1 = FETCH_PARAM(reg_pc);      \
      INC_PC(SIZE_1);                \
      p2 = FETCH_PARAM(reg_pc) << 8; \
      INC_PC(SIZE_1);                \
      CHECK_INTERRUPT();             \
      STORE_LONG(reg_sp, p2 >> 8);   \
      reg_sp--;                      \
      PUSH(p1);                      \
  } while (0)

#define PEI()                           \
  do {                                  \
      unsigned int value, ea;           \
                                        \
      INC_PC(SIZE_1);                   \
      p1 = FETCH_PARAM(reg_pc);         \
      DPR_DELAY                         \
      INC_PC(SIZE_1);                   \
      ea = p1 + reg_dpr;                \
      value = LOAD_BANK0(ea);           \
      value |= LOAD_BANK0(ea + 1) << 8; \
      CHECK_INTERRUPT();                \
      STORE_LONG(reg_sp, value >> 8);   \
      reg_sp--;                         \
      PUSH(value);                      \
  } while (0)

#define PER()                             \
  do {                                    \
      unsigned int dest_addr;             \
      INC_PC(SIZE_1);                     \
      p1 = FETCH_PARAM(reg_pc);           \
      INC_PC(SIZE_1);                     \
      p2 = FETCH_PARAM(reg_pc) << 8;      \
      FETCH_PARAM_DUMMY(reg_pc);          \
      INC_PC(SIZE_1);                     \
                                          \
      dest_addr = reg_pc + p1 + p2;       \
      CHECK_INTERRUPT();                  \
      STORE_LONG(reg_sp, dest_addr >> 8); \
      reg_sp--;                           \
      PUSH(dest_addr);                    \
  } while (0)

#define PHA(store_func)         \
  do {                          \
      if (LOCAL_65816_M()) {    \
          store_func(reg_a, 1); \
      } else {                  \
          store_func(reg_c, 0); \
      }                         \
  } while (0)

#define PHB(store_func) store_func(reg_dbr, 1);

#define PHD()                           \
  do {                                  \
      INC_PC(SIZE_1);                   \
      FETCH_PARAM_DUMMY(reg_pc);        \
      CHECK_INTERRUPT();                \
      STORE_LONG(reg_sp, reg_dpr >> 8); \
      reg_sp--;                         \
      PUSH(reg_dpr);                    \
  } while (0)

#define PHK(store_func) store_func(reg_pbr, 1);

#define PHP(store_func)                            \
  do {                                             \
      if (reg_emul) {                              \
          store_func(LOCAL_STATUS() | P_BREAK, 1); \
      } else {                                     \
          store_func(LOCAL_65816_STATUS(), 1);     \
      }                                            \
  } while (0)

#define PHX(store_func) store_func(reg_x, LOCAL_65816_X())

#define PHY(store_func) store_func(reg_y, LOCAL_65816_X())

#define PLA(load_func)            \
  do {                            \
      if (LOCAL_65816_M()) {      \
          load_func(reg_a, 1);    \
          LOCAL_SET_NZ(reg_a, 1); \
      } else {                    \
          load_func(reg_c, 0);    \
          LOCAL_SET_NZ(reg_c, 0); \
      }                           \
  } while (0)

#define PLB(load_func)          \
  do {                          \
      load_func(reg_dbr, 1);    \
      LOCAL_SET_NZ(reg_dbr, 1); \
  } while (0)

#define PLD()                               \
  do {                                      \
      INC_PC(SIZE_1);                       \
      FETCH_PARAM_DUMMY(reg_pc);            \
      FETCH_PARAM_DUMMY(reg_pc);            \
      CHECK_INTERRUPT();                    \
      reg_sp++;                             \
      reg_dpr = LOAD_LONG(reg_sp);          \
      reg_sp++;                             \
      reg_dpr |= LOAD_LONG(reg_sp) << 8;    \
      if (reg_emul) {                       \
          reg_sp = (reg_sp & 0xff) | 0x100; \
      }                                     \
      LOCAL_SET_NZ(reg_dpr, 0);             \
  } while (0)

#define PLP(load_func)       \
  do {                       \
      unsigned int s;        \
                             \
      load_func(s, 1);       \
      LOCAL_SET_STATUS(s);   \
      if (LOCAL_65816_X()) { \
          reg_x &= 0xff;     \
          reg_y &= 0xff;     \
      }                      \
  } while (0)

#define PLI(load_func, reg_r)     \
  do {                            \
      if (LOCAL_65816_X()) {      \
          load_func(reg_r, 1);    \
          LOCAL_SET_NZ(reg_r, 1); \
      } else {                    \
          load_func(reg_r, 0);    \
          LOCAL_SET_NZ(reg_r, 0); \
      }                           \
  } while (0)

#define PLX(load_func) PLI(load_func, reg_x)

#define PLY(load_func) PLI(load_func, reg_y)

#define REPSEP(load_func, v)             \
  do {                                   \
      unsigned int value;                \
                                         \
      INC_PC(SIZE_1);                    \
      CHECK_INTERRUPT();                 \
      p1 = FETCH_PARAM(reg_pc);          \
      value = p1;                        \
      INC_PC(SIZE_1);                    \
      FETCH_PARAM_DUMMY(reg_pc);         \
      if (value & 0x80) {                \
          LOCAL_SET_SIGN(v);             \
      }                                  \
      if (value & 0x40) {                \
          LOCAL_SET_OVERFLOW(v);         \
      }                                  \
      if ((value & 0x20) && !reg_emul) { \
          LOCAL_SET_65816_M(v);          \
      }                                  \
      if ((value & 0x10) && !reg_emul) { \
          LOCAL_SET_65816_X(v);          \
          if (v) {                       \
              reg_x &= 0xff;             \
              reg_y &= 0xff;             \
          }                              \
      }                                  \
      if (value & 0x08) {                \
          LOCAL_SET_DECIMAL(v);          \
      }                                  \
      if (value & 0x04) {                \
          LOCAL_SET_INTERRUPT(v);        \
      }                                  \
      if (value & 0x02) {                \
          LOCAL_SET_ZERO(v);             \
      }                                  \
      if (value & 0x01) {                \
          LOCAL_SET_CARRY(v);            \
      }                                  \
  } while (0)

#define REP(load_func) REPSEP(load_func, 0)

#define RES()                                            \
  do {                                                   \
      interrupt_ack_reset(CPU_INT_STATUS);               \
      cpu_reset();                                       \
      bank_start = bank_limit = 0; /* prevent caching */ \
      FETCH_PARAM(reg_pc);                               \
      FETCH_PARAM_DUMMY(reg_pc);                         \
      LOCAL_SET_BREAK(0);                                \
      reg_emul = 1;                                      \
      reg_x &= 0xff;                                     \
      reg_y &= 0xff;                                     \
      reg_sp = 0x100 | (reg_sp & 0xff);                  \
      reg_dpr = 0;                                       \
      reg_dbr = 0;                                       \
      reg_pbr = 0;                                       \
      EMULATION_MODE_CHANGED;                            \
      LOAD_LONG(reg_sp);                                 \
      reg_sp = 0x100 | ((reg_sp - 1) & 0xff);            \
      LOAD_LONG(reg_sp);                                 \
      reg_sp = 0x100 | ((reg_sp - 1) & 0xff);            \
      LOAD_LONG(reg_sp);                                 \
      reg_sp = 0x100 | ((reg_sp - 1) & 0xff);            \
      LOCAL_SET_INTERRUPT(1);                            \
      LOCAL_SET_DECIMAL(0);                              \
      CHECK_INTERRUPT();                                 \
      LOAD_INT_ADDR(0xfffc);                             \
      JUMP(reg_pc);                                      \
      DMA_ON_RESET;                                      \
  } while (0)

#define ROL(load_func, store_func) ASLROL(load_func, store_func, LOCAL_CARRY())

#define ROR(load_func, store_func) LSRROR(load_func, store_func, LOCAL_CARRY())

#define RTI()                    \
  do {                           \
      unsigned int tmp;          \
                                 \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      FETCH_PARAM_DUMMY(reg_pc); \
      tmp = PULL();              \
      LOCAL_SET_STATUS(tmp);     \
      if (LOCAL_65816_X()) {     \
          reg_x &= 0xff;         \
          reg_y &= 0xff;         \
      }                          \
      if (reg_emul) {            \
          CHECK_INTERRUPT();     \
          reg_pc = PULL();       \
          reg_pc |= PULL() << 8; \
      } else {                   \
          reg_pc = PULL();       \
          CHECK_INTERRUPT();     \
          reg_pc |= PULL() << 8; \
          reg_pbr = PULL();      \
      }                          \
      JUMP(reg_pc);              \
  } while (0)

#define RTL()                               \
  do {                                      \
      INC_PC(SIZE_1);                       \
      FETCH_PARAM_DUMMY(reg_pc);            \
      FETCH_PARAM_DUMMY(reg_pc);            \
      reg_sp++;                             \
      reg_pc = LOAD_LONG(reg_sp);           \
      reg_sp++;                             \
      CHECK_INTERRUPT();                    \
      reg_pc |= LOAD_LONG(reg_sp) << 8;     \
      reg_sp++;                             \
      reg_pbr = LOAD_LONG(reg_sp);          \
      if (reg_emul) {                       \
          reg_sp = (reg_sp & 0xff) | 0x100; \
      }                                     \
      INC_PC(SIZE_1);                       \
      JUMP(reg_pc);                         \
  } while (0)

#define RTS()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      FETCH_PARAM_DUMMY(reg_pc); \
      reg_pc = PULL();           \
      CHECK_INTERRUPT();         \
      reg_pc |= PULL() << 8;     \
      LOAD_LONG_DUMMY(reg_sp);   \
      INC_PC(SIZE_1);            \
      JUMP(reg_pc);              \
  } while (0)

#define SBC(load_func)                                                      \
  do {                                                                      \
      unsigned int tmp_value;                                               \
      unsigned int tmp, tmp2;                                               \
                                                                            \
      tmp = LOCAL_CARRY();                                                  \
      if (LOCAL_65816_M()) {                                                \
          load_func(tmp_value, 1);                                          \
          if (LOCAL_DECIMAL()) {                                            \
              tmp2  = (reg_a & 0x0f) - (tmp_value & 0x0f) + !tmp;           \
              tmp = (reg_a & 0xf0) - (tmp_value & 0xf0) + tmp2;             \
              if (tmp2 > 0xff) {                                            \
                  tmp -= 0x6;                                               \
              }                                                             \
              if (tmp > 0xff) {                                             \
                  tmp -= 0x60;                                              \
              }                                                             \
              LOCAL_SET_CARRY(!(tmp & 0x100));                              \
          } else {                                                          \
              tmp += (tmp_value ^ 0xff) + reg_a;                            \
              LOCAL_SET_CARRY(tmp & 0x100);                                 \
          }                                                                 \
          LOCAL_SET_OVERFLOW((reg_a ^ tmp_value) & (reg_a ^ tmp) & 0x80);   \
          LOCAL_SET_NZ(tmp, 1);                                             \
          reg_a = tmp;                                                      \
      } else {                                                              \
          load_func(tmp_value, 0);                                          \
          if (LOCAL_DECIMAL()) {                                            \
              tmp2 = (reg_c & 0x000f) - (tmp_value & 0x000f) + !tmp;        \
              tmp = (reg_c & 0x00f0) - (tmp_value & 0x00f0) + tmp2;         \
              if (tmp2 > 0xffff) {                                          \
                  tmp -= 0x6;                                               \
              }                                                             \
              tmp2 = (reg_c & 0x0f00) - (tmp_value & 0x0f00) + tmp;         \
              if (tmp > 0xffff) {                                           \
                  tmp2 -= 0x60;                                             \
              }                                                             \
              tmp = (reg_c & 0xf000) - (tmp_value & 0xf000) + tmp2;         \
              if (tmp2 > 0xffff) {                                          \
                  tmp -= 0x600;                                             \
              }                                                             \
              if (tmp > 0xffff) {                                           \
                  tmp -= 0x6000;                                            \
              }                                                             \
              LOCAL_SET_CARRY(!(tmp & 0x10000));                            \
          } else {                                                          \
              tmp += (tmp_value ^ 0xffff) + reg_c;                          \
              LOCAL_SET_CARRY(tmp & 0x10000);                               \
          }                                                                 \
          LOCAL_SET_OVERFLOW((reg_c ^ tmp_value) & (reg_c ^ tmp) & 0x8000); \
          LOCAL_SET_NZ(tmp, 0);                                             \
          reg_c = tmp;                                                      \
      }                                                                     \
  } while (0)

#undef SEC    /* defined in time.h on SunOS. */
#define SEC()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_CARRY(1);        \
  } while (0)

#define SED()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_DECIMAL(1);      \
  } while (0)

#define SEI()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      LOCAL_SET_INTERRUPT(1);    \
  } while (0)

#define SEP(load_func) REPSEP(load_func, 1)

#define STA(store_func) \
      store_func(reg_c, LOCAL_65816_M());

#define STP()                                                         \
  do {                                                                \
      if (!(CPU_INT_STATUS->global_pending_int & IK_RESET)) {         \
          do {                                                        \
              DO_INTERRUPT(CPU_INT_STATUS->global_pending_int);       \
              CLK_INC(CLK);                                           \
          } while (!(CPU_INT_STATUS->global_pending_int & IK_RESET)); \
          CLK_INC(CLK);                                               \
      }                                                               \
      CHECK_INTERRUPT();                                              \
      INC_PC(SIZE_1);                                                 \
      FETCH_PARAM_DUMMY(reg_pc);                                      \
      FETCH_PARAM_DUMMY(reg_pc);                                      \
  } while (0)

#define STX(store_func) \
      store_func(reg_x, LOCAL_65816_X())

#define STY(store_func) \
      store_func(reg_y, LOCAL_65816_X())

#define STZ(store_func) \
      store_func(0, LOCAL_65816_M())

#define TRANSI(reg_s, reg_r)      \
  do {                            \
      INC_PC(SIZE_1);             \
      FETCH_PARAM_DUMMY(reg_pc);  \
      reg_r = reg_s;              \
      if (LOCAL_65816_X()) {      \
          reg_r &= 0xff;          \
          LOCAL_SET_NZ(reg_r, 1); \
      } else {                    \
          LOCAL_SET_NZ(reg_r, 0); \
      }                           \
  } while (0)

#define TAX() TRANSI(reg_c, reg_x)
#define TAY() TRANSI(reg_c, reg_y)

#define TCD()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      reg_dpr = reg_c;           \
      LOCAL_SET_NZ(reg_dpr, 0);  \
  } while (0)

#define TCS()                     \
  do {                            \
      INC_PC(SIZE_1);             \
      FETCH_PARAM_DUMMY(reg_pc);  \
      if (reg_emul) {             \
          reg_sp = 0x100 | reg_a; \
      } else {                    \
          reg_sp = reg_c;         \
      }                           \
  } while (0)

#define TDC()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      reg_c = reg_dpr;           \
      LOCAL_SET_NZ(reg_c, 0);    \
  } while (0)

#define TRBTSB(load_func, store_func, logic)    \
  do {                                          \
      unsigned int tmp_value;                   \
                                                \
      if (LOCAL_65816_M()) {                    \
          load_func(tmp_value, 1);              \
          LOCAL_SET_ZERO(!(tmp_value & reg_a)); \
          tmp_value logic reg_a;                \
          store_func(tmp_value, 1);             \
      } else {                                  \
          load_func(tmp_value, 0);              \
          LOCAL_SET_ZERO(!(tmp_value & reg_c)); \
          tmp_value logic reg_c;                \
          store_func(tmp_value, 0);             \
      }                                         \
  } while (0)

#define TRB(load_func, store_func) TRBTSB(load_func, store_func, &=~)

#define TSB(load_func, store_func) TRBTSB(load_func, store_func, |=)

#define TSC()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      reg_c = reg_sp;            \
      LOCAL_SET_NZ(reg_c, 0);    \
  } while (0)


#define TSX() TRANSI(reg_sp, reg_x)

#define TRANSA(reg_r)             \
  do {                            \
      INC_PC(SIZE_1);             \
      FETCH_PARAM_DUMMY(reg_pc);  \
      if (LOCAL_65816_M()) {      \
          reg_a = (BYTE)reg_r;    \
          LOCAL_SET_NZ(reg_a, 1); \
      } else {                    \
          reg_c = reg_r;          \
          LOCAL_SET_NZ(reg_c, 0); \
      }                           \
  } while (0)

#define TXA() TRANSA(reg_x)

#define TXS()                     \
  do {                            \
      INC_PC(SIZE_1);             \
      FETCH_PARAM_DUMMY(reg_pc);  \
      if (LOCAL_65816_X()) {      \
          reg_sp = 0x100 | reg_x; \
      } else {                    \
          reg_sp = reg_x;         \
      }                           \
  } while (0)

#define TXY()                               \
  do {                                      \
      INC_PC(SIZE_1);                       \
      FETCH_PARAM_DUMMY(reg_pc);            \
      reg_y = reg_x;                        \
      LOCAL_SET_NZ(reg_y, LOCAL_65816_X()); \
  } while (0)

#define TYA() TRANSA(reg_y)

#define TYX()                               \
  do {                                      \
      INC_PC(SIZE_1);                       \
      FETCH_PARAM_DUMMY(reg_pc);            \
      reg_x = reg_y;                        \
      LOCAL_SET_NZ(reg_x, LOCAL_65816_X()); \
  } while (0)

#define WAI()                                                                             \
  do {                                                                                    \
      if (!(CPU_INT_STATUS->global_pending_int & (IK_RESET | IK_NMI | IK_IRQ))) {         \
          do {                                                                            \
              DO_INTERRUPT(CPU_INT_STATUS->global_pending_int);                           \
              CLK_INC(CLK);                                                               \
          } while (!(CPU_INT_STATUS->global_pending_int & (IK_RESET | IK_NMI | IK_IRQ))); \
          CLK_INC(CLK); /* yes, this is really this complicated with this extra cycle */  \
      }                                                                                   \
      CHECK_INTERRUPT();                                                                  \
      INC_PC(SIZE_1);                                                                     \
      FETCH_PARAM_DUMMY(reg_pc);                                                          \
      FETCH_PARAM_DUMMY(reg_pc);                                                          \
  } while (0)

#define WDM()                    \
  do {                           \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      INC_PC(SIZE_1);            \
  } while (0)

#define XBA()                    \
  do {                           \
      BYTE tmp;                  \
                                 \
      INC_PC(SIZE_1);            \
      FETCH_PARAM_DUMMY(reg_pc); \
      FETCH_PARAM_DUMMY(reg_pc); \
      tmp = reg_a;               \
      reg_a = reg_b;             \
      reg_b = tmp;               \
  } while (0)

#define XCE()                                   \
  do {                                          \
      INC_PC(SIZE_1);                           \
      FETCH_PARAM_DUMMY(reg_pc);                \
      if (LOCAL_CARRY() != reg_emul) {          \
          if (LOCAL_CARRY()) {                  \
              reg_emul = 1;                     \
              LOCAL_SET_CARRY(0);               \
              LOCAL_SET_BREAK(0);               \
              reg_x &= 0xff;                    \
              reg_y &= 0xff;                    \
              reg_sp = 0x100 | (reg_sp & 0xff); \
          } else {                              \
              reg_emul = 0;                     \
              LOCAL_SET_CARRY(1);               \
              LOCAL_SET_65816_M(1);             \
              LOCAL_SET_65816_X(1);             \
          }                                     \
          EMULATION_MODE_CHANGED;               \
      }                                         \
  } while (0)

/* ------------------------------------------------------------------------ */

/* Here, the CPU is emulated. */

{

    {
        unsigned int p0, p1, p2, p3;
#ifdef DEBUG
        CLOCK debug_clk = 0;
        unsigned int debug_pc = 0;
        WORD debug_c = 0;
        WORD debug_x = 0;
        WORD debug_y = 0;
        WORD debug_sp = 0;
        BYTE debug_pbr = 0;

        if (TRACEFLG) {
            debug_clk = maincpu_clk;
            debug_pc = reg_pc;
            debug_c = reg_c;
            debug_x = reg_x;
            debug_y = reg_y;
            debug_sp = reg_sp;
            debug_pbr = reg_pbr;
        }
#endif

        if (interrupt65816 != IK_NONE) {
            DO_INTERRUPT(interrupt65816);
            if (interrupt65816 & IK_RESET) {
                p0 = 0x300;
            } else if (interrupt65816 & IK_NMI) {
                p0 = 0x200;
            } else if (interrupt65816 & IK_IRQ) {
                p0 = 0x100;
            } else {
                CHECK_INTERRUPT();
                p0 = FETCH_PARAM(reg_pc);
            }
        } else {
            CHECK_INTERRUPT();
            p0 = FETCH_PARAM(reg_pc);
        }
        SET_LAST_ADDR(reg_pc);

#ifdef DEBUG
        if (debug.perform_break_into_monitor)
        {
            monitor_startup_trap();
            debug.perform_break_into_monitor = 0;
        }
#endif

trap_skipped:
        SET_LAST_OPCODE(p0);

        switch (p0) {

          case 0x00:            /* BRK */
            BRK();
            break;

          case 0x01:            /* ORA ($nn,X) */
            ORA(LOAD_INDIRECT_X_FUNC);
            break;

          case 0x02:            /* NOP #$nn - also used for traps */
            STATIC_ASSERT(TRAP_OPCODE == 0x02);
            COP_02();
            break;

          case 0x03:            /* ORA $nn,S */
            ORA(LOAD_STACK_REL_FUNC);
            break;

          case 0x04:            /* TSB $nn */
            TSB(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x05:            /* ORA $nn */
            ORA(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0x06:            /* ASL $nn */
            ASL(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x07:            /* ORA [$nn] */
            ORA(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0x08:            /* PHP */
            PHP(STORE_STACK);
            break;

          case 0x09:            /* ORA #$nn */
            ORA(LOAD_IMMEDIATE_FUNC);
            break;

          case 0x0a:            /* ASL A */
            ASL(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x0b:            /* PHD */
            PHD();
            break;

          case 0x0c:            /* TSB $nnnn */
            TSB(LOAD_ABS2_FUNC_RRW, STORE_ABS2_RRW);
            break;

          case 0x0d:            /* ORA $nnnn */
            ORA(LOAD_ABS_FUNC);
            break;

          case 0x0e:            /* ASL $nnnn */
            ASL(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0x0f:            /* ORA $nnnnnn */
            ORA(LOAD_ABS_LONG_FUNC);
            break;

          case 0x10:            /* BPL $nnnn */
            BRANCH(!LOCAL_SIGN());
            break;

          case 0x11:            /* ORA ($nn),Y */
            ORA(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0x12:            /* ORA ($nn) */
            ORA(LOAD_INDIRECT_FUNC);
            break;

          case 0x13:            /* ORA ($nn,S),Y */
            ORA(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0x14:            /* TRB $nn */
            TRB(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x15:            /* ORA $nn,X */
            ORA(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0x16:            /* ASL $nn,X */
            ASL(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0x17:            /* ORA [$nn],Y */
            ORA(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0x18:            /* CLC */
            CLC();
            break;

          case 0x19:            /* ORA $nnnn,Y */
            ORA(LOAD_ABS_Y_FUNC);
            break;

          case 0x1a:            /* INA */
            INC(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x1b:            /* TCS */
            TCS();
            break;

          case 0x1c:            /* TRB $nnnn */
            TRB(LOAD_ABS2_FUNC_RRW, STORE_ABS2_RRW);
            break;

          case 0x1d:            /* ORA $nnnn,X */
            ORA(LOAD_ABS_X_FUNC);
            break;

          case 0x1e:            /* ASL $nnnn,X */
            ASL(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0x1f:            /* ORA $nnnnnn,X */
            ORA(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0x20:            /* JSR $nnnn */
            JSR();
            break;

          case 0x21:            /* AND ($nn,X) */
            AND(LOAD_INDIRECT_X_FUNC);
            break;

          case 0x22:            /* JSR $nnnnnn */
            JSR_LONG();
            break;

          case 0x23:            /* AND $nn,S */
            AND(LOAD_STACK_REL_FUNC);
            break;

          case 0x24:            /* BIT $nn */
            BIT(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0x25:            /* AND $nn */
            AND(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0x26:            /* ROL $nn */
            ROL(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x27:            /* AND [$nn] */
            AND(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0x28:            /* PLP */
            PLP(LOAD_STACK);
            break;

          case 0x29:            /* AND #$nn */
            AND(LOAD_IMMEDIATE_FUNC);
            break;

          case 0x2a:            /* ROL A */
            ROL(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x2b:            /* PLD */
            PLD();
            break;

          case 0x2c:            /* BIT $nnnn */
            BIT(LOAD_ABS_FUNC);
            break;

          case 0x2d:            /* AND $nnnn */
            AND(LOAD_ABS_FUNC);
            break;

          case 0x2e:            /* ROL $nnnn */
            ROL(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0x2f:            /* AND $nnnnnn */
            AND(LOAD_ABS_LONG_FUNC);
            break;

          case 0x30:            /* BMI $nnnn */
            BRANCH(LOCAL_SIGN());
            break;

          case 0x31:            /* AND ($nn),Y */
            AND(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0x32:            /* AND ($nn) */
            AND(LOAD_INDIRECT_FUNC);
            break;

          case 0x33:            /* AND ($nn,S),Y */
            AND(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0x34:            /* BIT $nn,X */
            BIT(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0x35:            /* AND $nn,X */
            AND(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0x36:            /* ROL $nn,X */
            ROL(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0x37:            /* AND [$nn],Y */
            AND(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0x38:            /* SEC */
            SEC();
            break;

          case 0x39:            /* AND $nnnn,Y */
            AND(LOAD_ABS_Y_FUNC);
            break;

          case 0x3a:            /* DEA */
            DEC(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x3b:            /* TSC */
            TSC();
            break;

          case 0x3c:            /* BIT $nnnn,X */
            BIT(LOAD_ABS_X_FUNC);
            break;

          case 0x3d:            /* AND $nnnn,X */
            AND(LOAD_ABS_X_FUNC);
            break;

          case 0x3e:            /* ROL $nnnn,X */
            ROL(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0x3f:            /* AND $nnnnnn,X */
            AND(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0x40:            /* RTI */
            RTI();
            break;

          case 0x41:            /* EOR ($nn,X) */
            EOR(LOAD_INDIRECT_X_FUNC);
            break;

          case 0x42:            /* WDM */
            WDM();
            break;

          case 0x43:            /* EOR $nn,S */
            EOR(LOAD_STACK_REL_FUNC);
            break;

          case 0x44:            /* MVP $nn,$nn */
            MVP();
            break;

          case 0x45:            /* EOR $nn */
            EOR(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0x46:            /* LSR $nn */
            LSR(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x47:            /* EOR [$nn] */
            EOR(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0x48:            /* PHA */
            PHA(STORE_STACK);
            break;

          case 0x49:            /* EOR #$nn */
            EOR(LOAD_IMMEDIATE_FUNC);
            break;

          case 0x4a:            /* LSR A */
            LSR(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x4b:            /* PHK */
            PHK(STORE_STACK);
            break;

          case 0x4c:            /* JMP $nnnn */
            JMP();
            break;

          case 0x4d:            /* EOR $nnnn */
            EOR(LOAD_ABS_FUNC);
            break;

          case 0x4e:            /* LSR $nnnn */
            LSR(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0x4f:            /* EOR $nnnnnn */
            EOR(LOAD_ABS_LONG_FUNC);
            break;

          case 0x50:            /* BVC $nnnn */
            BRANCH(!LOCAL_OVERFLOW());
            break;

          case 0x51:            /* EOR ($nn),Y */
            EOR(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0x52:            /* EOR ($nn) */
            EOR(LOAD_INDIRECT_FUNC);
            break;

          case 0x53:            /* EOR ($nn,S),Y */
            EOR(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0x54:            /* MVN $nn,$nn */
            MVN();
            break;

          case 0x55:            /* EOR $nn,X */
            EOR(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0x56:            /* LSR $nn,X */
            LSR(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0x57:            /* EOR [$nn],Y */
            EOR(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0x58:            /* CLI */
            CLI();
            break;

          case 0x59:            /* EOR $nnnn,Y */
            EOR(LOAD_ABS_Y_FUNC);
            break;

          case 0x5a:            /* PHY */
            PHY(STORE_STACK);
            break;

          case 0x5b:            /* TCD */
            TCD();
            break;

          case 0x5c:            /* JMP $nnnnnn */
            JMP_LONG();
            break;

          case 0x5d:            /* EOR $nnnn,X */
            EOR(LOAD_ABS_X_FUNC);
            break;

          case 0x5e:            /* LSR $nnnn,X */
            LSR(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0x5f:            /* EOR $nnnnnn,X */
            EOR(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0x60:            /* RTS */
            RTS();
            break;

          case 0x61:            /* ADC ($nn,X) */
            ADC(LOAD_INDIRECT_X_FUNC);
            break;

          case 0x62:            /* PER $nnnn */
            PER();
            break;

          case 0x63:            /* ADC $nn,S */
            ADC(LOAD_STACK_REL_FUNC);
            break;

          case 0x64:            /* STZ $nn */
            STZ(STORE_DIRECT_PAGE);
            break;

          case 0x65:            /* ADC $nn */
            ADC(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0x66:            /* ROR $nn */
            ROR(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0x67:            /* ADC [$nn] */
            ADC(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0x68:            /* PLA */
            PLA(LOAD_STACK);
            break;

          case 0x69:            /* ADC #$nn */
            ADC(LOAD_IMMEDIATE_FUNC);
            break;

          case 0x6a:            /* ROR A */
            ROR(LOAD_ACCU_RRW, STORE_ACCU_RRW);
            break;

          case 0x6b:            /* RTL */
            RTL();
            break;

          case 0x6c:            /* JMP ($nnnn) */
            JMP_IND();
            break;

          case 0x6d:            /* ADC $nnnn */
            ADC(LOAD_ABS_FUNC);
            break;

          case 0x6e:            /* ROR $nnnn */
            ROR(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0x6f:            /* ADC $nnnnnn */
            ADC(LOAD_ABS_LONG_FUNC);
            break;

          case 0x70:            /* BVS $nnnn */
            BRANCH(LOCAL_OVERFLOW());
            break;

          case 0x71:            /* ADC ($nn),Y */
            ADC(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0x72:            /* ADC ($nn) */
            ADC(LOAD_INDIRECT_FUNC);
            break;

          case 0x73:            /* ADC ($nn,S),Y */
            ADC(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0x74:            /* STZ $nn,X */
            STZ(STORE_DIRECT_PAGE_X);
            break;

          case 0x75:            /* ADC $nn,X */
            ADC(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0x76:            /* ROR $nn,X */
            ROR(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0x77:            /* ADC [$nn],Y */
            ADC(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0x78:            /* SEI */
            SEI();
            break;

          case 0x79:            /* ADC $nnnn,Y */
            ADC(LOAD_ABS_Y_FUNC);
            break;

          case 0x7a:            /* PLY */
            PLY(LOAD_STACK);
            break;

          case 0x7b:            /* TDC */
            TDC();
            break;

          case 0x7c:            /* JMP ($nnnn,X) */
            JMP_IND_X();
            break;

          case 0x7d:            /* ADC $nnnn,X */
            ADC(LOAD_ABS_X_FUNC);
            break;

          case 0x7e:            /* ROR $nnnn,X */
            ROR(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0x7f:            /* ADC $nnnnnn,X */
            ADC(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0x80:            /* BRA $nnnn */
            BRANCH(1);
            break;

          case 0x81:            /* STA ($nn,X) */
            STA(STORE_INDIRECT_X);
            break;

          case 0x82:            /* BRL $nnnn */
            BRANCH_LONG();
            break;

          case 0x83:            /* STA $nn,S */
            STA(STORE_STACK_REL);
            break;

          case 0x84:            /* STY $nn */
            STY(STORE_DIRECT_PAGE);
            break;

          case 0x85:            /* STA $nn */
            STA(STORE_DIRECT_PAGE);
            break;

          case 0x86:            /* STX $nn */
            STX(STORE_DIRECT_PAGE);
            break;

          case 0x87:            /* STA [$nn] */
            STA(STORE_INDIRECT_LONG);
            break;

          case 0x88:            /* DEY */
            DEY();
            break;

          case 0x89:            /* BIT #$nn */
            BIT_IMM(LOAD_IMMEDIATE_FUNC);
            break;

          case 0x8a:            /* TXA */
            TXA();
            break;

          case 0x8b:            /* PHB */
            PHB(STORE_STACK);
            break;

          case 0x8c:            /* STY $nnnn */
            STY(STORE_ABS2);
            break;

          case 0x8d:            /* STA $nnnn */
            STA(STORE_ABS);
            break;

          case 0x8e:            /* STX $nnnn */
            STX(STORE_ABS2);
            break;

          case 0x8f:            /* STA $nnnnnn */
            STA(STORE_ABS_LONG);
            break;

          case 0x90:            /* BCC $nnnn */
            BRANCH(!LOCAL_CARRY());
            break;

          case 0x91:            /* STA ($nn),Y */
            STA(STORE_INDIRECT_Y);
            break;

          case 0x92:            /* STA ($nn) */
            STA(STORE_INDIRECT);
            break;

          case 0x93:            /* STA ($nn,S),Y */
            STA(STORE_STACK_REL_Y);
            break;

          case 0x94:            /* STY $nn,X */
            STY(STORE_DIRECT_PAGE_X);
            break;

          case 0x95:            /* STA $nn,X */
            STA(STORE_DIRECT_PAGE_X);
            break;

          case 0x96:            /* STX $nn,Y */
            STX(STORE_DIRECT_PAGE_Y);
            break;

          case 0x97:            /* STA [$nn],Y */
            STA(STORE_INDIRECT_LONG_Y);
            break;

          case 0x98:            /* TYA */
            TYA();
            break;

          case 0x99:            /* STA $nnnn,Y */
            STA(STORE_ABS_Y);
            break;

          case 0x9a:            /* TXS */
            TXS();
            break;

          case 0x9b:            /* TXY */
            TXY();
            break;

          case 0x9c:            /* STZ $nnnn */
            STZ(STORE_ABS);
            break;

          case 0x9d:            /* STA $nnnn,X */
            STA(STORE_ABS_X);
            break;

          case 0x9e:            /* STZ $nnnn,X */
            STZ(STORE_ABS_X);
            break;

          case 0x9f:            /* STA $nnnnnn,X */
            STA(STORE_ABS_LONG_X);
            break;

          case 0xa0:            /* LDY #$nn */
            LDY(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xa1:            /* LDA ($nn,X) */
            LDA(LOAD_INDIRECT_X_FUNC);
            break;

          case 0xa2:            /* LDX #$nn */
            LDX(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xa3:            /* LDA $nn,S */
            LDA(LOAD_STACK_REL_FUNC);
            break;

          case 0xa4:            /* LDY $nn */
            LDY(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xa5:            /* LDA $nn */
            LDA(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xa6:            /* LDX $nn */
            LDX(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xa7:            /* LDA [$nn] */
            LDA(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0xa8:            /* TAY */
            TAY();
            break;

          case 0xa9:            /* LDA #$nn */
            LDA(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xaa:            /* TAX */
            TAX();
            break;

          case 0xab:            /* PLB */
            PLB(LOAD_STACK);
            break;

          case 0xac:            /* LDY $nnnn */
            LDY(LOAD_ABS2_FUNC);
            break;

          case 0xad:            /* LDA $nnnn */
            LDA(LOAD_ABS_FUNC);
            break;

          case 0xae:            /* LDX $nnnn */
            LDX(LOAD_ABS2_FUNC);
            break;

          case 0xaf:            /* LDA $nnnnnn */
            LDA(LOAD_ABS_LONG_FUNC);
            break;

          case 0xb0:            /* BCS $nnnn */
            BRANCH(LOCAL_CARRY());
            break;

          case 0xb1:            /* LDA ($nn),Y */
            LDA(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0xb2:            /* LDA ($nn) */
            LDA(LOAD_INDIRECT_FUNC);
            break;

          case 0xb3:            /* LDA ($nn,S),Y */
            LDA(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0xb4:            /* LDY $nn,X */
            LDY(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0xb5:            /* LDA $nn,X */
            LDA(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0xb6:            /* LDX $nn,Y */
            LDX(LOAD_DIRECT_PAGE_Y_FUNC);
            break;

          case 0xb7:            /* LDA [$nn],Y */
            LDA(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0xb8:            /* CLV */
            CLV();
            break;

          case 0xb9:            /* LDA $nnnn,Y */
            LDA(LOAD_ABS_Y_FUNC);
            break;

          case 0xba:            /* TSX */
            TSX();
            break;

          case 0xbb:            /* TYX */
            TYX();
            break;

          case 0xbc:            /* LDY $nnnn,X */
            LDY(LOAD_ABS2_X_FUNC);
            break;

          case 0xbd:            /* LDA $nnnn,X */
            LDA(LOAD_ABS_X_FUNC);
            break;

          case 0xbe:            /* LDX $nnnn,Y */
            LDX(LOAD_ABS2_Y_FUNC);
            break;

          case 0xbf:            /* LDA $nnnnnn,X */
            LDA(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0xc0:            /* CPY #$nn */
            CPY(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xc1:            /* CMP ($nn,X) */
            CMP(LOAD_INDIRECT_X_FUNC);
            break;

          case 0xc2:            /* REP #$nn */
            REP(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xc3:            /* CMP $nn,S */
            CMP(LOAD_STACK_REL_FUNC);
            break;

          case 0xc4:            /* CPY $nn */
            CPY(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xc5:            /* CMP $nn */
            CMP(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xc6:            /* DEC $nn */
            DEC(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0xc7:            /* CMP [$nn] */
            CMP(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0xc8:            /* INY */
            INY();
            break;

          case 0xc9:            /* CMP #$nn */
            CMP(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xca:            /* DEX */
            DEX();
            break;

          case 0xcb:            /* WAI */
            WAI_65816();
            break;

          case 0xcc:            /* CPY $nnnn */
            CPY(LOAD_ABS_FUNC);
            break;

          case 0xcd:            /* CMP $nnnn */
            CMP(LOAD_ABS_FUNC);
            break;

          case 0xce:            /* DEC $nnnn */
            DEC(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0xcf:            /* CMP $nnnnnn */
            CMP(LOAD_ABS_LONG_FUNC);
            break;

          case 0xd0:            /* BNE $nnnn */
            BRANCH(!LOCAL_ZERO());
            break;

          case 0xd1:            /* CMP ($nn),Y */
            CMP(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0xd2:            /* CMP ($nn) */
            CMP(LOAD_INDIRECT_FUNC);
            break;

          case 0xd3:            /* CMP ($nn,S),Y */
            CMP(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0xd4:            /* PEI ($nn) */
            PEI();
            break;

          case 0xd5:            /* CMP $nn,X */
            CMP(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0xd6:            /* DEC $nn,X */
            DEC(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0xd7:            /* CMP [$nn],Y */
            CMP(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0xd8:            /* CLD */
            CLD();
            break;

          case 0xd9:            /* CMP $nnnn,Y */
            CMP(LOAD_ABS_Y_FUNC);
            break;

          case 0xda:            /* PHX */
            PHX(STORE_STACK);
            break;

          case 0xdb:            /* STP (WDC65C02) */
            STP_65816();
            break;

          case 0xdc:            /* JMP [$nnnn] */
            JMP_IND_LONG();
            break;

          case 0xdd:            /* CMP $nnnn,X */
            CMP(LOAD_ABS_X_FUNC);
            break;

          case 0xde:            /* DEC $nnnn,X */
            DEC(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0xdf:            /* CMP $nnnnnn,X */
            CMP(LOAD_ABS_LONG_X_FUNC);
            break;

          case 0xe0:            /* CPX #$nn */
            CPX(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xe1:            /* SBC ($nn,X) */
            SBC(LOAD_INDIRECT_X_FUNC);
            break;

          case 0xe2:            /* SEP #$nn */
            SEP(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xe3:            /* SBC $nn,S */
            SBC(LOAD_STACK_REL_FUNC);
            break;

          case 0xe4:            /* CPX $nn */
            CPX(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xe5:            /* SBC $nn */
            SBC(LOAD_DIRECT_PAGE_FUNC);
            break;

          case 0xe6:            /* INC $nn */
            INC(LOAD_DIRECT_PAGE_FUNC_RRW, STORE_DIRECT_PAGE_RRW);
            break;

          case 0xe7:            /* SBC [$nn] */
            SBC(LOAD_INDIRECT_LONG_FUNC);
            break;

          case 0xe8:            /* INX */
            INX();
            break;

          case 0xe9:            /* SBC #$nn */
            SBC(LOAD_IMMEDIATE_FUNC);
            break;

          case 0xea:            /* NOP */
            NOP();
            break;

          case 0xeb:            /* XBA */
            XBA();
            break;

          case 0xec:            /* CPX $nnnn */
            CPX(LOAD_ABS_FUNC);
            break;

          case 0xed:            /* SBC $nnnn */
            SBC(LOAD_ABS_FUNC);
            break;

          case 0xee:            /* INC $nnnn */
            INC(LOAD_ABS_FUNC_RRW, STORE_ABS_RRW);
            break;

          case 0xef:            /* SBC $nnnnnn */
            SBC(LOAD_ABS_LONG_FUNC);
            break;

          case 0xf0:            /* BEQ $nnnn */
            BRANCH(LOCAL_ZERO());
            break;

          case 0xf1:            /* SBC ($nn),Y */
            SBC(LOAD_INDIRECT_Y_FUNC);
            break;

          case 0xf2:            /* SBC ($nn) */
            SBC(LOAD_INDIRECT_FUNC);
            break;

          case 0xf3:            /* SBC ($nn,S),Y */
            SBC(LOAD_STACK_REL_Y_FUNC);
            break;

          case 0xf4:            /* PEA $nnnn */
            PEA();
            break;

          case 0xf5:            /* SBC $nn,X */
            SBC(LOAD_DIRECT_PAGE_X_FUNC);
            break;

          case 0xf6:            /* INC $nn,X */
            INC(LOAD_DIRECT_PAGE_X_FUNC_RRW, STORE_DIRECT_PAGE_X_RRW);
            break;

          case 0xf7:            /* SBC [$nn],Y */
            SBC(LOAD_INDIRECT_LONG_Y_FUNC);
            break;

          case 0xf8:            /* SED */
            SED();
            break;

          case 0xf9:            /* SBC $nnnn,Y */
            SBC(LOAD_ABS_Y_FUNC);
            break;

          case 0xfa:            /* PLX */
            PLX(LOAD_STACK);
            break;

          case 0xfb:            /* XCE */
            XCE();
            break;

          case 0xfc:            /* JSR ($nnnn,X) */
            JSR_IND_X();
            break;

          case 0xfd:            /* SBC $nnnn,X */
            SBC(LOAD_ABS_X_FUNC);
            break;

          case 0xfe:            /* INC $nnnn,X */
            INC(LOAD_ABS_X_FUNC_RRW, STORE_ABS_X_RRW);
            break;

          case 0xff:            /* SBC $nnnnnn,X */
            SBC(LOAD_ABS_LONG_X_FUNC);
            break;

          default:
            switch (p0) {
            case 0x100:           /* IRQ */
                IRQ();
                break;

            case 0x200:           /* NMI */
                NMI();
                break;

            case 0x300:           /* RES */
                RES();
                break;
            }
        }
#ifdef DEBUG
        if (TRACEFLG && p0 < 0x100) {
            BYTE op = (BYTE)(p0);
            BYTE lo = (BYTE)(p1);
            BYTE hi = (BYTE)(p2 >> 8);
            BYTE bk = (BYTE)(p3 >> 16);

            debug_main65816cpu((DWORD)(debug_pc), debug_clk,
                          mon_disassemble_to_string(e_comp_space,
                                                    debug_pc, op,
                                                    lo, hi, bk, 1, "65816"),
                          debug_c, debug_x, debug_y, debug_sp, debug_pbr);
        }
        if (debug.perform_break_into_monitor)
        {
            monitor_startup_trap();
            debug.perform_break_into_monitor = 0;
        }
#endif
    }
}
