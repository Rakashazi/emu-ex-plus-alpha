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

extern uint8_t plus4memrom_basic_rom[];
extern uint8_t plus4memrom_kernal_rom[];
extern uint8_t plus4memrom_kernal_trap_rom[];

extern uint8_t plus4memrom_kernal_read(uint16_t addr);
extern uint8_t plus4memrom_basic_read(uint16_t addr);
extern uint8_t plus4memrom_trap_read(uint16_t addr);
extern void plus4memrom_trap_store(uint16_t addr, uint8_t value);

extern uint8_t plus4memrom_rom_read(uint16_t addr);
extern void plus4memrom_rom_store(uint16_t addr, uint8_t value);

/* c0 - internal "function rom" */
extern int plus4cart_load_func_lo(const char *rom_name);
extern int plus4cart_load_func_hi(const char *rom_name);
extern uint8_t plus4memrom_extromlo1_read(uint16_t addr);
extern uint8_t plus4memrom_extromhi1_read(uint16_t addr);

/* c2 - internal expansion rom */
/* FIXME: c2 can also be used at the expansion port */
extern int plus4cart_load_c2lo(const char *rom_name);
extern int plus4cart_load_c2hi(const char *rom_name);
extern uint8_t plus4memrom_extromlo3_read(uint16_t addr);
extern uint8_t plus4memrom_extromhi3_read(uint16_t addr);

#endif
