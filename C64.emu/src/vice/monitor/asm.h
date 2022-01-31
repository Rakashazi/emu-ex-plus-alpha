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

/* CAUTION: when adding stuff to this enum, all addr_mode_size tables in all
            asmXXXX.c files must be updated accordingly */
enum asm_addr_mode {
    ASM_ADDR_MODE_IMPLIED,                  /* 6502, 6502DTV, 65816, R65C02, z80, 6809 */
    ASM_ADDR_MODE_ACCUMULATOR,              /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_IMMEDIATE,                /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ZERO_PAGE,                /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ZERO_PAGE_X,              /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ZERO_PAGE_Y,              /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ABSOLUTE,                 /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ABSOLUTE_X,               /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ABSOLUTE_Y,               /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_ABS_INDIRECT,             /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_INDIRECT_X,     /* 10 */  /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_INDIRECT_Y,               /* 6502, 6502DTV, 65816, R65C02, z80 */
    ASM_ADDR_MODE_RELATIVE,                 /* 6502, 6502DTV, 65816, R65C02, z80 */
    /* more modes needed for z80 */
    ASM_ADDR_MODE_ABSOLUTE_A,     /* 13 */  /* z80 */
    ASM_ADDR_MODE_ABSOLUTE_HL,              /* z80 */
    ASM_ADDR_MODE_ABSOLUTE_IX,              /* z80 */
    ASM_ADDR_MODE_ABSOLUTE_IY,              /* z80 */
    ASM_ADDR_MODE_Z80_ABSOLUTE_BC,          /* z80 */
    ASM_ADDR_MODE_Z80_ABSOLUTE_DE,          /* z80 */
    ASM_ADDR_MODE_Z80_ABSOLUTE_SP,          /* z80 */
    ASM_ADDR_MODE_ABS_INDIRECT_ZP,          /* z80 */
    ASM_ADDR_MODE_Z80_ABS_INDIRECT_EXT,     /* z80 */
    ASM_ADDR_MODE_IMMEDIATE_16,             /* z80, 65816 */
    ASM_ADDR_MODE_REG_B,                    /* z80 */
    ASM_ADDR_MODE_REG_C,          /* 24 */  /* z80 */
    ASM_ADDR_MODE_REG_D,                    /* z80 */
    ASM_ADDR_MODE_REG_E,                    /* z80 */
    ASM_ADDR_MODE_REG_H,                    /* z80 */
    ASM_ADDR_MODE_REG_IXH,                  /* z80 */
    ASM_ADDR_MODE_REG_IYH,                  /* z80 */
    ASM_ADDR_MODE_REG_L,                    /* z80 */
    ASM_ADDR_MODE_REG_IXL,                  /* z80 */
    ASM_ADDR_MODE_REG_IYL,                  /* z80 */
    ASM_ADDR_MODE_REG_AF,                   /* z80 */
    ASM_ADDR_MODE_REG_BC,         /* 34 */  /* z80 */
    ASM_ADDR_MODE_REG_DE,                   /* z80 */
    ASM_ADDR_MODE_REG_HL,                   /* z80 */
    ASM_ADDR_MODE_REG_IX,                   /* z80 */
    ASM_ADDR_MODE_REG_IY,                   /* z80 */
    ASM_ADDR_MODE_REG_SP,                   /* z80 */
    ASM_ADDR_MODE_REG_IND_BC,               /* z80 */
    ASM_ADDR_MODE_REG_IND_DE,               /* z80 */
    ASM_ADDR_MODE_REG_IND_HL,               /* z80 */
    ASM_ADDR_MODE_REG_IND_IX,               /* z80 */       /* B, (IX+$12) */
    ASM_ADDR_MODE_REG_IND_IY,     /* 44 */  /* z80 */       /* B, (IY+$12) */
    ASM_ADDR_MODE_REG_IND_SP,               /* z80 */
    ASM_ADDR_MODE_Z80_IND_IMMEDIATE,        /* z80 */
    ASM_ADDR_MODE_Z80_IND_REG,    /* 47 */  /* z80 */
    ASM_ADDR_MODE_IND_IX_REG,               /* z80 */       /* (IX+$12), B */
    ASM_ADDR_MODE_IND_IY_REG,               /* z80 */       /* (IY+$12), B */
    /* R65C02 */
    ASM_ADDR_MODE_INDIRECT,                 /* 65816, R65C02 */
    ASM_ADDR_MODE_ABS_INDIRECT_X,           /* 65816, R65C02 */
    ASM_ADDR_MODE_DOUBLE,
    ASM_ADDR_MODE_ZERO_PAGE_RELATIVE,       /* 65816, R65C02 */
    /* 65816 */
    ASM_ADDR_MODE_RELATIVE_LONG,            /* 65816 */
    ASM_ADDR_MODE_STACK_RELATIVE_Y,         /* 65816 */
    ASM_ADDR_MODE_STACK_RELATIVE,           /* 65816 */
    ASM_ADDR_MODE_INDIRECT_LONG,            /* 65816 */
    ASM_ADDR_MODE_ABSOLUTE_LONG,            /* 65816 */
    ASM_ADDR_MODE_INDIRECT_LONG_Y,          /* 65816 */
    ASM_ADDR_MODE_ABSOLUTE_LONG_X,          /* 65816 */
    ASM_ADDR_MODE_MOVE,                     /* 65816 */
    ASM_ADDR_MODE_ABS_IND_LONG,             /* 65816 */
    /* more modes needed for 6809 */
    ASM_ADDR_MODE_ILLEGAL,        /* 63 */                                          /* 6809 */
    ASM_ADDR_MODE_IMM_BYTE,       /* 64 looks like  ASM_ADDR_MODE_IMMEDIATE */      /* 6809 */
    ASM_ADDR_MODE_IMM_WORD,       /* 65 looks like  ASM_ADDR_MODE_IMMEDIATE_16 */   /* 6809 */
    ASM_ADDR_MODE_DIRECT,         /* 66 looks like  ASM_ADDR_MODE_ZERO_PAGE */      /* 6809 */
    ASM_ADDR_MODE_EXTENDED,       /* 67 looks like  ASM_ADDR_MODE_ABSOLUTE */       /* 6809 */
    ASM_ADDR_MODE_INDEXED,        /* 68 post-byte determines sub-mode */            /* 6809, 65816 */
    ASM_ADDR_MODE_REL_BYTE,       /* 69 */                                          /* 6809 */
    ASM_ADDR_MODE_REL_WORD,       /* 70 */                                          /* 6809 */
    ASM_ADDR_MODE_REG_POST,       /* 71 */                                          /* 6809 */
    ASM_ADDR_MODE_SYS_POST,       /* 72 */                                          /* 6809 */
    ASM_ADDR_MODE_USR_POST,       /* 73 */                                          /* 6809 */
    /* more modes needed for FULL 6809 */
    ASM_ADDR_MODE_F6809_INDEXED,  /* 74 post-byte determines sub-mode (FULL 6809) */
    /* more modes needed for 6309 */
    ASM_ADDR_MODE_IM_DIRECT,      /* 75 #$xx,<$xx */
    ASM_ADDR_MODE_IM_EXTENDED,    /* 76 #$xx,$xxxx */
    ASM_ADDR_MODE_IM_INDEXED,     /* 77 #$xx,sub-mode */
    ASM_ADDR_MODE_BITWISE,        /* 78 R,x,x,<$xx */
    ASM_ADDR_MODE_TFM_PP,         /* 79 R+,R+ */
    ASM_ADDR_MODE_TFM_MM,         /* 80 R-,R- */
    ASM_ADDR_MODE_TFM_PC,         /* 81 R+,R */
    ASM_ADDR_MODE_TFM_CP,         /* 82 R,R+ */
    ASM_ADDR_MODE_IMM_DWORD,      /* 83 #$xxxxxxxx */
    ASM_ADDR_MODE_H6309_INDEXED,  /* 84 post-byte determines sub-mode (6309 indexed) */
    ASM_ADDR_MODE_H6309_REG_POST, /* 85 (6309 post) */
    /* flag to tag undocumented opcodes, so we can ignore them in the assembler and tag them in the disassembler */
    ASM_ADDR_MODE_UNDOC = 0x1000
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
