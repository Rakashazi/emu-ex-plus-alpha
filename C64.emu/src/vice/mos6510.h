/*
 * mos6510.h - Definitions for emulation of the 6510 processor.
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

#ifndef VICE_MOS6510_H
#define VICE_MOS6510_H

#include "types.h"

/* Struct for the 6510 registers.  */
/* This contains some redundancy to make it compatible with the way registers
   are handled within the CPU emulation.  The struct should be accessed using
   the `MOS6510_REGS_*()' macros.  */
typedef struct mos6510_regs_s {
    unsigned int pc;        /* `unsigned int' required by the drive code. */
    BYTE a;
    BYTE x;
    BYTE y;
    BYTE sp;
    BYTE p;
    BYTE n;
    BYTE z;
} mos6510_regs_t;

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

#define MOS6510_REGS_SET_A(regs, val) (regs)->a = (val)
#define MOS6510_REGS_SET_X(regs, val) (regs)->x = (val)
#define MOS6510_REGS_SET_Y(regs, val) (regs)->y = (val)
#define MOS6510_REGS_SET_SP(regs, val) (regs)->sp = (val)
#define MOS6510_REGS_SET_PC(regs, val) (regs)->pc = (val)

#define MOS6510_REGS_SET_OVERFLOW(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_OVERFLOW;         \
        } else {                             \
            (regs)->p &= ~P_OVERFLOW;        \
        }                                    \
    } while (0)

#define MOS6510_REGS_SET_BREAK(regs, val) \
    do {                                  \
        if (val) {                        \
            (regs)->p |= P_BREAK;         \
        } else {                          \
            (regs)->p &= ~P_BREAK;        \
        }                                 \
    } while (0)

#define MOS6510_REGS_SET_DECIMAL(regs, val) \
    do {                                    \
        if (val) {                          \
            (regs)->p |= P_DECIMAL;         \
        } else {                            \
            (regs)->p &= ~P_DECIMAL;        \
        }                                   \
    } while (0)

#define MOS6510_REGS_SET_INTERRUPT(regs, val) \
    do {                                      \
        if (val) {                            \
            (regs)->p |= P_INTERRUPT;         \
        } else {                              \
            (regs)->p &= ~P_INTERRUPT;        \
        }                                     \
    } while (0)

#define MOS6510_REGS_SET_CARRY(regs, val) \
    do {                                  \
        if (val) {                        \
            (regs)->p |= P_CARRY;         \
        } else {                          \
            (regs)->p &= ~P_CARRY;        \
        }                                 \
    } while (0)

#define MOS6510_REGS_SET_SIGN(regs, val) ((regs)->n = (val) ? 0x80 : 0)

#define MOS6510_REGS_SET_ZERO(regs, val) ((regs)->z = !(val))

#define MOS6510_REGS_SET_STATUS(regs, val)        \
    ((regs)->p = ((val) & ~(P_ZERO | P_SIGN)),    \
     MOS6510_REGS_SET_ZERO(regs, (val) & P_ZERO), \
     (regs)->n = (val))

#define MOS6510_REGS_GET_A(regs) ((regs)->a)
#define MOS6510_REGS_GET_X(regs) ((regs)->x)
#define MOS6510_REGS_GET_Y(regs) ((regs)->y)
#define MOS6510_REGS_GET_SP(regs) ((regs)->sp)
#define MOS6510_REGS_GET_PC(regs) ((regs)->pc)
#define MOS6510_REGS_GET_FLAGS(regs) ((regs)->p)
#define MOS6510_REGS_GET_OVERFLOW(regs) ((regs)->p & P_OVERFLOW)
#define MOS6510_REGS_GET_BREAK(regs) ((regs)->p & P_BREAK)
#define MOS6510_REGS_GET_DECIMAL(regs) ((regs)->p & P_DECIMAL)
#define MOS6510_REGS_GET_INTERRUPT(regs) ((regs)->p & P_INTERRUPT)
#define MOS6510_REGS_GET_CARRY(regs) ((regs)->p & P_CARRY)
#define MOS6510_REGS_GET_SIGN(regs) ((regs)->n & 0x80)
#define MOS6510_REGS_GET_ZERO(regs) (!(regs)->z)
#define MOS6510_REGS_GET_STATUS(regs)          \
    ((regs)->p | ((regs)->n & 0x80) | P_UNUSED \
     | (MOS6510_REGS_GET_ZERO(regs) ? P_ZERO : 0))

#endif
