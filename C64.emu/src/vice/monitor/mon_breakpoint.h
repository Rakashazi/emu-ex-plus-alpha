/*
 * mon_breakpoint.h - The VICE built-in monitor breakpoint functions.
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

#ifndef VICE_MON_BREAKPOINT_H
#define VICE_MON_BREAKPOINT_H

#include "montypes.h"

typedef enum mon_breakpoint_type_e {
    BP_NONE,
    BP_INACTIVE,
    BP_ACTIVE
} mon_breakpoint_type_t;

extern void mon_breakpoint_init(void);

extern void mon_breakpoint_switch_checkpoint(int op, int breakpt_num);
extern void mon_breakpoint_set_ignore_count(int breakpt_num, int count);
extern void mon_breakpoint_print_checkpoints(void);
extern void mon_breakpoint_delete_checkpoint(int brknum);
extern void mon_breakpoint_set_checkpoint_condition(int brk_num, struct cond_node_s *cnode);
extern void mon_breakpoint_set_checkpoint_command(int brk_num, char *cmd);
extern bool mon_breakpoint_check_checkpoint(MEMSPACE mem, unsigned int addr,
                                            unsigned int lastpc, MEMORY_OP op);
extern int mon_breakpoint_add_checkpoint(MON_ADDR start_addr, MON_ADDR end_addr,
                                         bool stop, MEMORY_OP op, bool is_temp);

extern mon_breakpoint_type_t mon_breakpoint_is(MON_ADDR address);
extern void mon_breakpoint_set(MON_ADDR address);
extern void mon_breakpoint_unset(MON_ADDR address);
extern void mon_breakpoint_enable(MON_ADDR address);
extern void mon_breakpoint_disable(MON_ADDR address);

/* defined in mon_parse.y, and thus, in mon_parse.c */
extern void parse_and_execute_line(char *input);

#endif
