/*
 * h6809regs.h
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
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

#ifndef VICE_H6809REGS_H
#define VICE_H6809REGS_H

#include "types.h"

typedef struct h6809_regs_s {
    WORD reg_x;
    WORD reg_y;
    WORD reg_u;
    WORD reg_s;
    WORD reg_pc;
    BYTE reg_dp;
    BYTE reg_cc;
    BYTE reg_a;
    BYTE reg_b;
#if 0
    /* 6309 specific registers for future support */
    BYTE reg_e;
    BYTE reg_f;
    WORD reg_v;
    BYTE reg_md;
#endif
} h6809_regs_t;

extern h6809_regs_t h6809_regs;

#define H6809_REGS_GET_X(reg_ptr)  ((reg_ptr)->reg_x)
#define H6809_REGS_GET_Y(reg_ptr)  ((reg_ptr)->reg_y)
#define H6809_REGS_GET_U(reg_ptr)  ((reg_ptr)->reg_u)
#define H6809_REGS_GET_S(reg_ptr)  ((reg_ptr)->reg_s)
#define H6809_REGS_GET_PC(reg_ptr) ((reg_ptr)->reg_pc)
#define H6809_REGS_GET_DP(reg_ptr) ((reg_ptr)->reg_dp)
#define H6809_REGS_GET_CC(reg_ptr) ((reg_ptr)->reg_cc)
#define H6809_REGS_GET_A(reg_ptr)  ((reg_ptr)->reg_a)
#define H6809_REGS_GET_B(reg_ptr)  ((reg_ptr)->reg_b)
#define H6809_REGS_GET_D(reg_ptr)  (((reg_ptr)->reg_a << 8) | (reg_ptr)->reg_b)

#define H6809_REGS_GET_E(reg_ptr)  ((reg_ptr)->reg_e)
#define H6809_REGS_GET_F(reg_ptr)  ((reg_ptr)->reg_f)
#define H6809_REGS_GET_W(reg_ptr)  (((reg_ptr)->reg_e << 8) | (reg_ptr)->reg_f)
#define H6809_REGS_GET_Q(reg_ptr)  (((((reg_ptr)->reg_a << 24) | (reg_ptr)->reg_b << 16) | (reg_ptr)->reg_e << 8) | (reg_ptr)->reg_f)
#define H6809_REGS_GET_V(reg_ptr)  ((reg_ptr)->reg_v)
#define H6809_REGS_GET_MD(reg_ptr) ((reg_ptr)->reg_md)

#define H6809_REGS_TEST_E(reg_ptr) ((reg_ptr)->reg_cc & 0x80)
#define H6809_REGS_TEST_F(reg_ptr) ((reg_ptr)->reg_cc & 0x40)
#define H6809_REGS_TEST_H(reg_ptr) ((reg_ptr)->reg_cc & 0x20)
#define H6809_REGS_TEST_I(reg_ptr) ((reg_ptr)->reg_cc & 0x10)
#define H6809_REGS_TEST_N(reg_ptr) ((reg_ptr)->reg_cc & 0x08)
#define H6809_REGS_TEST_Z(reg_ptr) ((reg_ptr)->reg_cc & 0x04)
#define H6809_REGS_TEST_V(reg_ptr) ((reg_ptr)->reg_cc & 0x02)
#define H6809_REGS_TEST_C(reg_ptr) ((reg_ptr)->reg_cc & 0x01)

#define H6809_REGS_SET_X(reg_ptr, val)  ((reg_ptr)->reg_x = (val))
#define H6809_REGS_SET_Y(reg_ptr, val)  ((reg_ptr)->reg_y = (val))
#define H6809_REGS_SET_U(reg_ptr, val)  ((reg_ptr)->reg_u = (val))
#define H6809_REGS_SET_S(reg_ptr, val)  ((reg_ptr)->reg_s = (val))
#define H6809_REGS_SET_PC(reg_ptr, val) ((reg_ptr)->reg_pc = (val))
#define H6809_REGS_SET_DP(reg_ptr, val) ((reg_ptr)->reg_dp = (val))
#define H6809_REGS_SET_CC(reg_ptr, val) ((reg_ptr)->reg_cc = (val))
#define H6809_REGS_SET_A(reg_ptr, val)  ((reg_ptr)->reg_a = (val))
#define H6809_REGS_SET_B(reg_ptr, val)  ((reg_ptr)->reg_b = (val))

#define H6809_REGS_SET_D(reg_ptr, val)   \
    do {                                 \
        (reg_ptr)->reg_a = ((val) >> 8); \
        (reg_ptr)->reg_b = (val) & 0xFF; \
} while (0);

#define H6809_REGS_SET_E(reg_ptr, val)  ((reg_ptr)->reg_e = (val))
#define H6809_REGS_SET_F(reg_ptr, val)  ((reg_ptr)->reg_f = (val))

#define H6809_REGS_SET_W(reg_ptr, val)   \
    do {                                 \
        (reg_ptr)->reg_e = ((val) >> 8); \
        (reg_ptr)->reg_f = (val) & 0xFF; \
} while (0);

#define H6809_REGS_SET_Q(reg_ptr, val)           \
    do {                                         \
        (reg_ptr)->reg_a = ((val) >> 24);        \
        (reg_ptr)->reg_b = ((val) >> 16) & 0xFF; \
        (reg_ptr)->reg_e = ((val) >> 8) & 0xFF;  \
        (reg_ptr)->reg_f = (val) & 0xFF;         \
} while (0);

#define H6809_REGS_SET_V(reg_ptr, val)  ((reg_ptr)->reg_v = (val))
#define H6809_REGS_SET_MD(reg_ptr, val) ((reg_ptr)->reg_md = (val))

#endif
