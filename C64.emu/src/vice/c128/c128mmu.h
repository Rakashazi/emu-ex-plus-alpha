/*
 * c128mmu.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_C128MMU_H
#define VICE_C128MMU_H

#include "types.h"

extern uint8_t mmu_read(uint16_t addr);
extern uint8_t mmu_peek(uint16_t addr);
extern void mmu_store(uint16_t address, uint8_t value);
extern uint8_t mmu_ffxx_read(uint16_t addr);
extern void mmu_ffxx_store(uint16_t addr, uint8_t value);
extern uint8_t mmu_ffxx_read_z80(uint16_t addr);

extern void mmu_reset(void);
extern void mmu_set_config64(int config);
extern int mmu_is_c64config(void);

extern void mmu_init(void);
extern int mmu_resources_init(void);
extern int mmu_cmdline_options_init(void);

extern int mmu_dump(void *context, uint16_t addr);

#endif
