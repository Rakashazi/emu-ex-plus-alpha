/*
 * asm6502.c - 6502/10 Assembler-related utility functions.
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
    2  /* ASM_ADDR_MODE_RELATIVE */
};

static const asm_opcode_info_t opcode_list[] = {
    /* 00 */ { "BRK",   ASM_ADDR_MODE_IMPLIED },
    /* 01 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 02 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 03 */ { "SLO",   ASM_ADDR_MODE_INDIRECT_X },
    /* 04 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE },
    /* 05 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 06 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 07 */ { "SLO",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 08 */ { "PHP",   ASM_ADDR_MODE_IMPLIED },
    /* 09 */ { "ORA",   ASM_ADDR_MODE_IMMEDIATE },
    /* 0a */ { "ASL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 0b */ { "ANC",   ASM_ADDR_MODE_IMMEDIATE },
    /* 0c */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE },
    /* 0d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 0f */ { "SLO",   ASM_ADDR_MODE_ABSOLUTE },

    /* 10 */ { "BPL",   ASM_ADDR_MODE_RELATIVE },
    /* 11 */ { "ORA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 12 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 13 */ { "SLO",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 14 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 15 */ { "ORA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 16 */ { "ASL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 17 */ { "SLO",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 18 */ { "CLC",   ASM_ADDR_MODE_IMPLIED },
    /* 19 */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 1a */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 1b */ { "SLO",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 1c */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1d */ { "ORA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1e */ { "ASL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 1f */ { "SLO",   ASM_ADDR_MODE_ABSOLUTE_X },

    /* 20 */ { "JSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 21 */ { "AND",   ASM_ADDR_MODE_INDIRECT_X },
    /* 22 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 23 */ { "RLA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 24 */ { "BIT",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 25 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 26 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 27 */ { "RLA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 28 */ { "PLP",   ASM_ADDR_MODE_IMPLIED },
    /* 29 */ { "AND",   ASM_ADDR_MODE_IMMEDIATE },
    /* 2a */ { "ROL",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 2b */ { "ANC",   ASM_ADDR_MODE_IMMEDIATE },
    /* 2c */ { "BIT",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE },
    /* 2f */ { "RLA",   ASM_ADDR_MODE_ABSOLUTE },

    /* 30 */ { "BMI",   ASM_ADDR_MODE_RELATIVE },
    /* 31 */ { "AND",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 32 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 33 */ { "RLA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 34 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 35 */ { "AND",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 36 */ { "ROL",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 37 */ { "RLA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 38 */ { "SEC",   ASM_ADDR_MODE_IMPLIED },
    /* 39 */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 3a */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 3b */ { "RLA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 3c */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3d */ { "AND",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3e */ { "ROL",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 3f */ { "RLA",   ASM_ADDR_MODE_ABSOLUTE_X },

    /* 40 */ { "RTI",   ASM_ADDR_MODE_IMPLIED },
    /* 41 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_X },
    /* 42 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 43 */ { "SRE",   ASM_ADDR_MODE_INDIRECT_X },
    /* 44 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE },
    /* 45 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 46 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 47 */ { "SRE",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 48 */ { "PHA",   ASM_ADDR_MODE_IMPLIED },
    /* 49 */ { "EOR",   ASM_ADDR_MODE_IMMEDIATE },
    /* 4a */ { "LSR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 4b */ { "ASR",   ASM_ADDR_MODE_IMMEDIATE },
    /* 4c */ { "JMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 4f */ { "SRE",   ASM_ADDR_MODE_ABSOLUTE },

    /* 50 */ { "BVC",   ASM_ADDR_MODE_RELATIVE },
    /* 51 */ { "EOR",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 52 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 53 */ { "SRE",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 54 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 55 */ { "EOR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 56 */ { "LSR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 57 */ { "SRE",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 58 */ { "CLI",   ASM_ADDR_MODE_IMPLIED },
    /* 59 */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 5a */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 5b */ { "SRE",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 5c */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5d */ { "EOR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5e */ { "LSR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 5f */ { "SRE",   ASM_ADDR_MODE_ABSOLUTE_X },

    /* 60 */ { "RTS",   ASM_ADDR_MODE_IMPLIED },
    /* 61 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_X },
    /* 62 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 63 */ { "RRA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 64 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE },
    /* 65 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 66 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 67 */ { "RRA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 68 */ { "PLA",   ASM_ADDR_MODE_IMPLIED },
    /* 69 */ { "ADC",   ASM_ADDR_MODE_IMMEDIATE },
    /* 6a */ { "ROR",   ASM_ADDR_MODE_ACCUMULATOR },
    /* 6b */ { "ARR",   ASM_ADDR_MODE_IMMEDIATE },
    /* 6c */ { "JMP",   ASM_ADDR_MODE_ABS_INDIRECT },
    /* 6d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE },
    /* 6f */ { "RRA",   ASM_ADDR_MODE_ABSOLUTE },

    /* 70 */ { "BVS",   ASM_ADDR_MODE_RELATIVE },
    /* 71 */ { "ADC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 72 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 73 */ { "RRA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 74 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 75 */ { "ADC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 76 */ { "ROR",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 77 */ { "RRA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 78 */ { "SEI",   ASM_ADDR_MODE_IMPLIED },
    /* 79 */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 7a */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* 7b */ { "RRA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 7c */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7d */ { "ADC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7e */ { "ROR",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 7f */ { "RRA",   ASM_ADDR_MODE_ABSOLUTE_X },

    /* 80 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 81 */ { "STA",   ASM_ADDR_MODE_INDIRECT_X },
    /* 82 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 83 */ { "SAX",   ASM_ADDR_MODE_INDIRECT_X },
    /* 84 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 85 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 86 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 87 */ { "SAX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* 88 */ { "DEY",   ASM_ADDR_MODE_IMPLIED },
    /* 89 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* 8a */ { "TXA",   ASM_ADDR_MODE_IMPLIED },
    /* 8b */ { "ANE",   ASM_ADDR_MODE_IMMEDIATE },
    /* 8c */ { "STY",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8e */ { "STX",   ASM_ADDR_MODE_ABSOLUTE },
    /* 8f */ { "SAX",   ASM_ADDR_MODE_ABSOLUTE },

    /* 90 */ { "BCC",   ASM_ADDR_MODE_RELATIVE },
    /* 91 */ { "STA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 92 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* 93 */ { "SHA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* 94 */ { "STY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 95 */ { "STA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* 96 */ { "STX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* 97 */ { "SAX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* 98 */ { "TYA",   ASM_ADDR_MODE_IMPLIED },
    /* 99 */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 9a */ { "TXS",   ASM_ADDR_MODE_IMPLIED },
    /* 9b */ { "SHS",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 9c */ { "SHY",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9d */ { "STA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* 9e */ { "SHX",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* 9f */ { "SHA",   ASM_ADDR_MODE_ABSOLUTE_Y },

    /* a0 */ { "LDY",   ASM_ADDR_MODE_IMMEDIATE },
    /* a1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_X },
    /* a2 */ { "LDX",   ASM_ADDR_MODE_IMMEDIATE },
    /* a3 */ { "LAX",   ASM_ADDR_MODE_INDIRECT_X },
    /* a4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a7 */ { "LAX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* a8 */ { "TAY",   ASM_ADDR_MODE_IMPLIED },
    /* a9 */ { "LDA",   ASM_ADDR_MODE_IMMEDIATE },
    /* aa */ { "TAX",   ASM_ADDR_MODE_IMPLIED },
    /* ab */ { "LXA",   ASM_ADDR_MODE_IMMEDIATE },
    /* ac */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE },
    /* ad */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE },
    /* ae */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE },
    /* af */ { "LAX",   ASM_ADDR_MODE_ABSOLUTE },

    /* b0 */ { "BCS",   ASM_ADDR_MODE_RELATIVE },
    /* b1 */ { "LDA",   ASM_ADDR_MODE_INDIRECT_Y },
    /* b2 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* b3 */ { "LAX",   ASM_ADDR_MODE_INDIRECT_Y },
    /* b4 */ { "LDY",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b5 */ { "LDA",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* b6 */ { "LDX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* b7 */ { "LAX",   ASM_ADDR_MODE_ZERO_PAGE_Y },
    /* b8 */ { "CLV",   ASM_ADDR_MODE_IMPLIED },
    /* b9 */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* ba */ { "TSX",   ASM_ADDR_MODE_IMPLIED },
    /* bb */ { "LAS",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* bc */ { "LDY",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* bd */ { "LDA",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* be */ { "LDX",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* bf */ { "LAX",   ASM_ADDR_MODE_ABSOLUTE_Y },

    /* c0 */ { "CPY",   ASM_ADDR_MODE_IMMEDIATE },
    /* c1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_X },
    /* c2 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* c3 */ { "DCP",   ASM_ADDR_MODE_INDIRECT_X },
    /* c4 */ { "CPY",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c7 */ { "DCP",   ASM_ADDR_MODE_ZERO_PAGE },
    /* c8 */ { "INY",   ASM_ADDR_MODE_IMPLIED },
    /* c9 */ { "CMP",   ASM_ADDR_MODE_IMMEDIATE },
    /* ca */ { "DEX",   ASM_ADDR_MODE_IMPLIED },
    /* cb */ { "SBX",   ASM_ADDR_MODE_IMMEDIATE },
    /* cc */ { "CPY",   ASM_ADDR_MODE_ABSOLUTE },
    /* cd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE },
    /* ce */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE },
    /* cf */ { "DCP",   ASM_ADDR_MODE_ABSOLUTE },

    /* d0 */ { "BNE",   ASM_ADDR_MODE_RELATIVE },
    /* d1 */ { "CMP",   ASM_ADDR_MODE_INDIRECT_Y },
    /* d2 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* d3 */ { "DCP",   ASM_ADDR_MODE_INDIRECT_Y },
    /* d4 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d5 */ { "CMP",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d6 */ { "DEC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d7 */ { "DCP",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* d8 */ { "CLD",   ASM_ADDR_MODE_IMPLIED },
    /* d9 */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* da */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* db */ { "DCP",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* dc */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* dd */ { "CMP",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* de */ { "DEC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* df */ { "DCP",   ASM_ADDR_MODE_ABSOLUTE_X },

    /* e0 */ { "CPX",   ASM_ADDR_MODE_IMMEDIATE },
    /* e1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_X },
    /* e2 */ { "NOOP",  ASM_ADDR_MODE_IMMEDIATE },
    /* e3 */ { "ISB",   ASM_ADDR_MODE_INDIRECT_X },
    /* e4 */ { "CPX",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e7 */ { "ISB",   ASM_ADDR_MODE_ZERO_PAGE },
    /* e8 */ { "INX",   ASM_ADDR_MODE_IMPLIED },
    /* e9 */ { "SBC",   ASM_ADDR_MODE_IMMEDIATE },
    /* ea */ { "NOP",   ASM_ADDR_MODE_IMPLIED },
    /* eb */ { "USBC",  ASM_ADDR_MODE_IMMEDIATE },
    /* ec */ { "CPX",   ASM_ADDR_MODE_ABSOLUTE },
    /* ed */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ee */ { "INC",   ASM_ADDR_MODE_ABSOLUTE },
    /* ef */ { "ISB",   ASM_ADDR_MODE_ABSOLUTE },

    /* f0 */ { "BEQ",   ASM_ADDR_MODE_RELATIVE },
    /* f1 */ { "SBC",   ASM_ADDR_MODE_INDIRECT_Y },
    /* f2 */ { "JAM",   ASM_ADDR_MODE_IMPLIED },
    /* f3 */ { "ISB",   ASM_ADDR_MODE_INDIRECT_Y },
    /* f4 */ { "NOOP",  ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f5 */ { "SBC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f6 */ { "INC",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f7 */ { "ISB",   ASM_ADDR_MODE_ZERO_PAGE_X },
    /* f8 */ { "SED",   ASM_ADDR_MODE_IMPLIED },
    /* f9 */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* fa */ { "NOOP",  ASM_ADDR_MODE_IMPLIED },
    /* fb */ { "ISB",   ASM_ADDR_MODE_ABSOLUTE_Y },
    /* fc */ { "NOOP",  ASM_ADDR_MODE_ABSOLUTE_X },
    /* fd */ { "SBC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* fe */ { "INC",   ASM_ADDR_MODE_ABSOLUTE_X },
    /* ff */ { "ISB",   ASM_ADDR_MODE_ABSOLUTE_X }
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

void asm6502_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->cpu_type = CPU_6502;
    monitor_cpu_type->asm_addr_mode_get_size = asm_addr_mode_get_size;
    monitor_cpu_type->asm_opcode_info_get = asm_opcode_info_get;

    /* Once we have a generic processor specific init, this will move.  */
    mon_assemble6502_init(monitor_cpu_type);
    mon_register6502_init(monitor_cpu_type);
}
