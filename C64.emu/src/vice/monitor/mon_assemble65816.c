/*
 * mon_assemble65816.c - The VICE built-in monitor, 65816/65802 assembler module.
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

#include <string.h>

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

static int mon_assemble_instr(const char *opcode_name, asm_mode_addr_info_t operand)
{
    DWORD operand_value = operand.param;
    WORD operand_mode = operand.addr_mode;
    BYTE operand_extra_value = operand.addr_submode;
    BYTE i = 0, opcode = 0;
    int len, branch_offset;
    bool found = FALSE;
    MEMSPACE mem;
    WORD loc;

    mem = addr_memspace(asm_mode_addr);
    loc = addr_location(asm_mode_addr);

    do {
        const asm_opcode_info_t *opinfo;

        opinfo = (monitor_cpu_for_memspace[mem]->asm_opcode_info_get)(i, 0, 0);
        if (!strcasecmp(opinfo->mnemonic, opcode_name)) {
            if (opinfo->addr_mode == operand_mode) {
                opcode = i;
                found = TRUE;
                break;
            }

            /* Special case: Register A not specified for ACCUMULATOR mode. */
            if (operand_mode == ASM_ADDR_MODE_IMPLIED
                && opinfo->addr_mode == ASM_ADDR_MODE_ACCUMULATOR) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_ACCUMULATOR;
                found = TRUE;
                break;
            }

            /* Special case: RELATIVE mode looks like ZERO_PAGE or ABSOLUTE
               modes.  */
            if ((operand_mode == ASM_ADDR_MODE_ZERO_PAGE
                || operand_mode == ASM_ADDR_MODE_ABSOLUTE)
                && opinfo->addr_mode == ASM_ADDR_MODE_RELATIVE) {
                branch_offset = (operand_value - loc - 2) & 0xffff;
                if (branch_offset <= 0x7f || branch_offset >= 0xff80) {
                    operand_value = (branch_offset & 0xff);
                    operand_mode = ASM_ADDR_MODE_RELATIVE;
                    opcode = i;
                    found = TRUE;
                    break;
                }
            }

            /* Special case: RELATIVE LONG mode looks like ZERO_PAGE or
               ABSOLUTE modes.  */
            if ((operand_mode == ASM_ADDR_MODE_ZERO_PAGE
                || operand_mode == ASM_ADDR_MODE_ABSOLUTE)
                && opinfo->addr_mode == ASM_ADDR_MODE_RELATIVE_LONG) {
                branch_offset = operand_value - loc - 3;
                operand_value = branch_offset & 0xffff;
                operand_mode = ASM_ADDR_MODE_RELATIVE_LONG;
                opcode = i;
                found = TRUE;
                break;
            }

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

            /* It's safe to assume ABSOULTE if ZERO_PAGE not yet found since
             * ZERO_PAGE versions always precede ABSOLUTE versions if they
             * exist.
             */
            if (operand_mode == ASM_ADDR_MODE_ACCUMULATOR
                && opinfo->addr_mode == ASM_ADDR_MODE_ABSOLUTE) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_ABSOLUTE;
                operand_value = 0x000a;
                found = TRUE;
                break;
            }

            /* It's safe to assume ABSOULTE if ZERO_PAGE not yet found since
             * ZERO_PAGE versions always precede ABSOLUTE versions if they
             * exist.
             */
            if (operand_mode == ASM_ADDR_MODE_ZERO_PAGE
                && opinfo->addr_mode == ASM_ADDR_MODE_ABSOLUTE) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_ABSOLUTE;
                found = TRUE;
                break;
            }

            /* Special case: MOVE mode needs special handling. */
            if (opinfo->addr_mode == ASM_ADDR_MODE_MOVE
                && operand_mode == ASM_ADDR_MODE_DOUBLE && operand_value < 0x100) {
                operand_value = ((operand_extra_value & 0xff) << 8) | (operand_value & 0xff);
                opcode = i;
                operand_mode = ASM_ADDR_MODE_MOVE;
                found = TRUE;
                break;
            }

            /* Special case: INDIRECT LONG mode needs special handling because of 6809. */
            if (opinfo->addr_mode == ASM_ADDR_MODE_INDIRECT_LONG
                && operand_mode == ASM_ADDR_MODE_INDEXED && operand_value < 0x100) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_INDIRECT_LONG;
                found = TRUE;
                break;
            }

            /* Special case: ABS IND LONG mode needs special handling because of 6809. */
            if (opinfo->addr_mode == ASM_ADDR_MODE_ABS_IND_LONG
                && operand_mode == ASM_ADDR_MODE_INDEXED && operand_value < 0x10000) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_ABS_IND_LONG;
                found = TRUE;
                break;
            }

            /* Special case: 16 bit immediate */
            if (opinfo->addr_mode == ASM_ADDR_MODE_IMMEDIATE
                && operand_mode == ASM_ADDR_MODE_IMMEDIATE_16) {
                opcode = i;
                operand_mode = ASM_ADDR_MODE_IMMEDIATE;
                found = TRUE;
                break;
            }
        }
        i++;
    } while (i != 0);

    if (!found) {
        mon_out("Instruction not valid.\n");
        return -1;
    }

    len = (monitor_cpu_for_memspace[mem]->asm_addr_mode_get_size)
          ((unsigned int)(operand_mode), 0, 0, 0);

    /* EP 98.08.23 use correct memspace for assembling.  */
    mon_set_mem_val(mem, loc, opcode);
    if (len >= 2) {
        mon_set_mem_val(mem, (WORD)(loc + 1), (BYTE)(operand_value & 0xff));
    }
    if (len >= 3) {
        mon_set_mem_val(mem, (WORD)(loc + 2),
                        (BYTE)((operand_value >> 8) & 0xff));
    }
    if (len >= 4) {
        mon_set_mem_val(mem, (WORD)(loc + 3),
                        (BYTE)(operand_extra_value & 0xff));
    }

    if (len >= 0) {
        mon_inc_addr_location(&asm_mode_addr, len);
        dot_addr[mem] = asm_mode_addr;
    } else {
        mon_out("Assemble error: %d\n", len);
    }
    return len;
}

void mon_assemble65816_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_assemble_instr = mon_assemble_instr;
}
