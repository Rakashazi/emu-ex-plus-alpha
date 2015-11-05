/*
 * z80.c
 *
 * Written by
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

#include <stdlib.h>

#include "6510core.h"
#include "alarm.h"
#include "daa.h"
#include "debug.h"
#include "interrupt.h"
#include "log.h"
#include "maincpu.h"
#include "monitor.h"
#include "types.h"
#include "z80.h"
#include "z80mem.h"
#include "z80regs.h"

/*#define DEBUG_Z80*/

#ifdef Z80_4MHZ
#define CLK_ADD(clock, amount) clock = z80cpu_clock_add(clock, amount)
#else
#define CLK_ADD(clock, amount) clock += amount
#endif

static BYTE reg_a = 0;
static BYTE reg_b = 0;
static BYTE reg_c = 0;
static BYTE reg_d = 0;
static BYTE reg_e = 0;
static BYTE reg_f = 0;
static BYTE reg_h = 0;
static BYTE reg_l = 0;
static BYTE reg_ixh = 0;
static BYTE reg_ixl = 0;
static BYTE reg_iyh = 0;
static BYTE reg_iyl = 0;
static WORD reg_sp = 0;
static DWORD z80_reg_pc = 0;
static BYTE reg_i = 0;
static BYTE reg_r = 0;

static BYTE iff1 = 0;
static BYTE iff2 = 0;
static BYTE im_mode = 0;

static BYTE reg_a2 = 0;
static BYTE reg_b2 = 0;
static BYTE reg_c2 = 0;
static BYTE reg_d2 = 0;
static BYTE reg_e2 = 0;
static BYTE reg_f2 = 0;
static BYTE reg_h2 = 0;
static BYTE reg_l2 = 0;

static int dma_request = 0;

static BYTE *z80_bank_base;
static int z80_bank_limit;

void z80_trigger_dma(void)
{
    dma_request = 1;
}

void z80_reset(void)
{
    z80_reg_pc = 0;
    z80_regs.reg_pc = 0;
    iff1 = 0;
    iff2 = 0;
    im_mode = 0;
}

inline static BYTE *z80mem_read_base(int addr)
{
    BYTE *p = _z80mem_read_base_tab_ptr[addr >> 8];

    if (p == 0) {
        return p;
    }

    return p - (addr & 0xff00);
}

inline static int z80mem_read_limit(int addr)
{
    return z80mem_read_limit_tab_ptr[addr >> 8];
}

#define JUMP(addr)                                      \
    do {                                                \
        z80_reg_pc = (addr);                            \
        z80_bank_base = z80mem_read_base(z80_reg_pc);   \
        z80_bank_limit = z80mem_read_limit(z80_reg_pc); \
    } while (0)

#define LOAD(addr) (*_z80mem_read_tab_ptr[(addr) >> 8])((WORD)(addr))

#define STORE(addr, value) (*_z80mem_write_tab_ptr[(addr) >> 8])((WORD)(addr), (BYTE)(value))

/* undefine IN and OUT first for platforms that have them already defined as something else */
#undef IN
#define IN(addr) (io_read_tab[(addr) >> 8])((WORD)(addr))

#undef OUT
#define OUT(addr, value) (io_write_tab[(addr) >> 8])((WORD)(addr), (BYTE)(value))

#define opcode_t DWORD

#define FETCH_OPCODE(o) ((o) = (LOAD(z80_reg_pc)               \
                                | (LOAD(z80_reg_pc + 1) << 8)  \
                                | (LOAD(z80_reg_pc + 2) << 16) \
                                | (LOAD(z80_reg_pc + 3) << 24)))

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 ((opcode >> 16) & 0xff)
#define p3 (opcode >> 24)

#define p12 ((opcode >> 8) & 0xffff)
#define p23 ((opcode >> 16) & 0xffff)

#define CLK maincpu_clk

#define INC_PC(value) (z80_reg_pc += (value))

#ifdef Z80_4MHZ
static int z80_half_cycle = 0;

inline static CLOCK z80cpu_clock_add(CLOCK clock, int amount)
{
    CLOCK tmp_clock = clock;
    int left = amount;

    while (left > 0) {
        if (left >= (2 - z80_half_cycle)) {
            left -= (2 - z80_half_cycle);
            z80_half_cycle = 0;
            tmp_clock++;
        } else {
            z80_half_cycle += left;
        }
    }

    return tmp_clock;
}

void z80_clock_stretch(void)
{
    CLK++;
    z80_half_cycle = 0;
}
#endif

/* ------------------------------------------------------------------------- */

static unsigned int z80_last_opcode_info;
static unsigned int z80_last_opcode_addr;

#define LAST_OPCODE_INFO z80_last_opcode_info
#define LAST_OPCODE_ADDR z80_last_opcode_addr

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

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

/* ------------------------------------------------------------------------- */

/* Word register manipulation.  */

#define BC_WORD() ((reg_b << 8) | reg_c)
#define DE_WORD() ((reg_d << 8) | reg_e)
#define HL_WORD() ((reg_h << 8) | reg_l)
#define IX_WORD() ((reg_ixh << 8) | reg_ixl)
#define IY_WORD() ((reg_iyh << 8) | reg_iyl)

#define IX_WORD_OFF(offset) (IX_WORD() + (signed char)(offset))
#define IY_WORD_OFF(offset) (IY_WORD() + (signed char)(offset))

#define DEC_BC_WORD() \
    do {              \
        if (!reg_c) { \
            reg_b--;  \
        }             \
        reg_c--;      \
    } while (0)

#define DEC_DE_WORD() \
    do {              \
        if (!reg_e) { \
            reg_d--;  \
        }             \
        reg_e--;      \
    } while (0)

#define DEC_HL_WORD() \
    do {              \
        if (!reg_l) { \
            reg_h--;  \
        }             \
        reg_l--;      \
    } while (0)

#define DEC_IX_WORD()   \
    do {                \
        if (!reg_ixl) { \
            reg_ixh--;  \
        }               \
        reg_ixl--;      \
    } while (0)

#define DEC_IY_WORD()   \
    do {                \
        if (!reg_iyl) { \
            reg_iyh--;  \
        }               \
        reg_iyl--;      \
    } while (0)

#define INC_BC_WORD() \
    do {              \
        reg_c++;      \
        if (!reg_c) { \
            reg_b++;  \
        }             \
    } while (0)

#define INC_DE_WORD() \
    do {              \
        reg_e++;      \
        if (!reg_e) { \
            reg_d++;  \
        }             \
    } while (0)

#define INC_HL_WORD() \
    do {              \
        reg_l++;      \
        if (!reg_l) { \
            reg_h++;  \
        }             \
    } while (0)

#define INC_IX_WORD()   \
    do {                \
        reg_ixl++;      \
        if (!reg_ixl) { \
            reg_ixh++;  \
        }               \
    } while (0)

#define INC_IY_WORD()   \
    do {                \
        reg_iyl++;      \
        if (!reg_iyl) { \
            reg_iyh++;  \
        }               \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Flags.  */

#define C_FLAG  0x01
#define N_FLAG  0x02
#define P_FLAG  0x04
#define U3_FLAG 0x08
#define H_FLAG  0x10
#define U5_FLAG 0x20
#define Z_FLAG  0x40
#define S_FLAG  0x80

#define LOCAL_SET_CARRY(val)  \
    do {                      \
        if (val) {            \
            reg_f |= C_FLAG;  \
        } else {              \
            reg_f &= ~C_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_NADDSUB(val) \
    do {                       \
        if (val) {             \
            reg_f |= N_FLAG;   \
        } else {               \
            reg_f &= ~N_FLAG;  \
        }                      \
    } while (0)

#define LOCAL_SET_PARITY(val) \
    do {                      \
        if (val) {            \
            reg_f |= P_FLAG;  \
        } else {              \
            reg_f &= ~P_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_HALFCARRY(val) \
    do {                         \
        if (val) {               \
            reg_f |= H_FLAG;     \
        } else {                 \
            reg_f &= ~H_FLAG;    \
        }                        \
    } while (0)

#define LOCAL_SET_ZERO(val)   \
    do {                      \
        if (val) {            \
            reg_f |= Z_FLAG;  \
        } else {              \
            reg_f &= ~Z_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_SIGN(val)   \
    do {                      \
        if (val) {            \
            reg_f |= S_FLAG;  \
        } else {              \
            reg_f &= ~S_FLAG; \
        }                     \
    } while (0)

#define LOCAL_CARRY()     (reg_f & C_FLAG)
#define LOCAL_NADDSUB()   (reg_f & N_FLAG)
#define LOCAL_PARITY()    (reg_f & P_FLAG)
#define LOCAL_HALFCARRY() (reg_f & H_FLAG)
#define LOCAL_ZERO()      (reg_f & Z_FLAG)
#define LOCAL_SIGN()      (reg_f & S_FLAG)

static const BYTE SZP[256] = {
    P_FLAG | Z_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG
};

/* ------------------------------------------------------------------------- */

z80_regs_t z80_regs;

static void import_registers(void)
{
    reg_a = z80_regs.reg_af >> 8;
    reg_f = z80_regs.reg_af & 0xff;
    reg_b = z80_regs.reg_bc >> 8;
    reg_c = z80_regs.reg_bc & 0xff;
    reg_d = z80_regs.reg_de >> 8;
    reg_e = z80_regs.reg_de & 0xff;
    reg_h = z80_regs.reg_hl >> 8;
    reg_l = z80_regs.reg_hl & 0xff;
    reg_ixh = z80_regs.reg_ix >> 8;
    reg_ixl = z80_regs.reg_ix & 0xff;
    reg_iyh = z80_regs.reg_iy >> 8;
    reg_iyl = z80_regs.reg_iy & 0xff;
    reg_sp = z80_regs.reg_sp;
    z80_reg_pc = (DWORD)z80_regs.reg_pc;
    reg_i = z80_regs.reg_i;
    reg_r = z80_regs.reg_r;
    reg_a2 = z80_regs.reg_af2 >> 8;
    reg_f2 = z80_regs.reg_af2 & 0xff;
    reg_b2 = z80_regs.reg_bc2 >> 8;
    reg_c2 = z80_regs.reg_bc2 & 0xff;
    reg_d2 = z80_regs.reg_de2 >> 8;
    reg_e2 = z80_regs.reg_de2 & 0xff;
    reg_h2 = z80_regs.reg_hl2 >> 8;
    reg_l2 = z80_regs.reg_hl2 & 0xff;
}

static void export_registers(void)
{
    z80_regs.reg_af = (reg_a << 8) | reg_f;
    z80_regs.reg_bc = (reg_b << 8) | reg_c;
    z80_regs.reg_de = (reg_d << 8) | reg_e;
    z80_regs.reg_hl = (reg_h << 8) | reg_l;
    z80_regs.reg_ix = (reg_ixh << 8) | reg_ixl;
    z80_regs.reg_iy = (reg_iyh << 8) | reg_iyl;
    z80_regs.reg_sp = reg_sp;
    z80_regs.reg_pc = (WORD)z80_reg_pc;
    z80_regs.reg_i = reg_i;
    z80_regs.reg_r = reg_r;
    z80_regs.reg_af2 = (reg_a2 << 8) | reg_f2;
    z80_regs.reg_bc2 = (reg_b2 << 8) | reg_c2;
    z80_regs.reg_de2 = (reg_d2 << 8) | reg_e2;
    z80_regs.reg_hl2 = (reg_h2 << 8) | reg_l2;
}

/* ------------------------------------------------------------------------- */

/* Interrupt handling.  */

#define DO_INTERRUPT(int_kind)                                                            \
    do {                                                                                  \
        BYTE ik = (int_kind);                                                             \
                                                                                          \
        if (ik & (IK_IRQ | IK_NMI)) {                                                     \
            if ((ik & IK_NMI) && 0) {                                                     \
            } else if ((ik & IK_IRQ) && iff1 && !OPINFO_DISABLES_IRQ(LAST_OPCODE_INFO)) { \
                WORD jumpdst;                                                             \
                if (monitor_mask[e_comp_space] & (MI_STEP)) {                             \
                    monitor_check_icount_interrupt();                                     \
                }                                                                         \
                CLK_ADD(CLK, 4);                                                          \
                --reg_sp;                                                                 \
                STORE((reg_sp), ((BYTE)(z80_reg_pc >> 8)));                               \
                CLK_ADD(CLK, 4);                                                          \
                --reg_sp;                                                                 \
                STORE((reg_sp), ((BYTE)(z80_reg_pc & 0xff)));                             \
                iff1 = 0;                                                                 \
                iff2 = 0;                                                                 \
                if (im_mode == 1) {                                                       \
                    jumpdst = 0x38;                                                       \
                    CLK_ADD(CLK, 4);                                                      \
                    JUMP(jumpdst);                                                        \
                    CLK_ADD(CLK, 3);                                                      \
                } else {                                                                  \
                    jumpdst = (LOAD(reg_i << 8) << 8);                                    \
                    CLK_ADD(CLK, 4);                                                      \
                    jumpdst |= (LOAD((reg_i << 8) + 1));                                  \
                    JUMP(jumpdst);                                                        \
                    CLK_ADD(CLK, 3);                                                      \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
        if (ik & (IK_TRAP | IK_RESET)) {                                                  \
            if (ik & IK_TRAP) {                                                           \
                export_registers();                                                       \
                interrupt_do_trap(cpu_int_status, (WORD)z80_reg_pc);                      \
                import_registers();                                                       \
                if (cpu_int_status->global_pending_int & IK_RESET) {                      \
                    ik |= IK_RESET;                                                       \
                }                                                                         \
            }                                                                             \
            if (ik & IK_RESET) {                                                          \
                interrupt_ack_reset(cpu_int_status);                                      \
                maincpu_reset();                                                          \
            }                                                                             \
        }                                                                                 \
        if (ik & (IK_MONITOR)) {                                                          \
            if (monitor_force_import(e_comp_space)) {                                     \
                import_registers();                                                       \
            }                                                                             \
            if (monitor_mask[e_comp_space]) {                                             \
                export_registers();                                                       \
            }                                                                             \
            if (monitor_mask[e_comp_space] & (MI_STEP)) {                                 \
                monitor_check_icount((WORD)z80_reg_pc);                                   \
            }                                                                             \
            if (monitor_mask[e_comp_space] & (MI_BREAK)) {                                \
                if (monitor_check_breakpoints(e_comp_space, (WORD)z80_reg_pc)) {          \
                    monitor_startup(e_comp_space);                                        \
                }                                                                         \
            }                                                                             \
            if (monitor_mask[e_comp_space] & (MI_WATCH)) {                                \
                monitor_check_watchpoints(LAST_OPCODE_ADDR, (WORD)z80_reg_pc);            \
            }                                                                             \
        }                                                                                 \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Opcodes.  */

#define ADC(loadval, clk_inc1, clk_inc2, pc_inc)                                    \
    do {                                                                            \
        BYTE tmp, carry, value;                                                     \
                                                                                    \
        CLK_ADD(CLK, clk_inc1);                                                     \
        value = (BYTE)(loadval);                                                    \
        carry = LOCAL_CARRY();                                                      \
        tmp = reg_a + value + carry;                                                \
        reg_f = SZP[tmp];                                                           \
        LOCAL_SET_CARRY((WORD)((WORD)reg_a + (WORD)value + (WORD)(carry)) & 0x100); \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);                        \
        LOCAL_SET_PARITY((~(reg_a ^ value)) & (reg_a ^ tmp) & 0x80);                \
        reg_a = tmp;                                                                \
        CLK_ADD(CLK, clk_inc2);                                                     \
        INC_PC(pc_inc);                                                             \
    } while (0)

#define ADCHLREG(reg_valh, reg_vall)                                                       \
    do {                                                                                   \
        DWORD tmp, carry;                                                                  \
                                                                                           \
        carry = LOCAL_CARRY();                                                             \
        tmp = (DWORD)((reg_h << 8) + reg_l) + (DWORD)((reg_valh << 8) + reg_vall) + carry; \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                   \
        LOCAL_SET_NADDSUB(0);                                                              \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                      \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                    \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ reg_valh ^ reg_h) & H_FLAG);                     \
        LOCAL_SET_PARITY((~(reg_h ^ reg_valh)) & (reg_valh ^ (tmp >> 8)) & 0x80);          \
        reg_h = (BYTE)(tmp >> 8);                                                          \
        reg_l = (BYTE)(tmp & 0xff);                                                        \
        CLK_ADD(CLK, 15);                                                                  \
        INC_PC(2);                                                                         \
    } while (0)

#define ADCHLSP()                                                                           \
    do {                                                                                    \
        DWORD tmp, carry;                                                                   \
                                                                                            \
        carry = LOCAL_CARRY();                                                              \
        tmp = (DWORD)((reg_h << 8) + reg_l) + (DWORD)(reg_sp) + carry;                      \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                    \
        LOCAL_SET_NADDSUB(0);                                                               \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                       \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                     \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ (reg_sp >> 8) ^ reg_h) & H_FLAG);                 \
        LOCAL_SET_PARITY((~(reg_h ^ (reg_sp >> 8))) & ((reg_sp >> 8) ^ (tmp >> 8)) & 0x80); \
        reg_h = (BYTE)(tmp >> 8);                                                           \
        reg_l = (BYTE)(tmp & 0xff);                                                         \
        CLK_ADD(CLK, 15);                                                                   \
        INC_PC(2);                                                                          \
    } while (0)

#define ADD(loadval, clk_inc1, clk_inc2, pc_inc)                     \
    do {                                                             \
        BYTE tmp, value;                                             \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        value = (BYTE)(loadval);                                     \
        tmp = reg_a + value;                                         \
        reg_f = SZP[tmp];                                            \
        LOCAL_SET_CARRY((WORD)((WORD)reg_a + (WORD)value) & 0x100);  \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);         \
        LOCAL_SET_PARITY((~(reg_a ^ value)) & (reg_a ^ tmp) & 0x80); \
        reg_a = tmp;                                                 \
        CLK_ADD(CLK, clk_inc2);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define ADDXXREG(reg_dsth, reg_dstl, reg_valh, reg_vall, clk_inc, pc_inc)                \
    do {                                                                                 \
        DWORD tmp;                                                                       \
                                                                                         \
        tmp = (DWORD)((reg_dsth << 8) + reg_dstl) + (DWORD)((reg_valh << 8) + reg_vall); \
        LOCAL_SET_NADDSUB(0);                                                            \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                  \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ reg_valh ^ reg_dsth) & H_FLAG);                \
        reg_dsth = (BYTE)(tmp >> 8);                                                     \
        reg_dstl = (BYTE)(tmp & 0xff);                                                   \
        CLK_ADD(CLK, clk_inc);                                                           \
        INC_PC(pc_inc);                                                                  \
    } while (0)

#define ADDXXSP(reg_dsth, reg_dstl, clk_inc, pc_inc)                           \
    do {                                                                       \
        DWORD tmp;                                                             \
                                                                               \
        tmp = (DWORD)((reg_dsth << 8) + reg_dstl) + (DWORD)(reg_sp);           \
        LOCAL_SET_NADDSUB(0);                                                  \
        LOCAL_SET_CARRY(tmp & 0x10000);                                        \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ (reg_sp >> 8) ^ reg_dsth) & H_FLAG); \
        reg_dsth = (BYTE)(tmp >> 8);                                           \
        reg_dstl = (BYTE)(tmp & 0xff);                                         \
        CLK_ADD(CLK, clk_inc);                                                 \
        INC_PC(pc_inc);                                                        \
    } while (0)

#define AND(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        CLK_ADD(CLK, clk_inc1);                \
        reg_a &= (value);                      \
        reg_f = SZP[reg_a];                    \
        LOCAL_SET_HALFCARRY(1);                \
        CLK_ADD(CLK, clk_inc2);                \
        INC_PC(pc_inc);                        \
    } while (0)

#define BIT(reg_val, value, clk_inc1, clk_inc2, pc_inc) \
    do {                                                \
        CLK_ADD(CLK, clk_inc1);                         \
        LOCAL_SET_NADDSUB(0);                           \
        LOCAL_SET_HALFCARRY(1);                         \
        LOCAL_SET_ZERO(!((reg_val) & (1 << value)));    \
        CLK_ADD(CLK, clk_inc2);                         \
        INC_PC(pc_inc);                                 \
    } while (0)

#define BRANCH(cond, value, pc_inc)                                 \
    do {                                                            \
        if (cond) {                                                 \
            unsigned int dest_addr;                                 \
                                                                    \
            dest_addr = z80_reg_pc + pc_inc + (signed char)(value); \
            z80_reg_pc = dest_addr & 0xffff;                        \
            CLK_ADD(CLK, 7);                                        \
        } else {                                                    \
            CLK_ADD(CLK, 7);                                        \
            INC_PC(pc_inc);                                         \
        }                                                           \
    } while (0)

#define CALL(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                    \
        INC_PC(pc_inc);                                     \
        CLK_ADD(CLK, clk_inc1);                             \
        --reg_sp;                                           \
        STORE((reg_sp), ((BYTE)(z80_reg_pc >> 8)));         \
        CLK_ADD(CLK, clk_inc2);                             \
        --reg_sp;                                           \
        STORE((reg_sp), ((BYTE)(z80_reg_pc & 0xff)));       \
        JUMP(reg_val);                                      \
        CLK_ADD(CLK, clk_inc3);                             \
    } while (0)

#define CALL_COND(reg_value, cond, clk_inc1, clk_inc2, clk_inc3, clk_inc4, pc_inc) \
    do {                                                                           \
        if (cond) {                                                                \
            CALL(reg_value, clk_inc1, clk_inc2, clk_inc3, pc_inc);                 \
        } else {                                                                   \
            CLK_ADD(CLK, clk_inc4);                                                \
            INC_PC(3);                                                             \
        }                                                                          \
    } while (0)

#define CCF(clk_inc, pc_inc)                  \
    do {                                      \
        LOCAL_SET_HALFCARRY((LOCAL_CARRY())); \
        LOCAL_SET_CARRY(!(LOCAL_CARRY()));    \
        LOCAL_SET_NADDSUB(0);                 \
        CLK_ADD(CLK, clk_inc);                \
        INC_PC(pc_inc);                       \
    } while (0)

#define CP(loadval, clk_inc1, clk_inc2, pc_inc)                   \
    do {                                                          \
        BYTE tmp, value;                                          \
                                                                  \
        CLK_ADD(CLK, clk_inc1);                                   \
        value = (BYTE)(loadval);                                  \
        tmp = reg_a - value;                                      \
        reg_f = N_FLAG | SZP[tmp];                                \
        LOCAL_SET_CARRY(value > reg_a);                           \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);      \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80); \
        CLK_ADD(CLK, clk_inc2);                                   \
        INC_PC(pc_inc);                                           \
    } while (0)

#define CPDI(HL_FUNC)                                      \
    do {                                                   \
        BYTE val, tmp;                                     \
                                                           \
        CLK_ADD(CLK, 4);                                   \
        val = LOAD(HL_WORD());                             \
        tmp = reg_a - val;                                 \
        HL_FUNC;                                           \
        DEC_BC_WORD();                                     \
        reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();         \
        LOCAL_SET_HALFCARRY((reg_a ^ val ^ tmp) & H_FLAG); \
        LOCAL_SET_PARITY(reg_b | reg_c);                   \
        CLK_ADD(CLK, 1);                                   \
        INC_PC(2);                                         \
    } while (0)

#define CPDIR(HL_FUNC)                                         \
    do {                                                       \
        BYTE val, tmp;                                         \
                                                               \
        CLK_ADD(CLK, 4);                                       \
        val = LOAD(HL_WORD());                                 \
        tmp = reg_a - val;                                     \
        HL_FUNC;                                               \
        DEC_BC_WORD();                                         \
        CLK_ADD(CLK, 17);                                      \
        if (!(BC_WORD() && tmp)) {                             \
            reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();         \
            LOCAL_SET_HALFCARRY((reg_a ^ val ^ tmp) & H_FLAG); \
            LOCAL_SET_PARITY(reg_b | reg_c);                   \
            CLK_ADD(CLK, 5);                                   \
            INC_PC(2);                                         \
        }                                                      \
    } while (0)

#define CPL(clk_inc, pc_inc)    \
    do {                        \
        reg_a ^= 0xff;          \
        LOCAL_SET_NADDSUB(1);   \
        LOCAL_SET_HALFCARRY(1); \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define DAA(clk_inc, pc_inc)                                                                                         \
    do {                                                                                                             \
        WORD tmp;                                                                                                    \
                                                                                                                     \
        tmp = reg_a | (LOCAL_CARRY() ? 0x100 : 0) | (LOCAL_HALFCARRY() ? 0x200 : 0) | (LOCAL_NADDSUB() ? 0x400 : 0); \
        reg_a = daa_reg_a[tmp];                                                                                      \
        reg_f = daa_reg_f[tmp];                                                                                      \
        CLK_ADD(CLK, clk_inc);                                                                                       \
        INC_PC(pc_inc);                                                                                              \
    } while (0)

#define DECXXIND(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                        \
        BYTE tmp;                                               \
                                                                \
        CLK_ADD(CLK, clk_inc1);                                 \
        tmp = LOAD((reg_val));                                  \
        tmp--;                                                  \
        CLK_ADD(CLK, clk_inc2);                                 \
        STORE((reg_val), tmp);                                  \
        reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();              \
        LOCAL_SET_PARITY((tmp == 0x7f));                        \
        LOCAL_SET_HALFCARRY(((tmp & 0x0f) == 0x0f));            \
        CLK_ADD(CLK, clk_inc3);                                 \
        INC_PC(pc_inc);                                         \
    } while (0)

#define DECINC(FUNC, clk_inc, pc_inc) \
    do {                              \
        CLK_ADD(CLK, clk_inc);        \
        FUNC;                         \
        INC_PC(pc_inc);               \
    } while (0)

#define DECREG(reg_val, clk_inc, pc_inc)                 \
    do {                                                 \
        reg_val--;                                       \
        reg_f = N_FLAG | SZP[reg_val] | LOCAL_CARRY();   \
        LOCAL_SET_PARITY((reg_val == 0x7f));             \
        LOCAL_SET_HALFCARRY(((reg_val & 0x0f) == 0x0f)); \
        CLK_ADD(CLK, clk_inc);                           \
        INC_PC(pc_inc);                                  \
    } while (0)

#define DJNZ(value, pc_inc)           \
    do {                              \
        reg_b--;                      \
        BRANCH(reg_b, value, pc_inc); \
    } while (0)

#define DI(clk_inc, pc_inc)    \
    do {                       \
        iff1 = 0;              \
        iff2 = 0;              \
        OPCODE_DISABLES_IRQ(); \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define EI(clk_inc, pc_inc)    \
    do {                       \
        iff1 = 1;              \
        iff2 = 1;              \
        OPCODE_DISABLES_IRQ(); \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define EXAFAF(clk_inc, pc_inc) \
    do {                        \
        BYTE tmpl, tmph;        \
                                \
        tmph = reg_a;           \
        tmpl = reg_f;           \
        reg_a = reg_a2;         \
        reg_f = reg_f2;         \
        reg_a2 = tmph;          \
        reg_f2 = tmpl;          \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define EXX(clk_inc, pc_inc)   \
    do {                       \
        BYTE tmpl, tmph;       \
                               \
        tmph = reg_b;          \
        tmpl = reg_c;          \
        reg_b = reg_b2;        \
        reg_c = reg_c2;        \
        reg_b2 = tmph;         \
        reg_c2 = tmpl;         \
        tmph = reg_d;          \
        tmpl = reg_e;          \
        reg_d = reg_d2;        \
        reg_e = reg_e2;        \
        reg_d2 = tmph;         \
        reg_e2 = tmpl;         \
        tmph = reg_h;          \
        tmpl = reg_l;          \
        reg_h = reg_h2;        \
        reg_l = reg_l2;        \
        reg_h2 = tmph;         \
        reg_l2 = tmpl;         \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define EXDEHL(clk_inc, pc_inc) \
    do {                        \
        BYTE tmpl, tmph;        \
                                \
        tmph = reg_d;           \
        tmpl = reg_e;           \
        reg_d = reg_h;          \
        reg_e = reg_l;          \
        reg_h = tmph;           \
        reg_l = tmpl;           \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define EXXXSP(reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, clk_inc4, clk_inc5, pc_inc) \
    do {                                                                                     \
        BYTE tmpl, tmph;                                                                     \
                                                                                             \
        tmph = reg_valh;                                                                     \
        tmpl = reg_vall;                                                                     \
        CLK_ADD(CLK, clk_inc1);                                                              \
        reg_valh = LOAD(reg_sp + 1);                                                         \
        CLK_ADD(CLK, clk_inc2);                                                              \
        reg_vall = LOAD(reg_sp);                                                             \
        CLK_ADD(CLK, clk_inc3);                                                              \
        STORE((reg_sp + 1), tmph);                                                           \
        CLK_ADD(CLK, clk_inc4);                                                              \
        STORE(reg_sp, tmpl);                                                                 \
        CLK_ADD(CLK, clk_inc5);                                                              \
        INC_PC(pc_inc);                                                                      \
    } while (0)

/* FIXME: Continue if INT occurs.  */
#define HALT()           \
    do {                 \
        CLK_ADD(CLK, 4); \
    } while (0)

#define IM(value)        \
    do {                 \
        im_mode = value; \
        CLK_ADD(CLK, 8); \
        INC_PC(2);       \
    } while (0)

#define INA(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        CLK_ADD(CLK, clk_inc1);                \
        reg_a = IN((reg_a << 8) | value);      \
        CLK_ADD(CLK, clk_inc2);                \
        INC_PC(pc_inc);                        \
    } while (0)

#define INBC(reg_val, clk_inc1, clk_inc2, pc_inc)    \
    do {                                             \
        CLK_ADD(CLK, clk_inc1);                      \
        reg_val = IN(BC_WORD());                     \
        reg_f = SZP[reg_val & 0xff] | LOCAL_CARRY(); \
        CLK_ADD(CLK, clk_inc2);                      \
        INC_PC(pc_inc);                              \
    } while (0)

#define INBC0(clk_inc1, clk_inc2, pc_inc) \
    do {                                  \
        BYTE tmp;                         \
        CLK_ADD(CLK, clk_inc1);           \
        tmp = IN(BC_WORD());              \
        reg_f = SZP[tmp] | LOCAL_CARRY(); \
        CLK_ADD(CLK, clk_inc2);           \
        INC_PC(pc_inc);                   \
    } while (0)

#define INCXXIND(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                        \
        BYTE tmp;                                               \
                                                                \
        CLK_ADD(CLK, clk_inc1);                                 \
        tmp = LOAD((reg_val));                                  \
        tmp++;                                                  \
        CLK_ADD(CLK, clk_inc2);                                 \
        STORE((reg_val), tmp);                                  \
        reg_f = SZP[tmp] | LOCAL_CARRY();                       \
        LOCAL_SET_PARITY((tmp == 0x80));                        \
        LOCAL_SET_HALFCARRY(!(tmp & 0x0f));                     \
        CLK_ADD(CLK, clk_inc3);                                 \
        INC_PC(pc_inc);                                         \
    } while (0)

#define INCREG(reg_val, clk_inc, pc_inc)        \
    do {                                        \
        reg_val++;                              \
        reg_f = SZP[reg_val] | LOCAL_CARRY();   \
        LOCAL_SET_PARITY((reg_val == 0x80));    \
        LOCAL_SET_HALFCARRY(!(reg_val & 0x0f)); \
        CLK_ADD(CLK, clk_inc);                  \
        INC_PC(pc_inc);                         \
    } while (0)

#define INDI(HL_FUNC)           \
    do {                        \
        BYTE tmp;               \
                                \
        CLK_ADD(CLK, 4);        \
        tmp = IN(BC_WORD());    \
        CLK_ADD(CLK, 4);        \
        STORE(HL_WORD(), tmp);  \
        HL_FUNC;                \
        reg_b--;                \
        reg_f = N_FLAG;         \
        LOCAL_SET_ZERO(!reg_b); \
        CLK_ADD(CLK, 4);        \
        INC_PC(2);              \
    } while (0)

#define INDIR(HL_FUNC)               \
    do {                             \
        BYTE tmp;                    \
                                     \
        CLK_ADD(CLK, 4);             \
        tmp = IN(BC_WORD());         \
        CLK_ADD(CLK, 4);             \
        STORE(HL_WORD(), tmp);       \
        HL_FUNC;                     \
        reg_b--;                     \
        if (!reg_b) {                \
            CLK_ADD(CLK, 4);         \
            reg_f = N_FLAG | Z_FLAG; \
            INC_PC(2);               \
        } else {                     \
            reg_f = N_FLAG;          \
        }                            \
        CLK_ADD(CLK, 4);             \
    } while (0)

#define JMP(addr, clk_inc)     \
    do {                       \
        CLK_ADD(CLK, clk_inc); \
        JUMP(addr);            \
    } while (0)

#define JMP_COND(addr, cond, clk_inc1, clk_inc2) \
    do {                                         \
        if (cond) {                              \
            JMP(addr, clk_inc1);                 \
        } else {                                 \
            CLK_ADD(CLK, clk_inc2);              \
            INC_PC(3);                           \
        }                                        \
    } while (0)

#define LDAIR(reg_val)                      \
    do {                                    \
        CLK_ADD(CLK, 6);                    \
        reg_a = reg_val;                    \
        reg_f = SZP[reg_a] | LOCAL_CARRY(); \
        LOCAL_SET_PARITY(iff2);             \
        CLK_ADD(CLK, 3);                    \
        INC_PC(2);                          \
    } while (0)

#define LDDI(DE_FUNC, HL_FUNC)           \
    do {                                 \
        BYTE tmp;                        \
                                         \
        CLK_ADD(CLK, 4);                 \
        tmp = LOAD(HL_WORD());           \
        CLK_ADD(CLK, 4);                 \
        STORE(DE_WORD(), tmp);           \
        DEC_BC_WORD();                   \
        DE_FUNC;                         \
        HL_FUNC;                         \
        LOCAL_SET_NADDSUB(0);            \
        LOCAL_SET_PARITY(reg_b | reg_c); \
        LOCAL_SET_HALFCARRY(0);          \
        CLK_ADD(CLK, 12);                \
        INC_PC(2);                       \
    } while (0)

#define LDDIR(DE_FUNC, HL_FUNC)     \
    do {                            \
        BYTE tmp;                   \
                                    \
        CLK_ADD(CLK, 4);            \
        tmp = LOAD(HL_WORD());      \
        CLK_ADD(CLK, 4);            \
        STORE(DE_WORD(), tmp);      \
        DEC_BC_WORD();              \
        DE_FUNC;                    \
        HL_FUNC;                    \
        CLK_ADD(CLK, 13);           \
        if (!(BC_WORD())) {         \
            LOCAL_SET_NADDSUB(0);   \
            LOCAL_SET_PARITY(0);    \
            LOCAL_SET_HALFCARRY(0); \
            CLK_ADD(CLK, 5);        \
            INC_PC(2);              \
        }                           \
    } while (0)

#define LDIND(val, reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        CLK_ADD(CLK, clk_inc1);                                              \
        reg_vall = LOAD((val));                                              \
        CLK_ADD(CLK, clk_inc2);                                              \
        reg_valh = LOAD((val) + 1);                                          \
        CLK_ADD(CLK, clk_inc3);                                              \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define LDSP(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        reg_sp = (WORD)(value);                 \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#define LDSPIND(value, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                     \
        CLK_ADD(CLK, clk_inc1);                              \
        reg_sp = LOAD(value);                                \
        CLK_ADD(CLK, clk_inc2);                              \
        reg_sp |= LOAD(value + 1) << 8;                      \
        CLK_ADD(CLK, clk_inc3);                              \
        INC_PC(pc_inc);                                      \
    } while (0)

#define LDREG(reg_dest, value, clk_inc1, clk_inc2, pc_inc) \
    do {                                                   \
        BYTE tmp;                                          \
                                                           \
        CLK_ADD(CLK, clk_inc1);                            \
        tmp = (BYTE)(value);                               \
        reg_dest = tmp;                                    \
        CLK_ADD(CLK, clk_inc2);                            \
        INC_PC(pc_inc);                                    \
    } while (0)

#define LDW(value, reg_valh, reg_vall, clk_inc1, clk_inc2, pc_inc) \
    do {                                                           \
        CLK_ADD(CLK, clk_inc1);                                    \
        reg_vall = (BYTE)((value) & 0xff);                         \
        reg_valh = (BYTE)((value) >> 8);                           \
        CLK_ADD(CLK, clk_inc2);                                    \
        INC_PC(pc_inc);                                            \
    } while (0)

#define NEG()                                        \
    do {                                             \
        BYTE tmp;                                    \
                                                     \
        tmp = 0 - reg_a;                             \
        reg_f = N_FLAG | SZP[tmp];                   \
        LOCAL_SET_HALFCARRY((reg_a ^ tmp) & H_FLAG); \
        LOCAL_SET_PARITY(reg_a & tmp & 0x80);        \
        LOCAL_SET_CARRY(reg_a > 0);                  \
        reg_a = tmp;                                 \
        CLK_ADD(CLK, 8);                             \
        INC_PC(2);                                   \
    } while (0)

#define NOP(clk_inc, pc_inc)   \
    do {                       \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define OR(reg_val, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        reg_a |= reg_val;                       \
        reg_f = SZP[reg_a];                     \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#define OUTA(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        OUT((reg_a << 8) | value, reg_a);       \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#define OUTBC(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                         \
        CLK_ADD(CLK, clk_inc1);                  \
        OUT(BC_WORD(), value);                   \
        CLK_ADD(CLK, clk_inc2);                  \
        INC_PC(pc_inc);                          \
    } while (0)

#define OUTDI(HL_FUNC)          \
    do {                        \
        BYTE tmp;               \
                                \
        CLK_ADD(CLK, 4);        \
        tmp = LOAD(HL_WORD());  \
        CLK_ADD(CLK, 4);        \
        OUT(BC_WORD(), tmp);    \
        HL_FUNC;                \
        reg_b--;                \
        reg_f = N_FLAG;         \
        LOCAL_SET_ZERO(!reg_b); \
        CLK_ADD(CLK, 4);        \
        INC_PC(2);              \
    } while (0)

#define OTDIR(HL_FUNC)               \
    do {                             \
        BYTE tmp;                    \
                                     \
        CLK_ADD(CLK, 4);             \
        tmp = LOAD(HL_WORD());       \
        CLK_ADD(CLK, 4);             \
        OUT(BC_WORD(), tmp);         \
        HL_FUNC;                     \
        reg_b--;                     \
        if (!reg_b) {                \
            CLK_ADD(CLK, 4);         \
            reg_f = N_FLAG | Z_FLAG; \
            INC_PC(2);               \
        } else {                     \
            reg_f = N_FLAG;          \
        }                            \
        CLK_ADD(CLK, 4);             \
    } while (0)

#define POP(reg_valh, reg_vall, pc_inc) \
    do {                                \
        CLK_ADD(CLK, 4);                \
        reg_vall = LOAD(reg_sp);        \
        ++reg_sp;                       \
        CLK_ADD(CLK, 4);                \
        reg_valh = LOAD(reg_sp);        \
        ++reg_sp;                       \
        CLK_ADD(CLK, 2);                \
        INC_PC(pc_inc);                 \
    } while (0)

#define PUSH(reg_valh, reg_vall, pc_inc) \
    do {                                 \
        CLK_ADD(CLK, 4);                 \
        --reg_sp;                        \
        STORE((reg_sp), (reg_valh));     \
        CLK_ADD(CLK, 4);                 \
        --reg_sp;                        \
        STORE((reg_sp), (reg_vall));     \
        CLK_ADD(CLK, 3);                 \
        INC_PC(pc_inc);                  \
    } while (0)

#define RES(reg_val, value)         \
    do {                            \
        reg_val &= (~(1 << value)); \
        CLK_ADD(CLK, 8);            \
        INC_PC(2);                  \
    } while (0)

#define RESXX(value, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                         \
        BYTE tmp;                                                \
                                                                 \
        CLK_ADD(CLK, clk_inc1);                                  \
        tmp = LOAD((addr));                                      \
        tmp &= (~(1 << value));                                  \
        CLK_ADD(CLK, clk_inc2);                                  \
        STORE((addr), tmp);                                      \
        CLK_ADD(CLK, clk_inc3);                                  \
        INC_PC(pc_inc);                                          \
    } while (0)

#define RESXXREG(value, reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        BYTE tmp;                                                            \
                                                                             \
        CLK_ADD(CLK, clk_inc1);                                              \
        tmp = LOAD((addr));                                                  \
        tmp &= (~(1 << value));                                              \
        CLK_ADD(CLK, clk_inc2);                                              \
        STORE((addr), tmp);                                                  \
        reg_val = tmp;                                                       \
        CLK_ADD(CLK, clk_inc3);                                              \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define RET(clk_inc1, clk_inc2, clk_inc3) \
    do {                                  \
        WORD tmp;                         \
                                          \
        CLK_ADD(CLK, clk_inc1);           \
        tmp = LOAD(reg_sp);               \
        CLK_ADD(CLK, clk_inc2);           \
        tmp |= LOAD((reg_sp + 1)) << 8;   \
        reg_sp += 2;                      \
        JUMP(tmp);                        \
        CLK_ADD(CLK, clk_inc3);           \
    } while (0)

#define RET_COND(cond, clk_inc1, clk_inc2, clk_inc3, clk_inc4, pc_inc) \
    do {                                                               \
        if (cond) {                                                    \
            RET(clk_inc1, clk_inc2, clk_inc3);                         \
        } else {                                                       \
            CLK_ADD(CLK, clk_inc4);                                    \
            INC_PC(pc_inc);                                            \
        }                                                              \
    } while (0)

#define RETNI()                         \
    do {                                \
        WORD tmp;                       \
                                        \
        CLK_ADD(CLK, 4);                \
        tmp = LOAD(reg_sp);             \
        CLK_ADD(CLK, 4);                \
        tmp |= LOAD((reg_sp + 1)) << 8; \
        reg_sp += 2;                    \
        iff1 = iff2;                    \
        JUMP(tmp);                      \
        CLK_ADD(CLK, 2);                \
    } while (0)

#define RL(reg_val)                               \
    do {                                          \
        BYTE rot;                                 \
                                                  \
        rot = (reg_val & 0x80) ? C_FLAG : 0;      \
        reg_val = (reg_val << 1) | LOCAL_CARRY(); \
        reg_f = rot | SZP[reg_val];               \
        CLK_ADD(CLK, 8);                          \
        INC_PC(2);                                \
    } while (0)

#define RLA(clk_inc, pc_inc)                  \
    do {                                      \
        BYTE rot;                             \
                                              \
        rot = (reg_a & 0x80) ? C_FLAG : 0;    \
        reg_a = (reg_a << 1) | LOCAL_CARRY(); \
        LOCAL_SET_CARRY(rot);                 \
        LOCAL_SET_NADDSUB(0);                 \
        LOCAL_SET_HALFCARRY(0);               \
        CLK_ADD(CLK, clk_inc);                \
        INC_PC(pc_inc);                       \
    } while (0)

#define RLC(reg_val)                         \
    do {                                     \
        BYTE rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val = (reg_val << 1) | rot;      \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define RLCA(clk_inc, pc_inc)              \
    do {                                   \
        BYTE rot;                          \
                                           \
        rot = (reg_a & 0x80) ? C_FLAG : 0; \
        reg_a = (reg_a << 1) | rot;        \
        LOCAL_SET_CARRY(rot);              \
        LOCAL_SET_NADDSUB(0);              \
        LOCAL_SET_HALFCARRY(0);            \
        CLK_ADD(CLK, clk_inc);             \
        INC_PC(pc_inc);                    \
    } while (0)

#define RLCXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp = (tmp << 1) | rot;                           \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define RLCXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp = (tmp << 1) | rot;                                       \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define RLD()                                          \
    do {                                               \
        BYTE tmp;                                      \
                                                       \
        tmp = LOAD(HL_WORD());                         \
        CLK_ADD(CLK, 8);                               \
        STORE(HL_WORD(), (tmp << 4) | (reg_a & 0x0f)); \
        reg_a = (tmp >> 4) | (reg_a & 0xf0);           \
        reg_f = SZP[reg_a] | LOCAL_CARRY();            \
        CLK_ADD(CLK, 10);                              \
        INC_PC(2);                                     \
    } while (0)

#define RLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                 \
        BYTE rot, tmp;                                   \
                                                         \
        CLK_ADD(CLK, clk_inc1);                          \
        tmp = LOAD((addr));                              \
        rot = (tmp & 0x80) ? C_FLAG : 0;                 \
        tmp = (tmp << 1) | LOCAL_CARRY();                \
        CLK_ADD(CLK, clk_inc2);                          \
        STORE((addr), tmp);                              \
        reg_f = rot | SZP[tmp];                          \
        CLK_ADD(CLK, clk_inc3);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define RLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                             \
        BYTE rot, tmp;                                               \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        tmp = LOAD((addr));                                          \
        rot = (tmp & 0x80) ? C_FLAG : 0;                             \
        tmp = (tmp << 1) | LOCAL_CARRY();                            \
        CLK_ADD(CLK, clk_inc2);                                      \
        STORE((addr), tmp);                                          \
        reg_val = tmp;                                               \
        reg_f = rot | SZP[tmp];                                      \
        CLK_ADD(CLK, clk_inc3);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define RR(reg_val)                                            \
    do {                                                       \
        BYTE rot;                                              \
                                                               \
        rot = reg_val & C_FLAG;                                \
        reg_val = (reg_val >> 1) | (LOCAL_CARRY() ? 0x80 : 0); \
        reg_f = rot | SZP[reg_val];                            \
        CLK_ADD(CLK, 8);                                       \
        INC_PC(2);                                             \
    } while (0)

#define RRA(clk_inc, pc_inc)                               \
    do {                                                   \
        BYTE rot;                                          \
                                                           \
        rot = reg_a & C_FLAG;                              \
        reg_a = (reg_a >> 1) | (LOCAL_CARRY() ? 0x80 : 0); \
        LOCAL_SET_CARRY(rot);                              \
        LOCAL_SET_NADDSUB(0);                              \
        LOCAL_SET_HALFCARRY(0);                            \
        CLK_ADD(CLK, clk_inc);                             \
        INC_PC(pc_inc);                                    \
    } while (0)

#define RRC(reg_val)                                   \
    do {                                               \
        BYTE rot;                                      \
                                                       \
        rot = reg_val & C_FLAG;                        \
        reg_val = (reg_val >> 1) | ((rot) ? 0x80 : 0); \
        reg_f = rot | SZP[reg_val];                    \
        CLK_ADD(CLK, 8);                               \
        INC_PC(2);                                     \
    } while (0)

#define RRCA(clk_inc, pc_inc)                      \
    do {                                           \
        BYTE rot;                                  \
                                                   \
        rot = reg_a & C_FLAG;                      \
        reg_a = (reg_a >> 1) | ((rot) ? 0x80 : 0); \
        LOCAL_SET_CARRY(rot);                      \
        LOCAL_SET_NADDSUB(0);                      \
        LOCAL_SET_HALFCARRY(0);                    \
        CLK_ADD(CLK, clk_inc);                     \
        INC_PC(pc_inc);                            \
    } while (0)

#define RRCXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp = (tmp >> 1) | ((rot) ? 0x80 : 0);            \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define RRCXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp = (tmp >> 1) | ((rot) ? 0x80 : 0);                        \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define RRD()                                        \
    do {                                             \
        BYTE tmp;                                    \
                                                     \
        tmp = LOAD(HL_WORD());                       \
        CLK_ADD(CLK, 8);                             \
        STORE(HL_WORD(), (tmp >> 4) | (reg_a << 4)); \
        reg_a = (tmp & 0x0f) | (reg_a & 0xf0);       \
        reg_f = SZP[reg_a] | LOCAL_CARRY();          \
        CLK_ADD(CLK, 10);                            \
        INC_PC(2);                                   \
    } while (0)

#define RRXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                 \
        BYTE rot, tmp;                                   \
                                                         \
        CLK_ADD(CLK, clk_inc1);                          \
        tmp = LOAD((addr));                              \
        rot = tmp & C_FLAG;                              \
        tmp = (tmp >> 1) | (LOCAL_CARRY() ? 0x80 : 0);   \
        CLK_ADD(CLK, clk_inc2);                          \
        STORE((addr), tmp);                              \
        reg_f = rot | SZP[tmp];                          \
        CLK_ADD(CLK, clk_inc3);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define RRXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                             \
        BYTE rot, tmp;                                               \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        tmp = LOAD((addr));                                          \
        rot = tmp & C_FLAG;                                          \
        tmp = (tmp >> 1) | (LOCAL_CARRY() ? 0x80 : 0);               \
        CLK_ADD(CLK, clk_inc2);                                      \
        STORE((addr), tmp);                                          \
        reg_val = tmp;                                               \
        reg_f = rot | SZP[tmp];                                      \
        CLK_ADD(CLK, clk_inc3);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define SBCHLREG(reg_valh, reg_vall)                                                     \
    do {                                                                                 \
        DWORD tmp;                                                                       \
        BYTE carry;                                                                      \
                                                                                         \
        carry = LOCAL_CARRY();                                                           \
        tmp = (DWORD)(HL_WORD()) - (DWORD)((reg_valh << 8) + reg_vall) - (DWORD)(carry); \
        reg_f = N_FLAG;                                                                  \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                  \
        LOCAL_SET_HALFCARRY((reg_h ^ reg_valh ^ (tmp >> 8)) & H_FLAG);                   \
        LOCAL_SET_PARITY(((reg_h ^ (tmp >> 8)) & (reg_h ^ reg_valh)) & 0x80);            \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                 \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                    \
        reg_h = (BYTE)(tmp >> 8);                                                        \
        reg_l = (BYTE)(tmp & 0xff);                                                      \
        CLK_ADD(CLK, 15);                                                                \
        INC_PC(2);                                                                       \
    } while (0)

#define SBCHLSP()                                                                  \
    do {                                                                           \
        DWORD tmp;                                                                 \
        BYTE carry;                                                                \
                                                                                   \
        carry = LOCAL_CARRY();                                                     \
        tmp = (DWORD)(HL_WORD()) - (DWORD)reg_sp - (DWORD)(carry);                 \
        reg_f = N_FLAG;                                                            \
        LOCAL_SET_CARRY(tmp & 0x10000);                                            \
        LOCAL_SET_HALFCARRY((reg_h ^ (reg_sp >> 8) ^ (tmp >> 8)) & H_FLAG);        \
        LOCAL_SET_PARITY(((reg_h ^ (tmp >> 8)) & (reg_h ^ (reg_sp >> 8))) & 0x80); \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                           \
        LOCAL_SET_SIGN(tmp & 0x8000);                                              \
        reg_h = (BYTE)(tmp >> 8);                                                  \
        reg_l = (BYTE)(tmp & 0xff);                                                \
        CLK_ADD(CLK, 15);                                                          \
        INC_PC(2);                                                                 \
    } while (0)

#define SBC(loadval, clk_inc1, clk_inc2, pc_inc)                      \
    do {                                                              \
        BYTE tmp, carry, value;                                       \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        value = (BYTE)(loadval);                                      \
        carry = LOCAL_CARRY();                                        \
        tmp = reg_a - value - carry;                                  \
        reg_f = N_FLAG | SZP[tmp];                                    \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);          \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80);     \
        LOCAL_SET_CARRY((WORD)((WORD)value + (WORD)(carry)) > reg_a); \
        reg_a = tmp;                                                  \
        CLK_ADD(CLK, clk_inc2);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SCF(clk_inc, pc_inc)    \
    do {                        \
        LOCAL_SET_CARRY(1);     \
        LOCAL_SET_HALFCARRY(0); \
        LOCAL_SET_NADDSUB(0);   \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define SET(reg_val, value)      \
    do {                         \
        reg_val |= (1 << value); \
        CLK_ADD(CLK, 8);         \
        INC_PC(2);               \
    } while (0)

#define SETXX(value, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                         \
        BYTE tmp;                                                \
                                                                 \
        CLK_ADD(CLK, clk_inc1);                                  \
        tmp = LOAD((addr));                                      \
        tmp |= (1 << value);                                     \
        CLK_ADD(CLK, clk_inc2);                                  \
        STORE((addr), tmp);                                      \
        CLK_ADD(CLK, clk_inc3);                                  \
        INC_PC(pc_inc);                                          \
    } while (0)

#define SETXXREG(value, reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        BYTE tmp;                                                            \
                                                                             \
        CLK_ADD(CLK, clk_inc1);                                              \
        tmp = LOAD((addr));                                                  \
        tmp |= (1 << value);                                                 \
        CLK_ADD(CLK, clk_inc2);                                              \
        STORE((addr), tmp);                                                  \
        reg_val = tmp;                                                       \
        CLK_ADD(CLK, clk_inc3);                                              \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define SLA(reg_val)                         \
    do {                                     \
        BYTE rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val <<= 1;                       \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define SLAXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp <<= 1;                                        \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SLAXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp <<= 1;                                                    \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SLL(reg_val)                         \
    do {                                     \
        BYTE rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val = (reg_val << 1) | 1;        \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define SLLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp = (tmp << 1) | 1;                             \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SLLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp = (tmp << 1) | 1;                                         \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SRA(reg_val)                                 \
    do {                                             \
        BYTE rot;                                    \
                                                     \
        rot = reg_val & C_FLAG;                      \
        reg_val = (reg_val >> 1) | (reg_val & 0x80); \
        reg_f = rot | SZP[reg_val];                  \
        CLK_ADD(CLK, 8);                             \
        INC_PC(2);                                   \
    } while (0)

#define SRAXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp = (tmp >> 1) | (tmp & 0x80);                  \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SRAXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp = (tmp >> 1) | (tmp & 0x80);                              \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SRL(reg_val)                \
    do {                            \
        BYTE rot;                   \
                                    \
        rot = reg_val & C_FLAG;     \
        reg_val >>= 1;              \
        reg_f = rot | SZP[reg_val]; \
        CLK_ADD(CLK, 8);            \
        INC_PC(2);                  \
    } while (0)

#define SRLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        BYTE rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp >>= 1;                                        \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SRLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        BYTE rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp >>= 1;                                                    \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define STW(addr, reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                    \
        CLK_ADD(CLK, clk_inc1);                                             \
        STORE((WORD)(addr), reg_vall);                                      \
        CLK_ADD(CLK, clk_inc2);                                             \
        STORE((WORD)(addr + 1), reg_valh);                                  \
        CLK_ADD(CLK, clk_inc3);                                             \
        INC_PC(pc_inc);                                                     \
    } while (0)

#define STSPW(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        CLK_ADD(CLK, clk_inc1);                           \
        STORE((WORD)(addr), (reg_sp & 0xff));             \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((WORD)(addr + 1), (reg_sp >> 8));           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define STREG(addr, reg_val, clk_inc1, clk_inc2, pc_inc) \
    do {                                                 \
        CLK_ADD(CLK, clk_inc1);                          \
        STORE(addr, reg_val);                            \
        CLK_ADD(CLK, clk_inc2);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define SUB(loadval, clk_inc1, clk_inc2, pc_inc)                  \
    do {                                                          \
        BYTE tmp, value;                                          \
                                                                  \
        CLK_ADD(CLK, clk_inc1);                                   \
        value = (BYTE)(loadval);                                  \
        tmp = reg_a - value;                                      \
        reg_f = N_FLAG | SZP[tmp];                                \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);      \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80); \
        LOCAL_SET_CARRY(value > reg_a);                           \
        reg_a = tmp;                                              \
        CLK_ADD(CLK, clk_inc2);                                   \
        INC_PC(pc_inc);                                           \
    } while (0)

#define XOR(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        CLK_ADD(CLK, clk_inc1);                \
        reg_a ^= value;                        \
        reg_f = SZP[reg_a];                    \
        CLK_ADD(CLK, clk_inc2);                \
        INC_PC(pc_inc);                        \
    } while (0)


/* ------------------------------------------------------------------------- */

/* Extended opcodes.  */

static void opcode_cb(BYTE ip1, BYTE ip2, BYTE ip3, WORD ip12, WORD ip23)
{
    switch (ip1) {
        case 0x00: /* RLC B */
            RLC(reg_b);
            break;
        case 0x01: /* RLC C */
            RLC(reg_c);
            break;
        case 0x02: /* RLC D */
            RLC(reg_d);
            break;
        case 0x03: /* RLC E */
            RLC(reg_e);
            break;
        case 0x04: /* RLC H */
            RLC(reg_h);
            break;
        case 0x05: /* RLC L */
            RLC(reg_l);
            break;
        case 0x06: /* RLC (HL) */
            RLCXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x07: /* RLC A */
            RLC(reg_a);
            break;
        case 0x08: /* RRC B */
            RRC(reg_b);
            break;
        case 0x09: /* RRC C */
            RRC(reg_c);
            break;
        case 0x0a: /* RRC D */
            RRC(reg_d);
            break;
        case 0x0b: /* RRC E */
            RRC(reg_e);
            break;
        case 0x0c: /* RRC H */
            RRC(reg_h);
            break;
        case 0x0d: /* RRC L */
            RRC(reg_l);
            break;
        case 0x0e: /* RRC (HL) */
            RRCXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x0f: /* RRC A */
            RRC(reg_a);
            break;
        case 0x10: /* RL B */
            RL(reg_b);
            break;
        case 0x11: /* RL C */
            RL(reg_c);
            break;
        case 0x12: /* RL D */
            RL(reg_d);
            break;
        case 0x13: /* RL E */
            RL(reg_e);
            break;
        case 0x14: /* RL H */
            RL(reg_h);
            break;
        case 0x15: /* RL L */
            RL(reg_l);
            break;
        case 0x16: /* RL (HL) */
            RLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x17: /* RL A */
            RL(reg_a);
            break;
        case 0x18: /* RR B */
            RR(reg_b);
            break;
        case 0x19: /* RR C */
            RR(reg_c);
            break;
        case 0x1a: /* RR D */
            RR(reg_d);
            break;
        case 0x1b: /* RR E */
            RR(reg_e);
            break;
        case 0x1c: /* RR H */
            RR(reg_h);
            break;
        case 0x1d: /* RR L */
            RR(reg_l);
            break;
        case 0x1e: /* RR (HL) */
            RRXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x1f: /* RR A */
            RR(reg_a);
            break;
        case 0x20: /* SLA B */
            SLA(reg_b);
            break;
        case 0x21: /* SLA C */
            SLA(reg_c);
            break;
        case 0x22: /* SLA D */
            SLA(reg_d);
            break;
        case 0x23: /* SLA E */
            SLA(reg_e);
            break;
        case 0x24: /* SLA H */
            SLA(reg_h);
            break;
        case 0x25: /* SLA L */
            SLA(reg_l);
            break;
        case 0x26: /* SLA (HL) */
            SLAXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x27: /* SLA A */
            SLA(reg_a);
            break;
        case 0x28: /* SRA B */
            SRA(reg_b);
            break;
        case 0x29: /* SRA C */
            SRA(reg_c);
            break;
        case 0x2a: /* SRA D */
            SRA(reg_d);
            break;
        case 0x2b: /* SRA E */
            SRA(reg_e);
            break;
        case 0x2c: /* SRA H */
            SRA(reg_h);
            break;
        case 0x2d: /* SRA L */
            SRA(reg_l);
            break;
        case 0x2e: /* SRA (HL) */
            SRAXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x2f: /* SRA A */
            SRA(reg_a);
            break;
        case 0x30: /* SLL B */
            SLL(reg_b);
            break;
        case 0x31: /* SLL C */
            SLL(reg_c);
            break;
        case 0x32: /* SLL D */
            SLL(reg_d);
            break;
        case 0x33: /* SLL E */
            SLL(reg_e);
            break;
        case 0x34: /* SLL H */
            SLL(reg_h);
            break;
        case 0x35: /* SLL L */
            SLL(reg_l);
            break;
        case 0x36: /* SLL (HL) */
            SLLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x37: /* SLL A */
            SLL(reg_a);
            break;
        case 0x38: /* SRL B */
            SRL(reg_b);
            break;
        case 0x39: /* SRL C */
            SRL(reg_c);
            break;
        case 0x3a: /* SRL D */
            SRL(reg_d);
            break;
        case 0x3b: /* SRL E */
            SRL(reg_e);
            break;
        case 0x3c: /* SRL H */
            SRL(reg_h);
            break;
        case 0x3d: /* SRL L */
            SRL(reg_l);
            break;
        case 0x3e: /* SRL (HL) */
            SRLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x3f: /* SRL A */
            SRL(reg_a);
            break;
        case 0x40: /* BIT B 0 */
            BIT(reg_b, 0, 0, 8, 2);
            break;
        case 0x41: /* BIT C 0 */
            BIT(reg_c, 0, 0, 8, 2);
            break;
        case 0x42: /* BIT D 0 */
            BIT(reg_d, 0, 0, 8, 2);
            break;
        case 0x43: /* BIT E 0 */
            BIT(reg_e, 0, 0, 8, 2);
            break;
        case 0x44: /* BIT H 0 */
            BIT(reg_h, 0, 0, 8, 2);
            break;
        case 0x45: /* BIT L 0 */
            BIT(reg_l, 0, 0, 8, 2);
            break;
        case 0x46: /* BIT (HL) 0 */
            BIT(LOAD(HL_WORD()), 0, 4, 8, 2);
            break;
        case 0x47: /* BIT A 0 */
            BIT(reg_a, 0, 0, 8, 2);
            break;
        case 0x48: /* BIT B 1 */
            BIT(reg_b, 1, 0, 8, 2);
            break;
        case 0x49: /* BIT C 1 */
            BIT(reg_c, 1, 0, 8, 2);
            break;
        case 0x4a: /* BIT D 1 */
            BIT(reg_d, 1, 0, 8, 2);
            break;
        case 0x4b: /* BIT E 1 */
            BIT(reg_e, 1, 0, 8, 2);
            break;
        case 0x4c: /* BIT H 1 */
            BIT(reg_h, 1, 0, 8, 2);
            break;
        case 0x4d: /* BIT L 1 */
            BIT(reg_l, 1, 0, 8, 2);
            break;
        case 0x4e: /* BIT (HL) 1 */
            BIT(LOAD(HL_WORD()), 1, 4, 8, 2);
            break;
        case 0x4f: /* BIT A 1 */
            BIT(reg_a, 1, 0, 8, 2);
            break;
        case 0x50: /* BIT B 2 */
            BIT(reg_b, 2, 0, 8, 2);
            break;
        case 0x51: /* BIT C 2 */
            BIT(reg_c, 2, 0, 8, 2);
            break;
        case 0x52: /* BIT D 2 */
            BIT(reg_d, 2, 0, 8, 2);
            break;
        case 0x53: /* BIT E 2 */
            BIT(reg_e, 2, 0, 8, 2);
            break;
        case 0x54: /* BIT H 2 */
            BIT(reg_h, 2, 0, 8, 2);
            break;
        case 0x55: /* BIT L 2 */
            BIT(reg_l, 2, 0, 8, 2);
            break;
        case 0x56: /* BIT (HL) 2 */
            BIT(LOAD(HL_WORD()), 2, 4, 8, 2);
            break;
        case 0x57: /* BIT A 2 */
            BIT(reg_a, 2, 0, 8, 2);
            break;
        case 0x58: /* BIT B 3 */
            BIT(reg_b, 3, 0, 8, 2);
            break;
        case 0x59: /* BIT C 3 */
            BIT(reg_c, 3, 0, 8, 2);
            break;
        case 0x5a: /* BIT D 3 */
            BIT(reg_d, 3, 0, 8, 2);
            break;
        case 0x5b: /* BIT E 3 */
            BIT(reg_e, 3, 0, 8, 2);
            break;
        case 0x5c: /* BIT H 3 */
            BIT(reg_h, 3, 0, 8, 2);
            break;
        case 0x5d: /* BIT L 3 */
            BIT(reg_l, 3, 0, 8, 2);
            break;
        case 0x5e: /* BIT (HL) 3 */
            BIT(LOAD(HL_WORD()), 3, 4, 8, 2);
            break;
        case 0x5f: /* BIT A 3 */
            BIT(reg_a, 3, 0, 8, 2);
            break;
        case 0x60: /* BIT B 4 */
            BIT(reg_b, 4, 0, 8, 2);
            break;
        case 0x61: /* BIT C 4 */
            BIT(reg_c, 4, 0, 8, 2);
            break;
        case 0x62: /* BIT D 4 */
            BIT(reg_d, 4, 0, 8, 2);
            break;
        case 0x63: /* BIT E 4 */
            BIT(reg_e, 4, 0, 8, 2);
            break;
        case 0x64: /* BIT H 4 */
            BIT(reg_h, 4, 0, 8, 2);
            break;
        case 0x65: /* BIT L 4 */
            BIT(reg_l, 4, 0, 8, 2);
            break;
        case 0x66: /* BIT (HL) 4 */
            BIT(LOAD(HL_WORD()), 4, 4, 8, 2);
            break;
        case 0x67: /* BIT A 4 */
            BIT(reg_a, 4, 0, 8, 2);
            break;
        case 0x68: /* BIT B 5 */
            BIT(reg_b, 5, 0, 8, 2);
            break;
        case 0x69: /* BIT C 5 */
            BIT(reg_c, 5, 0, 8, 2);
            break;
        case 0x6a: /* BIT D 5 */
            BIT(reg_d, 5, 0, 8, 2);
            break;
        case 0x6b: /* BIT E 5 */
            BIT(reg_e, 5, 0, 8, 2);
            break;
        case 0x6c: /* BIT H 5 */
            BIT(reg_h, 5, 0, 8, 2);
            break;
        case 0x6d: /* BIT L 5 */
            BIT(reg_l, 5, 0, 8, 2);
            break;
        case 0x6e: /* BIT (HL) 5 */
            BIT(LOAD(HL_WORD()), 5, 4, 8, 2);
            break;
        case 0x6f: /* BIT A 5 */
            BIT(reg_a, 5, 0, 8, 2);
            break;
        case 0x70: /* BIT B 6 */
            BIT(reg_b, 6, 0, 8, 2);
            break;
        case 0x71: /* BIT C 6 */
            BIT(reg_c, 6, 0, 8, 2);
            break;
        case 0x72: /* BIT D 6 */
            BIT(reg_d, 6, 0, 8, 2);
            break;
        case 0x73: /* BIT E 6 */
            BIT(reg_e, 6, 0, 8, 2);
            break;
        case 0x74: /* BIT H 6 */
            BIT(reg_h, 6, 0, 8, 2);
            break;
        case 0x75: /* BIT L 6 */
            BIT(reg_l, 6, 0, 8, 2);
            break;
        case 0x76: /* BIT (HL) 6 */
            BIT(LOAD(HL_WORD()), 6, 4, 8, 2);
            break;
        case 0x77: /* BIT A 6 */
            BIT(reg_a, 6, 0, 8, 2);
            break;
        case 0x78: /* BIT B 7 */
            BIT(reg_b, 7, 0, 8, 2);
            break;
        case 0x79: /* BIT C 7 */
            BIT(reg_c, 7, 0, 8, 2);
            break;
        case 0x7a: /* BIT D 7 */
            BIT(reg_d, 7, 0, 8, 2);
            break;
        case 0x7b: /* BIT E 7 */
            BIT(reg_e, 7, 0, 8, 2);
            break;
        case 0x7c: /* BIT H 7 */
            BIT(reg_h, 7, 0, 8, 2);
            break;
        case 0x7d: /* BIT L 7 */
            BIT(reg_l, 7, 0, 8, 2);
            break;
        case 0x7e: /* BIT (HL) 7 */
            BIT(LOAD(HL_WORD()), 7, 4, 8, 2);
            break;
        case 0x7f: /* BIT A 7 */
            BIT(reg_a, 7, 0, 8, 2);
            break;
        case 0x80: /* RES B 0 */
            RES(reg_b, 0);
            break;
        case 0x81: /* RES C 0 */
            RES(reg_c, 0);
            break;
        case 0x82: /* RES D 0 */
            RES(reg_d, 0);
            break;
        case 0x83: /* RES E 0 */
            RES(reg_e, 0);
            break;
        case 0x84: /* RES H 0 */
            RES(reg_h, 0);
            break;
        case 0x85: /* RES L 0 */
            RES(reg_l, 0);
            break;
        case 0x86: /* RES (HL) 0 */
            RESXX(0, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x87: /* RES A 0 */
            RES(reg_a, 0);
            break;
        case 0x88: /* RES B 1 */
            RES(reg_b, 1);
            break;
        case 0x89: /* RES C 1 */
            RES(reg_c, 1);
            break;
        case 0x8a: /* RES D 1 */
            RES(reg_d, 1);
            break;
        case 0x8b: /* RES E 1 */
            RES(reg_e, 1);
            break;
        case 0x8c: /* RES H 1 */
            RES(reg_h, 1);
            break;
        case 0x8d: /* RES L 1 */
            RES(reg_l, 1);
            break;
        case 0x8e: /* RES (HL) 1 */
            RESXX(1, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x8f: /* RES A 1 */
            RES(reg_a, 1);
            break;
        case 0x90: /* RES B 2 */
            RES(reg_b, 2);
            break;
        case 0x91: /* RES C 2 */
            RES(reg_c, 2);
            break;
        case 0x92: /* RES D 2 */
            RES(reg_d, 2);
            break;
        case 0x93: /* RES E 2 */
            RES(reg_e, 2);
            break;
        case 0x94: /* RES H 2 */
            RES(reg_h, 2);
            break;
        case 0x95: /* RES L 2 */
            RES(reg_l, 2);
            break;
        case 0x96: /* RES (HL) 2 */
            RESXX(2, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x97: /* RES A 2 */
            RES(reg_a, 2);
            break;
        case 0x98: /* RES B 3 */
            RES(reg_b, 3);
            break;
        case 0x99: /* RES C 3 */
            RES(reg_c, 3);
            break;
        case 0x9a: /* RES D 3 */
            RES(reg_d, 3);
            break;
        case 0x9b: /* RES E 3 */
            RES(reg_e, 3);
            break;
        case 0x9c: /* RES H 3 */
            RES(reg_h, 3);
            break;
        case 0x9d: /* RES L 3 */
            RES(reg_l, 3);
            break;
        case 0x9e: /* RES (HL) 3 */
            RESXX(3, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x9f: /* RES A 3 */
            RES(reg_a, 3);
            break;
        case 0xa0: /* RES B 4 */
            RES(reg_b, 4);
            break;
        case 0xa1: /* RES C 4 */
            RES(reg_c, 4);
            break;
        case 0xa2: /* RES D 4 */
            RES(reg_d, 4);
            break;
        case 0xa3: /* RES E 4 */
            RES(reg_e, 4);
            break;
        case 0xa4: /* RES H 4 */
            RES(reg_h, 4);
            break;
        case 0xa5: /* RES L 4 */
            RES(reg_l, 4);
            break;
        case 0xa6: /* RES (HL) 4 */
            RESXX(4, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xa7: /* RES A 4 */
            RES(reg_a, 4);
            break;
        case 0xa8: /* RES B 5 */
            RES(reg_b, 5);
            break;
        case 0xa9: /* RES C 5 */
            RES(reg_c, 5);
            break;
        case 0xaa: /* RES D 5 */
            RES(reg_d, 5);
            break;
        case 0xab: /* RES E 5 */
            RES(reg_e, 5);
            break;
        case 0xac: /* RES H 5 */
            RES(reg_h, 5);
            break;
        case 0xad: /* RES L 5 */
            RES(reg_l, 5);
            break;
        case 0xae: /* RES (HL) 5 */
            RESXX(5, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xaf: /* RES A 5 */
            RES(reg_a, 5);
            break;
        case 0xb0: /* RES B 6 */
            RES(reg_b, 6);
            break;
        case 0xb1: /* RES C 6 */
            RES(reg_c, 6);
            break;
        case 0xb2: /* RES D 6 */
            RES(reg_d, 6);
            break;
        case 0xb3: /* RES E 6 */
            RES(reg_e, 6);
            break;
        case 0xb4: /* RES H 6 */
            RES(reg_h, 6);
            break;
        case 0xb5: /* RES L 6 */
            RES(reg_l, 6);
            break;
        case 0xb6: /* RES (HL) 6 */
            RESXX(6, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xb7: /* RES A 6 */
            RES(reg_a, 6);
            break;
        case 0xb8: /* RES B 7 */
            RES(reg_b, 7);
            break;
        case 0xb9: /* RES C 7 */
            RES(reg_c, 7);
            break;
        case 0xba: /* RES D 7 */
            RES(reg_d, 7);
            break;
        case 0xbb: /* RES E 7 */
            RES(reg_e, 7);
            break;
        case 0xbc: /* RES H 7 */
            RES(reg_h, 7);
            break;
        case 0xbd: /* RES L 7 */
            RES(reg_l, 7);
            break;
        case 0xbe: /* RES (HL) 7 */
            RESXX(7, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xbf: /* RES A 7 */
            RES(reg_a, 7);
            break;
        case 0xc0: /* SET B 0 */
            SET(reg_b, 0);
            break;
        case 0xc1: /* SET C 0 */
            SET(reg_c, 0);
            break;
        case 0xc2: /* SET D 0 */
            SET(reg_d, 0);
            break;
        case 0xc3: /* SET E 0 */
            SET(reg_e, 0);
            break;
        case 0xc4: /* SET H 0 */
            SET(reg_h, 0);
            break;
        case 0xc5: /* SET L 0 */
            SET(reg_l, 0);
            break;
        case 0xc6: /* SET (HL) 0 */
            SETXX(0, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xc7: /* SET A 0 */
            SET(reg_a, 0);
            break;
        case 0xc8: /* SET B 1 */
            SET(reg_b, 1);
            break;
        case 0xc9: /* SET C 1 */
            SET(reg_c, 1);
            break;
        case 0xca: /* SET D 1 */
            SET(reg_d, 1);
            break;
        case 0xcb: /* SET E 1 */
            SET(reg_e, 1);
            break;
        case 0xcc: /* SET H 1 */
            SET(reg_h, 1);
            break;
        case 0xcd: /* SET L 1 */
            SET(reg_l, 1);
            break;
        case 0xce: /* SET (HL) 1 */
            SETXX(1, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xcf: /* SET A 1 */
            SET(reg_a, 1);
            break;
        case 0xd0: /* SET B 2 */
            SET(reg_b, 2);
            break;
        case 0xd1: /* SET C 2 */
            SET(reg_c, 2);
            break;
        case 0xd2: /* SET D 2 */
            SET(reg_d, 2);
            break;
        case 0xd3: /* SET E 2 */
            SET(reg_e, 2);
            break;
        case 0xd4: /* SET H 2 */
            SET(reg_h, 2);
            break;
        case 0xd5: /* SET L 2 */
            SET(reg_l, 2);
            break;
        case 0xd6: /* SET (HL) 2 */
            SETXX(2, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xd7: /* SET A 2 */
            SET(reg_a, 2);
            break;
        case 0xd8: /* SET B 3 */
            SET(reg_b, 3);
            break;
        case 0xd9: /* SET C 3 */
            SET(reg_c, 3);
            break;
        case 0xda: /* SET D 3 */
            SET(reg_d, 3);
            break;
        case 0xdb: /* SET E 3 */
            SET(reg_e, 3);
            break;
        case 0xdc: /* SET H 3 */
            SET(reg_h, 3);
            break;
        case 0xdd: /* SET L 3 */
            SET(reg_l, 3);
            break;
        case 0xde: /* SET (HL) 3 */
            SETXX(3, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xdf: /* SET A 3 */
            SET(reg_a, 3);
            break;
        case 0xe0: /* SET B 4 */
            SET(reg_b, 4);
            break;
        case 0xe1: /* SET C 4 */
            SET(reg_c, 4);
            break;
        case 0xe2: /* SET D 4 */
            SET(reg_d, 4);
            break;
        case 0xe3: /* SET E 4 */
            SET(reg_e, 4);
            break;
        case 0xe4: /* SET H 4 */
            SET(reg_h, 4);
            break;
        case 0xe5: /* SET L 4 */
            SET(reg_l, 4);
            break;
        case 0xe6: /* SET (HL) 4 */
            SETXX(4, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xe7: /* SET A 4 */
            SET(reg_a, 4);
            break;
        case 0xe8: /* SET B 5 */
            SET(reg_b, 5);
            break;
        case 0xe9: /* SET C 5 */
            SET(reg_c, 5);
            break;
        case 0xea: /* SET D 5 */
            SET(reg_d, 5);
            break;
        case 0xeb: /* SET E 5 */
            SET(reg_e, 5);
            break;
        case 0xec: /* SET H 5 */
            SET(reg_h, 5);
            break;
        case 0xed: /* SET L 5 */
            SET(reg_l, 5);
            break;
        case 0xee: /* SET (HL) 5 */
            SETXX(5, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xef: /* SET A 5 */
            SET(reg_a, 5);
            break;
        case 0xf0: /* SET B 6 */
            SET(reg_b, 6);
            break;
        case 0xf1: /* SET C 6 */
            SET(reg_c, 6);
            break;
        case 0xf2: /* SET D 6 */
            SET(reg_d, 6);
            break;
        case 0xf3: /* SET E 6 */
            SET(reg_e, 6);
            break;
        case 0xf4: /* SET H 6 */
            SET(reg_h, 6);
            break;
        case 0xf5: /* SET L 6 */
            SET(reg_l, 6);
            break;
        case 0xf6: /* SET (HL) 6 */
            SETXX(6, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xf7: /* SET A 6 */
            SET(reg_a, 6);
            break;
        case 0xf8: /* SET B 7 */
            SET(reg_b, 7);
            break;
        case 0xf9: /* SET C 7 */
            SET(reg_c, 7);
            break;
        case 0xfa: /* SET D 7 */
            SET(reg_d, 7);
            break;
        case 0xfb: /* SET E 7 */
            SET(reg_e, 7);
            break;
        case 0xfc: /* SET H 7 */
            SET(reg_h, 7);
            break;
        case 0xfd: /* SET L 7 */
            SET(reg_l, 7);
            break;
        case 0xfe: /* SET (HL) 7 */
            SETXX(7, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xff: /* SET A 7 */
            SET(reg_a, 7);
            break;
        default:
            INC_PC(2);
    }
}

static void opcode_dd_cb(BYTE iip2, BYTE iip3, WORD iip23)
{
    switch (iip3) {
        case 0x00: /* RLC (IX+d),B */
            RLCXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x01: /* RLC (IX+d),C */
            RLCXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x02: /* RLC (IX+d),D */
            RLCXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x03: /* RLC (IX+d),E */
            RLCXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x04: /* RLC (IX+d),H */
            RLCXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x05: /* RLC (IX+d),L */
            RLCXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x06: /* RLC (IX+d) */
            RLCXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x07: /* RLC (IX+d),A */
            RLCXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x08: /* RRC (IX+d),B */
            RRCXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x09: /* RRC (IX+d),C */
            RRCXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0a: /* RRC (IX+d),D */
            RRCXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0b: /* RRC (IX+d),E */
            RRCXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0c: /* RRC (IX+d),H */
            RRCXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0d: /* RRC (IX+d),L */
            RRCXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0e: /* RRC (IX+d) */
            RRCXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0f: /* RRC (IX+d),A */
            RRCXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x10: /* RL (IX+d),B */
            RLXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x11: /* RL (IX+d),C */
            RLXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x12: /* RL (IX+d),D */
            RLXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x13: /* RL (IX+d),E */
            RLXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x14: /* RL (IX+d),H */
            RLXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x15: /* RL (IX+d),L */
            RLXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x16: /* RL (IX+d) */
            RLXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x17: /* RL (IX+d),A */
            RLXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x18: /* RR (IX+d),B */
            RRXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x19: /* RR (IX+d),C */
            RRXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1a: /* RR (IX+d),D */
            RRXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1b: /* RR (IX+d),E */
            RRXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1c: /* RR (IX+d),H */
            RRXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1d: /* RR (IX+d),L */
            RRXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1e: /* RR (IX+d) */
            RRXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1f: /* RR (IX+d),A */
            RRXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x20: /* SLA (IX+d),B */
            SLAXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x21: /* SLA (IX+d),C */
            SLAXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x22: /* SLA (IX+d),D */
            SLAXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x23: /* SLA (IX+d),E */
            SLAXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x24: /* SLA (IX+d),H */
            SLAXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x25: /* SLA (IX+d),L */
            SLAXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x26: /* SLA (IX+d) */
            SLAXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x27: /* SLA (IX+d),A */
            SLAXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x28: /* SRA (IX+d),B */
            SRAXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x29: /* SRA (IX+d),C */
            SRAXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2a: /* SRA (IX+d),D */
            SRAXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2b: /* SRA (IX+d),E */
            SRAXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2c: /* SRA (IX+d),H */
            SRAXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2d: /* SRA (IX+d),L */
            SRAXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2e: /* SRA (IX+d) */
            SRAXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2f: /* SRA (IX+d),A */
            SRAXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x30: /* SLL (IX+d),B */
            SLLXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x31: /* SLL (IX+d),C */
            SLLXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x32: /* SLL (IX+d),D */
            SLLXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x33: /* SLL (IX+d),E */
            SLLXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x34: /* SLL (IX+d),H */
            SLLXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x35: /* SLL (IX+d),L */
            SLLXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x36: /* SLL (IX+d) */
            SLLXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x37: /* SLL (IX+d),A */
            SLLXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x38: /* SRL (IX+d),B */
            SRLXXREG(reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x39: /* SRL (IX+d),C */
            SRLXXREG(reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3a: /* SRL (IX+d),D */
            SRLXXREG(reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3b: /* SRL (IX+d),E */
            SRLXXREG(reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3c: /* SRL (IX+d),H */
            SRLXXREG(reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3d: /* SRL (IX+d),L */
            SRLXXREG(reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3e: /* SRL (IX+d) */
            SRLXX(IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3f: /* SRL (IX+d),A */
            SRLXXREG(reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x40: /* BIT (IX+d) 0 */
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            BIT(LOAD(IX_WORD_OFF(iip2)), 0, 8, 12, 4);
            break;
        case 0x48: /* BIT (IX+d) 1 */
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            BIT(LOAD(IX_WORD_OFF(iip2)), 1, 8, 12, 4);
            break;
        case 0x50: /* BIT (IX+d) 2 */
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            BIT(LOAD(IX_WORD_OFF(iip2)), 2, 8, 12, 4);
            break;
        case 0x58: /* BIT (IX+d) 3 */
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            BIT(LOAD(IX_WORD_OFF(iip2)), 3, 8, 12, 4);
            break;
        case 0x60: /* BIT (IX+d) 4 */
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
            BIT(LOAD(IX_WORD_OFF(iip2)), 4, 8, 12, 4);
            break;
        case 0x68: /* BIT (IX+d) 5 */
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
            BIT(LOAD(IX_WORD_OFF(iip2)), 5, 8, 12, 4);
            break;
        case 0x70: /* BIT (IX+d) 6 */
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
            BIT(LOAD(IX_WORD_OFF(iip2)), 6, 8, 12, 4);
            break;
        case 0x78: /* BIT (IX+d) 7 */
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            BIT(LOAD(IX_WORD_OFF(iip2)), 7, 8, 12, 4);
            break;
        case 0x80: /* RES (IX+d),B 0 */
            RESXXREG(0, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x81: /* RES (IX+d),C 0 */
            RESXXREG(0, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x82: /* RES (IX+d),D 0 */
            RESXXREG(0, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x83: /* RES (IX+d),E 0 */
            RESXXREG(0, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x84: /* RES (IX+d),H 0 */
            RESXXREG(0, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x85: /* RES (IX+d),L 0 */
            RESXXREG(0, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x86: /* RES (IX+d) 0 */
            RESXX(0, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x87: /* RES (IX+d),A 0 */
            RESXXREG(0, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x88: /* RES (IX+d),B 1 */
            RESXXREG(1, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x89: /* RES (IX+d),C 1 */
            RESXXREG(1, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8a: /* RES (IX+d),D 1 */
            RESXXREG(1, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8b: /* RES (IX+d),E 1 */
            RESXXREG(1, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8c: /* RES (IX+d),H 1 */
            RESXXREG(1, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8d: /* RES (IX+d),L 1 */
            RESXXREG(1, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8e: /* RES (IX+d) 1 */
            RESXX(1, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8f: /* RES (IX+d),A 1 */
            RESXXREG(1, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x90: /* RES (IX+d),B 2 */
            RESXXREG(2, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x91: /* RES (IX+d),C 2 */
            RESXXREG(2, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x92: /* RES (IX+d),D 2 */
            RESXXREG(2, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x93: /* RES (IX+d),E 2 */
            RESXXREG(2, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x94: /* RES (IX+d),H 2 */
            RESXXREG(2, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x95: /* RES (IX+d),L 2 */
            RESXXREG(2, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x96: /* RES (IX+d) 2 */
            RESXX(2, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x97: /* RES (IX+d),A 2 */
            RESXXREG(2, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x98: /* RES (IX+d),B 3 */
            RESXXREG(3, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x99: /* RES (IX+d),C 3 */
            RESXXREG(3, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9a: /* RES (IX+d),D 3 */
            RESXXREG(3, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9b: /* RES (IX+d),E 3 */
            RESXXREG(3, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9c: /* RES (IX+d),H 3 */
            RESXXREG(3, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9d: /* RES (IX+d),L 3 */
            RESXXREG(3, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9e: /* RES (IX+d) 3 */
            RESXX(3, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9f: /* RES (IX+d),A 3 */
            RESXXREG(3, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa0: /* RES (IX+d),B 4 */
            RESXXREG(4, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa1: /* RES (IX+d),C 4 */
            RESXXREG(4, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa2: /* RES (IX+d),D 4 */
            RESXXREG(4, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa3: /* RES (IX+d),E 4 */
            RESXXREG(4, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa4: /* RES (IX+d),H 4 */
            RESXXREG(4, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa5: /* RES (IX+d),L 4 */
            RESXXREG(4, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa6: /* RES (IX+d) 4 */
            RESXX(4, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa7: /* RES (IX+d),A 4 */
            RESXXREG(4, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa8: /* RES (IX+d),B 5 */
            RESXXREG(5, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa9: /* RES (IX+d),C 5 */
            RESXXREG(5, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xaa: /* RES (IX+d),D 5 */
            RESXXREG(5, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xab: /* RES (IX+d),E 5 */
            RESXXREG(5, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xac: /* RES (IX+d),H 5 */
            RESXXREG(5, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xad: /* RES (IX+d),L 5 */
            RESXXREG(5, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xae: /* RES (IX+d) 5 */
            RESXX(5, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xaf: /* RES (IX+d),A 5 */
            RESXXREG(5, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb0: /* RES (IX+d),B 6 */
            RESXXREG(6, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb1: /* RES (IX+d),C 6 */
            RESXXREG(6, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb2: /* RES (IX+d),D 6 */
            RESXXREG(6, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb3: /* RES (IX+d),E 6 */
            RESXXREG(6, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb4: /* RES (IX+d),H 6 */
            RESXXREG(6, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb5: /* RES (IX+d),L 6 */
            RESXXREG(6, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb6: /* RES (IX+d) 6 */
            RESXX(6, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb7: /* RES (IX+d),A 6 */
            RESXXREG(6, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb8: /* RES (IX+d),B 7 */
            RESXXREG(7, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb9: /* RES (IX+d),C 7 */
            RESXXREG(7, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xba: /* RES (IX+d),D 7 */
            RESXXREG(7, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbb: /* RES (IX+d),E 7 */
            RESXXREG(7, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbc: /* RES (IX+d),H 7 */
            RESXXREG(7, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbd: /* RES (IX+d),L 7 */
            RESXXREG(7, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbe: /* RES (IX+d) 7 */
            RESXX(7, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbf: /* RES (IX+d),A 7 */
            RESXXREG(7, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc0: /* SET (IX+d),B 0 */
            SETXXREG(0, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc1: /* SET (IX+d),C 0 */
            SETXXREG(0, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc2: /* SET (IX+d),D 0 */
            SETXXREG(0, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc3: /* SET (IX+d),E 0 */
            SETXXREG(0, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc4: /* SET (IX+d),H 0 */
            SETXXREG(0, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc5: /* SET (IX+d),L 0 */
            SETXXREG(0, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc6: /* SET (IX+d) 0 */
            SETXX(0, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc7: /* SET (IX+d),A 0 */
            SETXXREG(0, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc8: /* SET (IX+d),B 1 */
            SETXXREG(1, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc9: /* SET (IX+d),C 1 */
            SETXXREG(1, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xca: /* SET (IX+d),D 1 */
            SETXXREG(1, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcb: /* SET (IX+d),E 1 */
            SETXXREG(1, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcc: /* SET (IX+d),H 1 */
            SETXXREG(1, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcd: /* SET (IX+d),L 1 */
            SETXXREG(1, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xce: /* SET (IX+d) 1 */
            SETXX(1, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcf: /* SET (IX+d),A 1 */
            SETXXREG(1, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd0: /* SET (IX+d),B 2 */
            SETXXREG(2, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd1: /* SET (IX+d),C 2 */
            SETXXREG(2, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd2: /* SET (IX+d),D 2 */
            SETXXREG(2, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd3: /* SET (IX+d),E 2 */
            SETXXREG(2, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd4: /* SET (IX+d),H 2 */
            SETXXREG(2, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd5: /* SET (IX+d),L 2 */
            SETXXREG(2, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd6: /* SET (IX+d) 2 */
            SETXX(2, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd7: /* SET (IX+d),A 2 */
            SETXXREG(2, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd8: /* SET (IX+d),B 3 */
            SETXXREG(3, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd9: /* SET (IX+d),C 3 */
            SETXXREG(3, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xda: /* SET (IX+d),D 3 */
            SETXXREG(3, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdb: /* SET (IX+d),E 3 */
            SETXXREG(3, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdc: /* SET (IX+d),H 3 */
            SETXXREG(3, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdd: /* SET (IX+d),L 3 */
            SETXXREG(3, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xde: /* SET (IX+d) 3 */
            SETXX(3, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdf: /* SET (IX+d),A 3 */
            SETXXREG(3, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe0: /* SET (IX+d),B 4 */
            SETXXREG(4, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe1: /* SET (IX+d),C 4 */
            SETXXREG(4, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe2: /* SET (IX+d),D 4 */
            SETXXREG(4, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe3: /* SET (IX+d),E 4 */
            SETXXREG(4, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe4: /* SET (IX+d),H 4 */
            SETXXREG(4, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe5: /* SET (IX+d),L 4 */
            SETXXREG(4, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe6: /* SET (IX+d) 4 */
            SETXX(4, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe7: /* SET (IX+d),A 4 */
            SETXXREG(4, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe8: /* SET (IX+d),B 5 */
            SETXXREG(5, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe9: /* SET (IX+d),C 5 */
            SETXXREG(5, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xea: /* SET (IX+d),D 5 */
            SETXXREG(5, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xeb: /* SET (IX+d),E 5 */
            SETXXREG(5, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xec: /* SET (IX+d),H 5 */
            SETXXREG(5, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xed: /* SET (IX+d),L 5 */
            SETXXREG(5, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xee: /* SET (IX+d) 5 */
            SETXX(5, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xef: /* SET (IX+d),A 5 */
            SETXXREG(5, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf0: /* SET (IX+d),B 6 */
            SETXXREG(6, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf1: /* SET (IX+d),C 6 */
            SETXXREG(6, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf2: /* SET (IX+d),D 6 */
            SETXXREG(6, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf3: /* SET (IX+d),E 6 */
            SETXXREG(6, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf4: /* SET (IX+d),H 6 */
            SETXXREG(6, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf5: /* SET (IX+d),L 6 */
            SETXXREG(6, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf6: /* SET (IX+d) 6 */
            SETXX(6, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf7: /* SET (IX+d),A 6 */
            SETXXREG(6, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf8: /* SET (IX+d),B 7 */
            SETXXREG(7, reg_b, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf9: /* SET (IX+d),C 7 */
            SETXXREG(7, reg_c, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfa: /* SET (IX+d),D 7 */
            SETXXREG(7, reg_d, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfb: /* SET (IX+d),E 7 */
            SETXXREG(7, reg_e, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfc: /* SET (IX+d),H 7 */
            SETXXREG(7, reg_h, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfd: /* SET (IX+d),L 7 */
            SETXXREG(7, reg_l, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfe: /* SET (IX+d) 7 */
            SETXX(7, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xff: /* SET (IX+d),A 7 */
            SETXXREG(7, reg_a, IX_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        default:
            INC_PC(4);
    }
}

static void opcode_dd(BYTE ip1, BYTE ip2, BYTE ip3, WORD ip12, WORD ip23)
{
    switch (ip1) {
        case 0x00: /* NOP */
            NOP(8, 2);
            break;
        case 0x01: /* LD BC # */
            LDW(ip23, reg_b, reg_c, 10, 0, 4);
            break;
        case 0x02: /* LD (BC) A */
            STREG(BC_WORD(), reg_a, 8, 3, 2);
            break;
        case 0x03: /* INC BC */
            DECINC(INC_BC_WORD(), 10, 2);
            break;
        case 0x04: /* INC B */
            INCREG(reg_b, 7, 2);
            break;
        case 0x05: /* DEC B */
            DECREG(reg_b, 7, 2);
            break;
        case 0x06: /* LD B # */
            LDREG(reg_b, ip2, 4, 5, 3);
            break;
        case 0x07: /* RLCA */
            RLCA(8, 2);
            break;
        case 0x08: /* EX AF AF' */
            EXAFAF(12, 2);
            break;
        case 0x09: /* ADD IX BC */
            ADDXXREG(reg_ixh, reg_ixl, reg_b, reg_c, 15, 2);
            break;
        case 0x0a: /* LD A (BC) */
            LDREG(reg_a, LOAD(BC_WORD()), 8, 3, 2);
            break;
        case 0x0b: /* DEC BC */
            DECINC(DEC_BC_WORD(), 10, 2);
            break;
        case 0x0c: /* INC C */
            INCREG(reg_c, 7, 2);
            break;
        case 0x0d: /* DEC C */
            DECREG(reg_c, 7, 2);
            break;
        case 0x0e: /* LD C # */
            LDREG(reg_c, ip2, 4, 5, 3);
            break;
        case 0x0f: /* RRCA */
            RRCA(8, 2);
            break;
        case 0x10: /* DJNZ */
            DJNZ(ip2, 3);
            break;
        case 0x11: /* LD DE # */
            LDW(ip23, reg_d, reg_e, 10, 0, 4);
            break;
        case 0x12: /* LD (DE) A */
            STREG(DE_WORD(), reg_a, 8, 3, 2);
            break;
        case 0x13: /* INC DE */
            DECINC(INC_DE_WORD(), 10, 2);
            break;
        case 0x14: /* INC D */
            INCREG(reg_d, 7, 2);
            break;
        case 0x15: /* DEC D */
            DECREG(reg_d, 7, 2);
            break;
        case 0x16: /* LD D # */
            LDREG(reg_d, ip2, 4, 5, 3);
            break;
        case 0x17: /* RLA */
            RLA(8, 2);
            break;
        case 0x19: /* ADD IX DE */
            ADDXXREG(reg_ixh, reg_ixl, reg_d, reg_e, 15, 2);
            break;
        case 0x1a: /* LD A DE */
            LDREG(reg_a, LOAD(DE_WORD()), 8, 3, 2);
            break;
        case 0x1b: /* DEC DE */
            DECINC(DEC_DE_WORD(), 10, 2);
            break;
        case 0x1c: /* INC E */
            INCREG(reg_e, 7, 2);
            break;
        case 0x1d: /* DEC E */
            DECREG(reg_e, 7, 2);
            break;
        case 0x1e: /* LD E # */
            LDREG(reg_e, ip2, 4, 5, 3);
            break;
        case 0x1f: /* RRA */
            RRA(8, 2);
            break;
        case 0x20: /* JR NZ */
            BRANCH(!LOCAL_ZERO(), ip2, 3);
            break;
        case 0x21: /* LD IX # */
            LDW(ip23, reg_ixh, reg_ixl, 10, 4, 4);
            break;
        case 0x22: /* LD (WORD) IX */
            STW(ip23, reg_ixh, reg_ixl, 4, 9, 7, 4);
            break;
        case 0x23: /* INC IX */
            DECINC(INC_IX_WORD(), 10, 2);
            break;
        case 0x24: /* INC IXH */
            INCREG(reg_ixh, 7, 2);
            break;
        case 0x25: /* DEC IXH */
            DECREG(reg_ixh, 7, 2);
            break;
        case 0x26: /* LD IXH # */
            LDREG(reg_ixh, ip2, 4, 5, 3);
            break;
        case 0x27: /* DAA */
            DAA(8, 2);
            break;
        case 0x29: /* ADD IX IX */
            ADDXXREG(reg_ixh, reg_ixl, reg_ixh, reg_ixl, 15, 2);
            break;
        case 0x28: /* JR Z */
            BRANCH(LOCAL_ZERO(), ip2, 3);
            break;
        case 0x2a: /* LD IX (WORD) */
            LDIND(ip23, reg_ixh, reg_ixl, 4, 4, 12, 4);
            break;
        case 0x2b: /* DEC IX */
            DECINC(DEC_IX_WORD(), 10, 2);
            break;
        case 0x2c: /* INC IXL */
            INCREG(reg_ixl, 7, 2);
            break;
        case 0x2d: /* DEC IXL */
            DECREG(reg_ixl, 7, 2);
            break;
        case 0x2e: /* LD IXL # */
            LDREG(reg_ixl, ip2, 4, 5, 3);
            break;
        case 0x2f: /* CPL */
            CPL(8, 2);
            break;
        case 0x30: /* JR NC */
            BRANCH(!LOCAL_CARRY(), ip2, 3);
            break;
        case 0x31: /* LD SP # */
            LDSP(ip23, 10, 0, 4);
            break;
        case 0x32: /* LD (WORD) A */
            STREG(ip23, reg_a, 10, 7, 4);
            break;
        case 0x33: /* INC SP */
            DECINC(reg_sp++, 10, 2);
            break;
        case 0x34: /* INC (IX+d) */
            INCXXIND(IX_WORD_OFF(ip2), 4, 7, 12, 3);
            break;
        case 0x35: /* DEC (IX+d) */
            DECXXIND(IX_WORD_OFF(ip2), 4, 7, 12, 3);
            break;
        case 0x36: /* LD (IX+d) # */
            STREG(IX_WORD_OFF(ip2), ip3, 8, 11, 4);
            break;
        case 0x37: /* SCF */
            SCF(8, 2);
            break;
        case 0x38: /* JR C */
            BRANCH(LOCAL_CARRY(), ip2, 3);
            break;
        case 0x39: /* ADD IX SP */
            ADDXXSP(reg_ixh, reg_ixl, 15, 2);
            break;
        case 0x3a: /* LD A (WORD) */
            LDREG(reg_a, LOAD(ip23), 10, 7, 4);
            break;
        case 0x3b: /* DEC SP */
            DECINC(reg_sp--, 10, 2);
            break;
        case 0x3c: /* INC A */
            INCREG(reg_a, 7, 2);
            break;
        case 0x3d: /* DEC A */
            DECREG(reg_a, 7, 2);
            break;
        case 0x3e: /* LD A # */
            LDREG(reg_a, ip2, 4, 5, 3);
            break;
        case 0x3f: /* CCF */
            CCF(8, 2);
            break;
        case 0x40: /* LD B B */
            LDREG(reg_b, reg_b, 0, 4, 2);
            break;
        case 0x41: /* LD B C */
            LDREG(reg_b, reg_c, 0, 4, 2);
            break;
        case 0x42: /* LD B D */
            LDREG(reg_b, reg_d, 0, 4, 2);
            break;
        case 0x43: /* LD B E */
            LDREG(reg_b, reg_e, 0, 4, 2);
            break;
        case 0x44: /* LD B IXH */
            LDREG(reg_b, reg_ixh, 0, 4, 2);
            break;
        case 0x45: /* LD B IXL */
            LDREG(reg_b, reg_ixl, 0, 4, 2);
            break;
        case 0x46: /* LD B (IX+d) */
            LDREG(reg_b, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x47: /* LD B A */
            LDREG(reg_b, reg_a, 0, 4, 2);
            break;
        case 0x48: /* LD C B */
            LDREG(reg_c, reg_b, 0, 4, 2);
            break;
        case 0x49: /* LD C C */
            LDREG(reg_c, reg_c, 0, 4, 2);
            break;
        case 0x4a: /* LD C D */
            LDREG(reg_c, reg_d, 0, 4, 2);
            break;
        case 0x4b: /* LD C E */
            LDREG(reg_c, reg_e, 0, 4, 2);
            break;
        case 0x4c: /* LD C IXH */
            LDREG(reg_c, reg_ixh, 0, 4, 2);
            break;
        case 0x4d: /* LD C IXL */
            LDREG(reg_c, reg_ixl, 0, 4, 2);
            break;
        case 0x4e: /* LD C (IX+d) */
            LDREG(reg_c, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x4f: /* LD C A */
            LDREG(reg_c, reg_a, 0, 4, 2);
            break;
        case 0x50: /* LD D B */
            LDREG(reg_d, reg_b, 0, 4, 2);
            break;
        case 0x51: /* LD D C */
            LDREG(reg_d, reg_c, 0, 4, 2);
            break;
        case 0x52: /* LD D D */
            LDREG(reg_d, reg_d, 0, 4, 2);
            break;
        case 0x53: /* LD D E */
            LDREG(reg_d, reg_e, 0, 4, 2);
            break;
        case 0x54: /* LD D IXH */
            LDREG(reg_d, reg_ixh, 0, 4, 2);
            break;
        case 0x55: /* LD D L */
            LDREG(reg_d, reg_ixl, 0, 4, 2);
            break;
        case 0x56: /* LD D (IX+d) */
            LDREG(reg_d, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x57: /* LD D A */
            LDREG(reg_d, reg_a, 0, 4, 2);
            break;
        case 0x58: /* LD E B */
            LDREG(reg_e, reg_b, 0, 4, 2);
            break;
        case 0x59: /* LD E C */
            LDREG(reg_e, reg_c, 0, 4, 2);
            break;
        case 0x5a: /* LD E D */
            LDREG(reg_e, reg_d, 0, 4, 2);
            break;
        case 0x5b: /* LD E E */
            LDREG(reg_e, reg_e, 0, 4, 2);
            break;
        case 0x5c: /* LD E IXH */
            LDREG(reg_e, reg_ixh, 0, 4, 2);
            break;
        case 0x5d: /* LD E IXL */
            LDREG(reg_e, reg_ixl, 0, 4, 2);
            break;
        case 0x5e: /* LD E (IX+d) */
            LDREG(reg_e, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x5f: /* LD E A */
            LDREG(reg_e, reg_a, 0, 4, 2);
            break;
        case 0x60: /* LD IXH B */
            LDREG(reg_ixh, reg_b, 0, 4, 2);
            break;
        case 0x61: /* LD IXH C */
            LDREG(reg_ixh, reg_c, 0, 4, 2);
            break;
        case 0x62: /* LD IXH D */
            LDREG(reg_ixh, reg_d, 0, 4, 2);
            break;
        case 0x63: /* LD IXH E */
            LDREG(reg_ixh, reg_e, 0, 4, 2);
            break;
        case 0x64: /* LD IXH IXH */
            LDREG(reg_ixh, reg_ixh, 0, 4, 2);
            break;
        case 0x65: /* LD IXH IXL */
            LDREG(reg_ixh, reg_ixl, 0, 4, 2);
            break;
        case 0x66: /* LD H (IX+d) */
            LDREG(reg_h, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x67: /* LD IXH A */
            LDREG(reg_ixh, reg_a, 0, 4, 2);
            break;
        case 0x68: /* LD IXL B */
            LDREG(reg_ixl, reg_b, 0, 4, 2);
            break;
        case 0x69: /* LD IXL C */
            LDREG(reg_ixl, reg_c, 0, 4, 2);
            break;
        case 0x6a: /* LD IXL D */
            LDREG(reg_ixl, reg_d, 0, 4, 2);
            break;
        case 0x6b: /* LD IXL E */
            LDREG(reg_ixl, reg_e, 0, 4, 2);
            break;
        case 0x6c: /* LD IXL IXH */
            LDREG(reg_ixl, reg_ixh, 0, 4, 2);
            break;
        case 0x6d: /* LD IXL IXL */
            LDREG(reg_ixl, reg_ixl, 0, 4, 2);
            break;
        case 0x6e: /* LD L (IX+d) */
            LDREG(reg_l, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x6f: /* LD IXL A */
            LDREG(reg_ixl, reg_a, 0, 4, 2);
            break;
        case 0x70: /* LD (IX+d) B */
            STREG(IX_WORD_OFF(ip2), reg_b, 8, 11, 3);
            break;
        case 0x71: /* LD (IX+d) C */
            STREG(IX_WORD_OFF(ip2), reg_c, 8, 11, 3);
            break;
        case 0x72: /* LD (IX+d) D */
            STREG(IX_WORD_OFF(ip2), reg_d, 8, 11, 3);
            break;
        case 0x73: /* LD (IX+d) E */
            STREG(IX_WORD_OFF(ip2), reg_e, 8, 11, 3);
            break;
        case 0x74: /* LD (IX+d) H */
            STREG(IX_WORD_OFF(ip2), reg_h, 8, 11, 3);
            break;
        case 0x75: /* LD (IX+d) L */
            STREG(IX_WORD_OFF(ip2), reg_l, 8, 11, 3);
            break;
        case 0x76: /* HALT */
            HALT();
            break;
        case 0x77: /* LD (IX+d) A */
            STREG(IX_WORD_OFF(ip2), reg_a, 8, 11, 3);
            break;
        case 0x78: /* LD A B */
            LDREG(reg_a, reg_b, 0, 4, 2);
            break;
        case 0x79: /* LD A C */
            LDREG(reg_a, reg_c, 0, 4, 2);
            break;
        case 0x7a: /* LD A D */
            LDREG(reg_a, reg_d, 0, 4, 2);
            break;
        case 0x7b: /* LD A E */
            LDREG(reg_a, reg_e, 0, 4, 2);
            break;
        case 0x7c: /* LD A IXH */
            LDREG(reg_a, reg_ixh, 0, 4, 2);
            break;
        case 0x7d: /* LD A IXL */
            LDREG(reg_a, reg_ixl, 0, 4, 2);
            break;
        case 0x7e: /* LD A (IX+d) */
            LDREG(reg_a, LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x7f: /* LD A A */
            LDREG(reg_a, reg_a, 0, 4, 2);
            break;
        case 0x80: /* ADD B */
            ADD(reg_b, 0, 4, 2);
            break;
        case 0x81: /* ADD C */
            ADD(reg_c, 0, 4, 2);
            break;
        case 0x82: /* ADD D */
            ADD(reg_d, 0, 4, 2);
            break;
        case 0x83: /* ADD E */
            ADD(reg_e, 0, 4, 2);
            break;
        case 0x84: /* ADD IXH */
            ADD(reg_ixh, 0, 4, 2);
            break;
        case 0x85: /* ADD IXL */
            ADD(reg_ixl, 0, 4, 2);
            break;
        case 0x86: /* ADD (IX+d) */
            ADD(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x87: /* ADD A */
            ADD(reg_a, 0, 4, 2);
            break;
        case 0x88: /* ADC B */
            ADC(reg_b, 0, 4, 2);
            break;
        case 0x89: /* ADC C */
            ADC(reg_c, 0, 4, 2);
            break;
        case 0x8a: /* ADC D */
            ADC(reg_d, 0, 4, 2);
            break;
        case 0x8b: /* ADC E */
            ADC(reg_e, 0, 4, 2);
            break;
        case 0x8c: /* ADC IXH */
            ADC(reg_ixh, 0, 4, 2);
            break;
        case 0x8d: /* ADC IXL */
            ADC(reg_ixl, 0, 4, 2);
            break;
        case 0x8e: /* ADC (IX+d) */
            ADC(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x8f: /* ADC A */
            ADC(reg_a, 0, 4, 2);
            break;
        case 0x90: /* SUB B */
            SUB(reg_b, 0, 4, 2);
            break;
        case 0x91: /* SUB C */
            SUB(reg_c, 0, 4, 2);
            break;
        case 0x92: /* SUB D */
            SUB(reg_d, 0, 4, 2);
            break;
        case 0x93: /* SUB E */
            SUB(reg_e, 0, 4, 2);
            break;
        case 0x94: /* SUB IXH */
            SUB(reg_ixh, 0, 4, 2);
            break;
        case 0x95: /* SUB IXL */
            SUB(reg_ixl, 0, 4, 2);
            break;
        case 0x96: /* SUB (IX+d) */
            SUB(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x97: /* SUB A */
            SUB(reg_a, 0, 4, 2);
            break;
        case 0x98: /* SBC B */
            SBC(reg_b, 0, 4, 2);
            break;
        case 0x99: /* SBC C */
            SBC(reg_c, 0, 4, 2);
            break;
        case 0x9a: /* SBC D */
            SBC(reg_d, 0, 4, 2);
            break;
        case 0x9b: /* SBC E */
            SBC(reg_e, 0, 4, 2);
            break;
        case 0x9c: /* SBC IXH */
            SBC(reg_ixh, 0, 4, 2);
            break;
        case 0x9d: /* SBC IXL */
            SBC(reg_ixl, 0, 4, 2);
            break;
        case 0x9e: /* SBC (IX+d) */
            SBC(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x9f: /* SBC A */
            SBC(reg_a, 0, 4, 2);
            break;
        case 0xa0: /* AND B */
            AND(reg_b, 0, 4, 2);
            break;
        case 0xa1: /* AND C */
            AND(reg_c, 0, 4, 2);
            break;
        case 0xa2: /* AND D */
            AND(reg_d, 0, 4, 2);
            break;
        case 0xa3: /* AND E */
            AND(reg_e, 0, 4, 2);
            break;
        case 0xa4: /* AND IXH */
            AND(reg_ixh, 0, 4, 2);
            break;
        case 0xa5: /* AND IXL */
            AND(reg_ixl, 0, 4, 2);
            break;
        case 0xa6: /* AND (IX+d) */
            AND(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xa7: /* AND A */
            AND(reg_a, 0, 4, 2);
            break;
        case 0xa8: /* XOR B */
            XOR(reg_b, 0, 4, 2);
            break;
        case 0xa9: /* XOR C */
            XOR(reg_c, 0, 4, 2);
            break;
        case 0xaa: /* XOR D */
            XOR(reg_d, 0, 4, 2);
            break;
        case 0xab: /* XOR E */
            XOR(reg_e, 0, 4, 2);
            break;
        case 0xac: /* XOR IXH */
            XOR(reg_ixh, 0, 4, 2);
            break;
        case 0xad: /* XOR IXL */
            XOR(reg_ixl, 0, 4, 2);
            break;
        case 0xae: /* XOR (IX+d) */
            XOR(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xaf: /* XOR A */
            XOR(reg_a, 0, 4, 2);
            break;
        case 0xb0: /* OR B */
            OR(reg_b, 0, 4, 2);
            break;
        case 0xb1: /* OR C */
            OR(reg_c, 0, 4, 2);
            break;
        case 0xb2: /* OR D */
            OR(reg_d, 0, 4, 2);
            break;
        case 0xb3: /* OR E */
            OR(reg_e, 0, 4, 2);
            break;
        case 0xb4: /* OR IXH */
            OR(reg_ixh, 0, 4, 2);
            break;
        case 0xb5: /* OR IXL */
            OR(reg_ixl, 0, 4, 2);
            break;
        case 0xb6: /* OR (IX+d) */
            OR(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xb7: /* OR A */
            OR(reg_a, 0, 4, 2);
            break;
        case 0xb8: /* CP B */
            CP(reg_b, 0, 4, 2);
            break;
        case 0xb9: /* CP C */
            CP(reg_c, 0, 4, 2);
            break;
        case 0xba: /* CP D */
            CP(reg_d, 0, 4, 2);
            break;
        case 0xbb: /* CP E */
            CP(reg_e, 0, 4, 2);
            break;
        case 0xbc: /* CP IXH */
            CP(reg_ixh, 0, 4, 2);
            break;
        case 0xbd: /* CP IXL */
            CP(reg_ixl, 0, 4, 2);
            break;
        case 0xbe: /* CP (IX+d) */
            CP(LOAD(IX_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xbf: /* CP A */
            CP(reg_a, 0, 4, 2);
            break;
        case 0xc1: /* POP BC */
            POP(reg_b, reg_c, 2);
            break;
        case 0xc5: /* PUSH BC */
            PUSH(reg_b, reg_c, 2);
            break;
        case 0xcb: /* OPCODE DD CB */
            opcode_dd_cb((BYTE)ip2, (BYTE)ip3, (WORD)ip23);
            break;
        case 0xd1: /* POP DE */
            POP(reg_d, reg_e, 2);
            break;
        case 0xd3: /* OUT A */
            OUTA(ip2, 8, 7, 3);
            break;
        case 0xd5: /* PUSH DE */
            PUSH(reg_d, reg_e, 2);
            break;
        case 0xd9: /* EXX */
            EXX(12, 2);
            break;
        case 0xdb: /* IN A */
            INA(ip2, 8, 7, 3);
            break;
        case 0xdd: /* Skip DD */
            NOP(4, 1);
            break;
        case 0xe1: /* POP IX */
            POP(reg_ixh, reg_ixl, 2);
            break;
        case 0xe3: /* EX IX (SP) */
            EXXXSP(reg_ixh, reg_ixl, 4, 4, 4, 4, 7, 2);
            break;
        case 0xe5: /* PUSH IX */
            PUSH(reg_ixh, reg_ixl, 2);
            break;
        case 0xe9: /* LD PC IX */
            JMP((IX_WORD()), 8);
            break;
        case 0xeb:
            EXDEHL(8, 2);
            break;
        case 0xed: /* Skip DD */
            NOP(4, 1);
            break;
        case 0xf1: /* POP AF */
            POP(reg_a, reg_f, 2);
            break;
        case 0xf3: /* DI */
            DI(8, 2);
            break;
        case 0xf5: /* PUSH AF */
            PUSH(reg_a, reg_f, 2);
            break;
        case 0xf9: /* LD SP IX */
            LDSP(IX_WORD(), 4, 6, 2);
            break;
        case 0xfb: /* EI */
            EI(8, 2);
            break;
        case 0xfd: /* Skip DD */
            NOP(4, 1);
            break;
        default:
#ifdef DEBUG_Z80
            log_message(LOG_DEFAULT,
                        "%i PC %04x A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x SP%04x OP DD %02x %02x %02x.",
                        (int)(CLK), (unsigned int)(z80_reg_pc), reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp, ip1, ip2, ip3);
#endif
            INC_PC(2);
    }
}

static void opcode_ed(BYTE ip1, BYTE ip2, BYTE ip3, WORD ip12, WORD ip23)
{
    switch (ip1) {
        case 0x40: /* IN B BC */
            INBC(reg_b, 4, 8, 2);
            break;
        case 0x41: /* OUT BC B */
            OUTBC(reg_b, 4, 8, 2);
            break;
        case 0x42: /* SBC HL BC */
            SBCHLREG(reg_b, reg_c);
            break;
        case 0x43: /* LD (WORD) BC */
            STW(ip23, reg_b, reg_c, 4, 13, 3, 4);
            break;
        case 0x44: /* NEG */
            NEG();
            break;
        case 0x45: /* RETN */
            RETNI();
            break;
        case 0x46: /* IM0 */
            IM(0);
            break;
        case 0x47: /* LD I A */
            LDREG(reg_i, reg_a, 6, 3, 2);
            break;
        case 0x48: /* IN C BC */
            INBC(reg_c, 4, 8, 2);
            break;
        case 0x49: /* OUT BC C */
            OUTBC(reg_c, 4, 8, 2);
            break;
        case 0x4a: /* ADC HL BC */
            ADCHLREG(reg_b, reg_c);
            break;
        case 0x4b: /* LD BC (WORD) */
            LDIND(ip23, reg_b, reg_c, 4, 4, 12, 4);
            break;
        case 0x4d: /* RETI */
            RETNI();
            break;
        case 0x4f: /* LD R A FIXME: Not emulated.  */
            NOP(8, 2);
            break;
        case 0x50: /* IN D BC */
            INBC(reg_d, 4, 8, 2);
            break;
        case 0x51: /* OUT BC D */
            OUTBC(reg_d, 4, 8, 2);
            break;
        case 0x52: /* SBC HL DE */
            SBCHLREG(reg_d, reg_e);
            break;
        case 0x53: /* LD (WORD) DE */
            STW(ip23, reg_d, reg_e, 4, 13, 3, 4);
            break;
        case 0x56: /* IM1 */
            IM(1);
            break;
        case 0x57: /* LD A I */
            LDAIR(reg_i);
            break;
        case 0x58: /* IN E BC */
            INBC(reg_e, 4, 8, 2);
            break;
        case 0x59: /* OUT BC E */
            OUTBC(reg_e, 4, 8, 2);
            break;
        case 0x5a: /* ADC HL DE */
            ADCHLREG(reg_d, reg_e);
            break;
        case 0x5b: /* LD DE (WORD) */
            LDIND(ip23, reg_d, reg_e, 4, 4, 12, 4);
            break;
        case 0x5e: /* IM2 */
            IM(2);
            break;
        case 0x5f: /* LD A R */
            LDAIR((BYTE)(CLK & 0xff));
            break;
        case 0x60: /* IN H BC */
            INBC(reg_h, 4, 8, 2);
            break;
        case 0x61: /* OUT BC H */
            OUTBC(reg_h, 4, 8, 2);
            break;
        case 0x62: /* SBC HL HL */
            SBCHLREG(reg_h, reg_l);
            break;
        case 0x63: /* LD (WORD) HL */
            STW(ip23, reg_h, reg_l, 4, 13, 3, 4);
            break;
        case 0x67: /* RRD */
            RRD();
            break;
        case 0x68: /* IN L BC */
            INBC(reg_l, 4, 8, 2);
            break;
        case 0x69: /* OUT BC L */
            OUTBC(reg_l, 4, 8, 2);
            break;
        case 0x6a: /* ADC HL HL */
            ADCHLREG(reg_h, reg_l);
            break;
        case 0x6b: /* LD HL (WORD) */
            LDIND(ip23, reg_h, reg_l, 4, 4, 12, 4);
            break;
        case 0x6f: /* RLD */
            RLD();
            break;
        case 0x70: /* IN F BC */
            INBC0(4, 8, 2);
            break;
        case 0x71: /* OUT BC #0 */
            OUTBC(0, 4, 8, 2);
            break;
        case 0x72: /* SBC HL SP */
            SBCHLSP();
            break;
        case 0x73: /* LD (WORD) SP */
            STSPW(ip23, 4, 13, 3, 4);
            break;
        case 0x78: /* IN A BC */
            INBC(reg_a, 4, 8, 2);
            break;
        case 0x79: /* OUT BC A */
            OUTBC(reg_a, 4, 8, 2);
            break;
        case 0x7a: /* ADC HL SP */
            ADCHLSP();
            break;
        case 0x7b: /* LD SP (WORD) */
            LDSPIND(ip23, 4, 4, 12, 4);
            break;
        case 0xa0: /* LDI */
            LDDI(INC_DE_WORD(), INC_HL_WORD());
            break;
        case 0xa1: /* CPI */
            CPDI(INC_HL_WORD());
            break;
        case 0xa2: /* INI */
            INDI(INC_HL_WORD());
            break;
        case 0xa3: /* OUTI */
            OUTDI(INC_HL_WORD());
            break;
        case 0xa8: /* LDD */
            LDDI(DEC_DE_WORD(), DEC_HL_WORD());
            break;
        case 0xa9: /* CPD */
            CPDI(DEC_HL_WORD());
            break;
        case 0xaa: /* IND */
            INDI(DEC_HL_WORD());
            break;
        case 0xab: /* OUTD */
            OUTDI(DEC_HL_WORD());
            break;
        case 0xb0: /* LDIR */
            LDDIR(INC_DE_WORD(), INC_HL_WORD());
            break;
        case 0xb1: /* CPIR */
            CPDIR(INC_HL_WORD());
            break;
        case 0xb2: /* INIR */
            INDIR(INC_HL_WORD());
            break;
        case 0xb3: /* OTIR */
            OTDIR(INC_HL_WORD());
            break;
        case 0xb8: /* LDDR */
            LDDIR(DEC_DE_WORD(), DEC_HL_WORD());
            break;
        case 0xb9: /* CPDR */
            CPDIR(DEC_HL_WORD());
            break;
        case 0xba: /* INDR */
            INDIR(DEC_HL_WORD());
            break;
        case 0xbb: /* OTDR */
            OTDIR(DEC_HL_WORD());
            break;
        case 0xcb: /* NOP */
            NOP(8, 2);
            break;
        case 0xdd: /* NOP */
            NOP(8, 2);
            break;
        case 0xed: /* NOP */
            NOP(8, 2);
            break;
        case 0xfd: /* NOP */
            NOP(8, 2);
            break;
        default:
#ifdef DEBUG_Z80
            log_message(LOG_DEFAULT,
                        "%i PC %04x A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x SP%04x OP ED %02x %02x %02x.",
                        (int)(CLK), (unsigned int)(z80_reg_pc), reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp, ip1, ip2, ip3);
#endif
            INC_PC(2);
    }
}

static void opcode_fd_cb(BYTE iip2, BYTE iip3, WORD iip23)
{
    switch (iip3) {
        case 0x00: /* RLC (IY+d),B */
            RLCXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x01: /* RLC (IY+d),C */
            RLCXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x02: /* RLC (IY+d),D */
            RLCXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x03: /* RLC (IY+d),E */
            RLCXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x04: /* RLC (IY+d),H */
            RLCXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x05: /* RLC (IY+d),L */
            RLCXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x06: /* RLC (IY+d) */
            RLCXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x07: /* RLC (IY+d),A */
            RLCXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x08: /* RRC (IY+d),B */
            RRCXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x09: /* RRC (IY+d),C */
            RRCXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0a: /* RRC (IY+d),D */
            RRCXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0b: /* RRC (IY+d),E */
            RRCXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0c: /* RRC (IY+d),H */
            RRCXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0d: /* RRC (IY+d),L */
            RRCXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0e: /* RRC (IY+d) */
            RRCXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x0f: /* RRC (IY+d),A */
            RRCXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x10: /* RL (IY+d),B */
            RLXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x11: /* RL (IY+d),C */
            RLXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x12: /* RL (IY+d),D */
            RLXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x13: /* RL (IY+d),E */
            RLXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x14: /* RL (IY+d),H */
            RLXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x15: /* RL (IY+d),L */
            RLXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x16: /* RL (IY+d) */
            RLXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x17: /* RL (IY+d),A */
            RLXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x18: /* RR (IY+d),B */
            RRXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x19: /* RR (IY+d),C */
            RRXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1a: /* RR (IY+d),D */
            RRXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1b: /* RR (IY+d),E */
            RRXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1c: /* RR (IY+d),H */
            RRXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1d: /* RR (IY+d),L */
            RRXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1e: /* RR (IY+d) */
            RRXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x1f: /* RR (IY+d),A */
            RRXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x20: /* SLA (IY+d),B */
            SLAXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x21: /* SLA (IY+d),C */
            SLAXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x22: /* SLA (IY+d),D */
            SLAXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x23: /* SLA (IY+d),E */
            SLAXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x24: /* SLA (IY+d),H */
            SLAXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x25: /* SLA (IY+d),L */
            SLAXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x26: /* SLA (IY+d) */
            SLAXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x27: /* SLA (IY+d),A */
            SLAXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x28: /* SRA (IY+d),B */
            SRAXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x29: /* SRA (IY+d),C */
            SRAXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2a: /* SRA (IY+d),D */
            SRAXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2b: /* SRA (IY+d),E */
            SRAXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2c: /* SRA (IY+d),H */
            SRAXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2d: /* SRA (IY+d),L */
            SRAXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2e: /* SRA (IY+d) */
            SRAXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x2f: /* SRA (IY+d),A */
            SRAXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x30: /* SLL (IY+d),B */
            SLLXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x31: /* SLL (IY+d),C */
            SLLXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x32: /* SLL (IY+d),D */
            SLLXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x33: /* SLL (IY+d),E */
            SLLXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x34: /* SLL (IY+d),H */
            SLLXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x35: /* SLL (IY+d),L */
            SLLXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x36: /* SLL (IY+d) */
            SLLXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x37: /* SLL (IY+d),A */
            SLLXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x38: /* SRL (IY+d),B */
            SRLXXREG(reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x39: /* SRL (IY+d),C */
            SRLXXREG(reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3a: /* SRL (IY+d),D */
            SRLXXREG(reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3b: /* SRL (IY+d),E */
            SRLXXREG(reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3c: /* SRL (IY+d),H */
            SRLXXREG(reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3d: /* SRL (IY+d),L */
            SRLXXREG(reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3e: /* SRL (IY+d) */
            SRLXX(IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x3f: /* SRL (IY+d),A */
            SRLXXREG(reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x40: /* BIT (IY+d) 0 */
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            BIT(LOAD(IY_WORD_OFF(iip2)), 0, 8, 12, 4);
            break;
        case 0x48: /* BIT (IY+d) 1 */
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            BIT(LOAD(IY_WORD_OFF(iip2)), 1, 8, 12, 4);
            break;
        case 0x50: /* BIT (IY+d) 2 */
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            BIT(LOAD(IY_WORD_OFF(iip2)), 2, 8, 12, 4);
            break;
        case 0x58: /* BIT (IY+d) 3 */
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            BIT(LOAD(IY_WORD_OFF(iip2)), 3, 8, 12, 4);
            break;
        case 0x60: /* BIT (IY+d) 4 */
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
            BIT(LOAD(IY_WORD_OFF(iip2)), 4, 8, 12, 4);
            break;
        case 0x68: /* BIT (IY+d) 5 */
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
            BIT(LOAD(IY_WORD_OFF(iip2)), 5, 8, 12, 4);
            break;
        case 0x70: /* BIT (IY+d) 6 */
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
            BIT(LOAD(IY_WORD_OFF(iip2)), 6, 8, 12, 4);
            break;
        case 0x78: /* BIT (IY+d) 7 */
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            BIT(LOAD(IY_WORD_OFF(iip2)), 7, 8, 12, 4);
            break;
        case 0x80: /* RES (IY+d),B 0 */
            RESXXREG(0, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x81: /* RES (IY+d),C 0 */
            RESXXREG(0, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x82: /* RES (IY+d),D 0 */
            RESXXREG(0, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x83: /* RES (IY+d),E 0 */
            RESXXREG(0, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x84: /* RES (IY+d),H 0 */
            RESXXREG(0, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x85: /* RES (IY+d),L 0 */
            RESXXREG(0, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x86: /* RES (IY+d) 0 */
            RESXX(0, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x87: /* RES (IY+d),A 0 */
            RESXXREG(0, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x88: /* RES (IY+d),B 1 */
            RESXXREG(1, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x89: /* RES (IY+d),C 1 */
            RESXXREG(1, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8a: /* RES (IY+d),D 1 */
            RESXXREG(1, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8b: /* RES (IY+d),E 1 */
            RESXXREG(1, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8c: /* RES (IY+d),H 1 */
            RESXXREG(1, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8d: /* RES (IY+d),L 1 */
            RESXXREG(1, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8e: /* RES (IY+d) 1 */
            RESXX(1, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x8f: /* RES (IY+d),A 1 */
            RESXXREG(1, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x90: /* RES (IY+d),B 2 */
            RESXXREG(2, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x91: /* RES (IY+d),C 2 */
            RESXXREG(2, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x92: /* RES (IY+d),D 2 */
            RESXXREG(2, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x93: /* RES (IY+d),E 2 */
            RESXXREG(2, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x94: /* RES (IY+d),H 2 */
            RESXXREG(2, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x95: /* RES (IY+d),L 2 */
            RESXXREG(2, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x96: /* RES (IY+d) 2 */
            RESXX(2, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x97: /* RES (IY+d),A 2 */
            RESXXREG(2, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x98: /* RES (IY+d),B 3 */
            RESXXREG(3, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x99: /* RES (IY+d),C 3 */
            RESXXREG(3, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9a: /* RES (IY+d),D 3 */
            RESXXREG(3, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9b: /* RES (IY+d),E 3 */
            RESXXREG(3, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9c: /* RES (IY+d),H 3 */
            RESXXREG(3, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9d: /* RES (IY+d),L 3 */
            RESXXREG(3, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9e: /* RES (IY+d) 3 */
            RESXX(3, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0x9f: /* RES (IY+d),A 3 */
            RESXXREG(3, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa0: /* RES (IY+d),B 4 */
            RESXXREG(4, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa1: /* RES (IY+d),C 4 */
            RESXXREG(4, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa2: /* RES (IY+d),D 4 */
            RESXXREG(4, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa3: /* RES (IY+d),E 4 */
            RESXXREG(4, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa4: /* RES (IY+d),H 4 */
            RESXXREG(4, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa5: /* RES (IY+d),L 4 */
            RESXXREG(4, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa6: /* RES (IY+d) 4 */
            RESXX(4, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa7: /* RES (IY+d),A 4 */
            RESXXREG(4, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa8: /* RES (IY+d),B 5 */
            RESXXREG(5, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xa9: /* RES (IY+d),C 5 */
            RESXXREG(5, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xaa: /* RES (IY+d),D 5 */
            RESXXREG(5, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xab: /* RES (IY+d),E 5 */
            RESXXREG(5, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xac: /* RES (IY+d),H 5 */
            RESXXREG(5, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xad: /* RES (IY+d),L 5 */
            RESXXREG(5, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xae: /* RES (IY+d) 5 */
            RESXX(5, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xaf: /* RES (IY+d),A 5 */
            RESXXREG(5, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb0: /* RES (IY+d),B 6 */
            RESXXREG(6, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb1: /* RES (IY+d),C 6 */
            RESXXREG(6, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb2: /* RES (IY+d),D 6 */
            RESXXREG(6, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb3: /* RES (IY+d),E 6 */
            RESXXREG(6, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb4: /* RES (IY+d),H 6 */
            RESXXREG(6, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb5: /* RES (IY+d),L 6 */
            RESXXREG(6, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb6: /* RES (IY+d) 6 */
            RESXX(6, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb7: /* RES (IY+d),A 6 */
            RESXXREG(6, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb8: /* RES (IY+d),B 7 */
            RESXXREG(7, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xb9: /* RES (IY+d),C 7 */
            RESXXREG(7, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xba: /* RES (IY+d),D 7 */
            RESXXREG(7, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbb: /* RES (IY+d),E 7 */
            RESXXREG(7, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbc: /* RES (IY+d),H 7 */
            RESXXREG(7, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbd: /* RES (IY+d),L 7 */
            RESXXREG(7, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbe: /* RES (IY+d) 7 */
            RESXX(7, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xbf: /* RES (IY+d),A 7 */
            RESXXREG(7, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc0: /* SET (IY+d),B 0 */
            SETXXREG(0, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc1: /* SET (IY+d),C 0 */
            SETXXREG(0, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc2: /* SET (IY+d),D 0 */
            SETXXREG(0, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc3: /* SET (IY+d),E 0 */
            SETXXREG(0, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc4: /* SET (IY+d),H 0 */
            SETXXREG(0, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc5: /* SET (IY+d),L 0 */
            SETXXREG(0, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc6: /* SET (IY+d) 0 */
            SETXX(0, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc7: /* SET (IY+d),A 0 */
            SETXXREG(0, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc8: /* SET (IY+d),B 1 */
            SETXXREG(1, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xc9: /* SET (IY+d),C 1 */
            SETXXREG(1, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xca: /* SET (IY+d),D 1 */
            SETXXREG(1, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcb: /* SET (IY+d),E 1 */
            SETXXREG(1, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcc: /* SET (IY+d),H 1 */
            SETXXREG(1, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcd: /* SET (IY+d),L 1 */
            SETXXREG(1, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xce: /* SET (IY+d) 1 */
            SETXX(1, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xcf: /* SET (IY+d),A 1 */
            SETXXREG(1, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd0: /* SET (IY+d),B 2 */
            SETXXREG(2, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd1: /* SET (IY+d),C 2 */
            SETXXREG(2, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd2: /* SET (IY+d),D 2 */
            SETXXREG(2, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd3: /* SET (IY+d),E 2 */
            SETXXREG(2, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd4: /* SET (IY+d),H 2 */
            SETXXREG(2, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd5: /* SET (IY+d),L 2 */
            SETXXREG(2, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd6: /* SET (IY+d) 2 */
            SETXX(2, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd7: /* SET (IY+d),A 2 */
            SETXXREG(2, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd8: /* SET (IY+d),B 3 */
            SETXXREG(3, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xd9: /* SET (IY+d),C 3 */
            SETXXREG(3, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xda: /* SET (IY+d),D 3 */
            SETXXREG(3, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdb: /* SET (IY+d),E 3 */
            SETXXREG(3, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdc: /* SET (IY+d),H 3 */
            SETXXREG(3, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdd: /* SET (IY+d),L 3 */
            SETXXREG(3, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xde: /* SET (IY+d) 3 */
            SETXX(3, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xdf: /* SET (IY+d),A 3 */
            SETXXREG(3, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe0: /* SET (IY+d),B 4 */
            SETXXREG(4, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe1: /* SET (IY+d),C 4 */
            SETXXREG(4, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe2: /* SET (IY+d),D 4 */
            SETXXREG(4, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe3: /* SET (IY+d),E 4 */
            SETXXREG(4, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe4: /* SET (IY+d),H 4 */
            SETXXREG(4, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe5: /* SET (IY+d),L 4 */
            SETXXREG(4, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe6: /* SET (IY+d) 4 */
            SETXX(4, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe7: /* SET (IY+d),A 4 */
            SETXXREG(4, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe8: /* SET (IY+d),B 5 */
            SETXXREG(5, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xe9: /* SET (IY+d),C 5 */
            SETXXREG(5, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xea: /* SET (IY+d),D 5 */
            SETXXREG(5, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xeb: /* SET (IY+d),E 5 */
            SETXXREG(5, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xec: /* SET (IY+d),H 5 */
            SETXXREG(5, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xed: /* SET (IY+d),L 5 */
            SETXXREG(5, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xee: /* SET (IY+d) 5 */
            SETXX(5, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xef: /* SET (IY+d),A 5 */
            SETXXREG(5, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf0: /* SET (IY+d),B 6 */
            SETXXREG(6, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf1: /* SET (IY+d),C 6 */
            SETXXREG(6, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf2: /* SET (IY+d),D 6 */
            SETXXREG(6, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf3: /* SET (IY+d),E 6 */
            SETXXREG(6, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf4: /* SET (IY+d),H 6 */
            SETXXREG(6, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf5: /* SET (IY+d),L 6 */
            SETXXREG(6, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf6: /* SET (IY+d) 6 */
            SETXX(6, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf7: /* SET (IY+d),A 6 */
            SETXXREG(6, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf8: /* SET (IY+d),B 7 */
            SETXXREG(7, reg_b, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xf9: /* SET (IY+d),C 7 */
            SETXXREG(7, reg_c, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfa: /* SET (IY+d),D 7 */
            SETXXREG(7, reg_d, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfb: /* SET (IY+d),E 7 */
            SETXXREG(7, reg_e, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfc: /* SET (IY+d),H 7 */
            SETXXREG(7, reg_h, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfd: /* SET (IY+d),L 7 */
            SETXXREG(7, reg_l, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xfe: /* SET (IY+d) 7 */
            SETXX(7, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        case 0xff: /* SET (IY+d),A 7 */
            SETXXREG(7, reg_a, IY_WORD_OFF(iip2), 4, 4, 15, 4);
            break;
        default:
            INC_PC(4);
    }
}

static void opcode_fd(BYTE ip1, BYTE ip2, BYTE ip3, WORD ip12, WORD ip23)
{
    switch (ip1) {
        case 0x00: /* NOP */
            NOP(8, 2);
            break;
        case 0x01: /* LD BC # */
            LDW(ip23, reg_b, reg_c, 10, 0, 4);
            break;
        case 0x02: /* LD (BC) A */
            STREG(BC_WORD(), reg_a, 8, 3, 2);
            break;
        case 0x03: /* INC BC */
            DECINC(INC_BC_WORD(), 10, 2);
            break;
        case 0x04: /* INC B */
            INCREG(reg_b, 7, 2);
            break;
        case 0x05: /* DEC B */
            DECREG(reg_b, 7, 2);
            break;
        case 0x06: /* LD B # */
            LDREG(reg_b, ip2, 4, 5, 3);
            break;
        case 0x07: /* RLCA */
            RLCA(8, 2);
            break;
        case 0x08: /* EX AF AF' */
            EXAFAF(12, 2);
            break;
        case 0x09: /* ADD IY BC */
            ADDXXREG(reg_iyh, reg_iyl, reg_b, reg_c, 15, 2);
            break;
        case 0x0a: /* LD A (BC) */
            LDREG(reg_a, LOAD(BC_WORD()), 8, 3, 2);
            break;
        case 0x0b: /* DEC BC */
            DECINC(DEC_BC_WORD(), 10, 2);
            break;
        case 0x0c: /* INC C */
            INCREG(reg_c, 7, 2);
            break;
        case 0x0d: /* DEC C */
            DECREG(reg_c, 7, 2);
            break;
        case 0x0e: /* LD C # */
            LDREG(reg_c, ip2, 4, 5, 3);
            break;
        case 0x0f: /* RRCA */
            RRCA(8, 2);
            break;
        case 0x10: /* DJNZ */
            DJNZ(ip2, 3);
            break;
        case 0x11: /* LD DE # */
            LDW(ip23, reg_d, reg_e, 10, 0, 4);
            break;
        case 0x12: /* LD (DE) A */
            STREG(DE_WORD(), reg_a, 8, 3, 2);
            break;
        case 0x13: /* INC DE */
            DECINC(INC_DE_WORD(), 10, 2);
            break;
        case 0x14: /* INC D */
            INCREG(reg_d, 7, 2);
            break;
        case 0x15: /* DEC D */
            DECREG(reg_d, 7, 2);
            break;
        case 0x16: /* LD D # */
            LDREG(reg_d, ip2, 4, 5, 3);
            break;
        case 0x17: /* RLA */
            RLA(8, 2);
            break;
        case 0x19: /* ADD IY DE */
            ADDXXREG(reg_iyh, reg_iyl, reg_d, reg_e, 15, 2);
            break;
        case 0x1a: /* LD A (DE) */
            LDREG(reg_a, LOAD(DE_WORD()), 8, 3, 2);
            break;
        case 0x1b: /* DEC DE */
            DECINC(DEC_DE_WORD(), 10, 2);
            break;
        case 0x1c: /* INC E */
            INCREG(reg_e, 7, 2);
            break;
        case 0x1d: /* DEC E */
            DECREG(reg_e, 7, 2);
            break;
        case 0x1e: /* LD E # */
            LDREG(reg_e, ip2, 4, 5, 3);
            break;
        case 0x1f: /* RRA */
            RRA(8, 2);
            break;
        case 0x20: /* JR NZ */
            BRANCH(!LOCAL_ZERO(), ip2, 3);
            break;
        case 0x21: /* LD IY # */
            LDW(ip23, reg_iyh, reg_iyl, 10, 4, 4);
            break;
        case 0x22: /* LD (WORD) IY */
            STW(ip23, reg_iyh, reg_iyl, 4, 9, 7, 4);
            break;
        case 0x23: /* INC IY */
            DECINC(INC_IY_WORD(), 10, 2);
            break;
        case 0x24: /* INC IYH */
            INCREG(reg_iyh, 7, 2);
            break;
        case 0x25: /* DEC IYH */
            DECREG(reg_iyh, 7, 2);
            break;
        case 0x26: /* LD IYH # */
            LDREG(reg_iyh, ip2, 4, 5, 3);
            break;
        case 0x27: /* DAA */
            DAA(8, 2);
            break;
        case 0x28: /* JR Z */
            BRANCH(LOCAL_ZERO(), ip2, 3);
            break;
        case 0x29: /* ADD IY IY */
            ADDXXREG(reg_iyh, reg_iyl, reg_iyh, reg_iyl, 15, 2);
            break;
        case 0x2a: /* LD IY (WORD) */
            LDIND(ip23, reg_iyh, reg_iyl, 4, 4, 12, 4);
            break;
        case 0x2b: /* DEC IY */
            DECINC(DEC_IY_WORD(), 10, 2);
            break;
        case 0x2c: /* INC IYL */
            INCREG(reg_iyl, 7, 2);
            break;
        case 0x2d: /* DEC IYL */
            DECREG(reg_iyl, 7, 2);
            break;
        case 0x2e: /* LD IYL # */
            LDREG(reg_iyl, ip2, 4, 5, 3);
            break;
        case 0x2f: /* CPL */
            CPL(8, 2);
            break;
        case 0x30: /* JR NC */
            BRANCH(!LOCAL_CARRY(), ip2, 3);
            break;
        case 0x31: /* LD SP # */
            LDSP(ip23, 10, 0, 4);
            break;
        case 0x32: /* LD (WORD) A */
            STREG(ip23, reg_a, 10, 7, 4);
            break;
        case 0x33: /* INC SP */
            DECINC(reg_sp++, 10, 2);
            break;
        case 0x34: /* INC (IY+d) */
            INCXXIND(IY_WORD_OFF(ip2), 4, 7, 12, 3);
            break;
        case 0x35: /* DEC (IY+d) */
            DECXXIND(IY_WORD_OFF(ip2), 4, 7, 12, 3);
            break;
        case 0x36: /* LD (IY+d) # */
            STREG(IY_WORD_OFF(ip2), ip3, 8, 11, 4);
            break;
        case 0x37: /* SCF */
            SCF(8, 2);
            break;
        case 0x38: /* JR C */
            BRANCH(LOCAL_CARRY(), ip2, 3);
            break;
        case 0x39: /* ADD IY SP */
            ADDXXSP(reg_iyh, reg_iyl, 15, 2);
            break;
        case 0x3a: /* LD A (WORD) */
            LDREG(reg_a, LOAD(ip23), 10, 7, 4);
            break;
        case 0x3b: /* DEC SP */
            DECINC(reg_sp--, 10, 2);
            break;
        case 0x3c: /* INC A */
            INCREG(reg_a, 7, 2);
            break;
        case 0x3d: /* DEC A */
            DECREG(reg_a, 7, 2);
            break;
        case 0x3e: /* LD A # */
            LDREG(reg_a, ip2, 4, 5, 3);
            break;
        case 0x3f: /* CCF */
            CCF(8, 2);
            break;
        case 0x40: /* LD B B */
            LDREG(reg_b, reg_b, 0, 4, 2);
            break;
        case 0x41: /* LD B C */
            LDREG(reg_b, reg_c, 0, 4, 2);
            break;
        case 0x42: /* LD B D */
            LDREG(reg_b, reg_d, 0, 4, 2);
            break;
        case 0x43: /* LD B E */
            LDREG(reg_b, reg_e, 0, 4, 2);
            break;
        case 0x44: /* LD B IYH */
            LDREG(reg_b, reg_iyh, 0, 4, 2);
            break;
        case 0x45: /* LD B IYL */
            LDREG(reg_b, reg_iyl, 0, 4, 2);
            break;
        case 0x46: /* LD B (IY+d) */
            LDREG(reg_b, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x47: /* LD B A */
            LDREG(reg_b, reg_a, 0, 4, 2);
            break;
        case 0x48: /* LD C B */
            LDREG(reg_c, reg_b, 0, 4, 2);
            break;
        case 0x49: /* LD C C */
            LDREG(reg_c, reg_c, 0, 4, 2);
            break;
        case 0x4a: /* LD C D */
            LDREG(reg_c, reg_d, 0, 4, 2);
            break;
        case 0x4b: /* LD C E */
            LDREG(reg_c, reg_e, 0, 4, 2);
            break;
        case 0x4c: /* LD C IYH */
            LDREG(reg_c, reg_iyh, 0, 4, 2);
            break;
        case 0x4d: /* LD C IYL */
            LDREG(reg_c, reg_iyl, 0, 4, 2);
            break;
        case 0x4e: /* LD C (IY+d) */
            LDREG(reg_c, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x4f: /* LD C A */
            LDREG(reg_c, reg_a, 0, 4, 2);
            break;
        case 0x50: /* LD D B */
            LDREG(reg_d, reg_b, 0, 4, 2);
            break;
        case 0x51: /* LD D C */
            LDREG(reg_d, reg_c, 0, 4, 2);
            break;
        case 0x52: /* LD D D */
            LDREG(reg_d, reg_d, 0, 4, 2);
            break;
        case 0x53: /* LD D E */
            LDREG(reg_d, reg_e, 0, 4, 2);
            break;
        case 0x54: /* LD D IYH */
            LDREG(reg_d, reg_iyh, 0, 4, 2);
            break;
        case 0x55: /* LD D IYL */
            LDREG(reg_d, reg_iyl, 0, 4, 2);
            break;
        case 0x56: /* LD D (IY+d) */
            LDREG(reg_d, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x57: /* LD D A */
            LDREG(reg_d, reg_a, 0, 4, 2);
            break;
        case 0x58: /* LD E B */
            LDREG(reg_e, reg_b, 0, 4, 2);
            break;
        case 0x59: /* LD E C */
            LDREG(reg_e, reg_c, 0, 4, 2);
            break;
        case 0x5a: /* LD E D */
            LDREG(reg_e, reg_d, 0, 4, 2);
            break;
        case 0x5b: /* LD E E */
            LDREG(reg_e, reg_e, 0, 4, 2);
            break;
        case 0x5c: /* LD E IYH */
            LDREG(reg_e, reg_iyh, 0, 4, 2);
            break;
        case 0x5d: /* LD E IYL */
            LDREG(reg_e, reg_iyl, 0, 4, 2);
            break;
        case 0x5e: /* LD E (IY+d) */
            LDREG(reg_e, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x5f: /* LD E A */
            LDREG(reg_e, reg_a, 0, 4, 2);
            break;
        case 0x60: /* LD IYH B */
            LDREG(reg_iyh, reg_b, 0, 4, 2);
            break;
        case 0x61: /* LD IYH C */
            LDREG(reg_iyh, reg_c, 0, 4, 2);
            break;
        case 0x62: /* LD IYH D */
            LDREG(reg_iyh, reg_d, 0, 4, 2);
            break;
        case 0x63: /* LD IYH E */
            LDREG(reg_iyh, reg_e, 0, 4, 2);
            break;
        case 0x64: /* LD IYH IYH */
            LDREG(reg_iyh, reg_iyh, 0, 4, 2);
            break;
        case 0x65: /* LD IYH IYL */
            LDREG(reg_iyh, reg_iyl, 0, 4, 2);
            break;
        case 0x66: /* LD H (IY+d) */
            LDREG(reg_h, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x67: /* LD IYH A */
            LDREG(reg_iyh, reg_a, 0, 4, 2);
            break;
        case 0x68: /* LD IYL B */
            LDREG(reg_iyl, reg_b, 0, 4, 2);
            break;
        case 0x69: /* LD IYL C */
            LDREG(reg_iyl, reg_c, 0, 4, 2);
            break;
        case 0x6a: /* LD IYL D */
            LDREG(reg_iyl, reg_d, 0, 4, 2);
            break;
        case 0x6b: /* LD IYL E */
            LDREG(reg_iyl, reg_e, 0, 4, 2);
            break;
        case 0x6c: /* LD IYL IYH */
            LDREG(reg_iyl, reg_iyh, 0, 4, 2);
            break;
        case 0x6d: /* LD IYL IYL */
            LDREG(reg_iyl, reg_iyl, 0, 4, 2);
            break;
        case 0x6e: /* LD L (IY+d) */
            LDREG(reg_l, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x6f: /* LD IYL A */
            LDREG(reg_iyl, reg_a, 0, 4, 2);
            break;
        case 0x70: /* LD (IY+d) B */
            STREG(IY_WORD_OFF(ip2), reg_b, 8, 11, 3);
            break;
        case 0x71: /* LD (IY+d) C */
            STREG(IY_WORD_OFF(ip2), reg_c, 8, 11, 3);
            break;
        case 0x72: /* LD (IY+d) D */
            STREG(IY_WORD_OFF(ip2), reg_d, 8, 11, 3);
            break;
        case 0x73: /* LD (IY+d) E */
            STREG(IY_WORD_OFF(ip2), reg_e, 8, 11, 3);
            break;
        case 0x74: /* LD (IY+d) H */
            STREG(IY_WORD_OFF(ip2), reg_h, 8, 11, 3);
            break;
        case 0x75: /* LD (IY+d) L */
            STREG(IY_WORD_OFF(ip2), reg_l, 8, 11, 3);
            break;
        case 0x76: /* HALT */
            HALT();
            break;
        case 0x77: /* LD (IY+d) A */
            STREG(IY_WORD_OFF(ip2), reg_a, 8, 11, 3);
            break;
        case 0x78: /* LD A B */
            LDREG(reg_a, reg_b, 0, 4, 2);
            break;
        case 0x79: /* LD A C */
            LDREG(reg_a, reg_c, 0, 4, 2);
            break;
        case 0x7a: /* LD A D */
            LDREG(reg_a, reg_d, 0, 4, 2);
            break;
        case 0x7b: /* LD A E */
            LDREG(reg_a, reg_e, 0, 4, 2);
            break;
        case 0x7c: /* LD A IYH */
            LDREG(reg_a, reg_iyh, 0, 4, 2);
            break;
        case 0x7d: /* LD A IYL */
            LDREG(reg_a, reg_iyl, 0, 4, 2);
            break;
        case 0x7e: /* LD A (IY+d) */
            LDREG(reg_a, LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x7f: /* LD A A */
            LDREG(reg_a, reg_a, 0, 4, 2);
            break;
        case 0x80: /* ADD B */
            ADD(reg_b, 0, 4, 2);
            break;
        case 0x81: /* ADD C */
            ADD(reg_c, 0, 4, 2);
            break;
        case 0x82: /* ADD D */
            ADD(reg_d, 0, 4, 2);
            break;
        case 0x83: /* ADD E */
            ADD(reg_e, 0, 4, 2);
            break;
        case 0x84: /* ADD IYH */
            ADD(reg_iyh, 0, 4, 2);
            break;
        case 0x85: /* ADD IYL */
            ADD(reg_iyl, 0, 4, 2);
            break;
        case 0x86: /* ADD (IY+d) */
            ADD(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x87: /* ADD A */
            ADD(reg_a, 0, 4, 2);
            break;
        case 0x88: /* ADC B */
            ADC(reg_b, 0, 4, 2);
            break;
        case 0x89: /* ADC C */
            ADC(reg_c, 0, 4, 2);
            break;
        case 0x8a: /* ADC D */
            ADC(reg_d, 0, 4, 2);
            break;
        case 0x8b: /* ADC E */
            ADC(reg_e, 0, 4, 2);
            break;
        case 0x8c: /* ADC IYH */
            ADC(reg_iyh, 0, 4, 2);
            break;
        case 0x8d: /* ADC IYL */
            ADC(reg_iyl, 0, 4, 2);
            break;
        case 0x8e: /* ADC (IY+d) */
            ADC(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x8f: /* ADC A */
            ADC(reg_a, 0, 4, 2);
            break;
        case 0x90: /* SUB B */
            SUB(reg_b, 0, 4, 2);
            break;
        case 0x91: /* SUB C */
            SUB(reg_c, 0, 4, 2);
            break;
        case 0x92: /* SUB D */
            SUB(reg_d, 0, 4, 2);
            break;
        case 0x93: /* SUB E */
            SUB(reg_e, 0, 4, 2);
            break;
        case 0x94: /* SUB IYH */
            SUB(reg_iyh, 0, 4, 2);
            break;
        case 0x95: /* SUB IYL */
            SUB(reg_iyl, 0, 4, 2);
            break;
        case 0x96: /* SUB (IY+d) */
            SUB(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x97: /* SUB A */
            SUB(reg_a, 0, 4, 2);
            break;
        case 0x98: /* SBC B */
            SBC(reg_b, 0, 4, 2);
            break;
        case 0x99: /* SBC C */
            SBC(reg_c, 0, 4, 2);
            break;
        case 0x9a: /* SBC D */
            SBC(reg_d, 0, 4, 2);
            break;
        case 0x9b: /* SBC E */
            SBC(reg_e, 0, 4, 2);
            break;
        case 0x9c: /* SBC IYH */
            SBC(reg_iyh, 0, 4, 2);
            break;
        case 0x9d: /* SBC IYL */
            SBC(reg_iyl, 0, 4, 2);
            break;
        case 0x9e: /* SBC (IY+d) */
            SBC(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0x9f: /* SBC A */
            SBC(reg_a, 0, 4, 2);
            break;
        case 0xa0: /* AND B */
            AND(reg_b, 0, 4, 2);
            break;
        case 0xa1: /* AND C */
            AND(reg_c, 0, 4, 2);
            break;
        case 0xa2: /* AND D */
            AND(reg_d, 0, 4, 2);
            break;
        case 0xa3: /* AND E */
            AND(reg_e, 0, 4, 2);
            break;
        case 0xa4: /* AND IYH */
            AND(reg_iyh, 0, 4, 2);
            break;
        case 0xa5: /* AND IYL */
            AND(reg_iyl, 0, 4, 2);
            break;
        case 0xa6: /* AND (IY+d) */
            AND(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xa7: /* AND A */
            AND(reg_a, 0, 4, 2);
            break;
        case 0xa8: /* XOR B */
            XOR(reg_b, 0, 4, 2);
            break;
        case 0xa9: /* XOR C */
            XOR(reg_c, 0, 4, 2);
            break;
        case 0xaa: /* XOR D */
            XOR(reg_d, 0, 4, 2);
            break;
        case 0xab: /* XOR E */
            XOR(reg_e, 0, 4, 2);
            break;
        case 0xac: /* XOR IYH */
            XOR(reg_iyh, 0, 4, 2);
            break;
        case 0xad: /* XOR IYL */
            XOR(reg_iyl, 0, 4, 2);
            break;
        case 0xae: /* XOR (IY+d) */
            XOR(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xaf: /* XOR A */
            XOR(reg_a, 0, 4, 2);
            break;
        case 0xb0: /* OR B */
            OR(reg_b, 0, 4, 2);
            break;
        case 0xb1: /* OR C */
            OR(reg_c, 0, 4, 2);
            break;
        case 0xb2: /* OR D */
            OR(reg_d, 0, 4, 2);
            break;
        case 0xb3: /* OR E */
            OR(reg_e, 0, 4, 2);
            break;
        case 0xb4: /* OR IYH */
            OR(reg_iyh, 0, 4, 2);
            break;
        case 0xb5: /* OR IYL */
            OR(reg_iyl, 0, 4, 2);
            break;
        case 0xb6: /* OR (IY+d) */
            OR(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xb7: /* OR A */
            OR(reg_a, 0, 4, 2);
            break;
        case 0xb8: /* CP B */
            CP(reg_b, 0, 4, 2);
            break;
        case 0xb9: /* CP C */
            CP(reg_c, 0, 4, 2);
            break;
        case 0xba: /* CP D */
            CP(reg_d, 0, 4, 2);
            break;
        case 0xbb: /* CP E */
            CP(reg_e, 0, 4, 2);
            break;
        case 0xbc: /* CP IYH */
            CP(reg_iyh, 0, 4, 2);
            break;
        case 0xbd: /* CP IYL */
            CP(reg_iyl, 0, 4, 2);
            break;
        case 0xbe: /* CP (IY+d) */
            CP(LOAD(IY_WORD_OFF(ip2)), 8, 11, 3);
            break;
        case 0xbf: /* CP A */
            CP(reg_a, 0, 4, 2);
            break;
        case 0xc1: /* POP BC */
            POP(reg_b, reg_c, 2);
            break;
        case 0xc5: /* PUSH BC */
            PUSH(reg_b, reg_c, 2);
            break;
        case 0xcb: /* OPCODE FD CB */
            opcode_fd_cb((BYTE)ip2, (BYTE)ip3, (WORD)ip23);
            break;
        case 0xd1: /* POP DE */
            POP(reg_d, reg_e, 2);
            break;
        case 0xd3: /* OUT A */
            OUTA(ip2, 8, 7, 3);
            break;
        case 0xd5: /* PUSH DE */
            PUSH(reg_d, reg_e, 2);
            break;
        case 0xd9: /* EXX */
            EXX(12, 2);
            break;
        case 0xdb: /* IN A */
            INA(ip2, 8, 7, 3);
            break;
        case 0xdd: /* Skip FD */
            NOP(4, 1);
            break;
        case 0xe1: /* POP IY */
            POP(reg_iyh, reg_iyl, 2);
            break;
        case 0xe3: /* EX IY (SP) */
            EXXXSP(reg_iyh, reg_iyl, 4, 4, 4, 4, 7, 2);
            break;
        case 0xe5: /* PUSH IY */
            PUSH(reg_iyh, reg_iyl, 2);
            break;
        case 0xe9: /* LD PC IY */
            JMP((IY_WORD()), 8);
            break;
        case 0xeb:
            EXDEHL(8, 2);
            break;
        case 0xed: /* Skip FD */
            NOP(4, 1);
            break;
        case 0xf1: /* POP AF */
            POP(reg_a, reg_f, 2);
            break;
        case 0xf3: /* DI */
            DI(8, 2);
            break;
        case 0xf5: /* PUSH AF */
            PUSH(reg_a, reg_f, 2);
            break;
        case 0xf9: /* LD SP IY */
            LDSP(IY_WORD(), 4, 6, 2);
            break;
        case 0xfb: /* EI */
            EI(8, 2);
            break;
        case 0xfd: /* Skip FD */
            NOP(4, 1);
            break;
        default:
#ifdef DEBUG_Z80
            log_message(LOG_DEFAULT,
                        "%i PC %04x A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x SP%04x OP FD %02x %02x %02x.",
                        (int)(CLK), (unsigned int)z80_reg_pc, reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp, ip1, ip2, ip3);
#endif
            INC_PC(2);
    }
}

/* ------------------------------------------------------------------------- */

void z80_resync_limits(void)
{
    z80_bank_base = z80mem_read_base(z80_reg_pc);
    z80_bank_limit = z80mem_read_limit(z80_reg_pc);
}

/* ------------------------------------------------------------------------- */

/* Z80 mainloop.  */

void z80_mainloop(interrupt_cpu_status_t *cpu_int_status, alarm_context_t *cpu_alarm_context)
{
    opcode_t opcode;

    import_registers();

    dma_request = 0;

    do {
        while (CLK >= alarm_context_next_pending_clk(cpu_alarm_context)) {
            alarm_context_dispatch(cpu_alarm_context, CLK);
        }
        {
            enum cpu_int pending_interrupt;

            pending_interrupt = cpu_int_status->global_pending_int;
            if (pending_interrupt != IK_NONE) {
                DO_INTERRUPT(pending_interrupt);
                while (CLK >= alarm_context_next_pending_clk(cpu_alarm_context)) {
                    alarm_context_dispatch(cpu_alarm_context, CLK);
                }
            }
        }

        SET_LAST_ADDR(reg_pc);
        FETCH_OPCODE(opcode);

#ifdef DEBUG
        if (debug.maincpu_traceflg) {
            log_message(LOG_DEFAULT,
                        ".%04x %i %-25s A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x S%04x",
                        (unsigned int)z80_reg_pc, 0, mon_disassemble_to_string(e_comp_space, z80_reg_pc, p0, p1, p2, p3, 1, "z80"),
                        reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp);
        }
#endif

        SET_LAST_OPCODE(p0);

        switch (p0) {
            case 0x00: /* NOP */
                NOP(4, 1);
                break;
            case 0x01: /* LD BC # */
                LDW(p12, reg_b, reg_c, 10, 0, 3);
                break;
            case 0x02: /* LD (BC) A */
                STREG(BC_WORD(), reg_a, 4, 3, 1);
                break;
            case 0x03: /* INC BC */
                DECINC(INC_BC_WORD(), 6, 1);
                break;
            case 0x04: /* INC B */
                INCREG(reg_b, 4, 1);
                break;
            case 0x05: /* DEC B */
                DECREG(reg_b, 4, 1);
                break;
            case 0x06: /* LD B # */
                LDREG(reg_b, p1, 4, 3, 2);
                break;
            case 0x07: /* RLCA */
                RLCA(4, 1);
                break;
            case 0x08: /* EX AF AF' */
                EXAFAF(8, 1);
                break;
            case 0x09: /* ADD HL BC */
                ADDXXREG(reg_h, reg_l, reg_b, reg_c, 11, 1);
                break;
            case 0x0a: /* LD A (BC) */
                LDREG(reg_a, LOAD(BC_WORD()), 4, 3, 1);
                break;
            case 0x0b: /* DEC BC */
                DECINC(DEC_BC_WORD(), 6, 1);
                break;
            case 0x0c: /* INC C */
                INCREG(reg_c, 4, 1);
                break;
            case 0x0d: /* DEC C */
                DECREG(reg_c, 4, 1);
                break;
            case 0x0e: /* LD C # */
                LDREG(reg_c, p1, 4, 3, 2);
                break;
            case 0x0f: /* RRCA */
                RRCA(4, 1);
                break;
            case 0x10: /* DJNZ */
                DJNZ(p1, 2);
                break;
            case 0x11: /* LD DE # */
                LDW(p12, reg_d, reg_e, 10, 0, 3);
                break;
            case 0x12: /* LD (DE) A */
                STREG(DE_WORD(), reg_a, 4, 3, 1);
                break;
            case 0x13: /* INC DE */
                DECINC(INC_DE_WORD(), 6, 1);
                break;
            case 0x14: /* INC D */
                INCREG(reg_d, 4, 1);
                break;
            case 0x15: /* DEC D */
                DECREG(reg_d, 4, 1);
                break;
            case 0x16: /* LD D # */
                LDREG(reg_d, p1, 4, 3, 2);
                break;
            case 0x17: /* RLA */
                RLA(4, 1);
                break;
            case 0x18: /* JR */
                BRANCH(1, p1, 2);
                break;
            case 0x19: /* ADD HL DE */
                ADDXXREG(reg_h, reg_l, reg_d, reg_e, 11, 1);
                break;
            case 0x1a: /* LD A (DE) */
                LDREG(reg_a, LOAD(DE_WORD()), 4, 3, 1);
                break;
            case 0x1b: /* DEC DE */
                DECINC(DEC_DE_WORD(), 6, 1);
                break;
            case 0x1c: /* INC E */
                INCREG(reg_e, 4, 1);
                break;
            case 0x1d: /* DEC E */
                DECREG(reg_e, 4, 1);
                break;
            case 0x1e: /* LD E # */
                LDREG(reg_e, p1, 4, 3, 2);
                break;
            case 0x1f: /* RRA */
                RRA(4, 1);
                break;
            case 0x20: /* JR NZ */
                BRANCH(!LOCAL_ZERO(), p1, 2);
                break;
            case 0x21: /* LD HL # */
                LDW(p12, reg_h, reg_l, 10, 0, 3);
                break;
            case 0x22: /* LD (WORD) HL */
                STW(p12, reg_h, reg_l, 4, 9, 3, 3);
                break;
            case 0x23: /* INC HL */
                DECINC(INC_HL_WORD(), 6, 1);
                break;
            case 0x24: /* INC H */
                INCREG(reg_h, 4, 1);
                break;
            case 0x25: /* DEC H */
                DECREG(reg_h, 4, 1);
                break;
            case 0x26: /* LD H # */
                LDREG(reg_h, p1, 4, 3, 2);
                break;
            case 0x27: /* DAA */
                DAA(4, 1);
                break;
            case 0x28: /* JR Z */
                BRANCH(LOCAL_ZERO(), p1, 2);
                break;
            case 0x29: /* ADD HL HL */
                ADDXXREG(reg_h, reg_l, reg_h, reg_l, 11, 1);
                break;
            case 0x2a: /* LD HL (WORD) */
                LDIND(p12, reg_h, reg_l, 4, 4, 8, 3);
                break;
            case 0x2b: /* DEC HL */
                DECINC(DEC_HL_WORD(), 6, 1);
                break;
            case 0x2c: /* INC L */
                INCREG(reg_l, 4, 1);
                break;
            case 0x2d: /* DEC L */
                DECREG(reg_l, 4, 1);
                break;
            case 0x2e: /* LD L # */
                LDREG(reg_l, p1, 4, 3, 2);
                break;
            case 0x2f: /* CPL */
                CPL(4, 1);
                break;
            case 0x30: /* JR NC */
                BRANCH(!LOCAL_CARRY(), p1, 2);
                break;
            case 0x31: /* LD SP # */
                LDSP(p12, 10, 0, 3);
                break;
            case 0x32: /* LD (WORD) A */
                STREG(p12, reg_a, 10, 3, 3);
                break;
            case 0x33: /* INC SP */
                DECINC(reg_sp++, 6, 1);
                break;
            case 0x34: /* INC (HL) */
                INCXXIND(HL_WORD(), 4, 4, 3, 1);
                break;
            case 0x35: /* DEC (HL) */
                DECXXIND(HL_WORD(), 4, 4, 3, 1);
                break;
            case 0x36: /* LD (HL) # */
                STREG(HL_WORD(), p1, 8, 2, 2);
                break;
            case 0x37: /* SCF */
                SCF(4, 1);
                break;
            case 0x38: /* JR C */
                BRANCH(LOCAL_CARRY(), p1, 2);
                break;
            case 0x39: /* ADD HL SP */
                ADDXXSP(reg_h, reg_l, 11, 1);
                break;
            case 0x3a: /* LD A (WORD) */
                LDREG(reg_a, LOAD(p12), 10, 3, 3);
                break;
            case 0x3b: /* DEC SP */
                DECINC(reg_sp--, 6, 1);
                break;
            case 0x3c: /* INC A */
                INCREG(reg_a, 4, 1);
                break;
            case 0x3d: /* DEC A */
                DECREG(reg_a, 4, 1);
                break;
            case 0x3e: /* LD A # */
                LDREG(reg_a, p1, 4, 3, 2);
                break;
            case 0x3f: /* CCF */
                CCF(4, 1);
                break;
            case 0x40: /* LD B B */
                LDREG(reg_b, reg_b, 0, 4, 1);
                break;
            case 0x41: /* LD B C */
                LDREG(reg_b, reg_c, 0, 4, 1);
                break;
            case 0x42: /* LD B D */
                LDREG(reg_b, reg_d, 0, 4, 1);
                break;
            case 0x43: /* LD B E */
                LDREG(reg_b, reg_e, 0, 4, 1);
                break;
            case 0x44: /* LD B H */
                LDREG(reg_b, reg_h, 0, 4, 1);
                break;
            case 0x45: /* LD B L */
                LDREG(reg_b, reg_l, 0, 4, 1);
                break;
            case 0x46: /* LD B (HL) */
                LDREG(reg_b, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x47: /* LD B A */
                LDREG(reg_b, reg_a, 0, 4, 1);
                break;
            case 0x48: /* LD C B */
                LDREG(reg_c, reg_b, 0, 4, 1);
                break;
            case 0x49: /* LD C C */
                LDREG(reg_c, reg_c, 0, 4, 1);
                break;
            case 0x4a: /* LD C D */
                LDREG(reg_c, reg_d, 0, 4, 1);
                break;
            case 0x4b: /* LD C E */
                LDREG(reg_c, reg_e, 0, 4, 1);
                break;
            case 0x4c: /* LD C H */
                LDREG(reg_c, reg_h, 0, 4, 1);
                break;
            case 0x4d: /* LD C L */
                LDREG(reg_c, reg_l, 0, 4, 1);
                break;
            case 0x4e: /* LD C (HL) */
                LDREG(reg_c, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x4f: /* LD C A */
                LDREG(reg_c, reg_a, 0, 4, 1);
                break;
            case 0x50: /* LD D B */
                LDREG(reg_d, reg_b, 0, 4, 1);
                break;
            case 0x51: /* LD D C */
                LDREG(reg_d, reg_c, 0, 4, 1);
                break;
            case 0x52: /* LD D D */
                LDREG(reg_d, reg_d, 0, 4, 1);
                break;
            case 0x53: /* LD D E */
                LDREG(reg_d, reg_e, 0, 4, 1);
                break;
            case 0x54: /* LD D H */
                LDREG(reg_d, reg_h, 0, 4, 1);
                break;
            case 0x55: /* LD D L */
                LDREG(reg_d, reg_l, 0, 4, 1);
                break;
            case 0x56: /* LD D (HL) */
                LDREG(reg_d, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x57: /* LD D A */
                LDREG(reg_d, reg_a, 0, 4, 1);
                break;
            case 0x58: /* LD E B */
                LDREG(reg_e, reg_b, 0, 4, 1);
                break;
            case 0x59: /* LD E C */
                LDREG(reg_e, reg_c, 0, 4, 1);
                break;
            case 0x5a: /* LD E D */
                LDREG(reg_e, reg_d, 0, 4, 1);
                break;
            case 0x5b: /* LD E E */
                LDREG(reg_e, reg_e, 0, 4, 1);
                break;
            case 0x5c: /* LD E H */
                LDREG(reg_e, reg_h, 0, 4, 1);
                break;
            case 0x5d: /* LD E L */
                LDREG(reg_e, reg_l, 0, 4, 1);
                break;
            case 0x5e: /* LD E (HL) */
                LDREG(reg_e, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x5f: /* LD E A */
                LDREG(reg_e, reg_a, 0, 4, 1);
                break;
            case 0x60: /* LD H B */
                LDREG(reg_h, reg_b, 0, 4, 1);
                break;
            case 0x61: /* LD H C */
                LDREG(reg_h, reg_c, 0, 4, 1);
                break;
            case 0x62: /* LD H D */
                LDREG(reg_h, reg_d, 0, 4, 1);
                break;
            case 0x63: /* LD H E */
                LDREG(reg_h, reg_e, 0, 4, 1);
                break;
            case 0x64: /* LD H H */
                LDREG(reg_h, reg_h, 0, 4, 1);
                break;
            case 0x65: /* LD H L */
                LDREG(reg_h, reg_l, 0, 4, 1);
                break;
            case 0x66: /* LD H (HL) */
                LDREG(reg_h, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x67: /* LD H A */
                LDREG(reg_h, reg_a, 0, 4, 1);
                break;
            case 0x68: /* LD L B */
                LDREG(reg_l, reg_b, 0, 4, 1);
                break;
            case 0x69: /* LD L C */
                LDREG(reg_l, reg_c, 0, 4, 1);
                break;
            case 0x6a: /* LD L D */
                LDREG(reg_l, reg_d, 0, 4, 1);
                break;
            case 0x6b: /* LD L E */
                LDREG(reg_l, reg_e, 0, 4, 1);
                break;
            case 0x6c: /* LD L H */
                LDREG(reg_l, reg_h, 0, 4, 1);
                break;
            case 0x6d: /* LD L L */
                LDREG(reg_l, reg_l, 0, 4, 1);
                break;
            case 0x6e: /* LD L (HL) */
                LDREG(reg_l, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x6f: /* LD L A */
                LDREG(reg_l, reg_a, 0, 4, 1);
                break;
            case 0x70: /* LD (HL) B */
                STREG(HL_WORD(), reg_b, 4, 3, 1);
                break;
            case 0x71: /* LD (HL) C */
                STREG(HL_WORD(), reg_c, 4, 3, 1);
                break;
            case 0x72: /* LD (HL) D */
                STREG(HL_WORD(), reg_d, 4, 3, 1);
                break;
            case 0x73: /* LD (HL) E */
                STREG(HL_WORD(), reg_e, 4, 3, 1);
                break;
            case 0x74: /* LD (HL) H */
                STREG(HL_WORD(), reg_h, 4, 3, 1);
                break;
            case 0x75: /* LD (HL) L */
                STREG(HL_WORD(), reg_l, 4, 3, 1);
                break;
            case 0x76: /* HALT */
                HALT();
                break;
            case 0x77: /* LD (HL) A */
                STREG(HL_WORD(), reg_a, 4, 3, 1);
                break;
            case 0x78: /* LD A B */
                LDREG(reg_a, reg_b, 0, 4, 1);
                break;
            case 0x79: /* LD A C */
                LDREG(reg_a, reg_c, 0, 4, 1);
                break;
            case 0x7a: /* LD A D */
                LDREG(reg_a, reg_d, 0, 4, 1);
                break;
            case 0x7b: /* LD A E */
                LDREG(reg_a, reg_e, 0, 4, 1);
                break;
            case 0x7c: /* LD A H */
                LDREG(reg_a, reg_h, 0, 4, 1);
                break;
            case 0x7d: /* LD A L */
                LDREG(reg_a, reg_l, 0, 4, 1);
                break;
            case 0x7e: /* LD A (HL) */
                LDREG(reg_a, LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x7f: /* LD A A */
                LDREG(reg_a, reg_a, 0, 4, 1);
                break;
            case 0x80: /* ADD B */
                ADD(reg_b, 0, 4, 1);
                break;
            case 0x81: /* ADD C */
                ADD(reg_c, 0, 4, 1);
                break;
            case 0x82: /* ADD D */
                ADD(reg_d, 0, 4, 1);
                break;
            case 0x83: /* ADD E */
                ADD(reg_e, 0, 4, 1);
                break;
            case 0x84: /* ADD H */
                ADD(reg_h, 0, 4, 1);
                break;
            case 0x85: /* ADD L */
                ADD(reg_l, 0, 4, 1);
                break;
            case 0x86: /* ADD (HL) */
                ADD(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x87: /* ADD A */
                ADD(reg_a, 0, 4, 1);
                break;
            case 0x88: /* ADC B */
                ADC(reg_b, 0, 4, 1);
                break;
            case 0x89: /* ADC C */
                ADC(reg_c, 0, 4, 1);
                break;
            case 0x8a: /* ADC D */
                ADC(reg_d, 0, 4, 1);
                break;
            case 0x8b: /* ADC E */
                ADC(reg_e, 0, 4, 1);
                break;
            case 0x8c: /* ADC H */
                ADC(reg_h, 0, 4, 1);
                break;
            case 0x8d: /* ADC L */
                ADC(reg_l, 0, 4, 1);
                break;
            case 0x8e: /* ADC (HL) */
                ADC(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x8f: /* ADC A */
                ADC(reg_a, 0, 4, 1);
                break;
            case 0x90: /* SUB B */
                SUB(reg_b, 0, 4, 1);
                break;
            case 0x91: /* SUB C */
                SUB(reg_c, 0, 4, 1);
                break;
            case 0x92: /* SUB D */
                SUB(reg_d, 0, 4, 1);
                break;
            case 0x93: /* SUB E */
                SUB(reg_e, 0, 4, 1);
                break;
            case 0x94: /* SUB H */
                SUB(reg_h, 0, 4, 1);
                break;
            case 0x95: /* SUB L */
                SUB(reg_l, 0, 4, 1);
                break;
            case 0x96: /* SUB (HL) */
                SUB(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x97: /* SUB A */
                SUB(reg_a, 0, 4, 1);
                break;
            case 0x98: /* SBC B */
                SBC(reg_b, 0, 4, 1);
                break;
            case 0x99: /* SBC C */
                SBC(reg_c, 0, 4, 1);
                break;
            case 0x9a: /* SBC D */
                SBC(reg_d, 0, 4, 1);
                break;
            case 0x9b: /* SBC E */
                SBC(reg_e, 0, 4, 1);
                break;
            case 0x9c: /* SBC H */
                SBC(reg_h, 0, 4, 1);
                break;
            case 0x9d: /* SBC L */
                SBC(reg_l, 0, 4, 1);
                break;
            case 0x9e: /* SBC (HL) */
                SBC(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0x9f: /* SBC A */
                SBC(reg_a, 0, 4, 1);
                break;
            case 0xa0: /* AND B */
                AND(reg_b, 0, 4, 1);
                break;
            case 0xa1: /* AND C */
                AND(reg_c, 0, 4, 1);
                break;
            case 0xa2: /* AND D */
                AND(reg_d, 0, 4, 1);
                break;
            case 0xa3: /* AND E */
                AND(reg_e, 0, 4, 1);
                break;
            case 0xa4: /* AND H */
                AND(reg_h, 0, 4, 1);
                break;
            case 0xa5: /* AND L */
                AND(reg_l, 0, 4, 1);
                break;
            case 0xa6: /* AND (HL) */
                AND(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0xa7: /* AND A */
                AND(reg_a, 0, 4, 1);
                break;
            case 0xa8: /* XOR B */
                XOR(reg_b, 0, 4, 1);
                break;
            case 0xa9: /* XOR C */
                XOR(reg_c, 0, 4, 1);
                break;
            case 0xaa: /* XOR D */
                XOR(reg_d, 0, 4, 1);
                break;
            case 0xab: /* XOR E */
                XOR(reg_e, 0, 4, 1);
                break;
            case 0xac: /* XOR H */
                XOR(reg_h, 0, 4, 1);
                break;
            case 0xad: /* XOR L */
                XOR(reg_l, 0, 4, 1);
                break;
            case 0xae: /* XOR (HL) */
                XOR(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0xaf: /* XOR A */
                XOR(reg_a, 0, 4, 1);
                break;
            case 0xb0: /* OR B */
                OR(reg_b, 0, 4, 1);
                break;
            case 0xb1: /* OR C */
                OR(reg_c, 0, 4, 1);
                break;
            case 0xb2: /* OR D */
                OR(reg_d, 0, 4, 1);
                break;
            case 0xb3: /* OR E */
                OR(reg_e, 0, 4, 1);
                break;
            case 0xb4: /* OR H */
                OR(reg_h, 0, 4, 1);
                break;
            case 0xb5: /* OR L */
                OR(reg_l, 0, 4, 1);
                break;
            case 0xb6: /* OR (HL) */
                OR(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0xb7: /* OR A */
                OR(reg_a, 0, 4, 1);
                break;
            case 0xb8: /* CP B */
                CP(reg_b, 0, 4, 1);
                break;
            case 0xb9: /* CP C */
                CP(reg_c, 0, 4, 1);
                break;
            case 0xba: /* CP D */
                CP(reg_d, 0, 4, 1);
                break;
            case 0xbb: /* CP E */
                CP(reg_e, 0, 4, 1);
                break;
            case 0xbc: /* CP H */
                CP(reg_h, 0, 4, 1);
                break;
            case 0xbd: /* CP L */
                CP(reg_l, 0, 4, 1);
                break;
            case 0xbe: /* CP (HL) */
                CP(LOAD(HL_WORD()), 4, 3, 1);
                break;
            case 0xbf: /* CP A */
                CP(reg_a, 0, 4, 1);
                break;
            case 0xc0: /* RET NZ */
                RET_COND(!LOCAL_ZERO(), 4, 4, 2, 5, 1);
                break;
            case 0xc1: /* POP BC */
                POP(reg_b, reg_c, 1);
                break;
            case 0xc2: /* JP NZ */
                JMP_COND(p12, !LOCAL_ZERO(), 10, 10);
                break;
            case 0xc3: /* JP */
                JMP(p12, 10);
                break;
            case 0xc4: /* CALL NZ */
                CALL_COND(p12, !LOCAL_ZERO(), 3, 3, 4, 10, 3);
                break;
            case 0xc5: /* PUSH BC */
                PUSH(reg_b, reg_c, 1);
                break;
            case 0xc6: /* ADD # */
                ADD(p1, 4, 3, 2);
                break;
            case 0xc7: /* RST 00 */
                CALL(0x00, 3, 3, 5, 1);
                break;
            case 0xc8: /* RET Z */
                RET_COND(LOCAL_ZERO(), 4, 4, 2, 5, 1);
                break;
            case 0xc9: /* RET */
                RET(4, 4, 2);
                break;
            case 0xca: /* JP Z */
                JMP_COND(p12, LOCAL_ZERO(), 10, 10);
                break;
            case 0xcb: /* OPCODE CB */
                opcode_cb((BYTE)p1, (BYTE)p2, (BYTE)p3, (WORD)p12, (WORD)p23);
                break;
            case 0xcc: /* CALL Z */
                CALL_COND(p12, LOCAL_ZERO(), 3, 3, 4, 10, 3);
                break;
            case 0xcd: /* CALL */
                CALL(p12, 3, 3, 11, 3);
                break;
            case 0xce: /* ADC # */
                ADC(p1, 4, 3, 2);
                break;
            case 0xcf: /* RST 08 */
                CALL(0x08, 3, 3, 5, 1);
                break;
            case 0xd0: /* RET NC */
                RET_COND(!LOCAL_CARRY(), 4, 4, 2, 5, 1);
                break;
            case 0xd1: /* POP DE */
                POP(reg_d, reg_e, 1);
                break;
            case 0xd2: /* JP NC */
                JMP_COND(p12, !LOCAL_CARRY(), 10, 10);
                break;
            case 0xd3: /* OUT A */
                OUTA(p1, 4, 7, 2);
                break;
            case 0xd4: /* CALL NC */
                CALL_COND(p12, !LOCAL_CARRY(), 3, 3, 4, 10, 3);
                break;
            case 0xd5: /* PUSH DE */
                PUSH(reg_d, reg_e, 1);
                break;
            case 0xd6: /* SUB # */
                SUB(p1, 4, 3, 2);
                break;
            case 0xd7: /* RST 10 */
                CALL(0x10, 3, 3, 5, 1);
                break;
            case 0xd8: /* RET C */
                RET_COND(LOCAL_CARRY(), 4, 4, 2, 5, 1);
                break;
            case 0xd9: /* EXX */
                EXX(8, 1);
                break;
            case 0xda: /* JP C */
                JMP_COND(p12, LOCAL_CARRY(), 10, 10);
                break;
            case 0xdb: /* IN A */
                INA(p1, 4, 7, 2);
                break;
            case 0xdc: /* CALL C */
                CALL_COND(p12, LOCAL_CARRY(), 3, 3, 4, 10, 3);
                break;
            case 0xdd: /*  OPCODE DD */
                opcode_dd((BYTE)p1, (BYTE)p2, (BYTE)p3, (WORD)p12, (WORD)p23);
                break;
            case 0xde: /* SBC # */
                SBC(p1, 4, 3, 2);
                break;
            case 0xdf: /* RST 18 */
                CALL(0x18, 3, 3, 5, 1);
                break;
            case 0xe0: /* RET PO */
                RET_COND(!LOCAL_PARITY(), 4, 4, 2, 5, 1);
                break;
            case 0xe1: /* POP HL */
                POP(reg_h, reg_l, 1);
                break;
            case 0xe2: /* JP PO */
                JMP_COND(p12, !LOCAL_PARITY(), 10, 10);
                break;
            case 0xe3: /* EX HL (SP) */
                EXXXSP(reg_h, reg_l, 4, 4, 4, 4, 3, 1);
                break;
            case 0xe4: /* CALL PO */
                CALL_COND(p12, !LOCAL_PARITY(), 3, 3, 4, 10, 3);
                break;
            case 0xe5: /* PUSH HL */
                PUSH(reg_h, reg_l, 1);
                break;
            case 0xe6: /* AND # */
                AND(p1, 4, 3, 2);
                break;
            case 0xe7: /* RST 20 */
                CALL(0x20, 3, 3, 5, 1);
                break;
            case 0xe8: /* RET PE */
                RET_COND(LOCAL_PARITY(), 4, 4, 2, 5, 1);
                break;
            case 0xe9: /* LD PC HL */
                JMP((HL_WORD()), 4);
                break;
            case 0xea: /* JP PE */
                JMP_COND(p12, LOCAL_PARITY(), 10, 10);
                break;
            case 0xeb: /* EX DE HL */
                EXDEHL(4, 1);
                break;
            case 0xec: /* CALL PE */
                CALL_COND(p12, LOCAL_PARITY(), 3, 3, 4, 10, 3);
                break;
            case 0xed: /* OPCODE ED */
                opcode_ed((BYTE)p1, (BYTE)p2, (BYTE)p3, (WORD)p12, (WORD)p23);
                break;
            case 0xee: /* XOR # */
                XOR(p1, 4, 3, 2);
                break;
            case 0xef: /* RST 28 */
                CALL(0x28, 3, 3, 5, 1);
                break;
            case 0xf0: /* RET P */
                RET_COND(!LOCAL_SIGN(), 4, 4, 2, 5, 1);
                break;
            case 0xf1: /* POP AF */
                POP(reg_a, reg_f, 1);
                break;
            case 0xf2: /* JP P */
                JMP_COND(p12, !LOCAL_SIGN(), 10, 10);
                break;
            case 0xf3: /* DI */
                DI(4, 1);
                break;
            case 0xf4: /* CALL P */
                CALL_COND(p12, !LOCAL_SIGN(), 3, 3, 4, 10, 3);
                break;
            case 0xf5: /* PUSH AF */
                PUSH(reg_a, reg_f, 1);
                break;
            case 0xf6: /* OR # */
                OR(p1, 4, 3, 2);
                break;
            case 0xf7: /* RST 30 */
                CALL(0x30, 3, 3, 5, 1);
                break;
            case 0xf8: /* RET M */
                RET_COND(LOCAL_SIGN(), 4, 4, 2, 5, 1);
                break;
            case 0xf9: /* LD SP HL */
                LDSP(HL_WORD(), 4, 2, 1);
                break;
            case 0xfa: /* JP M */
                JMP_COND(p12, LOCAL_SIGN(), 10, 10);
                break;
            case 0xfb: /* EI */
                EI(4, 1);
                break;
            case 0xfc: /* CALL M */
                CALL_COND(p12, LOCAL_SIGN(), 3, 3, 4, 10, 3);
                break;
            case 0xfd: /* OPCODE FD */
                opcode_fd((BYTE)p1, (BYTE)p2, (BYTE)p3, (WORD)p12, (WORD)p23);
                break;
            case 0xfe: /* CP # */
                CP(p1, 4, 3, 2);
                break;
            case 0xff: /* RST 38 */
                CALL(0x38, 3, 3, 5, 1);
                break;
        }

        cpu_int_status->num_dma_per_opcode = 0;
    } while (!dma_request);

    export_registers();
}
