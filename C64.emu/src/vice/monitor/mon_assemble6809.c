/*
 * mon_assemble6809.c - The VICE built-in monitor, 6809 assembler module.
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

#include <string.h>
#include <stdio.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "asm.h"
#include "montypes.h"
#include "mon_assemble.h"
#include "mon_util.h"
#include "types.h"
#include "uimon.h"
#include "util.h"

#define DBG(x)

static int make_offset_mode(int offset)
{
    if (offset >= -16 && offset < 16) {
        return offset & 0x1F;
    } else if (offset >= -128 && offset < 128) {
        return 0x80 | ASM_ADDR_MODE_INDEXED_OFF8;
    } else if (offset >= -32768 && offset < 32768) {
        return 0x80 | ASM_ADDR_MODE_INDEXED_OFF16;
    } else {
        mon_out("offset too large even for 16 bits (signed)\n");
        return 0x80 | ASM_ADDR_MODE_INDEXED_OFF16;
    }
}

static int mon_assemble_instr(const char *opcode_name, asm_mode_addr_info_t operand)
{
    WORD operand_value = operand.param;
    WORD operand_mode = operand.addr_mode;
    WORD operand_submode = operand.addr_submode;
    BYTE i = 0, j = 0, opcode = 0;
    int len, branch_offset;
    bool found = FALSE;
    MEMSPACE mem;
    WORD loc;
    int const prefix[3] = { -1, 0x10, 0x11 };
    BYTE opc[5];
    int opc_offset;
    int prefixlen;

    mem = addr_memspace(asm_mode_addr);
    loc = addr_location(asm_mode_addr);

    /*
     * Convert generic addressing modes to 6809 addressing modes.
     * The reason we have different numbers for them is that the disassembler
     * needs to skip prefix bytes in a 6809-specific manner.
     */
    switch (operand_mode) {
        case ASM_ADDR_MODE_IMMEDIATE:
            operand_mode = ASM_ADDR_MODE_IMM_BYTE;
            break;
        case ASM_ADDR_MODE_IMMEDIATE_16:
            operand_mode = ASM_ADDR_MODE_IMM_WORD;
            break;
        case ASM_ADDR_MODE_ZERO_PAGE:   /* we have no zero page */
            operand_mode = ASM_ADDR_MODE_EXTENDED;
            break;
        case ASM_ADDR_MODE_ABSOLUTE:
            operand_mode = ASM_ADDR_MODE_EXTENDED;
            break;
    }
    /*
     * Fix up another parsing ambiguity, for addr,X and addr,Y and addr,S.
     */
    if (operand_mode == ASM_ADDR_MODE_ZERO_PAGE_X ||
        operand_mode == ASM_ADDR_MODE_ABSOLUTE_X) {
        int reg = 0 << 5;
        operand_mode = ASM_ADDR_MODE_INDEXED;
        operand_submode = reg | make_offset_mode(operand_value);
    } else if (operand_mode == ASM_ADDR_MODE_ZERO_PAGE_Y ||
               operand_mode == ASM_ADDR_MODE_ABSOLUTE_Y) {
        int reg = 1 << 5;
        operand_mode = ASM_ADDR_MODE_INDEXED;
        operand_submode = reg | make_offset_mode(operand_value);
    } else if (operand_mode == ASM_ADDR_MODE_STACK_RELATIVE) {
        int reg = 3 << 5;
        operand_mode = ASM_ADDR_MODE_INDEXED;
        operand_submode = reg | make_offset_mode(operand_value);
    }

    DBG(printf("mon_assemble_instr: '%s' mode %d submode $%02x oper $%04x\n",
               opcode_name, operand_mode, operand_submode, operand_value));

    for (j = 0; j < 3 && !found; j++) {
        do {
            const asm_opcode_info_t *opinfo;

            if (prefix[j] == -1) {
                opinfo = (monitor_cpu_for_memspace[mem]->asm_opcode_info_get)(i, 0, 0);
                prefixlen = 0;
            } else {
                opinfo = (monitor_cpu_for_memspace[mem]->asm_opcode_info_get)(prefix[j], i, 0);
                prefixlen = 1;
            }

            if (!strcasecmp(opinfo->mnemonic, opcode_name)) {
                if (opinfo->addr_mode == operand_mode) {
                    opcode = i;
                    found = TRUE;
                    DBG(printf("found, prefix $%02x opcode $%02x\n", prefix[j], opcode));
                    break;
                }


                /* Special case: RELATIVE mode looks like EXTENDED mode.  */
                if (operand_mode == ASM_ADDR_MODE_EXTENDED
                    && opinfo->addr_mode == ASM_ADDR_MODE_REL_BYTE) {
                    branch_offset = (operand_value - (loc + prefixlen + 2)) & 0xffff;
                    if (branch_offset > 0x7f && branch_offset < 0xff80) {
                        mon_out("Branch offset too large.\n");
                        return -1;
                    }
                    operand_value = (branch_offset & 0xff);
                    operand_mode = ASM_ADDR_MODE_REL_BYTE;
                    opcode = i;
                    found = TRUE;
                    break;
                }

                /* Special case: RELATIVE mode looks like EXTENDED mode.  */
                if (operand_mode == ASM_ADDR_MODE_EXTENDED
                    && opinfo->addr_mode == ASM_ADDR_MODE_REL_WORD) {
                    branch_offset = operand_value - (loc + prefixlen + 3);
                    operand_value = (branch_offset & 0xffff);
                    operand_mode = ASM_ADDR_MODE_REL_WORD;
                    opcode = i;
                    found = TRUE;
                    break;
                }

#if 0
                /* Special case: opcode A - is A a register or $A? */
                /* If second case, is it zero page or absolute?  */
                if (operand_mode == ASM_ADDR_MODE_ACCUMULATOR
                    && opinfo->addr_mode == ASM_ADDR_MODE_ZERO_PAGE) {
                    opcode = i;
                    operand_mode = ASM_ADDR_MODE_ZERO_PAGE;
                    operand_value = 0x000a;
                    found = TRUE;
                    break;
                }
#endif
                /* If there is no operand and the opcode wants a register
                 * list, it could be an empty list.
                 */
                if (operand_mode == ASM_ADDR_MODE_IMPLIED
                    && (opinfo->addr_mode == ASM_ADDR_MODE_SYS_POST ||
                        opinfo->addr_mode == ASM_ADDR_MODE_USR_POST)) {
                    opcode = i;
                    operand_mode = opinfo->addr_mode;
                    operand_value = 0x00;
                    found = TRUE;
                    break;
                }
                /* If there are exactly 2 registers the parser thought it
                 * would be _REG_POST but it could also be a 2-item list.
                 * Fortunately it kept the other interpretation hidden away
                 * in the submode field.
                 */
                if (operand_mode == ASM_ADDR_MODE_REG_POST
                    && (opinfo->addr_mode == ASM_ADDR_MODE_SYS_POST ||
                        opinfo->addr_mode == ASM_ADDR_MODE_USR_POST)) {
                    opcode = i;
                    operand_mode = opinfo->addr_mode;
                    operand_value = operand_submode;
                    found = TRUE;
                    break;
                }
                /* The parser doesn't distinguish 2 kinds of register lists.
                 * Too bad if you write PSHS S or PSHU U.
                 */
                if (operand_mode == ASM_ADDR_MODE_SYS_POST
                    && opinfo->addr_mode == ASM_ADDR_MODE_USR_POST) {
                    opcode = i;
                    found = TRUE;
                    break;
                }
            }
            i++;
        }
        while (i != 0);
        if (found) {
            break;
        }
    }

    if (!found) {
        mon_out("Instruction not valid.\n");
        return -1;
    }

    opc_offset = 0;
    if (prefix[j] != -1) {
        opc[opc_offset++] = prefix[j];
    }
    opc[opc_offset++] = opcode;
    if (operand_mode == ASM_ADDR_MODE_INDEXED) {
        opc[opc_offset++] = (BYTE)operand_submode;
    }

    len = (monitor_cpu_for_memspace[mem]->asm_addr_mode_get_size)
              ((unsigned int)operand_mode, opc[0], opc[1], opc[2]);

    DBG(printf("len = %d\n", len));
    if (len == opc_offset + 1) {
        opc[opc_offset++] = operand_value & 0xFF;
    } else if (len == opc_offset + 2) {
        opc[opc_offset++] = operand_value >> 8;
        opc[opc_offset++] = operand_value & 0xFF;
    }

    for (i = 0; i < len; i++) {
        mon_set_mem_val(mem, (BYTE)(loc + i), opc[i]);
    }

    if (len >= 0) {
        mon_inc_addr_location(&asm_mode_addr, len);
        dot_addr[mem] = asm_mode_addr;
    } else {
        mon_out("Assemble error: %d\n", len);
    }
    return len;
}

void mon_assemble6809_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_assemble_instr = mon_assemble_instr;
}
