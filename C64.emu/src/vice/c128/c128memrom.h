/*
 * c128memrom.h -- C128 ROM access.
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

#ifndef VICE_C128MEMROM_H
#define VICE_C128MEMROM_H

#include "types.h"

extern BYTE c128memrom_basic_rom[];
extern BYTE c128memrom_kernal_rom[];
extern BYTE c128memrom_kernal_trap_rom[];

extern BYTE c128memrom_basic_read(WORD addr);
extern void c128memrom_basic_store(WORD addr, BYTE value);
extern BYTE c128memrom_kernal_read(WORD addr);
extern void c128memrom_kernal_store(WORD addr, BYTE value);
extern BYTE c128memrom_trap_read(WORD addr);
extern void c128memrom_trap_store(WORD addr, BYTE value);

extern BYTE c128memrom_rom_read(WORD addr);
extern void c128memrom_rom_store(WORD addr, BYTE value);

#endif
