/*
 * functionrom.c
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

#ifndef VICE_FUNCTIONROM_H
#define VICE_FUNCTIONROM_H

#include "types.h"

extern BYTE int_function_rom[];
extern BYTE ext_function_rom[];

extern BYTE internal_function_rom_read(WORD addr);
extern void internal_function_rom_store(WORD addr, BYTE value);
extern void internal_function_top_shared_store(WORD addr, BYTE value);
extern BYTE external_function_rom_read(WORD addr);
extern void external_function_rom_store(WORD addr, BYTE value);
extern void external_function_top_shared_store(WORD addr, BYTE value);

extern int functionrom_resources_init(void);
extern void functionrom_resources_shutdown(void);
extern int functionrom_cmdline_options_init(void);

#define INT_FUNCTION_NONE   0
#define INT_FUNCTION_ROM    1
#define INT_FUNCTION_RAM    2
#define INT_FUNCTION_RTC    3

#define EXT_FUNCTION_NONE   0
#define EXT_FUNCTION_ROM    1
#define EXT_FUNCTION_RAM    2
#define EXT_FUNCTION_RTC    3

#endif
