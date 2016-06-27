/*
 * mos6510dtv.h - Definitions for emulation of the DTV version of the 6510 processor.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on code by
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

#ifndef VICE_MOS6510DTV_H
#define VICE_MOS6510DTV_H

#include "types.h"

/* Struct for the 6510DTV registers.  */
/* This contains some redundancy to make it compatible with the way registers
   are handled within the CPU emulation.  The struct should be accessed using
   the `MOS6510DTV_REGS_*()' macros.  */
typedef struct mos6510dtv_regs_s {
    unsigned int pc;        /* `unsigned int' required by the drive code. */
    BYTE a;
    BYTE x;
    BYTE y;
    BYTE sp;
    BYTE p;
    BYTE n;
    BYTE z;
    BYTE r3;
    BYTE r4;
    BYTE r5;
    BYTE r6;
    BYTE r7;
    BYTE r8;
    BYTE r9;
    BYTE r10;
    BYTE r11;
    BYTE r12;
    BYTE r13;
    BYTE r14;
    BYTE r15;
    BYTE acm;
    BYTE yxm;
} mos6510dtv_regs_t;

/* These define the position of the status flags in the P (`status')
   register.  */
#define P_SIGN          0x80
#define P_OVERFLOW      0x40
#define P_UNUSED        0x20
#define P_BREAK         0x10
#define P_DECIMAL       0x08
#define P_INTERRUPT     0x04
#define P_ZERO          0x02
#define P_CARRY         0x01

#define MOS6510DTV_REGS_SET_A(regs, val) \
    (regs)->a = (val)
#define MOS6510DTV_REGS_SET_X(regs, val) \
    (regs)->x = (val)
#define MOS6510DTV_REGS_SET_Y(regs, val) \
    (regs)->y = (val)
#define MOS6510DTV_REGS_SET_SP(regs, val) \
    (regs)->sp = (val)
#define MOS6510DTV_REGS_SET_PC(regs, val) \
    (regs)->pc = (val)
#define MOS6510DTV_REGS_SET_R3(regs, val) \
    (regs)->r3 = (val)
#define MOS6510DTV_REGS_SET_R4(regs, val) \
    (regs)->r4 = (val)
#define MOS6510DTV_REGS_SET_R5(regs, val) \
    (regs)->r5 = (val)
#define MOS6510DTV_REGS_SET_R6(regs, val) \
    (regs)->r6 = (val)
#define MOS6510DTV_REGS_SET_R7(regs, val) \
    (regs)->r7 = (val)
#define MOS6510DTV_REGS_SET_R8(regs, val) \
    (regs)->r8 = (val)
#define MOS6510DTV_REGS_SET_R9(regs, val) \
    (regs)->r9 = (val)
#define MOS6510DTV_REGS_SET_R10(regs, val) \
    (regs)->r10 = (val)
#define MOS6510DTV_REGS_SET_R11(regs, val) \
    (regs)->r11 = (val)
#define MOS6510DTV_REGS_SET_R12(regs, val) \
    (regs)->r12 = (val)
#define MOS6510DTV_REGS_SET_R13(regs, val) \
    (regs)->r13 = (val)
#define MOS6510DTV_REGS_SET_R14(regs, val) \
    (regs)->r14 = (val)
#define MOS6510DTV_REGS_SET_R15(regs, val) \
    (regs)->r15 = (val)
#define MOS6510DTV_REGS_SET_ACM(regs, val) \
    (regs)->acm = (val)
#define MOS6510DTV_REGS_SET_YXM(regs, val) \
    (regs)->yxm = (val)

#define MOS6510DTV_REGS_SET_OVERFLOW(regs, val) \
    do {                                        \
        if (val) {                              \
            (regs)->p |= P_OVERFLOW;            \
        } else {                                \
            (regs)->p &= ~P_OVERFLOW;           \
        }                                       \
    } while (0)

#define MOS6510DTV_REGS_SET_BREAK(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_BREAK;            \
        } else {                             \
            (regs)->p &= ~P_BREAK;           \
        }                                    \
    } while (0)

#define MOS6510DTV_REGS_SET_DECIMAL(regs, val) \
    do {                                       \
        if (val) {                             \
            (regs)->p |= P_DECIMAL;            \
        } else {                               \
            (regs)->p &= ~P_DECIMAL;           \
        }                                      \
    } while (0)

#define MOS6510DTV_REGS_SET_INTERRUPT(regs, val) \
    do {                                         \
        if (val) {                               \
            (regs)->p |= P_INTERRUPT;            \
        } else {                                 \
            (regs)->p &= ~P_INTERRUPT;           \
        }                                        \
    } while (0)

#define MOS6510DTV_REGS_SET_CARRY(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_CARRY;            \
        } else {                             \
            (regs)->p &= ~P_CARRY;           \
        }                                    \
    } while (0)

#define MOS6510DTV_REGS_SET_SIGN(regs, val) ((regs)->n = (val) ? 0x80 : 0)

#define MOS6510DTV_REGS_SET_ZERO(regs, val) ((regs)->z = !(val))

#define MOS6510DTV_REGS_SET_STATUS(regs, val)        \
    ((regs)->p = ((val) & ~(P_ZERO | P_SIGN)),       \
     MOS6510DTV_REGS_SET_ZERO(regs, (val) & P_ZERO), \
     (regs)->n = (val))

#define MOS6510DTV_REGS_GET_A(regs) ((regs)->a)
#define MOS6510DTV_REGS_GET_X(regs) ((regs)->x)
#define MOS6510DTV_REGS_GET_Y(regs) ((regs)->y)
#define MOS6510DTV_REGS_GET_SP(regs) ((regs)->sp)
#define MOS6510DTV_REGS_GET_PC(regs) ((regs)->pc)
#define MOS6510DTV_REGS_GET_FLAGS(regs) ((regs)->p)
#define MOS6510DTV_REGS_GET_R3(regs) ((regs)->r3)
#define MOS6510DTV_REGS_GET_R4(regs) ((regs)->r4)
#define MOS6510DTV_REGS_GET_R5(regs) ((regs)->r5)
#define MOS6510DTV_REGS_GET_R6(regs) ((regs)->r6)
#define MOS6510DTV_REGS_GET_R7(regs) ((regs)->r7)
#define MOS6510DTV_REGS_GET_R8(regs) ((regs)->r8)
#define MOS6510DTV_REGS_GET_R9(regs) ((regs)->r9)
#define MOS6510DTV_REGS_GET_R10(regs) ((regs)->r10)
#define MOS6510DTV_REGS_GET_R11(regs) ((regs)->r11)
#define MOS6510DTV_REGS_GET_R12(regs) ((regs)->r12)
#define MOS6510DTV_REGS_GET_R13(regs) ((regs)->r13)
#define MOS6510DTV_REGS_GET_R14(regs) ((regs)->r14)
#define MOS6510DTV_REGS_GET_R15(regs) ((regs)->r15)
#define MOS6510DTV_REGS_GET_ACM(regs) ((regs)->acm)
#define MOS6510DTV_REGS_GET_YXM(regs) ((regs)->yxm)
#define MOS6510DTV_REGS_GET_OVERFLOW(regs) ((regs)->p & P_OVERFLOW)
#define MOS6510DTV_REGS_GET_BREAK(regs) ((regs)->p & P_BREAK)
#define MOS6510DTV_REGS_GET_DECIMAL(regs) ((regs)->p & P_DECIMAL)
#define MOS6510DTV_REGS_GET_INTERRUPT(regs) ((regs)->p & P_INTERRUPT)
#define MOS6510DTV_REGS_GET_CARRY(regs) ((regs)->p & P_CARRY)
#define MOS6510DTV_REGS_GET_SIGN(regs) ((regs)->n & 0x80)
#define MOS6510DTV_REGS_GET_ZERO(regs) (!(regs)->z)
#define MOS6510DTV_REGS_GET_STATUS(regs)       \
    ((regs)->p | ((regs)->n & 0x80) | P_UNUSED \
     | (MOS6510DTV_REGS_GET_ZERO(regs) ? P_ZERO : 0))

#endif
