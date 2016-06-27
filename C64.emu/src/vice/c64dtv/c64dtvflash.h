/*
 * c64dtvmem.h - C64DTV memory (2MB RAM/ROM) implementation
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

#ifndef VICE_C64DTVFLASH_H
#define VICE_C64DTVFLASH_H

#include "types.h"

extern int c64dtvflash_resources_init(void);
extern void c64dtvflash_resources_shutdown(void);
extern int c64dtvflash_cmdline_options_init(void);
extern void c64dtvflash_init(void);
extern void c64dtvflash_reset(void);
extern void c64dtvflash_shutdown(void);

extern BYTE c64dtvflash_read(int addr);
extern void c64dtvflash_store(int addr, BYTE value);
extern BYTE c64dtvflash_read_direct(int addr);
extern void c64dtvflash_store_direct(int addr, BYTE value);

extern void c64dtvflash_create_blank_image(char *filename, int copyroms);

extern BYTE c64dtvflash_mem[];
extern BYTE c64dtvflash_mem_lock[];
extern BYTE c64dtvflash_state;

extern int c64dtvflash_rom_loaded;

#endif
