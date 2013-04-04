/*
 * c64memrom.h -- C64 ROM access.
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

#ifndef VICE_C64MEMROM_H
#define VICE_C64MEMROM_H

#include "types.h"

extern BYTE c64memrom_basic64_rom[];
extern BYTE c64memrom_kernal64_rom[];
extern BYTE c64memrom_kernal64_trap_rom[];

extern BYTE c64memrom_kernal64_read(WORD addr);
extern BYTE c64memrom_basic64_read(WORD addr);
extern BYTE c64memrom_trap_read(WORD addr);
extern void c64memrom_trap_store(WORD addr, BYTE value);

extern BYTE c64memrom_rom64_read(WORD addr);
extern void c64memrom_rom64_store(WORD addr, BYTE value);

#endif
