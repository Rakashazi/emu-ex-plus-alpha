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

#define Z80_REGS_GET_AF(reg_ptr) ((reg_ptr)->reg_af)
#define Z80_REGS_GET_BC(reg_ptr) ((reg_ptr)->reg_bc)
#define Z80_REGS_GET_DE(reg_ptr) ((reg_ptr)->reg_de)
#define Z80_REGS_GET_HL(reg_ptr) ((reg_ptr)->reg_hl)
#define Z80_REGS_GET_IX(reg_ptr) ((reg_ptr)->reg_ix)
#define Z80_REGS_GET_IY(reg_ptr) ((reg_ptr)->reg_iy)
#define Z80_REGS_GET_SP(reg_ptr) ((reg_ptr)->reg_sp)
#define Z80_REGS_GET_PC(reg_ptr) ((reg_ptr)->reg_pc)
#define Z80_REGS_GET_I(reg_ptr) ((reg_ptr)->reg_i)
#define Z80_REGS_GET_R(reg_ptr) ((reg_ptr)->reg_r)
#define Z80_REGS_GET_AF2(reg_ptr) ((reg_ptr)->reg_af2)
#define Z80_REGS_GET_BC2(reg_ptr) ((reg_ptr)->reg_bc2)
#define Z80_REGS_GET_DE2(reg_ptr) ((reg_ptr)->reg_de2)
#define Z80_REGS_GET_HL2(reg_ptr) ((reg_ptr)->reg_hl2)

#define Z80_REGS_SET_AF(reg_ptr, val) ((reg_ptr)->reg_af = (val))
#define Z80_REGS_SET_BC(reg_ptr, val) ((reg_ptr)->reg_bc = (val))
#define Z80_REGS_SET_DE(reg_ptr, val) ((reg_ptr)->reg_de = (val))
#define Z80_REGS_SET_HL(reg_ptr, val) ((reg_ptr)->reg_hl = (val))
#define Z80_REGS_SET_IX(reg_ptr, val) ((reg_ptr)->reg_ix = (val))
#define Z80_REGS_SET_IY(reg_ptr, val) ((reg_ptr)->reg_iy = (val))
#define Z80_REGS_SET_SP(reg_ptr, val) ((reg_ptr)->reg_sp = (val))
#define Z80_REGS_SET_PC(reg_ptr, val) ((reg_ptr)->reg_pc = (val))
#define Z80_REGS_SET_I(reg_ptr, val) ((reg_ptr)->reg_i = (val))
#define Z80_REGS_SET_R(reg_ptr, val) ((reg_ptr)->reg_r = (val))
#define Z80_REGS_SET_AF2(reg_ptr, val) ((reg_ptr)->reg_af2 = (val))
#define Z80_REGS_SET_BC2(reg_ptr, val) ((reg_ptr)->reg_bc2 = (val))
#define Z80_REGS_SET_DE2(reg_ptr, val) ((reg_ptr)->reg_de2 = (val))
#define Z80_REGS_SET_HL2(reg_ptr, val) ((reg_ptr)->reg_hl2 = (val))

#endif
