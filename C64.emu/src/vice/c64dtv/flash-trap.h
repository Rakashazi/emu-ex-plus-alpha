/*
 * flash-trap.h
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_FLASH_TRAP_H
#define VICE_FLASH_TRAP_H

#include "traps.h"
#include "types.h"

extern int flash_trap_init(const struct trap_s *trap_list);
extern void flash_trap_shutdown(void);

extern int flash_trap_seek_next(void);
extern int flash_trap_load_body(void);
extern void flash_traps_reset(void);

extern int flash_trap_resources_init(void);
extern void flash_trap_resources_shutdown(void);
extern int flash_trap_cmdline_options_init(void);

#endif
