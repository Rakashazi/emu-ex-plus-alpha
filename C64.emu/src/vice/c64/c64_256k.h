/*
 * c64_256k.h - 256K EXPANSION emulation.
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

#ifndef VICE_C64_256K_H
#define VICE_C64_256K_H

#include "snapshot.h"
#include "types.h"

extern int c64_256k_start;
extern int c64_256k_enabled;

int c64_256k_resources_init(void);
void c64_256k_resources_shutdown(void);
int c64_256k_cmdline_options_init(void);

void c64_256k_init(void);
void c64_256k_reset(void);
void c64_256k_cia_set_vbank(int ciabank);
void c64_256k_shutdown(void);

void c64_256k_ram_inject(uint16_t addr, uint8_t value);
void c64_256k_ram_segment0_store(uint16_t addr, uint8_t value);
void c64_256k_ram_segment1_store(uint16_t addr, uint8_t value);
void c64_256k_ram_segment2_store(uint16_t addr, uint8_t value);
void c64_256k_ram_segment3_store(uint16_t addr, uint8_t value);
uint8_t c64_256k_ram_segment0_read(uint16_t addr);
uint8_t c64_256k_ram_segment1_read(uint16_t addr);
uint8_t c64_256k_ram_segment2_read(uint16_t addr);
uint8_t c64_256k_ram_segment3_read(uint16_t addr);

int set_c64_256k_enabled(int value, int disable_reset);

int c64_256k_snapshot_write(struct snapshot_s *s);
int c64_256k_snapshot_read(struct snapshot_s *s);

uint8_t c64_256k_read(uint16_t addr);
void c64_256k_store(uint16_t addr, uint8_t byte);

#endif
