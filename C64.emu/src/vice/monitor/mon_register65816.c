/*
 * mon_register65816.c - The VICE built-in monitor 65816/65802 register functions.
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

/* #define DEBUG_MON_REGS */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "mon_register.h"
#include "mon_util.h"
#include "montypes.h"
#include "uimon.h"
#include "wdc65816.h"

#ifdef DEBUG_MON_REGS
#define DBG(_x_) printf _x_
#else
#define DBG(_x_)
#endif

#define TEST(x) ((x)!=0)

/* TODO: make the other functions here use this table. when done also do the
 *       same with the other CPUs and finally move common code to mon_register.c
 */

#define REG_LIST_65816_SIZE (13 + 1)
static mon_reg_list_t mon_reg_list_65816[REG_LIST_65816_SIZE] = {
    {     "PBR",   e_PBR,  8,                      0, 0, 0 },
    {      "PC",    e_PC, 16,                      0, 0, 0 },
    {       "A",     e_A,  8,                      0, 0, 0 },
    {       "B",     e_B, 16,                      0, 0, 0 },
    {       "X",     e_X, 16,                      0, 0, 0 },
    {       "Y",     e_Y, 16,                      0, 0, 0 },
    {      "SP",    e_SP, 16,                      0, 0, 0 },
    {     "DPR",   e_DPR, 16,                      0, 0, 0 },
    {     "DBR",   e_DBR,  8,                      0, 0, 0 },
    {      "FL", e_FLAGS,  8,                      0, 0, 0 },
    {"NV-BDIZC", e_FLAGS,  8,  MON_REGISTER_IS_FLAGS, 0, 0 },
    {"NVMXDIZC", e_FLAGS,  8,  MON_REGISTER_IS_FLAGS, 0, 0 },
    {       "E",  e_E,     1,                      0, 0, 0 },
    { NULL, -1,  0,  0, 0, 0 }
};

static unsigned int mon_register_get_val(int mem, int reg_id)
{
    WDC65816_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return 0;
        }
    }

    reg_ptr = mon_interfaces[mem]->cpu_65816_regs;

    switch(reg_id) {
      case e_A:
        return WDC65816_REGS_GET_A(reg_ptr);
      case e_B:
        return WDC65816_REGS_GET_B(reg_ptr);
      case e_C:
        return (WDC65816_REGS_GET_B(reg_ptr) << 8) | WDC65816_REGS_GET_A(reg_ptr);
      case e_X:
        return WDC65816_REGS_GET_X(reg_ptr);
      case e_Y:
        return WDC65816_REGS_GET_Y(reg_ptr);
      case e_PC:
        return WDC65816_REGS_GET_PC(reg_ptr);
      case e_SP:
        return WDC65816_REGS_GET_SP(reg_ptr);
      case e_PBR:
        return WDC65816_REGS_GET_PBR(reg_ptr);
      case e_DBR:
        return WDC65816_REGS_GET_DBR(reg_ptr);
      case e_DPR:
        return WDC65816_REGS_GET_DPR(reg_ptr);
      case e_E:
        return WDC65816_REGS_GET_EMUL(reg_ptr);
      case e_FLAGS:
          return WDC65816_REGS_GET_FLAGS(reg_ptr)
              | WDC65816_REGS_GET_SIGN(reg_ptr)
              | (WDC65816_REGS_GET_ZERO(reg_ptr) << 1);
      default:
        log_error(LOG_ERR, "Unknown register!");
    }
    return 0;
}

static void mon_register_set_val(int mem, int reg_id, WORD val)
{
    WDC65816_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    }

    reg_ptr = mon_interfaces[mem]->cpu_65816_regs;

    switch(reg_id) {
      case e_A:
        WDC65816_REGS_SET_A(reg_ptr, (BYTE)val);
        break;
      case e_B:
        WDC65816_REGS_SET_B(reg_ptr, (BYTE)val);
        break;
      case e_C:
        WDC65816_REGS_SET_A(reg_ptr, (BYTE)val);
        WDC65816_REGS_SET_B(reg_ptr, (BYTE)(val >> 8));
        break;
      case e_X:
        WDC65816_REGS_SET_X(reg_ptr, (WORD)val);
        break;
      case e_Y:
        WDC65816_REGS_SET_Y(reg_ptr, (WORD)val);
        break;
      case e_PC:
        WDC65816_REGS_SET_PC(reg_ptr, val);
        if (monitor_diskspace_dnr(mem) >= 0) {
            mon_interfaces[mem]->set_bank_base(mon_interfaces[mem]->context);
        }
        break;
      case e_SP:
        WDC65816_REGS_SET_SP(reg_ptr, (WORD)val);
        break;
      case e_DPR:
        WDC65816_REGS_SET_DPR(reg_ptr, (BYTE)val);
        break;
      case e_PBR:
        WDC65816_REGS_SET_PBR(reg_ptr, (BYTE)val);
        break;
      case e_DBR:
        WDC65816_REGS_SET_DBR(reg_ptr, (BYTE)val);
        break;
      case e_FLAGS:
        WDC65816_REGS_SET_STATUS(reg_ptr, (BYTE)val);
        break;
      case e_E:
        WDC65816_REGS_SET_EMUL(reg_ptr, (BYTE)val);
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
    WDC65816_regs_t *regs;
    unsigned int line = 0, cycle = 0;
    int half_cycle = -1;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return;
    }

    regs = mon_interfaces[mem]->cpu_65816_regs;

    if (mem == e_comp_space && mon_interfaces[mem]->get_line_cycle != NULL) {
        mon_interfaces[mem]->get_line_cycle(&line, &cycle, &half_cycle);
    }

    if (mon_register_get_val(mem, e_E)) {
        mon_out("  PB ADDR A  B  X  Y  SP DPRE DB NV-BDIZC E");
        if (mem == e_comp_space && mon_interfaces[mem]->get_line_cycle != NULL) {
            mon_out(" LIN CYC");
            if (half_cycle != -1) {
                mon_out(".SB");
            }
        }
        mon_out("\n.;%02x %04x %02x %02x %02x %02x %02x %04x %02x %d%d1%d%d%d%d%d 1",
                  mon_register_get_val(mem, e_PBR),
                  addr_location(mon_register_get_val(mem, e_PC)),
                  mon_register_get_val(mem, e_A),
                  mon_register_get_val(mem, e_B),
                  mon_register_get_val(mem, e_X) & 0xff,
                  mon_register_get_val(mem, e_Y) & 0xff,
                  mon_register_get_val(mem, e_SP) & 0xff,
                  mon_register_get_val(mem, e_DPR),
                  mon_register_get_val(mem, e_DBR),
                  TEST(WDC65816_REGS_GET_SIGN(regs)),
                  TEST(WDC65816_REGS_GET_OVERFLOW(regs)),
                  TEST(WDC65816_REGS_GET_BREAK(regs)),
                  TEST(WDC65816_REGS_GET_DECIMAL(regs)),
                  TEST(WDC65816_REGS_GET_INTERRUPT(regs)),
                  TEST(WDC65816_REGS_GET_ZERO(regs)),
                  TEST(WDC65816_REGS_GET_CARRY(regs)));
    } else {
        mon_out("  PB ADDR");
        mon_out(WDC65816_REGS_GET_65816_M(regs) ? " A  B " : " CREG");
        mon_out(WDC65816_REGS_GET_65816_X(regs) ? " XH X  YH Y " : " X    Y   ");
        mon_out(" STCK DPRE DB NVMXDIZC E");

        if (mem == e_comp_space && mon_interfaces[mem]->get_line_cycle != NULL) {
            mon_out(" LIN CYC");
            if (half_cycle != -1) {
                mon_out(".SB");
            }
        }
        mon_out("\n.;%02x %04x",
                mon_register_get_val(mem, e_PBR),
                addr_location(mon_register_get_val(mem, e_PC)));

        if (WDC65816_REGS_GET_65816_M(regs)) {
            mon_out(" %02x %02x",
                    mon_register_get_val(mem, e_A),
                    mon_register_get_val(mem, e_B));
        } else {
            mon_out(" %02x%02x",
                    mon_register_get_val(mem, e_B),
                    mon_register_get_val(mem, e_A));
        }
        if (WDC65816_REGS_GET_65816_X(regs)) {
            mon_out(" %02x %02x %02x %02x",
                  mon_register_get_val(mem, e_X) >> 8,
                  mon_register_get_val(mem, e_X) & 0xff,
                  mon_register_get_val(mem, e_Y) >> 8,
                  mon_register_get_val(mem, e_Y) & 0xff);
        } else {
            mon_out(" %04x %04x",
                  mon_register_get_val(mem, e_X),
                  mon_register_get_val(mem, e_Y));
        }

        mon_out(" %04x %04x %02x %d%d%d%d%d%d%d%d 0",
                mon_register_get_val(mem, e_SP),
                mon_register_get_val(mem, e_DPR),
                mon_register_get_val(mem, e_DBR),
                TEST(WDC65816_REGS_GET_SIGN(regs)),
                TEST(WDC65816_REGS_GET_OVERFLOW(regs)),
                TEST(WDC65816_REGS_GET_65816_M(regs)),
                TEST(WDC65816_REGS_GET_65816_X(regs)),
                TEST(WDC65816_REGS_GET_DECIMAL(regs)),
                TEST(WDC65816_REGS_GET_INTERRUPT(regs)),
                TEST(WDC65816_REGS_GET_ZERO(regs)),
                TEST(WDC65816_REGS_GET_CARRY(regs)));
    }

    if (mem == e_comp_space && mon_interfaces[mem]->get_line_cycle != NULL) {
        mon_out(" %03i %03i", line, cycle);
        if (half_cycle != -1) {
            mon_out(".%02i", half_cycle);
        }
    }
    mon_out("\n");
}

/* TODO: try to make this a generic function, move it into mon_register.c and
         remove mon_register_list_get from the monitor_cpu_type_t struct */
static mon_reg_list_t *mon_register_list_get65816(int mem)
{
    mon_reg_list_t *mon_reg_list, *regs;

    mon_reg_list = regs = lib_malloc(sizeof(mon_reg_list_t) * REG_LIST_65816_SIZE);
    memcpy(mon_reg_list, mon_reg_list_65816, sizeof(mon_reg_list_t) * REG_LIST_65816_SIZE);

    do {
        regs->val = (unsigned int)mon_register_get_val(mem, regs->id);
        ++regs;
    } while (regs->name != NULL);

    return mon_reg_list;
}

void mon_register65816_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_register_get_val = mon_register_get_val;
    monitor_cpu_type->mon_register_set_val = mon_register_set_val;
    monitor_cpu_type->mon_register_print = mon_register_print;
    monitor_cpu_type->mon_register_print_ex = NULL;
    monitor_cpu_type->mon_register_list_get = mon_register_list_get65816;
}
