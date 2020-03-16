/*
 * asm65816.c - 65816/65802 Assembler-related utility functions.
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

/* FIXME: 65816/65802 16bit modes are not handled properly, every opcode is
   handled in 8bit mode only */

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
    3, /* ASM_ADDR_MODE_IMMEDIATE_16 */
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
    3, /* ASM_ADDR_MODE_RELATIVE_LONG */
    2, /* ASM_ADDR_MODE_STACK_RELATIVE_Y */
    2, /* ASM_ADDR_MODE_STACK_RELATIVE */
    2, /* ASM_ADDR_MODE_INDIRECT_LONG */
    4, /* ASM_ADDR_MODE_ABSOLUTE_LONG */
    2, /* ASM_ADDR_MODE_INDIRECT_LONG_Y */
    4, /* ASM_ADDR_MODE_ABSOLUTE_LONG_X */
    3, /* ASM_ADDR_MODE_MOVE */
    3  /* ASM_ADDR_MODE_ABS_IND_LONG */
};

static const asm_opcode_info_t opcode_list[] = {
    /* 00 */ { "BRK",   ASM_ADDR_MODE_IMPLIED },
    /* 01 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 02 */ { "COP",   ASM_ADDR_MODE_IMMEDIATE },
    /* 03 */ { "ORA",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* 04 */ { "TSB",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 05 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 06 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 07 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* 08 */ { "PHP",   ASM_ADDR_MODE_IMPLIED },
    /* 09 */ { "ORA",   ASM_ADDR_MODE_IMMEDIATE },
    /* 0a */ { "ASL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 0b */ { "PHD",   ASM_ADDR_MODE_IMPLIED },
    /* 0c */ { "TSB",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0f */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* 10 */ { "BPL",   ASM_ADDR_MODE_RELATIVE },
    /* 11 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 12 */ { "ORA",   ASM_ADDR_MODE_INDIRECT },
    /* 13 */ { "ORA",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* 14 */ { "TRB",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 15 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 16 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 17 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* 18 */ { "CLC",   ASM_ADDR_MODE_IMPLIED },
    /* 19 */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 1a */ { "INC",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 1b */ { "TCS",   ASM_ADDR_MODE_IMPLIED },
    /* 1c */ { "TRB",   ASM_ADDR_MODE_ABSOLUTE },
    /* 1d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1f */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* 20 */ { "JSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 21 */ { "AND",   ASM_ADDR_MODE_INDIRECT_X },
    /* 22 */ { "JSR",   ASM_ADDR_MODE_ABSOLUTE_LONG },
    /* 23 */ { "AND",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* 24 */ { "BIT",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 25 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 26 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 27 */ { "AND",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* 28 */ { "PLP",   ASM_ADDR_MODE_IMPLIED },
    /* 29 */ { "AND",   ASM_ADDR_MODE_IMMEDIATE },
    /* 2a */ { "ROL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 2b */ { "PLD",   ASM_ADDR_MODE_IMPLIED },
    /* 2c */ { "BIT",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2f */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* 30 */ { "BMI",   ASM_ADDR_MODE_RELATIVE },
    /* 31 */ { "AND",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 32 */ { "AND",   ASM_ADDR_MODE_INDIRECT },
    /* 33 */ { "AND",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* 34 */ { "BIT",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 35 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 36 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 37 */ { "AND",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* 38 */ { "SEC",   ASM_ADDR_MODE_IMPLIED },
    /* 39 */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 3a */ { "DEC",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 3b */ { "TSC",   ASM_ADDR_MODE_IMPLIED },
    /* 3c */ { "BIT",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3f */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* 40 */ { "RTI",   ASM_ADDR_MODE_IMPLIED },
    /* 41 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_X },
    /* 42 */ { "WDM",   ASM_ADDR_MODE_IMMEDIATE },
    /* 43 */ { "EOR",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* 44 */ { "MVP",   ASM_ADDR_MODE_MOVE },
    /* 45 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 46 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 47 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* 48 */ { "PHA",   ASM_ADDR_MODE_IMPLIED },
    /* 49 */ { "EOR",   ASM_ADDR_MODE_IMMEDIATE },
    /* 4a */ { "LSR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 4b */ { "PHK",   ASM_ADDR_MODE_IMPLIED },
    /* 4c */ { "JMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4f */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* 50 */ { "BVC",   ASM_ADDR_MODE_RELATIVE },
    /* 51 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 52 */ { "EOR",   ASM_ADDR_MODE_INDIRECT },
    /* 53 */ { "EOR",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* 54 */ { "MVN",   ASM_ADDR_MODE_MOVE },
    /* 55 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 56 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 57 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* 58 */ { "CLI",   ASM_ADDR_MODE_IMPLIED },
    /* 59 */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 5a */ { "PHY",   ASM_ADDR_MODE_IMPLIED },
    /* 5b */ { "TCD",   ASM_ADDR_MODE_IMPLIED },
    /* 5c */ { "JMP",   ASM_ADDR_MODE_ABSOLUTE_LONG },
    /* 5d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5f */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* 60 */ { "RTS",   ASM_ADDR_MODE_IMPLIED },
    /* 61 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_X },
    /* 62 */ { "PER",   ASM_ADDR_MODE_ABSOLUTE },
    /* 63 */ { "ADC",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* 64 */ { "STZ",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 65 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 66 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 67 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* 68 */ { "PLA",   ASM_ADDR_MODE_IMPLIED },
    /* 69 */ { "ADC",   ASM_ADDR_MODE_IMMEDIATE },
    /* 6a */ { "ROR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 6b */ { "RTL",   ASM_ADDR_MODE_IMPLIED },
    /* 6c */ { "JMP",   ASM_ADDR_MODE_ABS_INDIRECT },
    /* 6d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6f */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* 70 */ { "BVS",   ASM_ADDR_MODE_RELATIVE },
    /* 71 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 72 */ { "ADC",   ASM_ADDR_MODE_INDIRECT },
    /* 73 */ { "ADC",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* 74 */ { "STZ",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 75 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 76 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 77 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* 78 */ { "SEI",   ASM_ADDR_MODE_IMPLIED },
    /* 79 */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 7a */ { "PLY",   ASM_ADDR_MODE_IMPLIED },
    /* 7b */ { "TDC",   ASM_ADDR_MODE_IMPLIED },
    /* 7c */ { "JMP",   ASM_ADDR_MODE_ABS_INDIRECT_X },
    /* 7d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7f */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* 80 */ { "BRA",   ASM_ADDR_MODE_RELATIVE },
    /* 81 */ { "STA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 82 */ { "BRA",   ASM_ADDR_MODE_RELATIVE_LONG },
    /* 83 */ { "STA",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* 84 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 85 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 86 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 87 */ { "STA",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* 88 */ { "DEY",   ASM_ADDR_MODE_IMPLIED },
    /* 89 */ { "BIT",   ASM_ADDR_MODE_IMMEDIATE },
    /* 8a */ { "TXA",   ASM_ADDR_MODE_IMPLIED },
    /* 8b */ { "PHB",   ASM_ADDR_MODE_IMPLIED },
    /* 8c */ { "STY",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8e */ { "STX",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8f */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* 90 */ { "BCC",   ASM_ADDR_MODE_RELATIVE },
    /* 91 */ { "STA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 92 */ { "STA",   ASM_ADDR_MODE_INDIRECT },
    /* 93 */ { "STA",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* 94 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 95 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 96 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* 97 */ { "STA",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* 98 */ { "TYA",   ASM_ADDR_MODE_IMPLIED },
    /* 99 */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 9a */ { "TXS",   ASM_ADDR_MODE_IMPLIED },
    /* 9b */ { "TXY",   ASM_ADDR_MODE_IMPLIED },
    /* 9c */ { "STZ",   ASM_ADDR_MODE_ABSOLUTE },
    /* 9d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9e */ { "STZ",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9f */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* a0 */ { "LDY",   ASM_ADDR_MODE_IMMEDIATE },
    /* a1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_X },
    /* a2 */ { "LDX",   ASM_ADDR_MODE_IMMEDIATE },
    /* a3 */ { "LDA",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* a4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a7 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* a8 */ { "TAY",   ASM_ADDR_MODE_IMPLIED },
    /* a9 */ { "LDA",   ASM_ADDR_MODE_IMMEDIATE },
    /* aa */ { "TAX",   ASM_ADDR_MODE_IMPLIED },
    /* ab */ { "PLB",   ASM_ADDR_MODE_IMPLIED },
    /* ac */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE },
    /* ad */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE },
    /* ae */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE },
    /* af */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* b0 */ { "BCS",   ASM_ADDR_MODE_RELATIVE },
    /* b1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* b2 */ { "LDA",   ASM_ADDR_MODE_INDIRECT },
    /* b3 */ { "LDA",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* b4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* b7 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* b8 */ { "CLV",   ASM_ADDR_MODE_IMPLIED },
    /* b9 */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* ba */ { "TSX",   ASM_ADDR_MODE_IMPLIED },
    /* bb */ { "TYX",   ASM_ADDR_MODE_IMPLIED },
    /* bc */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* bd */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* be */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* bf */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* c0 */ { "CPY",   ASM_ADDR_MODE_IMMEDIATE },
    /* c1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_X },
    /* c2 */ { "REP",   ASM_ADDR_MODE_IMMEDIATE },
    /* c3 */ { "CMP",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* c4 */ { "CPY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c7 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* c8 */ { "INY",   ASM_ADDR_MODE_IMPLIED },
    /* c9 */ { "CMP",   ASM_ADDR_MODE_IMMEDIATE },
    /* ca */ { "DEX",   ASM_ADDR_MODE_IMPLIED },
    /* cb */ { "WAI",   ASM_ADDR_MODE_IMPLIED },
    /* cc */ { "CPY",   ASM_ADDR_MODE_ABSOLUTE },
    /* cd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* ce */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE },
    /* cf */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* d0 */ { "BNE",   ASM_ADDR_MODE_RELATIVE },
    /* d1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_Y },
    /* d2 */ { "CMP",   ASM_ADDR_MODE_INDIRECT },
    /* d3 */ { "CMP",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* d4 */ { "PEI",   ASM_ADDR_MODE_INDIRECT },
    /* d5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d7 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* d8 */ { "CLD",   ASM_ADDR_MODE_IMPLIED },
    /* d9 */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* da */ { "PHX",   ASM_ADDR_MODE_IMPLIED },
    /* db */ { "STP",   ASM_ADDR_MODE_IMPLIED },
    /* dc */ { "JMP",   ASM_ADDR_MODE_ABS_IND_LONG },
    /* dd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* de */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* df */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_LONG_X },

    /* e0 */ { "CPX",   ASM_ADDR_MODE_IMMEDIATE },
    /* e1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_X },
    /* e2 */ { "SEP",   ASM_ADDR_MODE_IMMEDIATE },
    /* e3 */ { "SBC",   ASM_ADDR_MODE_STACK_RELATIVE },
    /* e4 */ { "CPX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e7 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_LONG },
    /* e8 */ { "INX",   ASM_ADDR_MODE_IMPLIED },
    /* e9 */ { "SBC",   ASM_ADDR_MODE_IMMEDIATE },
    /* ea */ { "NOP",   ASM_ADDR_MODE_IMPLIED },
    /* eb */ { "XBA",   ASM_ADDR_MODE_IMPLIED },
    /* ec */ { "CPX",   ASM_ADDR_MODE_ABSOLUTE },
    /* ed */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ee */ { "INC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ef */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_LONG },

    /* f0 */ { "BEQ",   ASM_ADDR_MODE_RELATIVE },
    /* f1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* f2 */ { "SBC",   ASM_ADDR_MODE_INDIRECT },
    /* f3 */ { "SBC",   ASM_ADDR_MODE_STACK_RELATIVE_Y },
    /* f4 */ { "PEA",   ASM_ADDR_MODE_ABSOLUTE },
    /* f5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f7 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_LONG_Y },
    /* f8 */ { "SED",   ASM_ADDR_MODE_IMPLIED },
    /* f9 */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* fa */ { "PLX",   ASM_ADDR_MODE_IMPLIED },
    /* fb */ { "XCE",   ASM_ADDR_MODE_IMPLIED },
    /* fc */ { "JSR",   ASM_ADDR_MODE_ABS_INDIRECT_X },
    /* fd */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* fe */ { "INC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* ff */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_LONG_X }
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

void asm65816_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->cpu_type = CPU_65816;
    monitor_cpu_type->asm_addr_mode_get_size = asm_addr_mode_get_size;
    monitor_cpu_type->asm_opcode_info_get = asm_opcode_info_get;

    /* Once we have a generic processor specific init, this will move.  */
    mon_assemble65816_init(monitor_cpu_type);
    mon_register65816_init(monitor_cpu_type);
}
