/*
 * plus4memrom.h -- Plus4 ROM access.
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

#ifndef VICE_PLUS4MEMROM_H
#define VICE_PLUS4MEMROM_H

#include "types.h"

extern BYTE plus4memrom_basic_rom[];
extern BYTE plus4memrom_kernal_rom[];
extern BYTE plus4memrom_kernal_trap_rom[];

extern BYTE plus4memrom_kernal_read(WORD addr);
extern BYTE plus4memrom_basic_read(WORD addr);
extern BYTE plus4memrom_trap_read(WORD addr);
extern void plus4memrom_trap_store(WORD addr, BYTE value);

extern BYTE plus4memrom_extromlo1_read(WORD addr);
extern BYTE plus4memrom_extromlo2_read(WORD addr);
extern BYTE plus4memrom_extromlo3_read(WORD addr);
extern BYTE plus4memrom_extromhi1_read(WORD addr);
extern BYTE plus4memrom_extromhi2_read(WORD addr);
extern BYTE plus4memrom_extromhi3_read(WORD addr);

extern BYTE plus4memrom_rom_read(WORD addr);
extern void plus4memrom_rom_store(WORD addr, BYTE value);

#endif
