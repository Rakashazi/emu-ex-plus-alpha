/*
 * c128cart.h -- c128 cartridge memory interface.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_C128CART_H
#define VICE_C128CART_H

#include "types.h"

int c128cartridge_cmdline_options_init(void);
int c128cartridge_resources_init(void);
void c128cartridge_resources_shutdown(void);

/* only x128 actually implements this function */
void c128cartridge_setup_interface(void);

/* Image of the external function ROM.  */
#define EXTERNAL_FUNCTION_ROM_SIZE  0x8000
#define EXTERNAL_FUNCTION_ROM_BANKS 0x40    /* max: "Magic Desk 128" has 64 */
extern uint8_t ext_function_rom[EXTERNAL_FUNCTION_ROM_SIZE * EXTERNAL_FUNCTION_ROM_BANKS];

void external_function_rom_set_bank(int value);

uint8_t external_function_rom_read(uint16_t addr);
void external_function_rom_store(uint16_t addr, uint8_t value);
void external_function_top_shared_store(uint16_t addr, uint8_t value);
uint8_t external_function_rom_peek(uint16_t addr);
uint8_t c128cartridge_hi_read(uint16_t addr, uint8_t *value);
uint8_t c128cartridge_hi_store(uint16_t addr, uint8_t value);
uint8_t c128cartridge_basic_hi_read(uint16_t addr, uint8_t *value);
uint8_t c128cartridge_basic_hi_store(uint16_t addr, uint8_t value);
uint8_t c128cartridge_ram_read(uint16_t addr, uint8_t *value);
uint8_t c128cartridge_ram_store(uint16_t addr, uint8_t value);
int c128cartridge_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit, int mem_config);
void c128cartridge_switch_mode(int mode);

#endif
