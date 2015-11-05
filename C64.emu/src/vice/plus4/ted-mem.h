/*
 * ted-mem.h - Memory interface for the TED emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_TED_MEM_H
#define VICE_TED_MEM_H

#include "types.h"

extern void ted_store(WORD addr, BYTE value);
extern BYTE ted_read(WORD addr);
extern BYTE ted_peek(WORD addr);
extern BYTE colorram_read(WORD addr);
extern void colorram_store(WORD addr, BYTE value);
extern void ted_mem_vbank_store(WORD addr, BYTE value);
extern void ted_mem_vbank_store_32k(WORD addr, BYTE value);
extern void ted_mem_vbank_store_16k(WORD addr, BYTE value);
#if 0
extern void ted_mem_vbank_39xx_store(WORD addr, BYTE value);
extern void ted_mem_vbank_3fxx_store(WORD addr, BYTE value);
#endif

#endif
