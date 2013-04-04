/*
 * mon_register6502dtv.c - The VICE built-in monitor 6502 DTV register functions.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include "mos6510dtv.h"
#include "uimon.h"

#define TEST(x) ((x) != 0)

static unsigned int mon_register_get_val(int mem, int reg_id)
{
    mos6510dtv_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return 0;
        }
    }

    reg_ptr = mon_interfaces[mem]->dtv_cpu_regs;

    switch (reg_id) {
        case e_A:
            return MOS6510DTV_REGS_GET_A(reg_ptr);
        case e_X:
            return MOS6510DTV_REGS_GET_X(reg_ptr);
        case e_Y:
            return MOS6510DTV_REGS_GET_Y(reg_ptr);
        case e_PC:
            return MOS6510DTV_REGS_GET_PC(reg_ptr);
        case e_SP:
            return MOS6510DTV_REGS_GET_SP(reg_ptr);
        case e_FLAGS:
            return MOS6510DTV_REGS_GET_FLAGS(reg_ptr)
                   | MOS6510DTV_REGS_GET_SIGN(reg_ptr)
                   | (MOS6510DTV_REGS_GET_ZERO(reg_ptr) << 1);
        case e_R3:
            return MOS6510DTV_REGS_GET_R3(reg_ptr);
        case e_R4:
            return MOS6510DTV_REGS_GET_R4(reg_ptr);
        case e_R5:
            return MOS6510DTV_REGS_GET_R5(reg_ptr);
        case e_R6:
            return MOS6510DTV_REGS_GET_R6(reg_ptr);
        case e_R7:
            return MOS6510DTV_REGS_GET_R7(reg_ptr);
        case e_R8:
            return MOS6510DTV_REGS_GET_R8(reg_ptr);
        case e_R9:
            return MOS6510DTV_REGS_GET_R9(reg_ptr);
        case e_R10:
            return MOS6510DTV_REGS_GET_R10(reg_ptr);
        case e_R11:
            return MOS6510DTV_REGS_GET_R11(reg_ptr);
        case e_R12:
            return MOS6510DTV_REGS_GET_R12(reg_ptr);
        case e_R13:
            return MOS6510DTV_REGS_GET_R13(reg_ptr);
        case e_R14:
            return MOS6510DTV_REGS_GET_R14(reg_ptr);
        case e_R15:
            return MOS6510DTV_REGS_GET_R15(reg_ptr);
        case e_ACM:
            return MOS6510DTV_REGS_GET_ACM(reg_ptr);
        case e_YXM:
            return MOS6510DTV_REGS_GET_YXM(reg_ptr);
        default:
            log_error(LOG_ERR, "Unknown register!");
    }
    return 0;
}

static void mon_register_set_val(int mem, int reg_id, WORD val)
{
    mos6510dtv_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    }

    reg_ptr = mon_interfaces[mem]->dtv_cpu_regs;

    switch (reg_id) {
        case e_A:
            MOS6510DTV_REGS_SET_A(reg_ptr, (BYTE)val);
            break;
        case e_X:
            MOS6510DTV_REGS_SET_X(reg_ptr, (BYTE)val);
            break;
        case e_Y:
            MOS6510DTV_REGS_SET_Y(reg_ptr, (BYTE)val);
            break;
        case e_PC:
            MOS6510DTV_REGS_SET_PC(reg_ptr, val);
            if (monitor_diskspace_dnr(mem) >= 0) {
                mon_interfaces[mem]->set_bank_base(mon_interfaces[mem]->context);
            }
            break;
        case e_SP:
            MOS6510DTV_REGS_SET_SP(reg_ptr, (BYTE)val);
            break;
        case e_FLAGS:
            MOS6510DTV_REGS_SET_STATUS(reg_ptr, (BYTE)val);
            break;
        case e_R3:
            MOS6510DTV_REGS_SET_R3(reg_ptr, (BYTE)val);
            break;
        case e_R4:
            MOS6510DTV_REGS_SET_R4(reg_ptr, (BYTE)val);
            break;
        case e_R5:
            MOS6510DTV_REGS_SET_R5(reg_ptr, (BYTE)val);
            break;
        case e_R6:
            MOS6510DTV_REGS_SET_R6(reg_ptr, (BYTE)val);
            break;
        case e_R7:
            MOS6510DTV_REGS_SET_R7(reg_ptr, (BYTE)val);
            break;
        case e_R8:
            MOS6510DTV_REGS_SET_R8(reg_ptr, (BYTE)val);
            break;
        case e_R9:
            MOS6510DTV_REGS_SET_R9(reg_ptr, (BYTE)val);
            break;
        case e_R10:
            MOS6510DTV_REGS_SET_R10(reg_ptr, (BYTE)val);
            break;
        case e_R11:
            MOS6510DTV_REGS_SET_R11(reg_ptr, (BYTE)val);
            break;
        case e_R12:
            MOS6510DTV_REGS_SET_R12(reg_ptr, (BYTE)val);
            break;
        case e_R13:
            MOS6510DTV_REGS_SET_R13(reg_ptr, (BYTE)val);
            break;
        case e_R14:
            MOS6510DTV_REGS_SET_R14(reg_ptr, (BYTE)val);
            break;
        case e_R15:
            MOS6510DTV_REGS_SET_R15(reg_ptr, (BYTE)val);
            break;
        case e_ACM:
            MOS6510DTV_REGS_SET_ACM(reg_ptr, (BYTE)val);
            break;
        case e_YXM:
            MOS6510DTV_REGS_SET_YXM(reg_ptr, (BYTE)val);
            break;
        default:
            log_error(LOG_ERR, "Unknown register!");
            return;
    }
    force_array[mem] = 1;
}

static void mon_register_print(int mem)
{
    mos6510dtv_regs_t *regs;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return;
    }

    regs = mon_interfaces[mem]->dtv_cpu_regs;

    mon_out("  ADDR AC XR YR SP 00 01 NV-BDIZC ");

    if (mon_interfaces[mem]->get_line_cycle != NULL) {
        mon_out("LIN CYC  STOPWATCH\n");
    } else {
        mon_out(" STOPWATCH\n");
    }

    mon_out(".;%04x %02x %02x %02x %02x %02x %02x %d%d%c%d%d%d%d%d",
            addr_location(mon_register_get_val(mem, e_PC)),
            mon_register_get_val(mem, e_A),
            mon_register_get_val(mem, e_X),
            mon_register_get_val(mem, e_Y),
            mon_register_get_val(mem, e_SP),
            mon_get_mem_val(mem, 0),
            mon_get_mem_val(mem, 1),
            TEST(MOS6510DTV_REGS_GET_SIGN(regs)),
            TEST(MOS6510DTV_REGS_GET_OVERFLOW(regs)),
            '1',
            TEST(MOS6510DTV_REGS_GET_BREAK(regs)),
            TEST(MOS6510DTV_REGS_GET_DECIMAL(regs)),
            TEST(MOS6510DTV_REGS_GET_INTERRUPT(regs)),
            TEST(MOS6510DTV_REGS_GET_ZERO(regs)),
            TEST(MOS6510DTV_REGS_GET_CARRY(regs)));

    if (mon_interfaces[mem]->get_line_cycle != NULL) {
        unsigned int line, cycle;
        int half_cycle;

        mon_interfaces[mem]->get_line_cycle(&line, &cycle, &half_cycle);

        if (half_cycle == -1) {
            mon_out(" %03i %03i", line, cycle);
        } else {
            mon_out(" %03i %03i %i", line, cycle, half_cycle);
        }
    }
    mon_stopwatch_show(" ", "\n");

    if (mem == e_comp_space) {
        mon_out("R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15 ACM YXM\n");
        mon_out("%02x %02x %02x %02x %02x %02x %02x %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x\n",
                mon_register_get_val(mem, e_R3),
                mon_register_get_val(mem, e_R4),
                mon_register_get_val(mem, e_R5),
                mon_register_get_val(mem, e_R6),
                mon_register_get_val(mem, e_R7),
                mon_register_get_val(mem, e_R8),
                mon_register_get_val(mem, e_R9),
                mon_register_get_val(mem, e_R10),
                mon_register_get_val(mem, e_R11),
                mon_register_get_val(mem, e_R12),
                mon_register_get_val(mem, e_R13),
                mon_register_get_val(mem, e_R14),
                mon_register_get_val(mem, e_R15),
                mon_register_get_val(mem, e_ACM),
                mon_register_get_val(mem, e_YXM));
    }
}

static mon_reg_list_t *mon_register_list_get6502dtv(int mem)
{
    mon_reg_list_t *mon_reg_list;

    mon_reg_list = lib_malloc(sizeof(mon_reg_list_t) * 24);

    mon_reg_list[0].name = "PC";
    mon_reg_list[0].val = (unsigned int)mon_register_get_val(mem, e_PC);
    mon_reg_list[0].size = 16;
    mon_reg_list[0].flags = 0;
    mon_reg_list[0].next = &mon_reg_list[1];

    mon_reg_list[1].name = "AC";
    mon_reg_list[1].val = (unsigned int)mon_register_get_val(mem, e_A);
    mon_reg_list[1].size = 8;
    mon_reg_list[1].flags = 0;
    mon_reg_list[1].next = &mon_reg_list[2];

    mon_reg_list[2].name = "XR";
    mon_reg_list[2].val = (unsigned int)mon_register_get_val(mem, e_X);
    mon_reg_list[2].size = 8;
    mon_reg_list[2].flags = 0;
    mon_reg_list[2].next = &mon_reg_list[3];

    mon_reg_list[3].name = "YR";
    mon_reg_list[3].val = (unsigned int)mon_register_get_val(mem, e_Y);
    mon_reg_list[3].size = 8;
    mon_reg_list[3].flags = 0;
    mon_reg_list[3].next = &mon_reg_list[4];

    mon_reg_list[4].name = "SP";
    mon_reg_list[4].val = (unsigned int)mon_register_get_val(mem, e_SP);
    mon_reg_list[4].size = 8;
    mon_reg_list[4].flags = 0;
    mon_reg_list[4].next = &mon_reg_list[5];

    /* Note: The DTV always has reg 00 and 01 */
    mon_reg_list[5].name = "00";
    mon_reg_list[5].val = (unsigned int)mon_get_mem_val(mem, 0);
    mon_reg_list[5].size = 8;
    mon_reg_list[5].flags = 0;
    mon_reg_list[5].next = &mon_reg_list[6];

    mon_reg_list[6].name = "01";
    mon_reg_list[6].val = (unsigned int)mon_get_mem_val(mem, 1);
    mon_reg_list[6].size = 8;
    mon_reg_list[6].flags = 0;
    mon_reg_list[6].next = &mon_reg_list[7];

    mon_reg_list[7].name = "FL";
    mon_reg_list[7].val = (unsigned int)mon_register_get_val(mem, e_FLAGS)
                          | 0x20;
    mon_reg_list[7].size = 8;
    mon_reg_list[7].flags = 0;
    mon_reg_list[7].next = &mon_reg_list[8];

    mon_reg_list[8].name = "NV-BDIZC";
    mon_reg_list[8].val = (unsigned int)mon_register_get_val(mem, e_FLAGS)
                          | 0x20;
    mon_reg_list[8].size = 8;
    mon_reg_list[8].flags = 1;
    mon_reg_list[8].next = &mon_reg_list[9];

    mon_reg_list[9].name = "R3";
    mon_reg_list[9].val = (unsigned int)mon_register_get_val(mem, e_R3);
    mon_reg_list[9].size = 8;
    mon_reg_list[9].flags = 0;
    mon_reg_list[9].next = &mon_reg_list[10];

    mon_reg_list[10].name = "R4";
    mon_reg_list[10].val = (unsigned int)mon_register_get_val(mem, e_R4);
    mon_reg_list[10].size = 8;
    mon_reg_list[10].flags = 0;
    mon_reg_list[10].next = &mon_reg_list[11];

    mon_reg_list[11].name = "R5";
    mon_reg_list[11].val = (unsigned int)mon_register_get_val(mem, e_R5);
    mon_reg_list[11].size = 8;
    mon_reg_list[11].flags = 0;
    mon_reg_list[11].next = &mon_reg_list[12];

    mon_reg_list[12].name = "R6";
    mon_reg_list[12].val = (unsigned int)mon_register_get_val(mem, e_R6);
    mon_reg_list[12].size = 8;
    mon_reg_list[12].flags = 0;
    mon_reg_list[12].next = &mon_reg_list[13];

    mon_reg_list[13].name = "R7";
    mon_reg_list[13].val = (unsigned int)mon_register_get_val(mem, e_R7);
    mon_reg_list[13].size = 8;
    mon_reg_list[13].flags = 0;
    mon_reg_list[13].next = &mon_reg_list[14];

    mon_reg_list[14].name = "R8";
    mon_reg_list[14].val = (unsigned int)mon_register_get_val(mem, e_R8);
    mon_reg_list[14].size = 8;
    mon_reg_list[14].flags = 0;
    mon_reg_list[14].next = &mon_reg_list[15];

    mon_reg_list[15].name = "R9";
    mon_reg_list[15].val = (unsigned int)mon_register_get_val(mem, e_R9);
    mon_reg_list[15].size = 8;
    mon_reg_list[15].flags = 0;
    mon_reg_list[15].next = &mon_reg_list[16];

    mon_reg_list[16].name = "R10";
    mon_reg_list[16].val = (unsigned int)mon_register_get_val(mem, e_R10);
    mon_reg_list[16].size = 8;
    mon_reg_list[16].flags = 0;
    mon_reg_list[16].next = &mon_reg_list[17];

    mon_reg_list[17].name = "R11";
    mon_reg_list[17].val = (unsigned int)mon_register_get_val(mem, e_R11);
    mon_reg_list[17].size = 8;
    mon_reg_list[17].flags = 0;
    mon_reg_list[17].next = &mon_reg_list[18];

    mon_reg_list[18].name = "R12";
    mon_reg_list[18].val = (unsigned int)mon_register_get_val(mem, e_R12);
    mon_reg_list[18].size = 8;
    mon_reg_list[18].flags = 0;
    mon_reg_list[18].next = &mon_reg_list[19];

    mon_reg_list[19].name = "R13";
    mon_reg_list[19].val = (unsigned int)mon_register_get_val(mem, e_R13);
    mon_reg_list[19].size = 8;
    mon_reg_list[19].flags = 0;
    mon_reg_list[19].next = &mon_reg_list[20];

    mon_reg_list[20].name = "R14";
    mon_reg_list[20].val = (unsigned int)mon_register_get_val(mem, e_R14);
    mon_reg_list[20].size = 8;
    mon_reg_list[20].flags = 0;
    mon_reg_list[20].next = &mon_reg_list[21];

    mon_reg_list[21].name = "R15";
    mon_reg_list[21].val = (unsigned int)mon_register_get_val(mem, e_R15);
    mon_reg_list[21].size = 8;
    mon_reg_list[21].flags = 0;
    mon_reg_list[21].next = &mon_reg_list[22];

    mon_reg_list[22].name = "ACM";
    mon_reg_list[22].val = (unsigned int)mon_register_get_val(mem, e_ACM);
    mon_reg_list[22].size = 8;
    mon_reg_list[22].flags = 0;
    mon_reg_list[22].next = &mon_reg_list[23];

    mon_reg_list[23].name = "YXM";
    mon_reg_list[23].val = (unsigned int)mon_register_get_val(mem, e_YXM);
    mon_reg_list[23].size = 8;
    mon_reg_list[23].flags = 0;
    mon_reg_list[23].next = NULL;

    return mon_reg_list;
}

static void mon_register_list_set6502dtv(mon_reg_list_t *reg_list, int mem)
{
    do {
        if (!strcmp(reg_list->name, "PC")) {
            mon_register_set_val(mem, e_PC, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "AC")) {
            mon_register_set_val(mem, e_A, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "XR")) {
            mon_register_set_val(mem, e_X, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "YR")) {
            mon_register_set_val(mem, e_Y, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "SP")) {
            mon_register_set_val(mem, e_SP, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "00")) {
            mon_set_mem_val(mem, 0, (BYTE)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "01")) {
            mon_set_mem_val(mem, 1, (BYTE)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "NV-BDIZC")) {
            mon_register_set_val(mem, e_FLAGS, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R3")) {
            mon_register_set_val(mem, e_R3, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R4")) {
            mon_register_set_val(mem, e_R4, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R5")) {
            mon_register_set_val(mem, e_R5, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R6")) {
            mon_register_set_val(mem, e_R6, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R7")) {
            mon_register_set_val(mem, e_R7, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R8")) {
            mon_register_set_val(mem, e_R8, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R9")) {
            mon_register_set_val(mem, e_R9, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R10")) {
            mon_register_set_val(mem, e_R10, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R11")) {
            mon_register_set_val(mem, e_R11, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R12")) {
            mon_register_set_val(mem, e_R12, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R13")) {
            mon_register_set_val(mem, e_R13, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R14")) {
            mon_register_set_val(mem, e_R15, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "R15")) {
            mon_register_set_val(mem, e_R15, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "ACM")) {
            mon_register_set_val(mem, e_ACM, (WORD)(reg_list->val));
        }
        if (!strcmp(reg_list->name, "YXM")) {
            mon_register_set_val(mem, e_YXM, (WORD)(reg_list->val));
        }

        reg_list = reg_list->next;
    } while (reg_list != NULL);
}

void mon_register6502dtv_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_register_get_val = mon_register_get_val;
    monitor_cpu_type->mon_register_set_val = mon_register_set_val;
    monitor_cpu_type->mon_register_print = mon_register_print;
    monitor_cpu_type->mon_register_print_ex = NULL;
    monitor_cpu_type->mon_register_list_get = mon_register_list_get6502dtv;
    monitor_cpu_type->mon_register_list_set = mon_register_list_set6502dtv;
}
