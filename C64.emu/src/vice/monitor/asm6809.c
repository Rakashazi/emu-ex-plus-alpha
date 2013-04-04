/*
 * asm6809.c - 68090 Assembler-related utility functions.
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
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

#include "asm.h"
#include "mon_assemble.h"
#include "mon_register.h"
#include "montypes.h"
#include "types.h"

static const int addr_mode_size[] = {
    1, /* ASM_ADDR_MODE_IMPLIED */
    -1,/* ASM_ADDR_MODE_ACCUMULATOR */
    -1,/* ASM_ADDR_MODE_IMMEDIATE */
    -1,/* ASM_ADDR_MODE_ZERO_PAGE */
    -1,/* ASM_ADDR_MODE_ZERO_PAGE_X */
    -1,/* ASM_ADDR_MODE_ZERO_PAGE_Y */
    -1,/* ASM_ADDR_MODE_ABSOLUTE */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_X */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_Y */
    -1,/* ASM_ADDR_MODE_ABS_INDIRECT */
    -1,/* ASM_ADDR_MODE_INDIRECT_X */
    -1,/* ASM_ADDR_MODE_INDIRECT_Y */
    -1,/* ASM_ADDR_MODE_RELATIVE */
       /* more modes needed for z80 */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_A,*/
    -1,/* ASM_ADDR_MODE_ABSOLUTE_HL, */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_IX,*/
    -1,/* ASM_ADDR_MODE_ABSOLUTE_IY,*/
    -1,/* ASM_ADDR_MODE_ABS_INDIRECT_ZP, */
    -1,/* ASM_ADDR_MODE_IMMEDIATE_16, */
    -1,/* ASM_ADDR_MODE_REG_B, */
    -1,/* ASM_ADDR_MODE_REG_C, */
    -1,/* ASM_ADDR_MODE_REG_D, */
    -1,/* ASM_ADDR_MODE_REG_E, */
    -1,/* ASM_ADDR_MODE_REG_H, */
    -1,/* ASM_ADDR_MODE_REG_IXH, */
    -1,/* ASM_ADDR_MODE_REG_IYH, */
    -1,/* ASM_ADDR_MODE_REG_L, */
    -1,/* ASM_ADDR_MODE_REG_IXL, */
    -1,/* ASM_ADDR_MODE_REG_IYL, */
    -1,/* ASM_ADDR_MODE_REG_AF, */
    -1,/* ASM_ADDR_MODE_REG_BC, */
    -1,/* ASM_ADDR_MODE_REG_DE, */
    -1,/* ASM_ADDR_MODE_REG_HL, */
    -1,/* ASM_ADDR_MODE_REG_IX, */
    -1,/* ASM_ADDR_MODE_REG_IY, */
    -1,/* ASM_ADDR_MODE_REG_SP, */
    -1,/* ASM_ADDR_MODE_REG_IND_BC, */
    -1,/* ASM_ADDR_MODE_REG_IND_DE, */
    -1,/* ASM_ADDR_MODE_REG_IND_HL, */
    -1,/* ASM_ADDR_MODE_REG_IND_IX, */
    -1,/* ASM_ADDR_MODE_REG_IND_IY, */
    -1,/* ASM_ADDR_MODE_REG_IND_SP, */
       /* R65C02 */
    -1,/* ASM_ADDR_MODE_INDIRECT, */
    -1,/* ASM_ADDR_MODE_ABS_INDIRECT_X, */
    -1,/* ASM_ADDR_MODE_DOUBLE, */
    -1,/* ASM_ADDR_MODE_ZERO_PAGE_RELATIVE, */
       /* 65816 */
    -1,/* ASM_ADDR_MODE_RELATIVE_LONG */
    -1,/* ASM_ADDR_MODE_STACK_RELATIVE_Y */
    -1,/* ASM_ADDR_MODE_STACK_RELATIVE */
    -1,/* ASM_ADDR_MODE_INDIRECT_LONG */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_LONG */
    -1,/* ASM_ADDR_MODE_INDIRECT_LONG_Y */
    -1,/* ASM_ADDR_MODE_ABSOLUTE_LONG_X */
    -1,/* ASM_ADDR_MODE_MOVE */
    -1,/* ASM_ADDR_MODE_ABS_IND_LONG */
       /* more modes needed for 6809 */
    1, /* ASM_ADDR_MODE_ILLEGAL, */
    2, /* ASM_ADDR_MODE_IMM_BYTE, */
    3, /* ASM_ADDR_MODE_IMM_WORD, */
    2, /* ASM_ADDR_MODE_DIRECT, */
    3, /* ASM_ADDR_MODE_EXTENDED, */
    2, /* ASM_ADDR_MODE_INDEXED,        post-byte determines sub-mode */
    2, /* ASM_ADDR_MODE_REL_BYTE, */
    3, /* ASM_ADDR_MODE_REL_WORD, */
    2, /* ASM_ADDR_MODE_REG_POST, */
    2, /* ASM_ADDR_MODE_SYS_POST, */
    2, /* ASM_ADDR_MODE_USR_POST, */
};

static const int indexed_size[0x20] = {
    0, /* ASM_ADDR_MODE_INDEXED_INC1 */
    0, /* ASM_ADDR_MODE_INDEXED_INC2 */
    0, /* ASM_ADDR_MODE_INDEXED_DEC1 */
    0, /* ASM_ADDR_MODE_INDEXED_DEC2 */
    0, /* ASM_ADDR_MODE_INDEXED_OFF0 */
    0, /* ASM_ADDR_MODE_INDEXED_OFFB */
    0, /* ASM_ADDR_MODE_INDEXED_OFFA */
    0, /* ASM_ADDR_MODE_INDEXED_07 */
    1, /* ASM_ADDR_MODE_INDEXED_OFF8 */
    2, /* ASM_ADDR_MODE_INDEXED_OFF16 */
    0, /* ASM_ADDR_MODE_INDEXED_0A */
    0, /* ASM_ADDR_MODE_INDEXED_OFFD */
    0, /* ASM_ADDR_MODE_INDEXED_OFFPC8 */
    0, /* ASM_ADDR_MODE_INDEXED_OFFPC16 */
    0, /* ASM_ADDR_MODE_INDEXED_0E */
    0, /* ASM_ADDR_MODE_INDEXED_0F */
    0, /* ASM_ADDR_MODE_INDEXED_10 */
    0, /* ASM_ADDR_MODE_INDEXED_INC2_IND */
    0, /* ASM_ADDR_MODE_INDEXED_12 */
    0, /* ASM_ADDR_MODE_INDEXED_DEC2_IND */
    0, /* ASM_ADDR_MODE_INDEXED_OFF0_IND */
    0, /* ASM_ADDR_MODE_INDEXED_OFFB_IND */
    0, /* ASM_ADDR_MODE_INDEXED_OFFA_IND */
    0, /* ASM_ADDR_MODE_INDEXED_17 */
    1, /* ASM_ADDR_MODE_INDEXED_OFF8_IND */
    2, /* ASM_ADDR_MODE_INDEXED_OFF16_IND  */
    0, /* ASM_ADDR_MODE_INDEXED_1A */
    0, /* ASM_ADDR_MODE_INDEXED_OFFD_IND */
    1, /* ASM_ADDR_MODE_INDEXED_OFFPC8_IND */
    2, /* ASM_ADDR_MODE_INDEXED_OFFPC16_IND */
    0, /* ASM_ADDR_MODE_INDEXED_1E */
    2, /* ASM_ADDR_MODE_INDEXED_16_IND */
};

static const asm_opcode_info_t opcode_list[256] = {
    /* 00 */ { "NEG",    ASM_ADDR_MODE_DIRECT },
    /* 01 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NEG direct (6809), OIM direct (6309) */
    /* 02 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM direct : NEG direct (6809), AIM direct (6309) */
    /* 03 */ { "COM",    ASM_ADDR_MODE_DIRECT },
    /* 04 */ { "LSR",    ASM_ADDR_MODE_DIRECT },
    /* 05 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* LSR direct (6809), EIM direct (6309) */
    /* 06 */ { "ROR",    ASM_ADDR_MODE_DIRECT },
    /* 07 */ { "ASR",    ASM_ADDR_MODE_DIRECT },
    /* 08 */ { "ASL",    ASM_ADDR_MODE_DIRECT },
    /* 09 */ { "ROL",    ASM_ADDR_MODE_DIRECT },
    /* 0a */ { "DEC",    ASM_ADDR_MODE_DIRECT },
    /* 0b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* DEC direct (6809), TIM direct (6309) */
    /* 0c */ { "INC",    ASM_ADDR_MODE_DIRECT },
    /* 0d */ { "TST",    ASM_ADDR_MODE_DIRECT },
    /* 0e */ { "JMP",    ASM_ADDR_MODE_DIRECT },
    /* 0f */ { "CLR",    ASM_ADDR_MODE_DIRECT },
    /* 10 */ { "PREFIX", ASM_ADDR_MODE_ILLEGAL },
    /* 11 */ { "PREFIX", ASM_ADDR_MODE_ILLEGAL },
    /* 12 */ { "NOP",    ASM_ADDR_MODE_IMPLIED },
    /* 13 */ { "SYNC",   ASM_ADDR_MODE_IMPLIED },
    /* 14 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), SEXW (6309) */
    /* 15 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* 16 */ { "LBRA",   ASM_ADDR_MODE_REL_WORD },
    /* 17 */ { "LBSR",   ASM_ADDR_MODE_REL_WORD },
    /* 18 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* CCRS (6809), illegal trap (6309) */
    /* 19 */ { "DAA",    ASM_ADDR_MODE_IMPLIED },
    /* 1a */ { "ORCC",   ASM_ADDR_MODE_IMM_BYTE },
    /* 1b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NOP (6809), illegal trap (6309) */
    /* 1c */ { "ANDCC",  ASM_ADDR_MODE_IMM_BYTE },
    /* 1d */ { "SEX",    ASM_ADDR_MODE_IMPLIED },
    /* 1e */ { "EXG",    ASM_ADDR_MODE_REG_POST },
    /* 1f */ { "TFR",    ASM_ADDR_MODE_REG_POST },
    /* 20 */ { "BRA",    ASM_ADDR_MODE_REL_BYTE },
    /* 21 */ { "BRN",    ASM_ADDR_MODE_REL_BYTE },
    /* 22 */ { "BHI",    ASM_ADDR_MODE_REL_BYTE },
    /* 23 */ { "BLS",    ASM_ADDR_MODE_REL_BYTE },
    /* 24 */ { "BCC",    ASM_ADDR_MODE_REL_BYTE },
    /* 25 */ { "BCS",    ASM_ADDR_MODE_REL_BYTE },
    /* 26 */ { "BNE",    ASM_ADDR_MODE_REL_BYTE },
    /* 27 */ { "BEQ",    ASM_ADDR_MODE_REL_BYTE },
    /* 28 */ { "BVC",    ASM_ADDR_MODE_REL_BYTE },
    /* 29 */ { "BVS",    ASM_ADDR_MODE_REL_BYTE },
    /* 2a */ { "BPL",    ASM_ADDR_MODE_REL_BYTE },
    /* 2b */ { "BMI",    ASM_ADDR_MODE_REL_BYTE },
    /* 2c */ { "BGE",    ASM_ADDR_MODE_REL_BYTE },
    /* 2d */ { "BLT",    ASM_ADDR_MODE_REL_BYTE },
    /* 2e */ { "BGT",    ASM_ADDR_MODE_REL_BYTE },
    /* 2f */ { "BLE",    ASM_ADDR_MODE_REL_BYTE },
    /* 30 */ { "LEAX",   ASM_ADDR_MODE_INDEXED },
    /* 31 */ { "LEAY",   ASM_ADDR_MODE_INDEXED },
    /* 32 */ { "LEAS",   ASM_ADDR_MODE_INDEXED },
    /* 33 */ { "LEAU",   ASM_ADDR_MODE_INDEXED },
    /* 34 */ { "PSHS",   ASM_ADDR_MODE_SYS_POST },
    /* 35 */ { "PULS",   ASM_ADDR_MODE_SYS_POST },
    /* 36 */ { "PSHU",   ASM_ADDR_MODE_USR_POST },
    /* 37 */ { "PULU",   ASM_ADDR_MODE_USR_POST },
    /* 38 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* ANDCC immediate +1 cycle (6809), illegal trap (6309) */
    /* 39 */ { "RTS",    ASM_ADDR_MODE_IMPLIED },
    /* 3a */ { "ABX",    ASM_ADDR_MODE_IMPLIED },
    /* 3b */ { "RTI",    ASM_ADDR_MODE_IMPLIED },
    /* 3c */ { "CWAI",   ASM_ADDR_MODE_IMM_BYTE },
    /* 3d */ { "MUL",    ASM_ADDR_MODE_IMPLIED },
    /* 3e */ { "RESET",  ASM_ADDR_MODE_IMPLIED },
    /* 3f */ { "SWI",    ASM_ADDR_MODE_IMPLIED },
    /* 40 */ { "NEGA",   ASM_ADDR_MODE_IMPLIED },
    /* 41 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NEGA (6809), illegal trap (6309) */
    /* 42 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMA : NEGA (6809), illegal trap (6309) */
    /* 43 */ { "COMA",   ASM_ADDR_MODE_IMPLIED },
    /* 44 */ { "LSRA",   ASM_ADDR_MODE_IMPLIED },
    /* 45 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* LSRA (6809), illegal trap (6309) */
    /* 46 */ { "RORA",   ASM_ADDR_MODE_IMPLIED },
    /* 47 */ { "ASRA",   ASM_ADDR_MODE_IMPLIED },
    /* 48 */ { "ASLA",   ASM_ADDR_MODE_IMPLIED },
    /* 49 */ { "ROLA",   ASM_ADDR_MODE_IMPLIED },
    /* 4a */ { "DECA",   ASM_ADDR_MODE_IMPLIED },
    /* 4b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* DECA (6809), illegal trap (6309) */
    /* 4c */ { "INCA",   ASM_ADDR_MODE_IMPLIED },
    /* 4d */ { "TSTA",   ASM_ADDR_MODE_IMPLIED },
    /* 4e */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* CLRA (6809), illegal trap (6309) */
    /* 4f */ { "CLRA",   ASM_ADDR_MODE_IMPLIED },
    /* 50 */ { "NEGB",   ASM_ADDR_MODE_IMPLIED },
    /* 51 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NEGB (6809), illegal trap (6309) */
    /* 52 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMB : NEGB (6809), illegal trap (6309) */
    /* 53 */ { "COMB",   ASM_ADDR_MODE_IMPLIED },
    /* 54 */ { "LSRB",   ASM_ADDR_MODE_IMPLIED },
    /* 55 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* LSRB (6809), illegal trap (6309) */
    /* 56 */ { "RORB",   ASM_ADDR_MODE_IMPLIED },
    /* 57 */ { "ASRB",   ASM_ADDR_MODE_IMPLIED },
    /* 58 */ { "ASLB",   ASM_ADDR_MODE_IMPLIED },
    /* 59 */ { "ROLB",   ASM_ADDR_MODE_IMPLIED },
    /* 5a */ { "DECB",   ASM_ADDR_MODE_IMPLIED },
    /* 5b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* DECB (6809), illegal trap (6309) */
    /* 5c */ { "INCB",   ASM_ADDR_MODE_IMPLIED },
    /* 5d */ { "TSTB",   ASM_ADDR_MODE_IMPLIED },
    /* 5e */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* CLRB (6809), illegal trap (6309) */
    /* 5f */ { "CLRB",   ASM_ADDR_MODE_IMPLIED },
    /* 60 */ { "NEG",    ASM_ADDR_MODE_INDEXED },
    /* 61 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NEG indexed (6809), OIM indexed (6309) */
    /* 62 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM indexed : NEG indexed (6809), AIM indexed (6309) */
    /* 63 */ { "COM",    ASM_ADDR_MODE_INDEXED },
    /* 64 */ { "LSR",    ASM_ADDR_MODE_INDEXED },
    /* 65 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* LSR indexed (6809), EIM indexed (6309) */
    /* 66 */ { "ROR",    ASM_ADDR_MODE_INDEXED },
    /* 67 */ { "ASR",    ASM_ADDR_MODE_INDEXED },
    /* 68 */ { "ASL",    ASM_ADDR_MODE_INDEXED },
    /* 69 */ { "ROL",    ASM_ADDR_MODE_INDEXED },
    /* 6a */ { "DEC",    ASM_ADDR_MODE_INDEXED },
    /* 6b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* DEC indexed (6809), TIM indexed (6309) */
    /* 6c */ { "INC",    ASM_ADDR_MODE_INDEXED },
    /* 6d */ { "TST",    ASM_ADDR_MODE_INDEXED },
    /* 6e */ { "JMP",    ASM_ADDR_MODE_INDEXED },
    /* 6f */ { "CLR",    ASM_ADDR_MODE_INDEXED },
    /* 70 */ { "NEG",    ASM_ADDR_MODE_EXTENDED },
    /* 71 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* NEG extended (6809), OIM extended (6309) */
    /* 72 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM extended : NEG extended (6809), AIM extended (6309) */
    /* 73 */ { "COM",    ASM_ADDR_MODE_EXTENDED },
    /* 74 */ { "LSR",    ASM_ADDR_MODE_EXTENDED },
    /* 75 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* LSR extended (6809), EIM extended (6309) */
    /* 76 */ { "ROR",    ASM_ADDR_MODE_EXTENDED },
    /* 77 */ { "ASR",    ASM_ADDR_MODE_EXTENDED },
    /* 78 */ { "ASL",    ASM_ADDR_MODE_EXTENDED },
    /* 79 */ { "ROL",    ASM_ADDR_MODE_EXTENDED },
    /* 7a */ { "DEC",    ASM_ADDR_MODE_EXTENDED },
    /* 7b */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* DEC extended (6809), TIM extended (6309) */
    /* 7c */ { "INC",    ASM_ADDR_MODE_EXTENDED },
    /* 7d */ { "TST",    ASM_ADDR_MODE_EXTENDED },
    /* 7e */ { "JMP",    ASM_ADDR_MODE_EXTENDED },
    /* 7f */ { "CLR",    ASM_ADDR_MODE_EXTENDED },
    /* 80 */ { "SUBA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 81 */ { "CMPA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 82 */ { "SBCA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 83 */ { "SUBD",   ASM_ADDR_MODE_IMM_WORD },
    /* 84 */ { "ANDA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 85 */ { "BITA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 86 */ { "LDA",    ASM_ADDR_MODE_IMM_BYTE },
    /* 87 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* 88 */ { "EORA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 89 */ { "ADCA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 8a */ { "ORA",    ASM_ADDR_MODE_IMM_BYTE },
    /* 8b */ { "ADDA",   ASM_ADDR_MODE_IMM_BYTE },
    /* 8c */ { "CMPX",   ASM_ADDR_MODE_IMM_WORD },
    /* 8d */ { "BSR",    ASM_ADDR_MODE_REL_BYTE },
    /* 8e */ { "LDX",    ASM_ADDR_MODE_IMM_WORD },
    /* 8f */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* STX immediate (6809), illegal trap (6309) */
    /* 90 */ { "SUBA",   ASM_ADDR_MODE_DIRECT },
    /* 91 */ { "CMPA",   ASM_ADDR_MODE_DIRECT },
    /* 92 */ { "SBCA",   ASM_ADDR_MODE_DIRECT },
    /* 93 */ { "SUBD",   ASM_ADDR_MODE_DIRECT },
    /* 94 */ { "ANDA",   ASM_ADDR_MODE_DIRECT },
    /* 95 */ { "BITA",   ASM_ADDR_MODE_DIRECT },
    /* 96 */ { "LDA",    ASM_ADDR_MODE_DIRECT },
    /* 97 */ { "STA",    ASM_ADDR_MODE_DIRECT },
    /* 98 */ { "EORA",   ASM_ADDR_MODE_DIRECT },
    /* 99 */ { "ADCA",   ASM_ADDR_MODE_DIRECT },
    /* 9a */ { "ORA",    ASM_ADDR_MODE_DIRECT },
    /* 9b */ { "ADDA",   ASM_ADDR_MODE_DIRECT },
    /* 9c */ { "CMPX",   ASM_ADDR_MODE_DIRECT },
    /* 9d */ { "JSR",    ASM_ADDR_MODE_DIRECT },
    /* 9e */ { "LDX",    ASM_ADDR_MODE_DIRECT },
    /* 9f */ { "STX",    ASM_ADDR_MODE_DIRECT },
    /* a0 */ { "SUBA",   ASM_ADDR_MODE_INDEXED },
    /* a1 */ { "CMPA",   ASM_ADDR_MODE_INDEXED },
    /* a2 */ { "SBCA",   ASM_ADDR_MODE_INDEXED },
    /* a3 */ { "SUBD",   ASM_ADDR_MODE_INDEXED },
    /* a4 */ { "ANDA",   ASM_ADDR_MODE_INDEXED },
    /* a5 */ { "BITA",   ASM_ADDR_MODE_INDEXED },
    /* a6 */ { "LDA",    ASM_ADDR_MODE_INDEXED },
    /* a7 */ { "STA",    ASM_ADDR_MODE_INDEXED },
    /* a8 */ { "EORA",   ASM_ADDR_MODE_INDEXED },
    /* a9 */ { "ADCA",   ASM_ADDR_MODE_INDEXED },
    /* aa */ { "ORA",    ASM_ADDR_MODE_INDEXED },
    /* ab */ { "ADDA",   ASM_ADDR_MODE_INDEXED },
    /* ac */ { "CMPX",   ASM_ADDR_MODE_INDEXED },
    /* ad */ { "JSR",    ASM_ADDR_MODE_INDEXED },
    /* ae */ { "LDX",    ASM_ADDR_MODE_INDEXED },
    /* af */ { "STX",    ASM_ADDR_MODE_INDEXED },
    /* b0 */ { "SUBA",   ASM_ADDR_MODE_EXTENDED },
    /* b1 */ { "CMPA",   ASM_ADDR_MODE_EXTENDED },
    /* b2 */ { "SBCA",   ASM_ADDR_MODE_EXTENDED },
    /* b3 */ { "SUBD",   ASM_ADDR_MODE_EXTENDED },
    /* b4 */ { "ANDA",   ASM_ADDR_MODE_EXTENDED },
    /* b5 */ { "BITA",   ASM_ADDR_MODE_EXTENDED },
    /* b6 */ { "LDA",    ASM_ADDR_MODE_EXTENDED },
    /* b7 */ { "STA",    ASM_ADDR_MODE_EXTENDED },
    /* b8 */ { "EORA",   ASM_ADDR_MODE_EXTENDED },
    /* b9 */ { "ADCA",   ASM_ADDR_MODE_EXTENDED },
    /* ba */ { "ORA",    ASM_ADDR_MODE_EXTENDED },
    /* bb */ { "ADDA",   ASM_ADDR_MODE_EXTENDED },
    /* bc */ { "CMPX",   ASM_ADDR_MODE_EXTENDED },
    /* bd */ { "JSR",    ASM_ADDR_MODE_EXTENDED },
    /* be */ { "LDX",    ASM_ADDR_MODE_EXTENDED },
    /* bf */ { "STX",    ASM_ADDR_MODE_EXTENDED },
    /* c0 */ { "SUBB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c1 */ { "CMPB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c2 */ { "SBCB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c3 */ { "ADDD",   ASM_ADDR_MODE_IMM_WORD },
    /* c4 */ { "ANDB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c5 */ { "BITB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c6 */ { "LDB",    ASM_ADDR_MODE_IMM_BYTE },
    /* c7 */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* c8 */ { "EORB",   ASM_ADDR_MODE_IMM_BYTE },
    /* c9 */ { "ADCB",   ASM_ADDR_MODE_IMM_BYTE },
    /* ca */ { "ORB",    ASM_ADDR_MODE_IMM_BYTE },
    /* cb */ { "ADDB",   ASM_ADDR_MODE_IMM_BYTE },
    /* cc */ { "LDD",    ASM_ADDR_MODE_IMM_WORD },
    /* cd */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), LDQ immediate (6309) */
    /* ce */ { "LDU",    ASM_ADDR_MODE_IMM_WORD },
    /* cf */ { "UNDOC",  ASM_ADDR_MODE_ILLEGAL },        /* STU immediate (6809), illegal trap (6309) */
    /* d0 */ { "SUBB",   ASM_ADDR_MODE_DIRECT },
    /* d1 */ { "CMPB",   ASM_ADDR_MODE_DIRECT },
    /* d2 */ { "SBCB",   ASM_ADDR_MODE_DIRECT },
    /* d3 */ { "ADDD",   ASM_ADDR_MODE_DIRECT },
    /* d4 */ { "ANDB",   ASM_ADDR_MODE_DIRECT },
    /* d5 */ { "BITB",   ASM_ADDR_MODE_DIRECT },
    /* d6 */ { "LDB",    ASM_ADDR_MODE_DIRECT },
    /* d7 */ { "STB",    ASM_ADDR_MODE_DIRECT },
    /* d8 */ { "EORB",   ASM_ADDR_MODE_DIRECT },
    /* d9 */ { "ADCB",   ASM_ADDR_MODE_DIRECT },
    /* da */ { "ORB",    ASM_ADDR_MODE_DIRECT },
    /* db */ { "ADDB",   ASM_ADDR_MODE_DIRECT },
    /* dc */ { "LDD",    ASM_ADDR_MODE_DIRECT },
    /* dd */ { "STD",    ASM_ADDR_MODE_DIRECT },
    /* de */ { "LDU",    ASM_ADDR_MODE_DIRECT },
    /* df */ { "STU",    ASM_ADDR_MODE_DIRECT },
    /* e0 */ { "SUBB",   ASM_ADDR_MODE_INDEXED },
    /* e1 */ { "CMPB",   ASM_ADDR_MODE_INDEXED },
    /* e2 */ { "SBCB",   ASM_ADDR_MODE_INDEXED },
    /* e3 */ { "ADDD",   ASM_ADDR_MODE_INDEXED },
    /* e4 */ { "ANDB",   ASM_ADDR_MODE_INDEXED },
    /* e5 */ { "BITB",   ASM_ADDR_MODE_INDEXED },
    /* e6 */ { "LDB",    ASM_ADDR_MODE_INDEXED },
    /* e7 */ { "STB",    ASM_ADDR_MODE_INDEXED },
    /* e8 */ { "EORB",   ASM_ADDR_MODE_INDEXED },
    /* e9 */ { "ADCB",   ASM_ADDR_MODE_INDEXED },
    /* ea */ { "ORB",    ASM_ADDR_MODE_INDEXED },
    /* eb */ { "ADDB",   ASM_ADDR_MODE_INDEXED },
    /* ec */ { "LDD",    ASM_ADDR_MODE_INDEXED },
    /* ed */ { "STD",    ASM_ADDR_MODE_INDEXED },
    /* ee */ { "LDU",    ASM_ADDR_MODE_INDEXED },
    /* ef */ { "STU",    ASM_ADDR_MODE_INDEXED },
    /* f0 */ { "SUBB",   ASM_ADDR_MODE_EXTENDED },
    /* f1 */ { "CMPB",   ASM_ADDR_MODE_EXTENDED },
    /* f2 */ { "SBCB",   ASM_ADDR_MODE_EXTENDED },
    /* f3 */ { "ADDD",   ASM_ADDR_MODE_EXTENDED },
    /* f4 */ { "ANDB",   ASM_ADDR_MODE_EXTENDED },
    /* f5 */ { "BITB",   ASM_ADDR_MODE_EXTENDED },
    /* f6 */ { "LDB",    ASM_ADDR_MODE_EXTENDED },
    /* f7 */ { "STB",    ASM_ADDR_MODE_EXTENDED },
    /* f8 */ { "EORB",   ASM_ADDR_MODE_EXTENDED },
    /* f9 */ { "ADCB",   ASM_ADDR_MODE_EXTENDED },
    /* fa */ { "ORB",    ASM_ADDR_MODE_EXTENDED },
    /* fb */ { "ADDB",   ASM_ADDR_MODE_EXTENDED },
    /* fc */ { "LDD",    ASM_ADDR_MODE_EXTENDED },
    /* fd */ { "STD",    ASM_ADDR_MODE_EXTENDED },
    /* fe */ { "LDU",    ASM_ADDR_MODE_EXTENDED },
    /* ff */ { "STU",    ASM_ADDR_MODE_EXTENDED }
};

static const asm_opcode_info_t opcode_list_10[256] = {
    /* 00 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG direct (6809), illegal trap (6309) */
    /* 01 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG direct (6809), illegal trap (6309) */
    /* 02 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM direct : NEG direct, illegal trap (6309) */
    /* 03 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM direct (6809), illegal trap (6309) */
    /* 04 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR direct (6809), illegal trap (6309) */
    /* 05 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR direct (6809), illegal trap (6309) */
    /* 06 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR direct (6809), illegal trap (6309) */
    /* 07 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR direct (6809), illegal trap (6309) */
    /* 08 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL/LSL direct (6809), illegal trap (6309) */
    /* 09 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL direct (6809), illegal trap (6309) */
    /* 0a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC direct (6809), illegal trap (6309) */
    /* 0b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC direct (6809), illegal trap (6309) */
    /* 0c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC direct (6809), illegal trap (6309) */
    /* 0d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST direct (6809), illegal trap (6309) */
    /* 0e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP direct (6809), illegal trap (6309) */
    /* 0f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR direct (6809), illegal trap (6309) */
    /* 10 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ignore byte and read next (6809/6309) */
    /* 11 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ignore byte and read next (6809/6309) */
    /* 12 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NOP (6809), illegal trap (6309) */
    /* 13 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SYNC (6809), illegal trap (6309) */
    /* 14 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* 15 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* 16 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LBRA offset (6809), illegal trap (6309) */
    /* 17 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LBSR offset (6809), illegal trap (6309) */
    /* 18 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CCRS (6809), illegal trap (6309) */
    /* 19 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DAA (6809), illegal trap (6309) */
    /* 1a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORCC immediate (6809), illegal trap (6309) */
    /* 1b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NOP (6809), illegal trap (6309) */
    /* 1c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDCC immediate (6809), illegal trap (6309) */
    /* 1d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SEX (6809), illegal trap (6309) */
    /* 1e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EXG regs (6809), illegal trap (6309) */
    /* 1f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TFR regs (6809), illegal trap (6309) */
    /* 20 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LBRA offset (6809), illegal trap (6309) */
    /* 21 */ { "LBRN",  ASM_ADDR_MODE_REL_WORD },
    /* 22 */ { "LBHI",  ASM_ADDR_MODE_REL_WORD },
    /* 23 */ { "LBLS",  ASM_ADDR_MODE_REL_WORD },
    /* 24 */ { "LBCC",  ASM_ADDR_MODE_REL_WORD },
    /* 25 */ { "LBCS",  ASM_ADDR_MODE_REL_WORD },
    /* 26 */ { "LBNE",  ASM_ADDR_MODE_REL_WORD },
    /* 27 */ { "LBEQ",  ASM_ADDR_MODE_REL_WORD },
    /* 28 */ { "LBVC",  ASM_ADDR_MODE_REL_WORD },
    /* 29 */ { "LBVS",  ASM_ADDR_MODE_REL_WORD },
    /* 2a */ { "LBPL",  ASM_ADDR_MODE_REL_WORD },
    /* 2b */ { "LBMI",  ASM_ADDR_MODE_REL_WORD },
    /* 2c */ { "LBGE",  ASM_ADDR_MODE_REL_WORD },
    /* 2d */ { "LBLT",  ASM_ADDR_MODE_REL_WORD },
    /* 2e */ { "LBGT",  ASM_ADDR_MODE_REL_WORD },
    /* 2f */ { "LBLE",  ASM_ADDR_MODE_REL_WORD },
    /* 30 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAX indexed (6809), ADDR regs (6309) */
    /* 31 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAY indexed (6809), ADCR regs (6309) */
    /* 32 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAS indexed (6809), SUBR regs (6309) */
    /* 33 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAU indexed (6809), SBCR regs (6309) */
    /* 34 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PSHS post (6809), ANDR regs (6309) */
    /* 35 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PULS post (6809), ORR regs (6309) */
    /* 36 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PSHU post (6809), EORR regs (6309) */
    /* 37 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PULU post (6809), CMPR regs (6309) */
    /* 38 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDCC immediate + 1 cycle (6809), PSHSW (6309) */
    /* 39 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RTS (6809), PULSW (6309) */
    /* 3a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ABX (6809), PSHUW (6309) */
    /* 3b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RTI (6809), PULUW (6309) */
    /* 3c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CWAI immediate (6809), illegal trap (6309) */
    /* 3d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* MUL (6809), illegal trap (6309) */
    /* 3e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SWIRES (6809), illegal trap (6309) */
    /* 3f */ { "SWI2",  ASM_ADDR_MODE_IMPLIED },
    /* 40 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGA (6809), NEGD (6309) */
    /* 41 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGA (6809), illegal trap (6309) */
    /* 42 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMA : NEGA (6809), illegal trap (6309) */
    /* 43 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COMA (6809), COMD (6309) */
    /* 44 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRA (6809), LSRD (6309) */
    /* 45 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRA (6809), illegal trap (6309) */
    /* 46 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RORA (6809), RORD (6309) */
    /* 47 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASRA (6809), ASRD (6309) */
    /* 48 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASLA/LSLA (6809), ASLD (6309) */
    /* 49 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROLA (6809), ROLD (6309) */
    /* 4a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECA (6809), DECD (6309) */
    /* 4b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECA (6809), illegal trap (6309) */
    /* 4c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INCA (6809), INCD (6309) */
    /* 4d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TSTA (6809), TSTD (6309) */
    /* 4e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRA (6809), illegal trap (6309) */
    /* 4f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRA (6809), CLRD (6309) */
    /* 50 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGB (6809), illegal trap (6309) */
    /* 51 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGB (6809), illegal trap (6309) */
    /* 52 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMB : NEGB (6809), illegal trap (6309) */
    /* 53 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COMB (6809), COMW (6309) */
    /* 54 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRB (6809), LSRW (6309) */
    /* 55 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRB (6809), illegal trap (6309) */
    /* 56 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RORB (6809), RORW (6309) */
    /* 57 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASRB (6809), illegal trap (6309) */
    /* 58 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASLB/LSLB (6809), illegal trap (6309) */
    /* 59 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROLB (6809), ROLW (6309) */
    /* 5a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECB (6809), DECW (6309) */
    /* 5b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECB (6809), illegal trap (6309) */
    /* 5c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INCB (6809), INCW (6309) */
    /* 5d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TSTB (6809), TSTW (6309) */
    /* 5e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRB (6809), illegal trap (6309) */
    /* 5f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRB (6809), CLRW (6309) */
    /* 60 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG indexed (6809), illegal trap (6309) */
    /* 61 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG indexed (6809), illegal trap (6309) */
    /* 62 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM indexed : NEG indexed (6809), illegal trap (6309) */
    /* 63 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM indexed (6809), illegal trap (6309) */
    /* 64 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR indexed (6809), illegal trap (6309) */
    /* 65 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR indexed (6809), illegal trap (6309) */
    /* 66 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR indexed (6809), illegal trap (6309) */
    /* 67 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR indexed (6809), illegal trap (6309) */
    /* 68 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL/LSL indexed (6809), illegal trap (6309) */
    /* 69 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL indexed (6809), illegal trap (6309) */
    /* 6a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC indexed (6809), illegal trap (6309) */
    /* 6b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC indexed (6809), illegal trap (6309) */
    /* 6c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC indexed (6809), illegal trap (6309) */
    /* 6d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST indexed (6809), illegal trap (6309) */
    /* 6e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP indexed (6809), illegal trap (6309) */
    /* 6f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR indexed (6809), illegal trap (6309) */
    /* 70 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG extended (6809), illegal trap (6309) */
    /* 71 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG extended (6809), illegal trap (6309) */
    /* 72 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM extended : NEG extended (6809), illegal trap (6309) */
    /* 73 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM extended (6809), illegal trap (6309) */
    /* 74 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR extended (6809), illegal trap (6309) */
    /* 75 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR extended (6809), illegal trap (6309) */
    /* 76 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR extended (6809), illegal trap (6309) */
    /* 77 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR extended (6809), illegal trap (6309) */
    /* 78 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL extended (6809), illegal trap (6309) */
    /* 79 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL extended (6809), illegal trap (6309) */
    /* 7a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC extended (6809), illegal trap (6309) */
    /* 7b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC extended (6809), illegal trap (6309) */
    /* 7c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC extended (6809), illegal trap (6309) */
    /* 7d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST extended (6809), illegal trap (6309) */
    /* 7e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP extended (6809), illegal trap (6309) */
    /* 7f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR extended (6809), illegal trap (6309) */
    /* 80 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA immediate (6809), SUBW immediate (6309) */
    /* 81 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA immediate (6809), CMPW immediate (6309) */
    /* 82 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA immediate (6809), SBCD immediate (6309) */
    /* 83 */ { "CMPD",  ASM_ADDR_MODE_IMM_WORD },
    /* 84 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA immediate (6809), ANDD immediate (6309) */
    /* 85 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA immediate (6809), BITD immediate (6309) */
    /* 86 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA immediate (6809), LDW immediate (6309) */
    /* 87 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* 88 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA immediate (6809), EORD immediate (6309) */
    /* 89 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA immediate (6809), ADCD immediate (6309) */
    /* 8a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA immediate (6809), ORD immediate (6309) */
    /* 8b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA immediate (6809), ADDW immediate (6309) */
    /* 8c */ { "CMPY",  ASM_ADDR_MODE_IMM_WORD },
    /* 8d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BSR offset (6809), illegal trap (6309) */
    /* 8e */ { "LDY",   ASM_ADDR_MODE_IMM_WORD },
    /* 8f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STX immediate (6809), illegal trap (6309) */
    /* 90 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA direct (6809), SUBW direct (6309) */
    /* 91 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA direct (6809), CMPW direct (6309) */
    /* 92 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA direct (6809), SBCD direct (6309) */
    /* 93 */ { "CMPD",  ASM_ADDR_MODE_DIRECT },
    /* 94 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA direct (6809), ANDD direct (6309) */
    /* 95 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA direct (6809), BITD direct (6309) */
    /* 96 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA direct (6809), LDW direct (6309) */
    /* 97 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA direct (6809), STW direct (6309) */
    /* 98 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA direct (6809), EORD direct (6309) */
    /* 99 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA direct (6809), ADCD direct (6309) */
    /* 9a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA direct (6809), ORD direct (6309) */
    /* 9b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA direct (6809), ADDW direct (6309) */
    /* 9c */ { "CMPY",  ASM_ADDR_MODE_DIRECT },
    /* 9d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR direct (6809), illegal trap (6309) */
    /* 9e */ { "LDY",   ASM_ADDR_MODE_DIRECT },
    /* 9f */ { "STY",   ASM_ADDR_MODE_DIRECT },
    /* a0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA indexed (6809), SUBW indexed (6309) */
    /* a1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA indexed (6809), CMPW indexed (6309) */
    /* a2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA indexed (6809), SBCD indexed (6309) */
    /* a3 */ { "CMPD",  ASM_ADDR_MODE_INDEXED },
    /* a4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA indexed (6809), ANDD indexed (6309) */
    /* a5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA indexed (6809), BITD indexed (6309) */
    /* a6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA indexed (6809), LDW indexed (6309) */
    /* a7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA indexed (6809), STW indexed (6309) */
    /* a8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA indexed (6809), EORD indexed (6309) */
    /* a9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA indexed (6809), ADCD indexed (6309) */
    /* aa */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA indexed (6809), ORD indexed (6309) */
    /* ab */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA indexed (6809), ADDW indexed (6309) */
    /* ac */ { "CMPY",  ASM_ADDR_MODE_INDEXED },
    /* ad */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR indexed (6809), illegal trap (6309) */
    /* ae */ { "LDY",   ASM_ADDR_MODE_INDEXED },
    /* af */ { "STY",   ASM_ADDR_MODE_INDEXED },
    /* b0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA extended (6809), SUBW extended (6309) */
    /* b1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA extended (6809), CMPW extended (6309) */
    /* b2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA extended (6809), SBCD extended (6309) */
    /* b3 */ { "CMPD",  ASM_ADDR_MODE_EXTENDED },
    /* b4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA extended (6809), ANDD extended (6309) */
    /* b5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA extended (6809), BITD extended (6309) */
    /* b6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA extended (6809), LDW extended (6309) */
    /* b7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA extended (6809), STW extended (6309) */
    /* b8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA extended (6809), EORD extended (6309) */
    /* b9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA extended (6809), ADCD extended (6309) */
    /* ba */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA extended (6809), ORD extended (6309) */
    /* bb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA extended (6809), ADDW extended (6309) */
    /* bc */ { "CMPY",  ASM_ADDR_MODE_EXTENDED },
    /* bd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR extended (6809), illegal trap (6309) */
    /* be */ { "LDY",   ASM_ADDR_MODE_EXTENDED },
    /* bf */ { "STY",   ASM_ADDR_MODE_EXTENDED },
    /* c0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB immediate (6809), illegal trap (6309) */
    /* c1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB immediate (6809), illegal trap (6309) */
    /* c2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB immediate (6809), illegal trap (6309) */
    /* c3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD immediate (6809), illegal trap (6309) */
    /* c4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB immediate (6809), illegal trap (6309) */
    /* c5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB immediate (6809), illegal trap (6309) */
    /* c6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB immediate (6809), illegal trap (6309) */
    /* c7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* c8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB immediate (6809), illegal trap (6309) */
    /* c9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB immediate (6809), illegal trap (6309) */
    /* ca */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB immediate (6809), illegal trap (6309) */
    /* cb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB immediate (6809), illegal trap (6309) */
    /* cc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD immediate (6809), illegal trap (6309) */
    /* cd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* ce */ { "LDS",   ASM_ADDR_MODE_IMM_WORD },
    /* cf */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STU immediate (6809), illegal trap (6309) */
    /* d0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB direct (6809), illegal trap (6309) */
    /* d1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB direct (6809), illegal trap (6309) */
    /* d2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB direct (6809), illegal trap (6309) */
    /* d3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD direct (6809), illegal trap (6309) */
    /* d4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB direct (6809), illegal trap (6309) */
    /* d5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB direct (6809), illegal trap (6309) */
    /* d6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB direct (6809), illegal trap (6309) */
    /* d7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB direct (6809), illegal trap (6309) */
    /* d8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB direct (6809), illegal trap (6309) */
    /* d9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB direct (6809), illegal trap (6309) */
    /* da */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB direct (6809), illegal trap (6309) */
    /* db */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB direct (6809), illegal trap (6309) */
    /* dc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD direct (6809), LDQ direct (6309) */
    /* dd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD direct (6809), STQ direct (6309) */
    /* de */ { "LDS",   ASM_ADDR_MODE_DIRECT },
    /* df */ { "STS",   ASM_ADDR_MODE_DIRECT },
    /* e0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB indexed (6809), illegal trap (6309) */
    /* e1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB indexed (6809), illegal trap (6309) */
    /* e2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB indexed (6809), illegal trap (6309) */
    /* e3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD indexed (6809), illegal trap (6309) */
    /* e4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB indexed (6809), illegal trap (6309) */
    /* e5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB indexed (6809), illegal trap (6309) */
    /* e6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB indexed (6809), illegal trap (6309) */
    /* e7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB indexed (6809), illegal trap (6309) */
    /* e8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB indexed (6809), illegal trap (6309) */
    /* e9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB indexed (6809), illegal trap (6309) */
    /* ea */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB indexed (6809), illegal trap (6309) */
    /* eb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB indexed (6809), illegal trap (6309) */
    /* ec */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD indexed (6809), LDQ indexed (6309) */
    /* ed */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD indexed (6809), STQ indexed (6309) */
    /* ee */ { "LDS",   ASM_ADDR_MODE_INDEXED },
    /* ef */ { "STS",   ASM_ADDR_MODE_INDEXED },
    /* f0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB extended (6809), illegal trap (6309) */
    /* f1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB extended (6809), illegal trap (6309) */
    /* f2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB extended (6809), illegal trap (6309) */
    /* f3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD extended (6809), illegal trap (6309) */
    /* f4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB extended (6809), illegal trap (6309) */
    /* f5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB extended (6809), illegal trap (6309) */
    /* f6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB extended (6809), illegal trap (6309) */
    /* f7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB extended (6809), illegal trap (6309) */
    /* f8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB extended (6809), illegal trap (6309) */
    /* f9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB extended (6809), illegal trap (6309) */
    /* fa */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB extended (6809), illegal trap (6309) */
    /* fb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB extended (6809), illegal trap (6309) */
    /* fc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD extended (6809), LDQ extended (6309) */
    /* fd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD extended (6809), STQ extended (6309) */
    /* fe */ { "LDS",   ASM_ADDR_MODE_EXTENDED },
    /* ff */ { "STS",   ASM_ADDR_MODE_EXTENDED }
};

static const asm_opcode_info_t opcode_list_11[256] = {
    /* 00 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG direct (6809), illegal trap (6309) */
    /* 01 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG direct (6809), illegal trap (6309) */
    /* 02 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM direct : NEG direct (6809), illegal trap (6309) */
    /* 03 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM direct (6809), illegal trap (6309) */
    /* 04 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR direct (6809), illegal trap (6309) */
    /* 05 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR direct (6809), illegal trap (6309) */
    /* 06 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR direct (6809), illegal trap (6309) */
    /* 07 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR direct (6809), illegal trap (6309) */
    /* 08 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL/LSL direct (6809), illegal trap (6309) */
    /* 09 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL direct (6809), illegal trap (6309) */
    /* 0a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC direct (6809), illegal trap (6309) */
    /* 0b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC direct (6809), illegal trap (6309) */
    /* 0c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC direct (6809), illegal trap (6309) */
    /* 0d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST direct (6809), illegal trap (6309) */
    /* 0e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP direct (6809), illegal trap (6309) */
    /* 0f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR direct (6809), illegal trap (6309) */
    /* 10 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ignore byte and read next (6809/6309) */
    /* 11 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ignore byte and read next (6809/6309) */
    /* 12 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NOP (6809), illegal trap (6309) */
    /* 13 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SYNC (6809), illegal trap (6309) */
    /* 14 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* 15 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* 16 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LBRA offset (6809), illegal trap (6309) */
    /* 17 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LBSR offset (6809), illegal trap (6309) */
    /* 18 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CCRS (6809), illegal trap (6309) */
    /* 19 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DAA (6809), illegal trap (6309) */
    /* 1a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORCC immediate (6809), illegal trap (6309) */
    /* 1b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NOP (6809), illegal trap (6309) */
    /* 1c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDCC immediate (6809), illegal trap (6309) */
    /* 1d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SEX (6809), illegal trap (6309) */
    /* 1e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EXG regs (6809), illegal trap (6309) */
    /* 1f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TFR regs (6809), illegal trap (6309) */
    /* 20 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BRA offset (6809), illegal trap (6309) */
    /* 21 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BRN offset (6809), illegal trap (6309) */
    /* 22 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BHI offset (6809), illegal trap (6309) */
    /* 23 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BLS offset (6809), illegal trap (6309) */
    /* 24 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BHS/BCC offset (6809), illegal trap (6309) */
    /* 25 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BLO/BCS offset (6809), illegal trap (6309) */
    /* 26 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BNE offset (6809), illegal trap (6309) */
    /* 27 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BEQ offset (6809), illegal trap (6309) */
    /* 28 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BVC offset (6809), illegal trap (6309) */
    /* 29 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BVS offset (6809), illegal trap (6309) */
    /* 2a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BPL offset (6809), illegal trap (6309) */
    /* 2b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BMI offset (6809), illegal trap (6309) */
    /* 2c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BGE offset (6809), illegal trap (6309) */
    /* 2d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BLT offset (6809), illegal trap (6309) */
    /* 2e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BGT offset (6809), illegal trap (6309) */
    /* 2f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BLE offset (6809), illegal trap (6309) */
    /* 30 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAX indexed (6809), BAND direct (6309) */
    /* 31 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAY indexed (6809), BIAND direct (6309) */
    /* 32 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAS indexed (6809), BOR direct (6309) */
    /* 33 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LEAU indexed (6809), BIOR direct (6309) */
    /* 34 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PSHS post (6809), BEOR direct (6309) */
    /* 35 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PULS post (6809), BIEOR direct (6309) */
    /* 36 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PSHU post (6809), LDBT direct (6309) */
    /* 37 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* PULU post (6809), STBT direct (6309) */
    /* 38 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDCC immediate + 1 cycle (6809), TFM R+,R+ (6309) */
    /* 39 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RTS cycle (6809), TFM R-,R- (6309) */
    /* 3a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ABX (6809), TFM R+,R (6309) */
    /* 3b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RTI (6809), TFM R,R+ (6309) */
    /* 3c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CWAI immediate (6809), BITMD immediate (6309) */
    /* 3d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* MUL (6809), LDMD immediate (6309) */
    /* 3e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SWIRES (6809), illegal trap (6309) */
    /* 3f */ { "SWI3",  ASM_ADDR_MODE_IMPLIED },
    /* 40 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGA (6809), illegal trap (6309) */
    /* 41 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGA (6809), illegal trap (6309) */
    /* 42 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMA : NEGA (6809), illegal trap (6309) */
    /* 43 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COMA (6809), COME (6309) */
    /* 44 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRA (6809), illegal trap (6309) */
    /* 45 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRA (6809), illegal trap (6309) */
    /* 46 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RORA (6809), illegal trap (6309) */
    /* 47 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASRA (6809), illegal trap (6309) */
    /* 48 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASLA/LSLA (6809), illegal trap (6309) */
    /* 49 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROLA (6809), illegal trap (6309) */
    /* 4a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECA (6809), DECE (6309) */
    /* 4b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECA (6809), illegal trap (6309) */
    /* 4c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INCA (6809), INCE (6309) */
    /* 4d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TSTA (6809), TSTE (6309) */
    /* 4e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRA (6809), illegal trap (6309) */
    /* 4f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRA (6809), CLRE (6309) */
    /* 50 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGB (6809), illegal trap (6309) */
    /* 51 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEGB (6809), illegal trap (6309) */
    /* 52 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COMB : NEGB (6809), illegal trap (6309) */
    /* 53 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COMB (6809), COMF (6309) */
    /* 54 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRB (6809), illegal trap (6309) */
    /* 55 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSRB (6809), illegal trap (6309) */
    /* 56 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* RORB (6809), illegal trap (6309) */
    /* 57 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASRB (6809), illegal trap (6309) */
    /* 58 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASLB/LSLB (6809), illegal trap (6309) */
    /* 59 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROLB (6809), illegal trap (6309) */
    /* 5a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECB (6809), DECF (6309) */
    /* 5b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DECB (6809), illegal trap (6309) */
    /* 5c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INCB (6809), INCF (6309) */
    /* 5d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TSTB (6809), TSTF (6309) */
    /* 5e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRB (6809), illegal trap (6309) */
    /* 5f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLRB (6809), CLRF (6309) */
    /* 60 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG indexed (6809), illegal trap (6309) */
    /* 61 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG indexed (6809), illegal trap (6309) */
    /* 62 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM indexed : NEG indexed (6809), illegal trap (6309) */
    /* 63 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM indexed (6809), illegal trap (6309) */
    /* 64 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR indexed (6809), illegal trap (6309) */
    /* 65 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR indexed (6809), illegal trap (6309) */
    /* 66 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR indexed (6809), illegal trap (6309) */
    /* 67 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR indexed (6809), illegal trap (6309) */
    /* 68 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL/LSL indexed (6809), illegal trap (6309) */
    /* 69 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL indexed (6809), illegal trap (6309) */
    /* 6a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC indexed (6809), illegal trap (6309) */
    /* 6b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC indexed (6809), illegal trap (6309) */
    /* 6c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC indexed (6809), illegal trap (6309) */
    /* 6d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST indexed (6809), illegal trap (6309) */
    /* 6e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP indexed (6809), illegal trap (6309) */
    /* 6f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR indexed (6809), illegal trap (6309) */
    /* 70 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG extended (6809), illegal trap (6309) */
    /* 71 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* NEG extended (6809), illegal trap (6309) */
    /* 72 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* carry ? COM extended : NEG extended (6809), illegal trap (6309) */
    /* 73 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* COM extended (6809), illegal trap (6309) */
    /* 74 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR extended (6809), illegal trap (6309) */
    /* 75 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LSR extended (6809), illegal trap (6309) */
    /* 76 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROR extended (6809), illegal trap (6309) */
    /* 77 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASR extended (6809), illegal trap (6309) */
    /* 78 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ASL/LSL extended (6809), illegal trap (6309) */
    /* 79 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ROL extended (6809), illegal trap (6309) */
    /* 7a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC extended (6809), illegal trap (6309) */
    /* 7b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* DEC extended (6809), illegal trap (6309) */
    /* 7c */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* INC extended (6809), illegal trap (6309) */
    /* 7d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* TST extended (6809), illegal trap (6309) */
    /* 7e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JMP extended (6809), illegal trap (6309) */
    /* 7f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CLR extended (6809), illegal trap (6309) */
    /* 80 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA immediate (6809), SUBE immediate (6309) */
    /* 81 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA immediate (6809), CMPE immediate (6309) */
    /* 82 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA immediate (6809), illegal trap (6309) */
    /* 83 */ { "CMPU",  ASM_ADDR_MODE_IMM_WORD },
    /* 84 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA immediate (6809), illegal trap (6309) */
    /* 85 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA immediate (6809), illegal trap (6309) */
    /* 86 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA immediate (6809), LDE immediate (6309) */
    /* 87 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* 88 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA immediate (6809), illegal trap (6309) */
    /* 89 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA immediate (6809), illegal trap (6309) */
    /* 8a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA immediate (6809), illegal trap (6309) */
    /* 8b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA immediate (6809), ADDE immediate (6309) */
    /* 8c */ { "CMPS",  ASM_ADDR_MODE_IMM_WORD },
    /* 8d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BSR offset (6809), DIVD immediate (6309) */
    /* 8e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDX immediate (6809), DIVQ immediate (6309) */
    /* 8f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STX immediate (6809), MULD immediate (6309) */
    /* 90 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA direct (6809), SUBE direct (6309) */
    /* 91 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA direct (6809), CMPE direct (6309) */
    /* 92 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA direct (6809), illegal trap (6309) */
    /* 93 */ { "CMPU",  ASM_ADDR_MODE_DIRECT },
    /* 94 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA direct (6809), illegal trap (6309) */
    /* 95 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA direct (6809), illegal trap (6309) */
    /* 96 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA direct (6809), LDE direct (6309) */
    /* 97 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA direct (6809), STE direct (6309) */
    /* 98 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA direct (6809), illegal trap (6309) */
    /* 99 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA direct (6809), illegal trap (6309) */
    /* 9a */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA direct (6809), illegal trap (6309) */
    /* 9b */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA direct (6809), ADDE direct (6309) */
    /* 9c */ { "CMPS",  ASM_ADDR_MODE_DIRECT },
    /* 9d */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR direct (6809), DIVD direct (6309) */
    /* 9e */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDX direct (6809), DIVQ direct (6309) */
    /* 9f */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STX direct (6809), MULD direct (6309) */
    /* a0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA indexed (6809), SUBE indexed (6309) */
    /* a1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA indexed (6809), CMPE indexed (6309) */
    /* a2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA indexed (6809), illegal trap (6309) */
    /* a3 */ { "CMPU",  ASM_ADDR_MODE_INDEXED },
    /* a4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA indexed (6809), illegal trap (6309) */
    /* a5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA indexed (6809), illegal trap (6309) */
    /* a6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA indexed (6809), LDE indexed (6309) */
    /* a7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA indexed (6809), STE indexed (6309) */
    /* a8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA indexed (6809), illegal trap (6309) */
    /* a9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA indexed (6809), illegal trap (6309) */
    /* aa */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA indexed (6809), illegal trap (6309) */
    /* ab */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA indexed (6809), ADDE indexed (6309) */
    /* ac */ { "CMPS",  ASM_ADDR_MODE_INDEXED },
    /* ad */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR indexed (6809), DIVD indexed (6309) */
    /* ae */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDX indexed (6809), DIVQ indexed (6309) */
    /* af */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STX indexed (6809), MULD indexed (6309) */
    /* b0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBA extended (6809), SUBE extended (6309) */
    /* b1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPA extended (6809), CMPE extended (6309) */
    /* b2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCA extended (6809), illegal trap (6309) */
    /* b3 */ { "CMPU",  ASM_ADDR_MODE_EXTENDED },
    /* b4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDA extended (6809), illegal trap (6309) */
    /* b5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITA extended (6809), illegal trap (6309) */
    /* b6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDA extended (6809), LDE extended (6309) */
    /* b7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STA extended (6809), STE extended (6309) */
    /* b8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORA extended (6809), illegal trap (6309) */
    /* b9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCA extended (6809), illegal trap (6309) */
    /* ba */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORA extended (6809), illegal trap (6309) */
    /* bb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDA extended (6809), ADDE extended (6309) */
    /* bc */ { "CMPS",  ASM_ADDR_MODE_EXTENDED },
    /* bd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* JSR extended (6809), DIVD extended (6309) */
    /* be */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDX extended (6809), DIVQ extended (6309) */
    /* bf */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STX extended (6809), MULD extended (6309) */
    /* c0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB immediate (6809), SUBF immediate (6309) */
    /* c1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB immediate (6809), CMPF immediate (6309) */
    /* c2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB immediate (6809), illegal trap (6309) */
    /* c3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD immediate (6809), illegal trap (6309) */
    /* c4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB immediate (6809), illegal trap (6309) */
    /* c5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB immediate (6809), illegal trap (6309) */
    /* c6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB immediate (6809), LDF immediate (6309) */
    /* c7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SCC immediate (6809), illegal trap (6309) */
    /* c8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB immediate (6809), illegal trap (6309) */
    /* c9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB immediate (6809), illegal trap (6309) */
    /* ca */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB immediate (6809), illegal trap (6309) */
    /* cb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB immediate (6809), ADDF immediate (6309) */
    /* cc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD immediate (6809), illegal trap (6309) */
    /* cd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* HCF (6809), illegal trap (6309) */
    /* ce */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDU immediate (6809), illegal trap (6309) */
    /* cf */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STU immediate (6809), illegal trap (6309) */
    /* d0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB direct (6809), SUBF direct (6309) */
    /* d1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB direct (6809), CMPF direct (6309) */
    /* d2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB direct (6809), illegal trap (6309) */
    /* d3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD direct (6809), illegal trap (6309) */
    /* d4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB direct (6809), illegal trap (6309) */
    /* d5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB direct (6809), illegal trap (6309) */
    /* d6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB direct (6809), LDF direct (6309) */
    /* d7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB direct (6809), STF direct (6309) */
    /* d8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB direct (6809), illegal trap (6309) */
    /* d9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB direct (6809), illegal trap (6309) */
    /* da */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB direct (6809), illegal trap (6309) */
    /* db */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB direct (6809), ADDF direct (6309) */
    /* dc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD direct (6809), illegal trap (6309) */
    /* dd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD direct (6809), illegal trap (6309) */
    /* de */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDU direct (6809), illegal trap (6309) */
    /* df */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STU direct (6809), illegal trap (6309) */
    /* e0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB indexed (6809), SUBF indexed (6309) */
    /* e1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB indexed (6809), CMPF indexed (6309) */
    /* e2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB indexed (6809), illegal trap (6309) */
    /* e3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD indexed (6809), illegal trap (6309) */
    /* e4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB indexed (6809), illegal trap (6309) */
    /* e5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB indexed (6809), illegal trap (6309) */
    /* e6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB indexed (6809), LDF indexed (6309) */
    /* e7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB indexed (6809), STF indexed (6309) */
    /* e8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB indexed (6809), illegal trap (6309) */
    /* e9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB indexed (6809), illegal trap (6309) */
    /* ea */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB indexed (6809), illegal trap (6309) */
    /* eb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB indexed (6809), ADDF indexed (6309) */
    /* ec */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD indexed (6809), illegal trap (6309) */
    /* ed */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD indexed (6809), illegal trap (6309) */
    /* ee */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDU indexed (6809), illegal trap (6309) */
    /* ef */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STU indexed (6809), illegal trap (6309) */
    /* f0 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SUBB extended (6809), SUBF extended (6309) */
    /* f1 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* CMPB extended (6809), CMPF extended (6309) */
    /* f2 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* SBCB extended (6809), illegal trap (6309) */
    /* f3 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDD extended (6809), illegal trap (6309) */
    /* f4 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ANDB extended (6809), illegal trap (6309) */
    /* f5 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* BITB extended (6809), illegal trap (6309) */
    /* f6 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDB extended (6809), LDF extended (6309) */
    /* f7 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STB extended (6809), STF extended (6309) */
    /* f8 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* EORB extended (6809), illegal trap (6309) */
    /* f9 */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADCB extended (6809), illegal trap (6309) */
    /* fa */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ORB extended (6809), illegal trap (6309) */
    /* fb */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* ADDB extended (6809), ADDF extended (6309) */
    /* fc */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDD extended (6809), illegal trap (6309) */
    /* fd */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* STD extended (6809), illegal trap (6309) */
    /* fe */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL },        /* LDU extended (6809), illegal trap (6309) */
    /* ff */ { "UNDOC", ASM_ADDR_MODE_ILLEGAL }         /* STU extended (6809), illegal trap (6309) */
};

static const asm_opcode_info_t *asm_opcode_info_get(unsigned int p0, unsigned int p1, unsigned int p2)
{
    /*
     * Extra prefix bytes after the first one should be ignored (UNDOC),
     * but since we get only a limited amount of bytes here we can't
     * really do that.
     */
    if (p0 == 0x10) {
        return opcode_list_10 + p1;
    }
    if (p0 == 0x11) {
        return opcode_list_11 + p1;
    }
    return opcode_list + p0;
}

static unsigned int asm_addr_mode_get_size(unsigned int mode, unsigned int p0,
                                           unsigned int p1, unsigned int p2)
{
    int size = 0;

    if (p0 == 0x10 || p0 == 0x11) {
        size++;
        p0 = p1;
        p1 = p2;
    }
    if (mode == ASM_ADDR_MODE_INDEXED) {
        /* post-byte determines submode */
        if (p1 & 0x80) {
            size += indexed_size[p1 & 0x1F];
        }
    }
    return size + addr_mode_size[mode];
}

void asm6809_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->cpu_type = CPU_6809;
    monitor_cpu_type->asm_addr_mode_get_size = asm_addr_mode_get_size;
    monitor_cpu_type->asm_opcode_info_get = asm_opcode_info_get;

    /* Once we have a generic processor specific init, this will move.  */
    mon_assemble6809_init(monitor_cpu_type);
    mon_register6809_init(monitor_cpu_type);
}
