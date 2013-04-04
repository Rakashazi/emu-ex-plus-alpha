/*
 * mon_disassemble.h - The VICE built-in monitor, disassembler module.
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

#ifndef VICE_MON_DISASSEMBLE_H
#define VICE_MON_DISASSEMBLE_H

#include "montypes.h"
#include "types.h"

extern const char *mon_disassemble_to_string_ex(MEMSPACE, unsigned int addr, unsigned int x,
                                                unsigned int p1, unsigned int p2, unsigned int p3,
                                                int hex_mode, unsigned *len);

extern void mon_disassemble_with_regdump(MEMSPACE mem, unsigned int addr);

extern void mon_disassemble_lines(MON_ADDR start_addr, MON_ADDR end_addr);

extern unsigned mon_disassemble_instr(MON_ADDR addr);

#endif
