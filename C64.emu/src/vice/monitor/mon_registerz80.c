/*
 * mon_registerz80.c - The VICE built-in monitor Z80 register functions.
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "mon_register.h"
#include "mon_util.h"
#include "montypes.h"
#include "uimon.h"
#include "z80regs.h"


static unsigned int mon_register_get_val(int mem, int reg_id)
{
    z80_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return 0;
        }
    }

    reg_ptr = mon_interfaces[mem]->z80_cpu_regs;

    switch (reg_id) {
        case e_AF:
            return Z80_REGS_GET_AF(reg_ptr);
        case e_BC:
            return Z80_REGS_GET_BC(reg_ptr);
        case e_DE:
            return Z80_REGS_GET_DE(reg_ptr);
        case e_HL:
            return Z80_REGS_GET_HL(reg_ptr);
        case e_IX:
            return Z80_REGS_GET_IX(reg_ptr);
        case e_IY:
            return Z80_REGS_GET_IY(reg_ptr);
        case e_SP:
            return Z80_REGS_GET_SP(reg_ptr);
        case e_PC:
            return Z80_REGS_GET_PC(reg_ptr);
        case e_I:
            return Z80_REGS_GET_I(reg_ptr);
        case e_R:
            return Z80_REGS_GET_R(reg_ptr);
        case e_AF2:
            return Z80_REGS_GET_AF2(reg_ptr);
        case e_BC2:
            return Z80_REGS_GET_BC2(reg_ptr);
        case e_DE2:
            return Z80_REGS_GET_DE2(reg_ptr);
        case e_HL2:
            return Z80_REGS_GET_HL2(reg_ptr);
        default:
            log_error(LOG_ERR, "Unknown register!");
    }
    return 0;
}

static void mon_register_set_val(int mem, int reg_id, WORD val)
{
    z80_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    }

    reg_ptr = mon_interfaces[mem]->z80_cpu_regs;

    switch (reg_id) {
        case e_AF:
            Z80_REGS_SET_AF(reg_ptr, val);
            break;
        case e_BC:
            Z80_REGS_SET_BC(reg_ptr, val);
            break;
        case e_DE:
            Z80_REGS_SET_DE(reg_ptr, val);
            break;
        case e_HL:
            Z80_REGS_SET_HL(reg_ptr, val);
            break;
        case e_IX:
            Z80_REGS_SET_IX(reg_ptr, val);
            break;
        case e_IY:
            Z80_REGS_SET_IY(reg_ptr, val);
            break;
        case e_SP:
            Z80_REGS_SET_SP(reg_ptr, val);
            break;
        case e_PC:
            Z80_REGS_SET_PC(reg_ptr, val);
            break;
        case e_I:
            Z80_REGS_SET_I(reg_ptr, (BYTE)val);
            break;
        case e_R:
            Z80_REGS_SET_R(reg_ptr, (BYTE)val);
            break;
        case e_AF2:
            Z80_REGS_SET_AF2(reg_ptr, val);
            break;
        case e_BC2:
            Z80_REGS_SET_BC2(reg_ptr, val);
            break;
        case e_DE2:
            Z80_REGS_SET_DE2(reg_ptr, val);
            break;
        case e_HL2:
            Z80_REGS_SET_HL2(reg_ptr, val);
            break;
        default:
            log_error(LOG_ERR, "Unknown register!");
            return;
    }
    force_array[mem] = 1;
}

static void mon_register_print(int mem)
{
    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return;
    }
    mon_out("  ADDR AF   BC   DE   HL   IX   IY   SP   I  R  AF'  BC'  DE'  HL'\n");
    mon_out(".;%04x %04x %04x %04x %04x %04x %04x %04x %02x %02x %04x %04x %04x %04x\n",
            addr_location(mon_register_get_val(mem, e_PC)),
            mon_register_get_val(mem, e_AF),
            mon_register_get_val(mem, e_BC),
            mon_register_get_val(mem, e_DE),
            mon_register_get_val(mem, e_HL),
            mon_register_get_val(mem, e_IX),
            mon_register_get_val(mem, e_IY),
            mon_register_get_val(mem, e_SP),
            mon_register_get_val(mem, e_I),
            mon_register_get_val(mem, e_R),
            mon_register_get_val(mem, e_AF2),
            mon_register_get_val(mem, e_BC2),
            mon_register_get_val(mem, e_DE2),
            mon_register_get_val(mem, e_HL2));
}

static mon_reg_list_t *mon_register_list_getz80(int mem)
{
    mon_reg_list_t *mon_reg_list;

    mon_reg_list = lib_malloc(sizeof(mon_reg_list_t) * 14);

    mon_reg_list[0].name = "PC";
    mon_reg_list[0].val = (unsigned int)mon_register_get_val(mem, e_PC);
    mon_reg_list[0].size = 16;
    mon_reg_list[0].flags = 0;
    mon_reg_list[0].next = &mon_reg_list[1];

    mon_reg_list[1].name = "AF";
    mon_reg_list[1].val = (unsigned int)mon_register_get_val(mem, e_AF);
    mon_reg_list[1].size = 16;
    mon_reg_list[1].flags = 0;
    mon_reg_list[1].next = &mon_reg_list[2];

    mon_reg_list[2].name = "BC";
    mon_reg_list[2].val = (unsigned int)mon_register_get_val(mem, e_BC);
    mon_reg_list[2].size = 16;
    mon_reg_list[2].flags = 0;
    mon_reg_list[2].next = &mon_reg_list[3];

    mon_reg_list[3].name = "DE";
    mon_reg_list[3].val = (unsigned int)mon_register_get_val(mem, e_DE);
    mon_reg_list[3].size = 16;
    mon_reg_list[3].flags = 0;
    mon_reg_list[3].next = &mon_reg_list[4];

    mon_reg_list[4].name = "HL";
    mon_reg_list[4].val = (unsigned int)mon_register_get_val(mem, e_HL);
    mon_reg_list[4].size = 16;
    mon_reg_list[4].flags = 0;
    mon_reg_list[4].next = &mon_reg_list[5];

    mon_reg_list[5].name = "IX";
    mon_reg_list[5].val = (unsigned int)mon_register_get_val(mem, e_IX);
    mon_reg_list[5].size = 16;
    mon_reg_list[5].flags = 0;
    mon_reg_list[5].next = &mon_reg_list[6];

    mon_reg_list[6].name = "IY";
    mon_reg_list[6].val = (unsigned int)mon_register_get_val(mem, e_IY);
    mon_reg_list[6].size = 16;
    mon_reg_list[6].flags = 0;
    mon_reg_list[6].next = &mon_reg_list[7];

    mon_reg_list[7].name = "SP";
    mon_reg_list[7].val = (unsigned int)mon_register_get_val(mem, e_SP);
    mon_reg_list[7].size = 16;
    mon_reg_list[7].flags = 0;
    mon_reg_list[7].next = &mon_reg_list[8];

    mon_reg_list[8].name = "I";
    mon_reg_list[8].val = (unsigned int)mon_register_get_val(mem, e_I);
    mon_reg_list[8].size = 8;
    mon_reg_list[8].flags = 0;
    mon_reg_list[8].next = &mon_reg_list[9];

    mon_reg_list[9].name = "R";
    mon_reg_list[9].val = (unsigned int)mon_register_get_val(mem, e_R);
    mon_reg_list[9].size = 8;
    mon_reg_list[9].flags = 0;
    mon_reg_list[9].next = &mon_reg_list[10];

    mon_reg_list[10].name = "AF'";
    mon_reg_list[10].val = (unsigned int)mon_register_get_val(mem, e_AF2);
    mon_reg_list[10].size = 16;
    mon_reg_list[10].flags = 0;
    mon_reg_list[10].next = &mon_reg_list[11];

    mon_reg_list[11].name = "BC'";
    mon_reg_list[11].val = (unsigned int)mon_register_get_val(mem, e_BC2);
    mon_reg_list[11].size = 16;
    mon_reg_list[11].flags = 0;
    mon_reg_list[11].next = &mon_reg_list[12];

    mon_reg_list[12].name = "DE'";
    mon_reg_list[12].val = (unsigned int)mon_register_get_val(mem, e_DE2);
    mon_reg_list[12].size = 16;
    mon_reg_list[12].flags = 0;
    mon_reg_list[12].next = &mon_reg_list[13];

    mon_reg_list[13].name = "HL'";
    mon_reg_list[13].val = (unsigned int)mon_register_get_val(mem, e_HL2);
    mon_reg_list[13].size = 16;
    mon_reg_list[13].flags = 0;
    mon_reg_list[13].next = NULL;

    return mon_reg_list;
}

static void mon_register_list_setz80(mon_reg_list_t *reg_list, int mem)
{
    do {
        if (!strcmp(reg_list->name, "PC")) {
            mon_register_set_val(mem, e_PC, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "AF")) {
            mon_register_set_val(mem, e_AF, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "BC")) {
            mon_register_set_val(mem, e_BC, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "DE")) {
            mon_register_set_val(mem, e_DE, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "HL")) {
            mon_register_set_val(mem, e_HL, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "IX")) {
            mon_register_set_val(mem, e_IX, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "IY")) {
            mon_register_set_val(mem, e_IY, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "SP")) {
            mon_register_set_val(mem, e_SP, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "I")) {
            mon_register_set_val(mem, e_I, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R")) {
            mon_register_set_val(mem, e_R, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "AF'")) {
            mon_register_set_val(mem, e_AF2, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "BC'")) {
            mon_register_set_val(mem, e_BC2, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "DE'")) {
            mon_register_set_val(mem, e_DE2, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "HL'")) {
            mon_register_set_val(mem, e_HL2, (WORD)(reg_list->val));
        }

        reg_list = reg_list->next;
    } while (reg_list != NULL);
}

void mon_registerz80_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_register_get_val = mon_register_get_val;
    monitor_cpu_type->mon_register_set_val = mon_register_set_val;
    monitor_cpu_type->mon_register_print = mon_register_print;
    monitor_cpu_type->mon_register_print_ex = NULL;
    monitor_cpu_type->mon_register_list_get = mon_register_list_getz80;
    monitor_cpu_type->mon_register_list_set = mon_register_list_setz80;
}
