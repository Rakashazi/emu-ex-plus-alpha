/*
 * mon_register6809.c - The VICE built-in monitor 6809 register functions.
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
#include "h6809regs.h"

#ifdef DEBUG_MON_REGS
#define DBG(_x_) printf _x_
#else
#define DBG(_x_)
#endif

/* TODO: make the other functions here use this table. when done also do the
 *       same with the other CPUs and finally move common code to mon_register.c
 */

#define REG_LIST_6809_SIZE (11 + 1)
static mon_reg_list_t mon_reg_list_6809[REG_LIST_6809_SIZE] = {
    {       "X",     e_X, 16,                     0, 0, 0 },
    {       "Y",     e_Y, 16,                     0, 0, 0 },
    {       "U",     e_U, 16,                     0, 0, 0 },
    {       "S",    e_SP, 16,                     0, 0, 0 },
    {      "PC",    e_PC, 16,                     0, 0, 0 },
    {      "DP",    e_DP,  8,                     0, 0, 0 },
    {      "CC", e_FLAGS,  8,                     0, 0, 0 },
    {"EFHINZVC", e_FLAGS,  8, MON_REGISTER_IS_FLAGS, 0, 0 },
    {       "A",     e_A,  8,                     0, 0, 0 },
    {       "B",     e_B,  8,                     0, 0, 0 },
    {       "D",     e_D, 16,                     0, 0, 0 },
#if 0
    /* 6309 specific registers, for future support */
    {       "E",     e_E,  8,                     0, 0, 0 },
    {       "F",     e_F,  8,                     0, 0, 0 },
    {       "W",     e_W, 16,                     0, 0, 0 },
    {       "Q",     e_W, 32,                     0, 0, 0 },
    {       "V",     e_V, 16,                     0, 0, 0 },
    {       "MD",   e_MD,  8, MON_REGISTER_IS_FLAGS, 0, 0 },
#endif
    { NULL, -1,  0,  0, 0, 0 }
};

static unsigned int mon_register_get_val(int mem, int reg_id)
{
    h6809_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return 0;
        }
    }

    reg_ptr = mon_interfaces[mem]->h6809_cpu_regs;

    switch (reg_id) {
        case e_X:
            return H6809_REGS_GET_X(reg_ptr);
        case e_Y:
            return H6809_REGS_GET_Y(reg_ptr);
        case e_U:
            return H6809_REGS_GET_U(reg_ptr);
        case e_SP:
            return H6809_REGS_GET_S(reg_ptr);
        case e_PC:
            return H6809_REGS_GET_PC(reg_ptr);
        case e_DP:
            return H6809_REGS_GET_DP(reg_ptr);
        case e_FLAGS:
            return H6809_REGS_GET_CC(reg_ptr);
        case e_A:
            return H6809_REGS_GET_A(reg_ptr);
        case e_B:
            return H6809_REGS_GET_B(reg_ptr);
        case e_D:
            return H6809_REGS_GET_D(reg_ptr);
#if 0
        /* 6309 specific registers, for future support */
        case e_E:
            return H6809_REGS_GET_E(reg_ptr);
        case e_F:
            return H6809_REGS_GET_F(reg_ptr);
        case e_W:
            return H6809_REGS_GET_W(reg_ptr);
        case e_Q:
            return H6809_REGS_GET_Q(reg_ptr);
        case e_V:
            return H6809_REGS_GET_V(reg_ptr);
        case e_MD:
            return H6809_REGS_GET_MD(reg_ptr);
#endif
        default:
            log_error(LOG_ERR, "Unknown register!");
    }
    return 0;
}

static void mon_register_set_val(int mem, int reg_id, WORD val)
{
    h6809_regs_t *reg_ptr;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    }

    reg_ptr = mon_interfaces[mem]->h6809_cpu_regs;

    switch (reg_id) {
        case e_X:
            H6809_REGS_SET_X(reg_ptr, val);
            break;
        case e_Y:
            H6809_REGS_SET_Y(reg_ptr, val);
            break;
        case e_U:
            H6809_REGS_SET_U(reg_ptr, val);
            break;
        case e_SP:
            H6809_REGS_SET_S(reg_ptr, val);
            break;
        case e_PC:
            H6809_REGS_SET_PC(reg_ptr, val);
            break;
        case e_DP:
            H6809_REGS_SET_DP(reg_ptr, (BYTE)val);
            break;
        case e_FLAGS:
            H6809_REGS_SET_CC(reg_ptr, (BYTE)val);
            break;
        case e_A:
            H6809_REGS_SET_A(reg_ptr, (BYTE)val);
            break;
        case e_B:
            H6809_REGS_SET_B(reg_ptr, (BYTE)val);
            break;
        case e_D:
            H6809_REGS_SET_D(reg_ptr, val);
            break;
#if 0
        /* 6309 specific registers, for future support */
        case e_E:
            H6809_REGS_SET_E(reg_ptr, (BYTE)val);
            break;
        case e_F:
            H6809_REGS_SET_F(reg_ptr, (BYTE)val);
            break;
        case e_W:
            H6809_REGS_SET_W(reg_ptr, (WORD)val);
            break;
        case e_Q:
            H6809_REGS_SET_Q(reg_ptr, (DWORD)val);
            break;
        case e_V:
            H6809_REGS_SET_V(reg_ptr, (WORD)val);
            break;
        case e_MD:
            H6809_REGS_SET_MD(reg_ptr, (BYTE)val);
            break;
#endif
        default:
            log_error(LOG_ERR, "Unknown register!");
            return;
    }
    force_array[mem] = 1;
}

/* TODO: should use mon_register_list_get */
static void mon_register_print(int mem)
{
    h6809_regs_t *regs;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return;
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return;
    }
    regs = mon_interfaces[mem]->h6809_cpu_regs;

    mon_out("  ADDR A  B  X    Y    SP   U    DP EFHINZVC\n");
    mon_out(".;%04x %02x %02x %04x %04x %04x %04x %02x %c%c%c%c%c%c%c%c\n",
            addr_location(mon_register_get_val(mem, e_PC)),
            mon_register_get_val(mem, e_A),
            mon_register_get_val(mem, e_B),
            mon_register_get_val(mem, e_X),
            mon_register_get_val(mem, e_Y),
            mon_register_get_val(mem, e_SP),
            mon_register_get_val(mem, e_U),
            mon_register_get_val(mem, e_DP),
            (H6809_REGS_TEST_E(regs) ? '1' : '.'),
            (H6809_REGS_TEST_F(regs) ? '1' : '.'),
            (H6809_REGS_TEST_H(regs) ? '1' : '.'),
            (H6809_REGS_TEST_I(regs) ? '1' : '.'),
            (H6809_REGS_TEST_N(regs) ? '1' : '.'),
            (H6809_REGS_TEST_Z(regs) ? '1' : '.'),
            (H6809_REGS_TEST_V(regs) ? '1' : '.'),
            (H6809_REGS_TEST_C(regs) ? '1' : '.')
            );
}

/* TODO: should use mon_register_list_get */
static const char* mon_register_print_ex(int mem)
{
    static char buff[80];
    h6809_regs_t *regs;

    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return "";
        }
    } else if (mem != e_comp_space) {
        log_error(LOG_ERR, "Unknown memory space!");
        return "";
    }

    regs = mon_interfaces[mem]->h6809_cpu_regs;

    sprintf(buff, "A:%02X B:%02X X:%04X Y:%04X SP:%04X U:%04X DP:%02x %c%c%c%c%c%c%c%c",
            mon_register_get_val(mem, e_A),
            mon_register_get_val(mem, e_B),
            mon_register_get_val(mem, e_X),
            mon_register_get_val(mem, e_Y),
            mon_register_get_val(mem, e_SP),
            mon_register_get_val(mem, e_U),
            mon_register_get_val(mem, e_DP),
            H6809_REGS_TEST_E(regs) ? 'E' : '.',
            H6809_REGS_TEST_F(regs) ? 'F' : '.',
            H6809_REGS_TEST_H(regs) ? 'H' : '.',
            H6809_REGS_TEST_I(regs) ? 'I' : '.',
            H6809_REGS_TEST_N(regs) ? 'N' : '.',
            H6809_REGS_TEST_Z(regs) ? 'Z' : '.',
            H6809_REGS_TEST_V(regs) ? 'V' : '.',
            H6809_REGS_TEST_C(regs) ? 'C' : '.'
            );

    return buff;
}

/* TODO: try to make this a generic function, move it into mon_register.c and
         remove mon_register_list_get from the monitor_cpu_type_t struct */
static mon_reg_list_t *mon_register_list_get6809(int mem)
{
    mon_reg_list_t *mon_reg_list, *regs;

    mon_reg_list = regs = lib_malloc(sizeof(mon_reg_list_t) * REG_LIST_6809_SIZE);
    memcpy(mon_reg_list, mon_reg_list_6809, sizeof(mon_reg_list_t) * REG_LIST_6809_SIZE);

    do {
        regs->val = (unsigned int)mon_register_get_val(mem, regs->id);
        ++regs;
    } while (regs->name != NULL);

    return mon_reg_list;
}

void mon_register6809_init(monitor_cpu_type_t *monitor_cpu_type)
{
    monitor_cpu_type->mon_register_get_val = mon_register_get_val;
    monitor_cpu_type->mon_register_set_val = mon_register_set_val;
    monitor_cpu_type->mon_register_print = mon_register_print;
    monitor_cpu_type->mon_register_print_ex = mon_register_print_ex;
    monitor_cpu_type->mon_register_list_get = mon_register_list_get6809;
}
