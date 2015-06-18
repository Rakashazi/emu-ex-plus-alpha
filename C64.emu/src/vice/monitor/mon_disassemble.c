/*
 * mon_disassemble.c - The VICE built-in monitor, disassembler module.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "log.h"
#include "mon_disassemble.h"
#include "mon_util.h"
#include "monitor.h"
#include "types.h"
#include "uimon.h"

#define SKIP_PREFIX() remove_6809_prefix(&prefix, opc)

static void remove_6809_prefix(int *prefix, BYTE *opc)
{
    if (opc[0] == 0x10 || opc[0] == 0x11) {
        ++*prefix;
    }
}

static char *get_6309_bitwise_reg(BYTE val)
{
    switch (val & 0xc0) {
        case 0x00:
            return "CC";
        case 0x80:
            return "A";
        case 0xC0:
            return "B";
    }
    return "?";
}

static char *reg6809[] = {
    "D", "X", "Y", "U", "S", "PC", "?R6", "?R7",
    "A", "B", "CC", "DP", "?RC", "?RD", "?RE", "?RF"
};

static char *reg6309[] = {
    "D", "X", "Y", "U", "S", "PC", "W", "V",
    "A", "B", "CC", "DP", "0", "0", "E", "F"
};

char index_reg6809[] = { 'X', 'Y', 'U', 'S' };

static const char *mon_disassemble_to_string_internal(MEMSPACE memspace,
                                                      unsigned int addr, BYTE opc[5],
                                                      int hex_mode, unsigned *opc_size_p,
                                                      monitor_cpu_type_t *mon_cpu_type)
{
    static char buff[256];
    const char *string;
    char *buffp, *addr_name;
    int addr_mode;
    unsigned opc_size;
    WORD ival;
    WORD ival2;
    const asm_opcode_info_t *opinfo;
    int prefix = 0;

#define x       opc[0]
#define p1      opc[1]
#define p2      opc[2]
#define p3      opc[3]
#define p4      opc[4]

    ival = (WORD)(p1 & 0xff);

    buffp = buff;

    if (!mon_cpu_type) {
        mon_cpu_type = monitor_cpu_for_memspace[memspace];
    }
    opinfo = (mon_cpu_type->asm_opcode_info_get)(x, p1, p2);
    string = opinfo->mnemonic;
    addr_mode = opinfo->addr_mode;
    opc_size = (mon_cpu_type->asm_addr_mode_get_size)((unsigned int)(addr_mode), x, p1, p2);


    if (opc_size_p) {
        *opc_size_p = opc_size;
    }

    switch (opc_size) {
        case 1:
            sprintf(buff, "%02X          %s", x, string);
            break;
        case 2:
            sprintf(buff, "%02X %02X       %s", x, p1 & 0xff, string);
            break;
        case 3:
            sprintf(buff, "%02X %02X %02X    %s", x, p1 & 0xff, p2 & 0xff, string);
            break;
        case 4:
            sprintf(buff, "%02X %02X %02X %02X %s", x, p1 & 0xff, p2 & 0xff, p3 & 0xff, string);
            break;
        case 5:
            sprintf(buff, "%02X%02X%02X %02X%02X %s", x, p1 & 0xff, p2 & 0xff, p3 & 0xff, p4 & 0xFF, string);
            break;
        default:
            mon_out("Invalid opcode length: %d\n", opc_size);
            sprintf(buff, "            %s", string);
    }

    while (*++buffp) {
    }

    /* Print arguments of the machine instruction. */
    switch (addr_mode) {
        case ASM_ADDR_MODE_IMPLIED:
            break;

        case ASM_ADDR_MODE_ACCUMULATOR:
            sprintf(buffp, " A");
            break;

        case ASM_ADDR_MODE_IMMEDIATE:
            sprintf(buffp, (hex_mode ? " #$%02X" : " #%3d"), ival);
            break;

        case ASM_ADDR_MODE_ZERO_PAGE:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%02X" : " %3d"), ival);
            } else {
                sprintf(buffp, " %s", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ZERO_PAGE_X:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%02X,X" : " %3d,X"), ival);
            } else {
                sprintf(buffp, " %s,X", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ZERO_PAGE_Y:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%02X,Y" : " %3d,Y"), ival);
            } else {
                sprintf(buffp, " %s,Y", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE:
            ival |= (WORD)((p2 & 0xff) << 8);
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " %s", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, (WORD)(ival - 1)))) {
                    sprintf(buffp, " %s+1", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " $%04X" : " %5d"), ival);
                }
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_LONG:
            ival |= (WORD)((p2 & 0xff) << 8);
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " %s", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space,
                                (WORD)(ival - 1)))) {
                    sprintf(buffp, " %s+1", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " $%06X" : " %7d"), ival | ((p3 & 0xff) << 16));
                }
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_LONG_X:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%06X,X" : " %7d,X"), ival | ((p3 & 0xff) << 16));
            } else {
                sprintf(buffp, " %s,X", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_X:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%04X,X" : " %5d,X"), ival);
            } else {
                sprintf(buffp, " %s,X", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_Y:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%04X,Y" : " %5d,Y"), ival);
            } else {
                sprintf(buffp, " %s,Y", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABS_INDIRECT:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " ($%04X)" : " (%5d)"), ival);
            } else {
                sprintf(buffp, " (%s)", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABS_IND_LONG:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " [$%04X]" : " [%5d]"), ival);
            } else {
                sprintf(buffp, " [%s]", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABS_INDIRECT_X:
            ival |= (WORD)((p2 & 0xff) << 8);
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " ($%04X,X)" : " (%5d,X)"), ival);
            } else {
                sprintf(buffp, " (%s,X)", addr_name);
            }
            break;

        case ASM_ADDR_MODE_INDIRECT_X:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " ($%02X,X)" : " (%3d,X)"), ival);
            } else {
                sprintf(buffp, " (%s,X)", addr_name);
            }
            break;

        case ASM_ADDR_MODE_INDIRECT_Y:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " ($%02X),Y" : " (%3d),Y"), ival);
            } else {
                sprintf(buffp, " (%s),Y", addr_name);
            }
            break;

        case ASM_ADDR_MODE_INDIRECT:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " ($%02X)" : " (%3d)"), ival);
            } else {
                sprintf(buffp, " (%s)", addr_name);
            }
            break;

        case ASM_ADDR_MODE_INDIRECT_LONG:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " [$%02X]" : " [%3d]"), ival);
            } else {
                sprintf(buffp, " [%s]", addr_name);
            }
            break;

        case ASM_ADDR_MODE_INDIRECT_LONG_Y:
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " [$%02X],Y" : " [%3d],Y"), ival);
            } else {
                sprintf(buffp, " [%s],Y", addr_name);
            }
            break;

        case ASM_ADDR_MODE_STACK_RELATIVE:
            sprintf(buffp, (hex_mode ? " $%02X,S" : " %3d,S"), ival);
            break;

        case ASM_ADDR_MODE_STACK_RELATIVE_Y:
            sprintf(buffp, (hex_mode ? " ($%02X,S),Y" : " (%3d,S),Y"), ival);
            break;

        case ASM_ADDR_MODE_MOVE:
            sprintf(buffp, (hex_mode ? " $%02X,$%02X" : " %3d,%3d"), p2 & 0xff, ival);
            break;

        case ASM_ADDR_MODE_RELATIVE:
            if (0x80 & ival) {
                ival -= 256;
            }
            ival += addr;
            ival += 2;
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%04X" : " %5d"), ival);
            } else {
                sprintf(buffp, " %s", addr_name);
            }
            break;

        case ASM_ADDR_MODE_RELATIVE_LONG:
            ival |= (p2 & 0xff) << 8;
#if 0
            /* useless subtraction since ival is 16bit */
            if (0x8000 & ival) {
                ival -= 65536;
            }
#endif
            ival += addr;
            ival += 3;
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, (hex_mode ? " $%04X" : " %5d"), ival);
            } else {
                sprintf(buffp, " %s", addr_name);
            }
            break;

        case ASM_ADDR_MODE_ZERO_PAGE_RELATIVE:
            ival2 = (p2 & 0xff);
            if (0x80 & ival2) {
                ival2 -= 256;
            }
            ival2 += addr;
            ival2 += 3;
            if (!(addr_name = mon_symbol_table_lookup_name(e_comp_space, ival2))) {
                sprintf(buffp, (hex_mode ? " $%02X, $%04X" : " %3d, %5d"), ival, ival2);
            } else {
                sprintf(buffp, (hex_mode ? " $%02X, %s" : " %3d, %s"), ival, addr_name);
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_A:
            ival |= (WORD)((p2 & 0xff) << 8);
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " (%s),A", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, (WORD)(ival - 1)))) {
                    sprintf(buffp, " (%s+1),A", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " ($%04X),A" : " (%5d),A"), ival);
                }
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_HL:
            ival |= (WORD)((p2 & 0xff) << 8);
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " (%s),HL", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, (WORD)(ival - 1)))) {
                    sprintf(buffp, " (%s+1),HL", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " ($%04X),HL" : " (%5d),HL"), ival);
                }
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_IX:
            ival = (WORD)((p2 & 0xff) | ((p3 & 0xff) << 8));
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " (%s),IX", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space,
                                                              (WORD)(ival - 1)))) {
                    sprintf(buffp, " (%s+1),IX", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " ($%04X),IX" : " (%5d),IX"), ival);
                }
            }
            break;

        case ASM_ADDR_MODE_ABSOLUTE_IY:
            ival = (WORD)((p2 & 0xff) | ((p3 & 0xff) << 8));
            if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, ival))) {
                sprintf(buffp, " (%s),IY", addr_name);
            } else {
                if ((addr_name = mon_symbol_table_lookup_name(e_comp_space, (WORD)(ival - 1)))) {
                    sprintf(buffp, " (%s+1),IY", addr_name);
                } else {
                    sprintf(buffp, (hex_mode ? " ($%04X),IY" : " (%5d),IY"), ival);
                }
            }
            break;

        case ASM_ADDR_MODE_IMMEDIATE_16:
            ival |= (WORD)((p2 & 0xff) << 8);
            sprintf(buffp, (hex_mode ? " #$%04X" : " #%5d"), ival);
            break;

        case ASM_ADDR_MODE_REG_B:
            sprintf(buffp, " B");
            break;

        case ASM_ADDR_MODE_REG_C:
            sprintf(buffp, " C");
            break;

        case ASM_ADDR_MODE_REG_D:
            sprintf(buffp, " D");
            break;

        case ASM_ADDR_MODE_REG_E:
            sprintf(buffp, " E");
            break;

        case ASM_ADDR_MODE_REG_H:
            sprintf(buffp, " H");
            break;

        case ASM_ADDR_MODE_REG_IXH:
            sprintf(buffp, " IXH");
            break;

        case ASM_ADDR_MODE_REG_IYH:
            sprintf(buffp, " IYH");
            break;

        case ASM_ADDR_MODE_REG_L:
            sprintf(buffp, " L");
            break;

        case ASM_ADDR_MODE_REG_IXL:
            sprintf(buffp, " IXL");
            break;

        case ASM_ADDR_MODE_REG_IYL:
            sprintf(buffp, " IYL");
            break;

        case ASM_ADDR_MODE_REG_AF:
            sprintf(buffp, " AF");
            break;

        case ASM_ADDR_MODE_REG_BC:
            sprintf(buffp, " BC");
            break;

        case ASM_ADDR_MODE_REG_DE:
            sprintf(buffp, " DE");
            break;

        case ASM_ADDR_MODE_REG_HL:
            sprintf(buffp, " HL");
            break;

        case ASM_ADDR_MODE_REG_IX:
            sprintf(buffp, " IX");
            break;

        case ASM_ADDR_MODE_REG_IY:
            sprintf(buffp, " IY");
            break;

        case ASM_ADDR_MODE_REG_SP:
            sprintf(buffp, " SP");
            break;

        case ASM_ADDR_MODE_REG_IND_BC:
            sprintf(buffp, " (BC)");
            break;

        case ASM_ADDR_MODE_REG_IND_DE:
            sprintf(buffp, " (DE)");
            break;

        case ASM_ADDR_MODE_REG_IND_HL:
            sprintf(buffp, " (HL)");
            break;

        case ASM_ADDR_MODE_REG_IND_IX:
            sprintf(buffp, " (IX)");
            break;

        case ASM_ADDR_MODE_REG_IND_IY:
            sprintf(buffp, " (IY)");
            break;

        case ASM_ADDR_MODE_REG_IND_SP:
            sprintf(buffp, " (SP)");
            break;

        /* 6809 modes */
        case ASM_ADDR_MODE_ILLEGAL:
            break;

        case ASM_ADDR_MODE_IMM_BYTE:
            SKIP_PREFIX();
            sprintf(buffp, " #$%02X", opc[prefix + 1]);
            break;

        case ASM_ADDR_MODE_IMM_WORD:
            SKIP_PREFIX();
            ival = (opc[prefix + 1] << 8) + opc[prefix + 2];
            sprintf(buffp, " #$%04X", ival);
            break;

        case ASM_ADDR_MODE_IMM_DWORD:
            SKIP_PREFIX();
            ival = (opc[prefix + 1] << 24) + (opc[prefix + 2] << 16) + (opc[prefix + 3] << 8) + opc[prefix + 4];
            sprintf(buffp, " #$%08X", ival);
            break;

        case ASM_ADDR_MODE_DIRECT:
            SKIP_PREFIX();
            sprintf(buffp, " <$%02X", opc[prefix + 1]);
            break;

        case ASM_ADDR_MODE_IM_DIRECT:
            SKIP_PREFIX();
            sprintf(buffp, " #$%02X,<$%02X", opc[prefix + 1], opc[prefix + 2]);
            break;

        case ASM_ADDR_MODE_EXTENDED:
            SKIP_PREFIX();
            ival = (opc[prefix + 1] << 8) + opc[prefix + 2];
            sprintf(buffp, " $%04X", ival);
            break;

        case ASM_ADDR_MODE_IM_EXTENDED:
            SKIP_PREFIX();
            ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
            sprintf(buffp, " #$%02X,$%04X", opc[prefix + 1], ival);
            break;

        case ASM_ADDR_MODE_INDEXED:     /* post-byte determines sub-mode */
            {
                char R;
                SKIP_PREFIX();
                ival = opc[prefix + 1];
                R = index_reg6809[(ival >> 5) & 3];

                if ((ival & 0x80) == 0) {
                    int offset = ival & 0x1F;
                    if (offset & 0x10) {        /* sign extend 5-bit value */
                        offset -= 0x20;
                    }
                    sprintf(buffp, " %d,%c", offset, R);
                    break;
                }

                switch (ival & 0x1f) {
                    /* ASM_ADDR_MODE_INDEXED_INC1   0x00*/
                    case 0x00:
                        sprintf(buffp, " ,%c+", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2   0x01*/
                    case 0x01:
                        sprintf(buffp, " ,%c++", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC1      0x02*/
                    case 0x02:
                        sprintf(buffp, " ,-%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2   0x03*/
                    case 0x03:
                        sprintf(buffp, " ,--%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0   0x04*/
                    case 0x04:
                        sprintf(buffp, " ,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB   0x05*/
                    case 0x05:
                        sprintf(buffp, " B,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA   0x06*/
                    case 0x06:
                        sprintf(buffp, " A,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8   0x08*/
                    case 0x08:      /* TODO should be signed! */
                        sprintf(buffp, " $%02X,%c", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16  0x09*/
                    case 0x09:      /* TODO should signed! */
                        sprintf(buffp, " $%04X,%c", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD   0x0B*/
                    case 0x0B:
                        sprintf(buffp, " D,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8 0x0C*/
                    case 0x0C:
                        sprintf(buffp, " $%04X,PCR /* $%02X,PC */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16        0x0D*/
                    case 0x0D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " $%04X,PCR /* $%04X,PC */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2_IND       0x11*/
                    case 0x11:
                        sprintf(buffp, " [,%c++]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2_IND       0x13*/
                    case 0x13:
                        sprintf(buffp, " [,--%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0_IND       0x14*/
                    case 0x14:
                        sprintf(buffp, " [,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB_IND       0x15*/
                    case 0x15:
                        sprintf(buffp, " [B,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA_IND       0x16*/
                    case 0x16:
                        sprintf(buffp, " [A,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8_IND       0x18*/
                    case 0x18:
                        sprintf(buffp, " [$%02X,%c]", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16_IND 0x19*/
                    case 0x19:
                        sprintf(buffp, " [$%04X,%c]", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD_IND       0x1B*/
                    case 0x1B:
                        sprintf(buffp, " [D,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8_IND   0x1C*/
                    case 0x1C:
                        sprintf(buffp, " [$%04X,PCR] /* [$%02X,PC] */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16_IND  0x1D*/
                    case 0x1D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " [$%04X,PCR] /* [$%04X,PC] */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_EXTENDED_INDIRECT    0x1F*/
                    case 0x1F:
                        sprintf(buffp, " [$%04X]", ((opc[prefix + 2] << 8) + opc[prefix + 3]));
                        break;

                    /* ASM_ADDR_MODE_INDEXED_07     0x07*/
                    /* ASM_ADDR_MODE_INDEXED_0A     0x0A*/
                    /* ASM_ADDR_MODE_INDEXED_0E     0x0E*/
                    /* ASM_ADDR_MODE_INDEXED_0F     0x0F*/
                    /* ASM_ADDR_MODE_INDEXED_10     0x10*/
                    /* ASM_ADDR_MODE_INDEXED_12     0x12*/
                    /* ASM_ADDR_MODE_INDEXED_17     0x17*/
                    /* ASM_ADDR_MODE_INDEXED_1A     0x1A*/
                    /* ASM_ADDR_MODE_INDEXED_1E     0x1E*/
                    default:
                        sprintf(buffp, " ???");
                        break;
                }
            }
            break;

        case ASM_ADDR_MODE_F6809_INDEXED:     /* post-byte determines sub-mode */
            {
                char R;
                SKIP_PREFIX();
                ival = opc[prefix + 1];
                R = index_reg6809[(ival >> 5) & 3];

                if ((ival & 0x80) == 0) {
                    int offset = ival & 0x1F;
                    if (offset & 0x10) {        /* sign extend 5-bit value */
                        offset -= 0x20;
                    }
                    sprintf(buffp, " %d,%c", offset, R);
                    break;
                }

                switch (ival & 0x1f) {
                    /* ASM_ADDR_MODE_INDEXED_INC1   0x00*/
                    case 0x00:
                        sprintf(buffp, " ,%c+", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2   0x01*/
                    case 0x01:
                        sprintf(buffp, " ,%c++", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC1      0x02*/
                    case 0x02:
                        sprintf(buffp, " ,-%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2   0x03*/
                    case 0x03:
                        sprintf(buffp, " ,--%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0       0x04*/
                    /* ASM_ADDR_MODE_INDEXED_F6809_OFF0 0x07*/
                    case 0x04:
                    case 0x07:
                        sprintf(buffp, " ,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB   0x05*/
                    case 0x05:
                        sprintf(buffp, " B,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA   0x06*/
                    case 0x06:
                        sprintf(buffp, " A,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8   0x08*/
                    case 0x08:      /* TODO should be signed! */
                        sprintf(buffp, " $%02X,%c", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16  0x09*/
                    case 0x09:      /* TODO should signed! */
                        sprintf(buffp, " $%04X,%c", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_F6809_OFFPCORFF 0x0A*/
                    case 0x0A:
                        sprintf(buffp, " PCL");
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD   0x0B*/
                    case 0x0B:
                        sprintf(buffp, " D,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8 0x0C*/
                    case 0x0C:
                        sprintf(buffp, " $%04X,PCR /* $%02X,PC */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16        0x0D*/
                    case 0x0D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " $%04X,PCR /* $%04X,PC */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_F6809_EXTENDED 0x0F*/
                    case 0x0F:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " $%04X", ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_F6809_INC1_IND 0x10*/
                    case 0x10:
                        sprintf(buffp, " [,%c+]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2_IND       0x11*/
                    case 0x11:
                        sprintf(buffp, " [,%c++]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_F6809_DEC1_IND 0x12*/
                    case 0x12:
                        sprintf(buffp, " [,-%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2_IND       0x13*/
                    case 0x13:
                        sprintf(buffp, " [,--%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0_IND       0x14*/
                    /* ASM_ADDR_MODE_INDEXED_F6809_OFF0_IND 0x17*/
                    case 0x14:
                    case 0x17:
                        sprintf(buffp, " [,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB_IND       0x15*/
                    case 0x15:
                        sprintf(buffp, " [B,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA_IND       0x16*/
                    case 0x16:
                        sprintf(buffp, " [A,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8_IND       0x18*/
                    case 0x18:
                        sprintf(buffp, " [$%02X,%c]", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16_IND 0x19*/
                    case 0x19:
                        sprintf(buffp, " [$%04X,%c]", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_F6809_OFFPCORFF_IND 0x1A*/
                    case 0x1A:
                        sprintf(buffp, " [PCL]");
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD_IND       0x1B*/
                    case 0x1B:
                        sprintf(buffp, " [D,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8_IND   0x1C*/
                    case 0x1C:
                        sprintf(buffp, " [$%04X,PCR] /* [$%02X,PC] */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16_IND  0x1D*/
                    case 0x1D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " [$%04X,PCR] /* [$%04X,PC] */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_EXTENDED_INDIRECT    0x1F*/
                    case 0x1F:
                        sprintf(buffp, " [$%04X]", ((opc[prefix + 2] << 8) + opc[prefix + 3]));
                        break;

                    /* ASM_ADDR_MODE_INDEXED_0E     0x0E*/
                    /* ASM_ADDR_MODE_INDEXED_1E     0x1E*/
                    default:
                        sprintf(buffp, " ???");
                        break;
                }
            }
            break;

        case ASM_ADDR_MODE_H6309_INDEXED:     /* post-byte determines sub-mode */
            {
                char R;
                SKIP_PREFIX();
                ival = opc[prefix + 1];
                R = index_reg6809[(ival >> 5) & 3];

                if ((ival & 0x80) == 0) {
                    int offset = ival & 0x1F;
                    if (offset & 0x10) {        /* sign extend 5-bit value */
                        offset -= 0x20;
                    }
                    sprintf(buffp, " %d,%c", offset, R);
                    break;
                }

                switch (ival & 0x1f) {
                    /* ASM_ADDR_MODE_INDEXED_INC1   0x00*/
                    case 0x00:
                        sprintf(buffp, " ,%c+", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2   0x01*/
                    case 0x01:
                        sprintf(buffp, " ,%c++", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC1      0x02*/
                    case 0x02:
                        sprintf(buffp, " ,-%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2   0x03*/
                    case 0x03:
                        sprintf(buffp, " ,--%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0       0x04*/
                    case 0x04:
                        sprintf(buffp, " ,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB   0x05*/
                    case 0x05:
                        sprintf(buffp, " B,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA   0x06*/
                    case 0x06:
                        sprintf(buffp, " A,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFE 0x07*/
                    case 0x07:
                        sprintf(buffp, " E,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8   0x08*/
                    case 0x08:      /* TODO should be signed! */
                        sprintf(buffp, " $%02X,%c", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16  0x09*/
                    case 0x09:      /* TODO should signed! */
                        sprintf(buffp, " $%04X,%c", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFF 0x0A*/
                    case 0x0A:
                        sprintf(buffp, " F,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD   0x0B*/
                    case 0x0B:
                        sprintf(buffp, " D,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8 0x0C*/
                    case 0x0C:
                        sprintf(buffp, " $%04X,PCR /* $%02X,PC */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16        0x0D*/
                    case 0x0D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " $%04X,PCR /* $%04X,PC */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFW 0x0E*/
                    case 0x0E:
                        sprintf(buffp, " W,%c", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFWREL 0x0F*/
                    case 0x0F:
                        switch (ival & 0x60) {
                            /* ,W */
                            case 0x00:
                                sprintf(buffp, " ,W");
                                break;

                            /* 16bit,W */
                            case 0x20:
                                sprintf(buffp, " $%04X,W", (opc[prefix + 2] << 8) + opc[prefix + 3]);
                                break;

                            /* ,W++ */
                            case 0x40:
                                sprintf(buffp, " ,W++");
                                break;

                            /* ,--W */
                            case 0x60:
                                sprintf(buffp, " ,--W");
                                break;
                        }
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFWREL_IND 0x10*/
                    case 0x10:
                        switch (ival & 0x60) {
                            /* [,W] */
                            case 0x00:
                                sprintf(buffp, " [,W]");
                                break;

                            /* [16bit,W] */
                            case 0x20:
                                sprintf(buffp, " [$%04X,W]", (opc[prefix + 2] << 8) + opc[prefix + 3]);
                                break;

                            /* [,W++] */
                            case 0x40:
                                sprintf(buffp, " [,W++]");
                                break;

                            /* [,--W] */
                            case 0x60:
                                sprintf(buffp, " [,--W]");
                                break;
                        }
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2_IND       0x11*/
                    case 0x11:
                        sprintf(buffp, " [,%c++]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_DEC1_IND 0x12*/
                    case 0x12:
                        sprintf(buffp, " [,-%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2_IND       0x13*/
                    case 0x13:
                        sprintf(buffp, " [,--%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0_IND       0x14*/
                    case 0x14:
                        sprintf(buffp, " [,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB_IND       0x15*/
                    case 0x15:
                        sprintf(buffp, " [B,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA_IND       0x16*/
                    case 0x16:
                        sprintf(buffp, " [A,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFE_IND 0x17*/
                    case 0x17:
                        sprintf(buffp, " [E,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8_IND       0x18*/
                    case 0x18:
                        sprintf(buffp, " [$%02X,%c]", opc[prefix + 2], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16_IND 0x19*/
                    case 0x19:
                        sprintf(buffp, " [$%04X,%c]", (opc[prefix + 2] << 8) + opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFF_IND 0x1A*/
                    case 0x1A:
                        sprintf(buffp, " [F,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD_IND       0x1B*/
                    case 0x1B:
                        sprintf(buffp, " [D,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8_IND   0x1C*/
                    case 0x1C:
                        sprintf(buffp, " [$%04X,PCR] /* [$%02X,PC] */", (SIGNED_CHAR)opc[prefix + 2] + addr + opc_size, opc[prefix + 2]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16_IND  0x1D*/
                    case 0x1D:
                        ival = (opc[prefix + 2] << 8) + opc[prefix + 3];
                        sprintf(buffp, " [$%04X,PCR] /* [$%04X,PC] */", (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFW_IND 0x1E*/
                    case 0x1E:
                        sprintf(buffp, " [W,%c]", R);
                        break;

                    /* ASM_ADDR_MODE_EXTENDED_INDIRECT    0x1F*/
                    case 0x1F:
                        sprintf(buffp, " [$%04X]", ((opc[prefix + 2] << 8) + opc[prefix + 3]));
                        break;
                }
            }
            break;

        case ASM_ADDR_MODE_IM_INDEXED:     /* post-byte determines sub-mode */
            {
                char R;
                SKIP_PREFIX();
                ival = opc[prefix + 2];
                R = index_reg6809[(ival >> 5) & 3];

                if ((ival & 0x80) == 0) {
                    int offset = ival & 0x1F;
                    if (offset & 0x10) {        /* sign extend 5-bit value */
                        offset -= 0x20;
                    }
                    sprintf(buffp, " #$%02X,%d,%c", opc[prefix + 1], offset, R);
                    break;
                }

                switch (ival & 0x1f) {
                    /* ASM_ADDR_MODE_INDEXED_INC1   0x00*/
                    case 0x00:
                        sprintf(buffp, " #$%02X,%c+", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2   0x01*/
                    case 0x01:
                        sprintf(buffp, " #$%02X,%c++", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC1      0x02*/
                    case 0x02:
                        sprintf(buffp, " #$%02X,-%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2   0x03*/
                    case 0x03:
                        sprintf(buffp, " #$%02X,--%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0       0x04*/
                    case 0x04:
                        sprintf(buffp, " #$%02X,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB   0x05*/
                    case 0x05:
                        sprintf(buffp, " #$%02X,B,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA   0x06*/
                    case 0x06:
                        sprintf(buffp, " #$%02X,A,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFE 0x07*/
                    case 0x07:
                        sprintf(buffp, " #$%02X,E,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8   0x08*/
                    case 0x08:      /* TODO should be signed! */
                        sprintf(buffp, " #$%02X,$%02X,%c", opc[prefix + 1], opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16  0x09*/
                    case 0x09:      /* TODO should signed! */
                        sprintf(buffp, " #$%02X,$%04X,%c", opc[prefix + 1], (opc[prefix + 3] << 8) + opc[prefix + 4], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFF 0x0A*/
                    case 0x0A:
                        sprintf(buffp, " #$%02X,F,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD   0x0B*/
                    case 0x0B:
                        sprintf(buffp, " #$%02X,D,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8 0x0C*/
                    case 0x0C:
                        sprintf(buffp, " #$%02X,$%04X,PCR /* $%02X,PC */", opc[prefix + 1], (SIGNED_CHAR)opc[prefix + 3] + addr + opc_size, opc[prefix + 3]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16        0x0D*/
                    case 0x0D:
                        ival = (opc[prefix + 3] << 8) + opc[prefix + 4];
                        sprintf(buffp, " #$%02X,$%04X,PCR /* $%04X,PC */", opc[prefix + 1], (WORD)(ival + addr + opc_size), ival);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFW 0x0E*/
                    case 0x0E:
                        sprintf(buffp, " #$%02X,W,%c", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFWREL 0x0F*/
                    case 0x0F:
                        switch (ival & 0x60) {
                            /* ,W */
                            case 0x00:
                                sprintf(buffp, " #$%02X,W", opc[prefix + 1]);
                                break;

                            /* 16bit,W */
                            case 0x20:
                                sprintf(buffp, " #$%02X,$%04X,W", opc[prefix + 1], (opc[prefix + 3] << 8) + opc[prefix + 4]);
                                break;

                            /* ,W++ */
                            case 0x40:
                                sprintf(buffp, " #$%02X,W++", opc[prefix + 1]);
                                break;

                            /* ,--W */
                            case 0x60:
                                sprintf(buffp, " #$%02X,--W", opc[prefix + 1]);
                                break;
                        }
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFWREL_IND 0x10*/
                    case 0x10:
                        switch (ival & 0x60) {
                            /* [,W] */
                            case 0x00:
                                sprintf(buffp, " #$%02X,[,W]", opc[prefix + 1]);
                                break;

                            /* [16bit,W] */
                            case 0x20:
                                sprintf(buffp, " #$%02X,[$%04X,W]", opc[prefix + 1], (opc[prefix + 3] << 8) + opc[prefix + 4]);
                                break;

                            /* [,W++] */
                            case 0x40:
                                sprintf(buffp, " #$%02X,[,W++]", opc[prefix + 1]);
                                break;

                            /* [,--W] */
                            case 0x60:
                                sprintf(buffp, " #$%02X,[,--W]", opc[prefix + 1]);
                                break;
                        }
                        break;

                    /* ASM_ADDR_MODE_INDEXED_INC2_IND       0x11*/
                    case 0x11:
                        sprintf(buffp, " #$%02X,[,%c++]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_DEC1_IND 0x12*/
                    case 0x12:
                        sprintf(buffp, " #$%02X,[,-%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_DEC2_IND       0x13*/
                    case 0x13:
                        sprintf(buffp, " #$%02X,[,--%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF0_IND       0x14*/
                    case 0x14:
                        sprintf(buffp, " #$%02X,[,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFB_IND       0x15*/
                    case 0x15:
                        sprintf(buffp, " #$%02X,[B,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFA_IND       0x16*/
                    case 0x16:
                        sprintf(buffp, " #$%02X,[A,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFE_IND 0x17*/
                    case 0x17:
                        sprintf(buffp, " #$%02X,[E,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF8_IND       0x18*/
                    case 0x18:
                        sprintf(buffp, " #$%02X,[$%02X,%c]", opc[prefix + 1], opc[prefix + 3], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFF16_IND 0x19*/
                    case 0x19:
                        sprintf(buffp, " #$%02X,[$%04X,%c]", opc[prefix + 1], (opc[prefix + 3] << 8) + opc[prefix + 4], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFF_IND 0x1A*/
                    case 0x1A:
                        sprintf(buffp, " #$%02X,[F,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFD_IND       0x1B*/
                    case 0x1B:
                        sprintf(buffp, " #$%02X,[D,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC8_IND   0x1C*/
                    case 0x1C:
                        sprintf(buffp, " #$%02X,[$%04X,PCR] /* [$%02X,PC] */", opc[prefix + 1], (SIGNED_CHAR)opc[prefix + 3] + addr + opc_size, opc[prefix + 3]);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_OFFPC16_IND  0x1D*/
                    case 0x1D:
                        ival = (opc[prefix + 3] << 8) + opc[prefix + 4];
                        sprintf(buffp, " #$%02X,[$%04X,PCR] /* [$%04X,PC] */", (WORD)(ival + addr + opc_size), ival, 0 /*FIXME*/);
                        break;

                    /* ASM_ADDR_MODE_INDEXED_H6309_OFFW_IND 0x1E*/
                    case 0x1E:
                        sprintf(buffp, " #$%02X,[W,%c]", opc[prefix + 1], R);
                        break;

                    /* ASM_ADDR_MODE_EXTENDED_INDIRECT    0x1F*/
                    case 0x1F:
                        sprintf(buffp, " #$%02X,[$%04X]", opc[prefix + 1], ((opc[prefix + 3] << 8) + opc[prefix + 4]));
                        break;
                }
            }
            break;

        case ASM_ADDR_MODE_BITWISE:
            SKIP_PREFIX();
            sprintf(buffp, " %s,%d,%d,<$%02X", get_6309_bitwise_reg(opc[prefix + 1]), (opc[prefix + 1] & 0x38) >> 3, opc[prefix + 1] & 7, opc[prefix + 2]);
            break;

        case ASM_ADDR_MODE_REL_BYTE:
            SKIP_PREFIX();
            sprintf(buffp, " $%04X", (SIGNED_CHAR)opc[prefix + 1] + addr + opc_size);
            break;

        case ASM_ADDR_MODE_REL_WORD:
            SKIP_PREFIX();
            sprintf(buffp, " $%04X", (WORD)((opc[prefix + 1] << 8) + opc[prefix + 2] + addr + opc_size));
            break;

        case ASM_ADDR_MODE_REG_POST:
            SKIP_PREFIX();
            sprintf(buffp, " %s,%s", reg6809[opc[prefix + 1] >> 4], reg6809[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_H6309_REG_POST:
            SKIP_PREFIX();
            sprintf(buffp, " %s,%s", reg6309[opc[prefix + 1] >> 4], reg6309[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_TFM_PP:
            SKIP_PREFIX();
            sprintf(buffp, " %s+,%s+", reg6309[opc[prefix + 1] >> 4], reg6309[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_TFM_MM:
            SKIP_PREFIX();
            sprintf(buffp, " %s-,%s-", reg6309[opc[prefix + 1] >> 4], reg6309[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_TFM_PC:
            SKIP_PREFIX();
            sprintf(buffp, " %s+,%s", reg6309[opc[prefix + 1] >> 4], reg6309[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_TFM_CP:
            SKIP_PREFIX();
            sprintf(buffp, " %s,%s+", reg6309[opc[prefix + 1] >> 4], reg6309[opc[prefix + 1] & 15]);
            break;

        case ASM_ADDR_MODE_SYS_POST:
        case ASM_ADDR_MODE_USR_POST:
            SKIP_PREFIX();
            ival = opc[prefix + 1];
            strcat(buffp, " ");
            if (ival & 0x80) {
                strcat(buffp, "PC,");
            }
            if (ival & 0x40) {
                strcat(buffp, addr_mode == ASM_ADDR_MODE_USR_POST ? "S," : "U,");
            }
            if (ival & 0x20) {
                strcat(buffp, "Y,");
            }
            if (ival & 0x10) {
                strcat(buffp, "X,");
            }
            if (ival & 0x08) {
                strcat(buffp, "DP,");
            }
            if ((ival & 0x06) == 0x06) {
                strcat(buffp, "D,");
            } else {
                if (ival & 0x04) {
                    strcat(buffp, "B,");
                }
                if (ival & 0x02) {
                    strcat(buffp, "A,");
                }
            }
            if (ival & 0x01) {
                strcat(buffp, "CC,");
            }
            buffp[strlen(buffp) - 1] = '\0';
            break;
        case ASM_ADDR_MODE_DOUBLE: /* not a real addressing mode */
            break;
    }

    return buff;
}

#undef x
#undef p1
#undef p2
#undef p3
#undef p4

static const char* mon_disassemble_instr_interal(unsigned *opc_size, MON_ADDR addr)
{
    static char buff[256];
    BYTE opc[5];
    MEMSPACE mem;
    WORD loc;
    int hex_mode = 1;
    const char *dis_inst;

    mem = addr_memspace(addr);
    loc = addr_location(addr);

    opc[0] = mon_get_mem_val(mem, loc);
    opc[1] = mon_get_mem_val(mem, (WORD)(loc + 1));
    opc[2] = mon_get_mem_val(mem, (WORD)(loc + 2));
    opc[3] = mon_get_mem_val(mem, (WORD)(loc + 3));
    opc[4] = mon_get_mem_val(mem, (WORD)(loc + 4));

    dis_inst = mon_disassemble_to_string_internal(mem, loc, opc, hex_mode, opc_size, monitor_cpu_for_memspace[mem]);

    sprintf(buff, ".%s:%04x  %s", mon_memspace_string[mem], loc, dis_inst);

    return buff;
}

const char *mon_disassemble_to_string(MEMSPACE memspace, unsigned int addr,
                                      unsigned int x, unsigned int p1, unsigned int p2, unsigned int p3,
                                      int hex_mode, const char *cpu_type)
{
    BYTE opc[5];

    opc[0] = x;
    opc[1] = p1;
    opc[2] = p2;
    opc[3] = p3;
    opc[4] = 0;

    return mon_disassemble_to_string_internal(memspace, addr, opc, hex_mode, NULL, monitor_find_cpu_type_from_string(cpu_type));
}

const char *mon_disassemble_to_string_ex(MEMSPACE memspace, unsigned int addr,
                                         unsigned int x, unsigned int p1, unsigned int p2, unsigned int p3,
                                         int hex_mode, unsigned *opc_size_p)
{
    BYTE opc[5];

    opc[0] = x;
    opc[1] = p1;
    opc[2] = p2;
    opc[3] = p3;
    opc[4] = 0;

    return mon_disassemble_to_string_internal(memspace, addr, opc, hex_mode, opc_size_p, monitor_cpu_for_memspace[memspace]);
}


unsigned mon_disassemble_instr(MON_ADDR addr)
{
    MEMSPACE mem;
    WORD loc;
    char *label;
    unsigned opc_size;

    mem = addr_memspace(addr);
    loc = addr_location(addr);

    /* Print the label for this location - if we have one */
    label = mon_symbol_table_lookup_name(mem, loc);
    if (label) {
        mon_out(".%s:%04x   %s:\n", mon_memspace_string[mem], loc, label);
    }

    /* Print the disassembled instruction */
    mon_out("%s\n", mon_disassemble_instr_interal(&opc_size, addr));

    return opc_size;
}


void mon_disassemble_with_regdump(MEMSPACE mem, unsigned int addr)
{
    monitor_cpu_type_t *monitor_cpu;
    const char *dis_inst;
    unsigned opc_size;

    monitor_cpu = monitor_cpu_for_memspace[mem];

    dis_inst = mon_disassemble_instr_interal(&opc_size, addr);
    if (monitor_cpu->mon_register_print_ex) {
        mon_out("%-35s - %s ", dis_inst, monitor_cpu->mon_register_print_ex(mem));
    } else {
        mon_out("%s ", dis_inst);
    }
    mon_stopwatch_show("", "\n");
}


void mon_disassemble_lines(MON_ADDR start_addr, MON_ADDR end_addr)
{
    MEMSPACE mem;
    long len, i, bytes;

    len = mon_evaluate_address_range(&start_addr, &end_addr, FALSE, DEFAULT_DISASSEMBLY_SIZE);

    if (len < 0) {
        log_error(LOG_ERR, "Invalid address range");
        return;
    }

    mem = addr_memspace(start_addr);
    dot_addr[mem] = start_addr;

    i = 0;
    while (i <= len) {
        bytes = mon_disassemble_instr(dot_addr[mem]);
        i += bytes;
        mon_inc_addr_location(&(dot_addr[mem]), bytes);
        if (mon_stop_output != 0) {
            break;
        }
    }
}
