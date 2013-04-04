/*
 * asmR65C02.c - R65C02 Assembler-related utility functions.
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

#include "vice.h"

#include "asm.h"
#include "mon_assemble.h"
#include "mon_register.h"
#include "montypes.h"
#include "types.h"

static const int addr_mode_size[] = {
    1, /* ASM_ADDR_MODE_IMPLIED */
    1, /* ASM_ADDR_MODE_ACCUMULATOR */
    2, /* ASM_ADDR_MODE_IMMEDIATE */
    2, /* ASM_ADDR_MODE_ZERO_PAGE */
    2, /* ASM_ADDR_MODE_ZERO_PAGE_X */
    2, /* ASM_ADDR_MODE_ZERO_PAGE_Y */
    3, /* ASM_ADDR_MODE_ABSOLUTE */
    3, /* ASM_ADDR_MODE_ABSOLUTE_X */
    3, /* ASM_ADDR_MODE_ABSOLUTE_Y */
    3, /* ASM_ADDR_MODE_ABS_INDIRECT */
    2, /* ASM_ADDR_MODE_INDIRECT_X */
    2, /* ASM_ADDR_MODE_INDIRECT_Y */
    2, /* ASM_ADDR_MODE_RELATIVE */
    0, /* ASM_ADDR_MODE_ABSOLUTE_A */
    0, /* ASM_ADDR_MODE_ABSOLUTE_HL */
    0, /* ASM_ADDR_MODE_ABSOLUTE_IX */
    0, /* ASM_ADDR_MODE_ABSOLUTE_IY */
    0, /* ASM_ADDR_MODE_ABS_INDIRECT_ZP */
    0, /* ASM_ADDR_MODE_IMMEDIATE_16 */
    0, /* ASM_ADDR_MODE_REG_B */
    0, /* ASM_ADDR_MODE_REG_C */
    0, /* ASM_ADDR_MODE_REG_D */
    0, /* ASM_ADDR_MODE_REG_E */
    0, /* ASM_ADDR_MODE_REG_H */
    0, /* ASM_ADDR_MODE_REG_IXH */
    0, /* ASM_ADDR_MODE_REG_IYH */
    0, /* ASM_ADDR_MODE_REG_L */
    0, /* ASM_ADDR_MODE_REG_IXL */
    0, /* ASM_ADDR_MODE_REG_IYL */
    0, /* ASM_ADDR_MODE_REG_AF */
    0, /* ASM_ADDR_MODE_REG_BC */
    0, /* ASM_ADDR_MODE_REG_DE */
    0, /* ASM_ADDR_MODE_REG_HL */
    0, /* ASM_ADDR_MODE_REG_IX */
    0, /* ASM_ADDR_MODE_REG_IY */
    0, /* ASM_ADDR_MODE_REG_SP */
    0, /* ASM_ADDR_MODE_REG_IND_BC */
    0, /* ASM_ADDR_MODE_REG_IND_DE */
    0, /* ASM_ADDR_MODE_REG_IND_HL */
    0, /* ASM_ADDR_MODE_REG_IND_IX */
    0, /* ASM_ADDR_MODE_REG_IND_IY */
    0, /* ASM_ADDR_MODE_REG_IND_SP */
    2, /* ASM_ADDR_MODE_INDIRECT */
    3, /* ASM_ADDR_MODE_ABS_INDIRECT_X */
   -1, /* ASM_ADDR_MODE_DOUBLE */
    3, /* ASM_ADDR_MODE_ZERO_PAGE_RELATIVE */
};

static const asm_opcode_info_t opcode_list[] = {
    /* 00 */ { "BRK",   ASM_ADDR_MODE_IMPLIED },
    /* 01 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 02 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 03 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 04 */ { "TSB",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 05 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 06 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 07 */ { "RMB 0,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 08 */ { "PHP",   ASM_ADDR_MODE_IMPLIED },
    /* 09 */ { "ORA",   ASM_ADDR_MODE_IMMEDIATE },
    /* 0a */ { "ASL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 0b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 0c */ { "TSB",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0f */ { "BBR 0,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 10 */ { "BPL",   ASM_ADDR_MODE_RELATIVE },
    /* 11 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 12 */ { "ORA",   ASM_ADDR_MODE_INDIRECT },
    /* 13 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 14 */ { "TRB",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 15 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 16 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 17 */ { "RMB 1,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 18 */ { "CLC",   ASM_ADDR_MODE_IMPLIED },
    /* 19 */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 1a */ { "INC",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 1b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 1c */ { "TRB",   ASM_ADDR_MODE_ABSOLUTE },
    /* 1d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1f */ { "BBR 1,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 20 */ { "JSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 21 */ { "AND",   ASM_ADDR_MODE_INDIRECT_X },
    /* 22 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 23 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 24 */ { "BIT",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 25 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 26 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 27 */ { "RMB 2,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 28 */ { "PLP",   ASM_ADDR_MODE_IMPLIED },
    /* 29 */ { "AND",   ASM_ADDR_MODE_IMMEDIATE },
    /* 2a */ { "ROL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 2b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 2c */ { "BIT",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2f */ { "BBR 2,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 30 */ { "BMI",   ASM_ADDR_MODE_RELATIVE },
    /* 31 */ { "AND",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 32 */ { "AND",   ASM_ADDR_MODE_INDIRECT },
    /* 33 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 34 */ { "BIT",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 35 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 36 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 37 */ { "RMB 3,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 38 */ { "SEC",   ASM_ADDR_MODE_IMPLIED },
    /* 39 */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 3a */ { "DEC",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 3b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 3c */ { "BIT",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3f */ { "BBR 3,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 40 */ { "RTI",   ASM_ADDR_MODE_IMPLIED },
    /* 41 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_X },
    /* 42 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 43 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 44 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE },
    /* 45 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 46 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 47 */ { "RMB 4,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 48 */ { "PHA",   ASM_ADDR_MODE_IMPLIED },
    /* 49 */ { "EOR",   ASM_ADDR_MODE_IMMEDIATE },
    /* 4a */ { "LSR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 4b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 4c */ { "JMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4f */ { "BBR 4,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 50 */ { "BVC",   ASM_ADDR_MODE_RELATIVE },
    /* 51 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 52 */ { "EOR",   ASM_ADDR_MODE_INDIRECT },
    /* 53 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 54 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 55 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 56 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 57 */ { "RMB 5,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 58 */ { "CLI",   ASM_ADDR_MODE_IMPLIED },
    /* 59 */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 5a */ { "PHY",   ASM_ADDR_MODE_IMPLIED },
    /* 5b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 5c */ { "NOOP8", ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5f */ { "BBR 5,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 60 */ { "RTS",   ASM_ADDR_MODE_IMPLIED },
    /* 61 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_X },
    /* 62 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 63 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 64 */ { "STZ",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 65 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 66 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 67 */ { "RMB 6,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 68 */ { "PLA",   ASM_ADDR_MODE_IMPLIED },
    /* 69 */ { "ADC",   ASM_ADDR_MODE_IMMEDIATE },
    /* 6a */ { "ROR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 6b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 6c */ { "JMP",   ASM_ADDR_MODE_ABS_INDIRECT },
    /* 6d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6f */ { "BBR 6,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 70 */ { "BVS",   ASM_ADDR_MODE_RELATIVE },
    /* 71 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 72 */ { "ADC",   ASM_ADDR_MODE_INDIRECT },
    /* 73 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 74 */ { "STZ",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 75 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 76 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 77 */ { "RMB 7,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 78 */ { "SEI",   ASM_ADDR_MODE_IMPLIED },
    /* 79 */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 7a */ { "PLY",   ASM_ADDR_MODE_IMPLIED },
    /* 7b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 7c */ { "JMP",   ASM_ADDR_MODE_ABS_INDIRECT_X },
    /* 7d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7f */ { "BBR 7,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 80 */ { "BRA",   ASM_ADDR_MODE_RELATIVE },
    /* 81 */ { "STA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 82 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 83 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 84 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 85 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 86 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 87 */ { "SMB 0,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 88 */ { "DEY",   ASM_ADDR_MODE_IMPLIED },
    /* 89 */ { "BIT",   ASM_ADDR_MODE_IMMEDIATE },
    /* 8a */ { "TXA",   ASM_ADDR_MODE_IMPLIED },
    /* 8b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 8c */ { "STY",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8e */ { "STX",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8f */ { "BBS 0,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* 90 */ { "BCC",   ASM_ADDR_MODE_RELATIVE },
    /* 91 */ { "STA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 92 */ { "STA",   ASM_ADDR_MODE_INDIRECT },
    /* 93 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 94 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 95 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 96 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* 97 */ { "SMB 1,",ASM_ADDR_MODE_ZERO_PAGE },
    /* 98 */ { "TYA",   ASM_ADDR_MODE_IMPLIED },
    /* 99 */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 9a */ { "TXS",   ASM_ADDR_MODE_IMPLIED },
    /* 9b */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 9c */ { "STZ",   ASM_ADDR_MODE_ABSOLUTE },
    /* 9d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9e */ { "STZ",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9f */ { "BBS 1,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* a0 */ { "LDY",   ASM_ADDR_MODE_IMMEDIATE },
    /* a1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_X },
    /* a2 */ { "LDX",   ASM_ADDR_MODE_IMMEDIATE },
    /* a3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* a4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a7 */ { "SMB 2,",ASM_ADDR_MODE_ZERO_PAGE },
    /* a8 */ { "TAY",   ASM_ADDR_MODE_IMPLIED },
    /* a9 */ { "LDA",   ASM_ADDR_MODE_IMMEDIATE },
    /* aa */ { "TAX",   ASM_ADDR_MODE_IMPLIED },
    /* ab */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* ac */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE },
    /* ad */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE },
    /* ae */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE },
    /* af */ { "BBS 2,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* b0 */ { "BCS",   ASM_ADDR_MODE_RELATIVE },
    /* b1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* b2 */ { "LDA",   ASM_ADDR_MODE_INDIRECT },
    /* b3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* b4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* b7 */ { "SMB 3,",ASM_ADDR_MODE_ZERO_PAGE },
    /* b8 */ { "CLV",   ASM_ADDR_MODE_IMPLIED },
    /* b9 */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* ba */ { "TSX",   ASM_ADDR_MODE_IMPLIED },
    /* bb */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* bc */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* bd */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* be */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* bf */ { "BBS 3,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* c0 */ { "CPY",   ASM_ADDR_MODE_IMMEDIATE },
    /* c1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_X },
    /* c2 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* c3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* c4 */ { "CPY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c7 */ { "SMB 4,",ASM_ADDR_MODE_ZERO_PAGE },
    /* c8 */ { "INY",   ASM_ADDR_MODE_IMPLIED },
    /* c9 */ { "CMP",   ASM_ADDR_MODE_IMMEDIATE },
    /* ca */ { "DEX",   ASM_ADDR_MODE_IMPLIED },
    /* cb */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* cc */ { "CPY",   ASM_ADDR_MODE_ABSOLUTE },
    /* cd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* ce */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE },
    /* cf */ { "BBS 4,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* d0 */ { "BNE",   ASM_ADDR_MODE_RELATIVE },
    /* d1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_Y },
    /* d2 */ { "CMP",   ASM_ADDR_MODE_INDIRECT },
    /* d3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* d4 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d7 */ { "SMB 5,",ASM_ADDR_MODE_ZERO_PAGE },
    /* d8 */ { "CLD",   ASM_ADDR_MODE_IMPLIED },
    /* d9 */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* da */ { "PHX",   ASM_ADDR_MODE_IMPLIED },
    /* db */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* dc */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* dd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* de */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* df */ { "BBS 5,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* e0 */ { "CPX",   ASM_ADDR_MODE_IMMEDIATE },
    /* e1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_X },
    /* e2 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* e3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* e4 */ { "CPX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e7 */ { "SMB 6,",ASM_ADDR_MODE_ZERO_PAGE },
    /* e8 */ { "INX",   ASM_ADDR_MODE_IMPLIED },
    /* e9 */ { "SBC",   ASM_ADDR_MODE_IMMEDIATE },
    /* ea */ { "NOP",   ASM_ADDR_MODE_IMPLIED },
    /* eb */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* ec */ { "CPX",   ASM_ADDR_MODE_ABSOLUTE },
    /* ed */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ee */ { "INC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ef */ { "BBS 6,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE },

    /* f0 */ { "BEQ",   ASM_ADDR_MODE_RELATIVE },
    /* f1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* f2 */ { "SBC",   ASM_ADDR_MODE_INDIRECT },
    /* f3 */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* f4 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f7 */ { "SMB 7,",ASM_ADDR_MODE_ZERO_PAGE },
    /* f8 */ { "SED",   ASM_ADDR_MODE_IMPLIED },
    /* f9 */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* fa */ { "PLX",   ASM_ADDR_MODE_IMPLIED },
    /* fb */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* fc */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* fd */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* fe */ { "INC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* ff */ { "BBS 7,",ASM_ADDR_MODE_ZERO_PAGE_RELATIVE }
};

static const asm_opcode_info_t *asm_opcode_info_get(unsigned int p0, unsigned int p1, unsigned int p2)
{
    return opcode_list + p0;
}

static unsigned int asm_addr_mode_get_size(unsigned int mode, unsigned int p0,
                                           unsigned int p1, unsigned int p2)
{
    return addr_mode_size[mode];
}

void asmR65C02_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->cpu_type = CPU_R65C02;
    monitor_cpu_type->asm_addr_mode_get_size = asm_addr_mode_get_size;
    monitor_cpu_type->asm_opcode_info_get = asm_opcode_info_get;

    /* Once we have a generic processor specific init, this will move.  */
    mon_assembleR65C02_init(monitor_cpu_type);
    mon_registerR65C02_init(monitor_cpu_type);
}
