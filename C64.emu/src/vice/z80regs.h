/*
 * z80regs.h
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

#ifndef VICE_Z80REGS_H
#define VICE_Z80REGS_H

#include "types.h"

typedef struct z80_regs_s {
    WORD reg_af;
    WORD reg_bc;
    WORD reg_de;
    WORD reg_hl;
    WORD reg_ix;
    WORD reg_iy;
    WORD reg_sp;
    WORD reg_pc;
    BYTE reg_i;
    BYTE reg_r;
    WORD reg_af2;
    WORD reg_bc2;
    WORD reg_de2;
    WORD reg_hl2;
} z80_regs_t;

#define Z80_REGS_GET_A(reg_ptr)     ((reg_ptr)->reg_af >> 8)
#define Z80_REGS_GET_FLAGS(reg_ptr) ((reg_ptr)->reg_af & 0xFF)
#define Z80_REGS_GET_AF(reg_ptr)    ((reg_ptr)->reg_af)
#define Z80_REGS_GET_B(reg_ptr)     ((reg_ptr)->reg_bc >> 8)
#define Z80_REGS_GET_C(reg_ptr)     ((reg_ptr)->reg_bc & 0xFF)
#define Z80_REGS_GET_BC(reg_ptr)    ((reg_ptr)->reg_bc)
#define Z80_REGS_GET_D(reg_ptr)     ((reg_ptr)->reg_de >> 8)
#define Z80_REGS_GET_E(reg_ptr)     ((reg_ptr)->reg_de & 0xFF)
#define Z80_REGS_GET_DE(reg_ptr)    ((reg_ptr)->reg_de)
#define Z80_REGS_GET_H(reg_ptr)     ((reg_ptr)->reg_hl >> 8)
#define Z80_REGS_GET_L(reg_ptr)     ((reg_ptr)->reg_hl & 0xFF)
#define Z80_REGS_GET_HL(reg_ptr)    ((reg_ptr)->reg_hl)
#define Z80_REGS_GET_IXH(reg_ptr)   ((reg_ptr)->reg_ix >> 8)
#define Z80_REGS_GET_IXL(reg_ptr)   ((reg_ptr)->reg_ix & 0xFF)
#define Z80_REGS_GET_IX(reg_ptr)    ((reg_ptr)->reg_ix)
#define Z80_REGS_GET_IYH(reg_ptr)   ((reg_ptr)->reg_iy >> 8)
#define Z80_REGS_GET_IYL(reg_ptr)   ((reg_ptr)->reg_iy & 0xFF)
#define Z80_REGS_GET_IY(reg_ptr)    ((reg_ptr)->reg_iy)
#define Z80_REGS_GET_SP(reg_ptr)    ((reg_ptr)->reg_sp)
#define Z80_REGS_GET_PC(reg_ptr)    ((reg_ptr)->reg_pc)
#define Z80_REGS_GET_I(reg_ptr)     ((reg_ptr)->reg_i)
#define Z80_REGS_GET_R(reg_ptr)     ((reg_ptr)->reg_r)
#define Z80_REGS_GET_AF2(reg_ptr)   ((reg_ptr)->reg_af2)
#define Z80_REGS_GET_BC2(reg_ptr)   ((reg_ptr)->reg_bc2)
#define Z80_REGS_GET_DE2(reg_ptr)   ((reg_ptr)->reg_de2)
#define Z80_REGS_GET_HL2(reg_ptr)   ((reg_ptr)->reg_hl2)

#define Z80_REGS_SET_A(reg_ptr, val)     ((reg_ptr)->reg_af = ((reg_ptr)->reg_af | 0xFF) & (val << 8))
#define Z80_REGS_SET_FLAGS(reg_ptr, val) ((reg_ptr)->reg_af = ((reg_ptr)->reg_af | 0xFF00) & (val))
#define Z80_REGS_SET_AF(reg_ptr, val)    ((reg_ptr)->reg_af = (val))
#define Z80_REGS_SET_B(reg_ptr, val)     ((reg_ptr)->reg_bc = ((reg_ptr)->reg_bc | 0xFF) & (val << 8))
#define Z80_REGS_SET_C(reg_ptr, val)     ((reg_ptr)->reg_bc = ((reg_ptr)->reg_bc | 0xFF00) & (val))
#define Z80_REGS_SET_BC(reg_ptr, val)    ((reg_ptr)->reg_bc = (val))
#define Z80_REGS_SET_D(reg_ptr, val)     ((reg_ptr)->reg_de = ((reg_ptr)->reg_de | 0xFF) & (val << 8))
#define Z80_REGS_SET_E(reg_ptr, val)     ((reg_ptr)->reg_de = ((reg_ptr)->reg_de | 0xFF00) & (val))
#define Z80_REGS_SET_DE(reg_ptr, val)    ((reg_ptr)->reg_de = (val))
#define Z80_REGS_SET_H(reg_ptr, val)     ((reg_ptr)->reg_hl = ((reg_ptr)->reg_hl | 0xFF) & (val << 8))
#define Z80_REGS_SET_L(reg_ptr, val)     ((reg_ptr)->reg_hl = ((reg_ptr)->reg_hl | 0xFF00) & (val))
#define Z80_REGS_SET_HL(reg_ptr, val)    ((reg_ptr)->reg_hl = (val))
#define Z80_REGS_SET_IXH(reg_ptr, val)   ((reg_ptr)->reg_ix = ((reg_ptr)->reg_ix | 0xFF) & (val << 8))
#define Z80_REGS_SET_IXL(reg_ptr, val)   ((reg_ptr)->reg_ix = ((reg_ptr)->reg_ix | 0xFF00) & (val))
#define Z80_REGS_SET_IX(reg_ptr, val)    ((reg_ptr)->reg_ix = (val))
#define Z80_REGS_SET_IYH(reg_ptr, val)   ((reg_ptr)->reg_iy = ((reg_ptr)->reg_iy | 0xFF) & (val << 8))
#define Z80_REGS_SET_IYL(reg_ptr, val)   ((reg_ptr)->reg_iy = ((reg_ptr)->reg_iy | 0xFF00) & (val))
#define Z80_REGS_SET_IY(reg_ptr, val)    ((reg_ptr)->reg_iy = (val))
#define Z80_REGS_SET_SP(reg_ptr, val)    ((reg_ptr)->reg_sp = (val))
#define Z80_REGS_SET_PC(reg_ptr, val)    ((reg_ptr)->reg_pc = (val))
#define Z80_REGS_SET_I(reg_ptr, val)     ((reg_ptr)->reg_i = (val))
#define Z80_REGS_SET_R(reg_ptr, val)     ((reg_ptr)->reg_r = (val))
#define Z80_REGS_SET_AF2(reg_ptr, val)   ((reg_ptr)->reg_af2 = (val))
#define Z80_REGS_SET_BC2(reg_ptr, val)   ((reg_ptr)->reg_bc2 = (val))
#define Z80_REGS_SET_DE2(reg_ptr, val)   ((reg_ptr)->reg_de2 = (val))
#define Z80_REGS_SET_HL2(reg_ptr, val)   ((reg_ptr)->reg_hl2 = (val))

#endif
