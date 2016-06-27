/*
 * vicii-mem.h - Memory interface for the MOS6569 (VIC-II) emulation.
 *
 * Written by
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

#ifndef VICE_VICII_MEM_H
#define VICE_VICII_MEM_H

#include "types.h"

extern void vicii_store(WORD addr, BYTE value);
extern BYTE vicii_read(WORD addr);
extern BYTE vicii_peek(WORD addr);
extern void vicii_mem_vbank_store(WORD addr, BYTE value);
extern void vicii_mem_vbank_39xx_store(WORD addr, BYTE value);
extern void vicii_mem_vbank_3fxx_store(WORD addr, BYTE value);
extern void vicii_palette_store(WORD addr, BYTE value);
extern BYTE vicii_palette_read(WORD addr);
extern int vicii_extended_regs(void);
extern void viciidtv_update_colorram(void);

#endif
