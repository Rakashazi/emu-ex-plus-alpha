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

/* #define DEBUG_MON_REGS */

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

#ifdef DEBUG_MON_REGS
#define DBG(_x_) printf _x_
#else
#define DBG(_x_)
#endif

#define TEST(x) ((x) != 0)

/* TODO: make the other functions here use this table. when done also do the
 *       same with the other CPUs and finally move common code to mon_register.c
 */

#define REG_LIST_6510DTV_SIZE (24 + 1)
static mon_reg_list_t mon_reg_list_6510dtv[REG_LIST_6510DTV_SIZE] = {
    {      "PC",    e_PC, 16,                      0, 0, 0 },
    {       "A",     e_A,  8,                      0, 0, 0 },
    {       "X",     e_X,  8,                      0, 0, 0 },
    {       "Y",     e_Y,  8,                      0, 0, 0 },
    {      "SP",    e_SP,  8,                      0, 0, 0 },
    {      "00",      -1,  8, MON_REGISTER_IS_MEMORY, 0, 0 },
    {      "01",      -1,  8, MON_REGISTER_IS_MEMORY, 1, 0 },
    {      "FL", e_FLAGS,  8,                      0, 0, 0 },
    {"NV-BDIZC", e_FLAGS,  8,  MON_REGISTER_IS_FLAGS, 0, 0 },
    {      "R3",    e_R3,  8,                      0, 0, 0 },
    {      "R4",    e_R4,  8,                      0, 0, 0 },
    {      "R5",    e_R5,  8,                      0, 0, 0 },
    {      "R6",    e_R6,  8,                      0, 0, 0 },
    {      "R7",    e_R7,  8,                      0, 0, 0 },
    {      "R8",    e_R8,  8,                      0, 0, 0 },
    {      "R9",    e_R9,  8,                      0, 0, 0 },
    {     "R10",   e_R10,  8,                      0, 0, 0 },
    {     "R11",   e_R11,  8,                      0, 0, 0 },
    {     "R12",   e_R12,  8,                      0, 0, 0 },
    {     "R13",   e_R13,  8,                      0, 0, 0 },
    {     "R14",   e_R14,  8,                      0, 0, 0 },
    {     "R15",   e_R15,  8,                      0, 0, 0 },
    {     "ACM",   e_ACM,  8,                      0, 0, 0 },
    {     "YXM",   e_YXM,  8,                      0, 0, 0 },
    { NULL, -1,  0,  0, 0, 0 }
};

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

/* TODO: should use mon_register_list_get */
static void mon_register_print(int mem)
{
    mos6510dtv_regs_t *regs;
    int current_bank;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return;
    }

    regs = mon_interfaces[mem]->dtv_cpu_regs;

    mon_out("  ADDR A  X  Y  SP 00 01 NV-BDIZC ");

    if (mon_interfaces[mem]->get_line_cycle != NULL) {
        mon_out("LIN CYC  STOPWATCH\n");
    } else {
        mon_out(" STOPWATCH\n");
    }

    current_bank = mon_interfaces[mem]->current_bank;
    if (mon_interfaces[mem]->mem_bank_from_name != NULL) {
        mon_interfaces[mem]->current_bank = mon_interfaces[mem]->mem_bank_from_name("cpu");
    } else {
        mon_interfaces[mem]->current_bank = 0;
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

    mon_interfaces[mem]->current_bank = current_bank;

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

/* TODO: try to make this a generic function, move it into mon_register.c and
         remove mon_register_list_get from the monitor_cpu_type_t struct */
static mon_reg_list_t *mon_register_list_get6502dtv(int mem)
{
    mon_reg_list_t *mon_reg_list, *regs;

    mon_reg_list = regs = lib_malloc(sizeof(mon_reg_list_t) * REG_LIST_6510DTV_SIZE);
    memcpy(mon_reg_list, mon_reg_list_6510dtv, sizeof(mon_reg_list_t) * REG_LIST_6510DTV_SIZE);

    do {
        if (regs->flags & MON_REGISTER_IS_MEMORY) {
            int current_bank = mon_interfaces[mem]->current_bank;
            mon_interfaces[mem]->current_bank = mon_interfaces[mem]->mem_bank_from_name("cpu");
            regs->val = (unsigned int)mon_get_mem_val(mem, (WORD)regs->extra);
            mon_interfaces[mem]->current_bank = current_bank;
        } else if (regs->flags & MON_REGISTER_IS_FLAGS) {
            regs->val = (unsigned int)mon_register_get_val(mem, regs->id) | 0x20;
        } else {
            regs->val = (unsigned int)mon_register_get_val(mem, regs->id);
        }
        ++regs;
    } while (regs->name != NULL);

    return mon_reg_list;
}

void mon_register6502dtv_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_register_get_val = mon_register_get_val;
    monitor_cpu_type->mon_register_set_val = mon_register_set_val;
    monitor_cpu_type->mon_register_print = mon_register_print;
    monitor_cpu_type->mon_register_print_ex = NULL;
    monitor_cpu_type->mon_register_list_get = mon_register_list_get6502dtv;
}
