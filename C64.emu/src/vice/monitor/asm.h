/*
 * asm.h - Assembler-related utility functions.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on older code by
 *  Vesa-Matti Puro <vmp@lut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#ifndef VICE_ASM_H
#define VICE_ASM_H

#include "types.h"

enum asm_addr_mode {
    ASM_ADDR_MODE_IMPLIED,
    ASM_ADDR_MODE_ACCUMULATOR,
    ASM_ADDR_MODE_IMMEDIATE,
    ASM_ADDR_MODE_ZERO_PAGE,
    ASM_ADDR_MODE_ZERO_PAGE_X,
    ASM_ADDR_MODE_ZERO_PAGE_Y,
    ASM_ADDR_MODE_ABSOLUTE,
    ASM_ADDR_MODE_ABSOLUTE_X,
    ASM_ADDR_MODE_ABSOLUTE_Y,
    ASM_ADDR_MODE_ABS_INDIRECT,
    ASM_ADDR_MODE_INDIRECT_X,     /* 10 */
    ASM_ADDR_MODE_INDIRECT_Y,
    ASM_ADDR_MODE_RELATIVE,
    /* more modes needed for z80 */
    ASM_ADDR_MODE_ABSOLUTE_A,     /* 13 */
    ASM_ADDR_MODE_ABSOLUTE_HL,
    ASM_ADDR_MODE_ABSOLUTE_IX,
    ASM_ADDR_MODE_ABSOLUTE_IY,
    ASM_ADDR_MODE_ABS_INDIRECT_ZP,
    ASM_ADDR_MODE_IMMEDIATE_16,
    ASM_ADDR_MODE_REG_B,
    ASM_ADDR_MODE_REG_C,          /* 20 */
    ASM_ADDR_MODE_REG_D,
    ASM_ADDR_MODE_REG_E,
    ASM_ADDR_MODE_REG_H,
    ASM_ADDR_MODE_REG_IXH,
    ASM_ADDR_MODE_REG_IYH,
    ASM_ADDR_MODE_REG_L,
    ASM_ADDR_MODE_REG_IXL,
    ASM_ADDR_MODE_REG_IYL,
    ASM_ADDR_MODE_REG_AF,
    ASM_ADDR_MODE_REG_BC,         /* 30 */
    ASM_ADDR_MODE_REG_DE,
    ASM_ADDR_MODE_REG_HL,
    ASM_ADDR_MODE_REG_IX,
    ASM_ADDR_MODE_REG_IY,
    ASM_ADDR_MODE_REG_SP,
    ASM_ADDR_MODE_REG_IND_BC,
    ASM_ADDR_MODE_REG_IND_DE,
    ASM_ADDR_MODE_REG_IND_HL,
    ASM_ADDR_MODE_REG_IND_IX,
    ASM_ADDR_MODE_REG_IND_IY,     /* 40 */
    ASM_ADDR_MODE_REG_IND_SP,
    /* R65C02 */
    ASM_ADDR_MODE_INDIRECT,
    ASM_ADDR_MODE_ABS_INDIRECT_X,
    ASM_ADDR_MODE_DOUBLE,
    ASM_ADDR_MODE_ZERO_PAGE_RELATIVE,
    /* 65816 */
    ASM_ADDR_MODE_RELATIVE_LONG,
    ASM_ADDR_MODE_STACK_RELATIVE_Y,
    ASM_ADDR_MODE_STACK_RELATIVE,
    ASM_ADDR_MODE_INDIRECT_LONG,
    ASM_ADDR_MODE_ABSOLUTE_LONG,
    ASM_ADDR_MODE_INDIRECT_LONG_Y,
    ASM_ADDR_MODE_ABSOLUTE_LONG_X,
    ASM_ADDR_MODE_MOVE,
    ASM_ADDR_MODE_ABS_IND_LONG,
    /* more modes needed for 6809 */
    ASM_ADDR_MODE_ILLEGAL,        /* 42 */
    ASM_ADDR_MODE_IMM_BYTE,       /* 43 looks like  ASM_ADDR_MODE_IMMEDIATE */
    ASM_ADDR_MODE_IMM_WORD,       /* 44 looks like  ASM_ADDR_MODE_IMMEDIATE_16 */
    ASM_ADDR_MODE_DIRECT,         /* 45 looks like  ASM_ADDR_MODE_ZERO_PAGE */
    ASM_ADDR_MODE_EXTENDED,       /* 46 looks like  ASM_ADDR_MODE_ABSOLUTE */
    ASM_ADDR_MODE_INDEXED,        /* 47 post-byte determines sub-mode */
    ASM_ADDR_MODE_REL_BYTE,       /* 48 */
    ASM_ADDR_MODE_REL_WORD,       /* 48 */
    ASM_ADDR_MODE_REG_POST,       /* 50 */
    ASM_ADDR_MODE_SYS_POST,       /* 51 */
    ASM_ADDR_MODE_USR_POST,       /* 52 */
    /* more modes needed for FULL 6809 */
    ASM_ADDR_MODE_F6809_INDEXED,  /* 53 post-byte determines sub-mode (FULL 6809) */
    /* more modes needed for 6309 */
    ASM_ADDR_MODE_IM_DIRECT,      /* 54 #$xx,<$xx */
    ASM_ADDR_MODE_IM_EXTENDED,    /* 55 #$xx,$xxxx */
    ASM_ADDR_MODE_IM_INDEXED,     /* 56 #$xx,sub-mode */
    ASM_ADDR_MODE_BITWISE,        /* 57 R,x,x,<$xx */
    ASM_ADDR_MODE_TFM_PP,         /* 58 R+,R+ */
    ASM_ADDR_MODE_TFM_MM,         /* 59 R-,R- */
    ASM_ADDR_MODE_TFM_PC,         /* 60 R+,R */
    ASM_ADDR_MODE_TFM_CP,         /* 61 R,R+ */
    ASM_ADDR_MODE_IMM_DWORD,      /* 62 #$xxxxxxxx */
    ASM_ADDR_MODE_H6309_INDEXED,  /* 63 post-byte determines sub-mode (6309 indexed) */
    ASM_ADDR_MODE_H6309_REG_POST  /* 64 (6309 post) */
};
typedef enum asm_addr_mode asm_addr_mode_t;

/*
 * 6809 sub-adressing modes for ASM_ADDR_MODE_INDEXED
 * as found in the index post-byte.
 */

#define ASM_ADDR_MODE_INDEXED_INC1                  0x00
#define ASM_ADDR_MODE_INDEXED_INC2                  0x01
#define ASM_ADDR_MODE_INDEXED_DEC1                  0x02
#define ASM_ADDR_MODE_INDEXED_DEC2                  0x03
#define ASM_ADDR_MODE_INDEXED_OFF0                  0x04
#define ASM_ADDR_MODE_INDEXED_OFFB                  0x05
#define ASM_ADDR_MODE_INDEXED_OFFA                  0x06
#define ASM_ADDR_MODE_INDEXED_07                    0x07
#define ASM_ADDR_MODE_INDEXED_F6809_OFF0            0x07
#define ASM_ADDR_MODE_INDEXED_H6309_OFFE            0x07
#define ASM_ADDR_MODE_INDEXED_OFF8                  0x08
#define ASM_ADDR_MODE_INDEXED_OFF16                 0x09
#define ASM_ADDR_MODE_INDEXED_0A                    0x0A
#define ASM_ADDR_MODE_INDEXED_F6809_OFFPCORFF       0x0A
#define ASM_ADDR_MODE_INDEXED_H6309_OFFF            0x0A
#define ASM_ADDR_MODE_INDEXED_OFFD                  0x0B
#define ASM_ADDR_MODE_INDEXED_OFFPC8                0x0C
#define ASM_ADDR_MODE_INDEXED_OFFPC16               0x0D
#define ASM_ADDR_MODE_INDEXED_0E                    0x0E
#define ASM_ADDR_MODE_INDEXED_H6309_OFFW            0x0E
#define ASM_ADDR_MODE_INDEXED_0F                    0x0F
#define ASM_ADDR_MODE_INDEXED_F6809_EXTENDED        0x0F
#define ASM_ADDR_MODE_INDEXED_H6309_OFFWREL         0x0F
#define ASM_ADDR_MODE_INDEXED_10                    0x10
#define ASM_ADDR_MODE_INDEXED_F6809_INC1_IND        0x10
#define ASM_ADDR_MODE_INDEXED_H6309_OFFWREL_IND     0x10
#define ASM_ADDR_MODE_INDEXED_INC2_IND              0x11
#define ASM_ADDR_MODE_INDEXED_12                    0x12
#define ASM_ADDR_MODE_INDEXED_F6809_DEC1_IND        0x12
#define ASM_ADDR_MODE_INDEXED_H6309_DEC1_IND        0x12
#define ASM_ADDR_MODE_INDEXED_DEC2_IND              0x13
#define ASM_ADDR_MODE_INDEXED_OFF0_IND              0x14
#define ASM_ADDR_MODE_INDEXED_OFFB_IND              0x15
#define ASM_ADDR_MODE_INDEXED_OFFA_IND              0x16
#define ASM_ADDR_MODE_INDEXED_17                    0x17
#define ASM_ADDR_MODE_INDEXED_F6809_OFF0_IND        0x17
#define ASM_ADDR_MODE_INDEXED_H6309_OFFE_IND        0x17
#define ASM_ADDR_MODE_INDEXED_OFF8_IND              0x18
#define ASM_ADDR_MODE_INDEXED_OFF16_IND             0x19
#define ASM_ADDR_MODE_INDEXED_1A                    0x1A
#define ASM_ADDR_MODE_INDEXED_F6809_OFFPCORFF_IND   0x1A
#define ASM_ADDR_MODE_INDEXED_H6309_OFFF_IND        0x1A
#define ASM_ADDR_MODE_INDEXED_OFFD_IND              0x1B
#define ASM_ADDR_MODE_INDEXED_OFFPC8_IND            0x1C
#define ASM_ADDR_MODE_INDEXED_OFFPC16_IND           0x1D
#define ASM_ADDR_MODE_INDEXED_1E                    0x1E
#define ASM_ADDR_MODE_INDEXED_H6309_OFFW_IND        0x1E
#define ASM_ADDR_MODE_EXTENDED_INDIRECT             0x1F

struct asm_opcode_info_s {
    const char *mnemonic;
    asm_addr_mode_t addr_mode;
};
typedef struct asm_opcode_info_s asm_opcode_info_t;

struct asm_mode_addr_info_s {
    asm_addr_mode_t addr_mode;
    int addr_submode;
    int param;
};
typedef struct asm_mode_addr_info_s asm_mode_addr_info_t;

struct monitor_cpu_type_s;

#endif
