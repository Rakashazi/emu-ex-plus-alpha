/*
 * WDC65816.h - Definitions for emulation of the WDC65816 processor.
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

#ifndef VICE_WDC65816_H
#define VICE_WDC65816_H

#include "types.h"

/* Struct for the WDC65816 registers.  */
/* This contains some redundancy to make it compatible with the way registers
   are handled within the CPU emulation.  The struct should be accessed using
   the `WDC65816_REGS_*()' macros.  */
typedef struct WDC65816_regs_s {
    unsigned int pc;
    BYTE a;
    BYTE b;
    WORD x;
    WORD y;
    BYTE pbr;
    BYTE dbr;
    WORD dpr;
    WORD sp;
    BYTE emul;
    BYTE p;
    BYTE n;
    BYTE z;
} WDC65816_regs_t;

/* These define the position of the status flags in the P (`status')
   register.  */
#define P_SIGN          0x80
#define P_OVERFLOW      0x40
#define P_UNUSED        0x20
#define P_65816_M       0x20
#define P_65816_X       0x10
#define P_BREAK         0x10
#define P_DECIMAL       0x08
#define P_INTERRUPT     0x04
#define P_ZERO          0x02
#define P_CARRY         0x01

#define WDC65816_REGS_SET_A(regs, val) \
    (regs)->a = (val)
#define WDC65816_REGS_SET_B(regs, val) \
    (regs)->b = (val)
#define WDC65816_REGS_SET_X(regs, val) \
    (regs)->x = (val)
#define WDC65816_REGS_SET_Y(regs, val) \
    (regs)->y = (val)
#define WDC65816_REGS_SET_PBR(regs, val) \
    (regs)->pbr = (val)
#define WDC65816_REGS_SET_DBR(regs, val) \
    (regs)->dbr = (val)
#define WDC65816_REGS_SET_DPR(regs, val) \
    (regs)->dpr = (val)
#define WDC65816_REGS_SET_SP(regs, val) \
    (regs)->sp = (val)
#define WDC65816_REGS_SET_PC(regs, val) \
    (regs)->pc = (val)
#define WDC65816_REGS_SET_EMUL(regs, val) \
    (regs)->emul = (val)

#define WDC65816_REGS_SET_OVERFLOW(regs, val) \
    do {                                      \
        if (val) {                            \
            (regs)->p |= P_OVERFLOW;          \
        } else {                              \
            (regs)->p &= ~P_OVERFLOW;         \
        }                                     \
    } while (0)

#define WDC65816_REGS_SET_BREAK(regs, val) \
    do {                                   \
        if (val) {                         \
            (regs)->p |= P_BREAK;          \
        } else {                           \
            (regs)->p &= ~P_BREAK;         \
        }                                  \
    } while (0)

#define WDC65816_REGS_SET_DECIMAL(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_DECIMAL;          \
        } else {                             \
            (regs)->p &= ~P_DECIMAL;         \
        }                                    \
    } while (0)

#define WDC65816_REGS_SET_INTERRUPT(regs, val) \
    do {                                       \
        if (val) {                             \
            (regs)->p |= P_INTERRUPT;          \
        } else {                               \
            (regs)->p &= ~P_INTERRUPT;         \
        }                                      \
    } while (0)

#define WDC65816_REGS_SET_CARRY(regs, val) \
    do {                                   \
        if (val) {                         \
            (regs)->p |= P_CARRY;          \
        } else {                           \
            (regs)->p &= ~P_CARRY;         \
        }                                  \
    } while (0)

#define WDC65816_REGS_SET_65816_M(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_65816_M;          \
        } else {                             \
            (regs)->p &= ~P_65816_M;         \
        }                                    \
    } while (0)

#define WDC65816_REGS_SET_65816_X(regs, val) \
    do {                                     \
        if (val) {                           \
            (regs)->p |= P_65816_X;          \
        } else {                             \
            (regs)->p &= ~P_65816_X;         \
        }                                    \
    } while (0)

#define WDC65816_REGS_SET_SIGN(regs, val) \
    ((regs)->n = (val) ? 0x80 : 0)

#define WDC65816_REGS_SET_ZERO(regs, val) \
    ((regs)->z = !(val))

#define WDC65816_REGS_SET_STATUS(regs, val) \
    ((regs)->p = ((val) & ~(P_ZERO | P_SIGN)), \
     WDC65816_REGS_SET_ZERO(regs, (val) & P_ZERO), \
     (regs)->n = (val))

#define WDC65816_REGS_GET_A(regs) \
    ((regs)->a)
#define WDC65816_REGS_GET_B(regs) \
    ((regs)->b)
#define WDC65816_REGS_GET_X(regs) \
    ((regs)->x)
#define WDC65816_REGS_GET_Y(regs) \
    ((regs)->y)
#define WDC65816_REGS_GET_PBR(regs) \
    ((regs)->pbr)
#define WDC65816_REGS_GET_DBR(regs) \
    ((regs)->dbr)
#define WDC65816_REGS_GET_DPR(regs) \
    ((regs)->dpr)
#define WDC65816_REGS_GET_EMUL(regs) \
    ((regs)->emul)
#define WDC65816_REGS_GET_SP(regs) \
    ((regs)->sp)
#define WDC65816_REGS_GET_PC(regs) \
    ((regs)->pc)
#define WDC65816_REGS_GET_FLAGS(regs) \
    ((regs)->p)
#define WDC65816_REGS_GET_OVERFLOW(regs) \
    ((regs)->p & P_OVERFLOW)
#define WDC65816_REGS_GET_BREAK(regs) \
    ((regs)->p & P_BREAK)
#define WDC65816_REGS_GET_DECIMAL(regs) \
    ((regs)->p & P_DECIMAL)
#define WDC65816_REGS_GET_INTERRUPT(regs) \
    ((regs)->p & P_INTERRUPT)
#define WDC65816_REGS_GET_CARRY(regs) \
    ((regs)->p & P_CARRY)
#define WDC65816_REGS_GET_65816_M(regs) \
    ((regs)->p & P_65816_M)
#define WDC65816_REGS_GET_65816_X(regs) \
    ((regs)->p & P_65816_X)
#define WDC65816_REGS_GET_SIGN(regs) \
    ((regs)->n & 0x80)
#define WDC65816_REGS_GET_ZERO(regs) \
    (!(regs)->z)
#define WDC65816_REGS_GET_STATUS(regs) \
    ((regs)->p | ((regs)->n & 0x80) \
     | (WDC65816_REGS_GET_ZERO(regs) ? P_ZERO : 0))

#endif
