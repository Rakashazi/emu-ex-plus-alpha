/*
 * mon_register.h - The VICE built-in monitor register functions.
 *                  CPU agnostic functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifdef DEBUG_MON_REGS
#define DBG(_x_) printf _x_
#else
#define DBG(_x_)
#endif

/* check if register id is valid, returns 1 on valid, 0 on invalid */
int mon_register_valid(int mem, int reg_id)
{
    mon_reg_list_t *mon_reg_list, *regs;
    int ret = 0;

    DBG(("mon_register_valid mem: %d id: %d\n", mem, reg_id));
    if (monitor_diskspace_dnr(mem) >= 0) {
        if (!check_drive_emu_level_ok(monitor_diskspace_dnr(mem) + 8)) {
            return 0;
        }
    }

    mon_reg_list = regs = mon_register_list_get(mem);

    do {
        if ((!(regs->flags & MON_REGISTER_IS_MEMORY)) && (regs->id == (unsigned int)reg_id)) {
            ret = 1;
            break;
        }
        ++regs;
    } while (regs->name != NULL);

    lib_free(mon_reg_list);

    return ret;
}

/* takes a register by name, and returns its id. returns -1 on error */
int mon_register_name_to_id(int mem, char *name)
{
    mon_reg_list_t *reg_list, *regs;
    int reg_id = -1;

    DBG(("mon_register_name_to_id reg: %s\n", name));
    reg_list = regs = mon_register_list_get(mem);
    do {
        if (!strcmp(regs->name, name)) {
            reg_id = regs->id;
            break;
        }
        ++regs;
    } while (regs->name != NULL);
    lib_free(reg_list);
    DBG(("mon_register_name_to_id found id: %d\n", reg_id));
    return reg_id;
}

/* checks if a given string is a valid register name,
   returns 1 on valid, 0 on invalid */
int mon_register_name_valid(int mem, char *name)
{
    DBG(("mon_register_name_valid reg: %s\n", name));
    return mon_register_name_to_id(mem, name) == -1 ? 0 : 1;
}

/* takes a register by id, and returns its value. returns -1 on error */
int mon_register_id_to_value(int mem, int reg_id)
{
    DBG(("mon_register_id_to_value reg: %d\n", reg_id));
    if (mon_register_valid(mem, reg_id)) {
        return (monitor_cpu_for_memspace[mem]->mon_register_get_val)(mem, reg_id);
    }
    return -1;
}

/* takes a register by name, and returns its value. returns -1 on error */
int mon_register_name_to_value(int mem, char *name)
{
    int reg_id = mon_register_name_to_id(mem, name);
    DBG(("mon_register_name_to_value reg: %s id: %d\n", name, reg_id));
    if (reg_id >= 0) {
        return mon_register_id_to_value(mem, reg_id);
    }
    return -1;
}


