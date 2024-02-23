/*
 * plus4memcsory256k.h - CSORY 256K EXPANSION emulation.
 *
 * Written by
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

#ifndef VICE_CS256K_H
#define VICE_CS256K_H

#include "types.h"

extern int cs256k_enabled;

void cs256k_init(void);
void cs256k_reset(void);
void cs256k_shutdown(void);

void cs256k_ram_inject(uint16_t addr, uint8_t value);
void cs256k_store(uint16_t addr, uint8_t value);
uint8_t cs256k_read(uint16_t addr);

int set_cs256k_enabled(int value);

#endif
