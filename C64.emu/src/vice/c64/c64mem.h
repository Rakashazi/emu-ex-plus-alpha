/*
 * c64mem.h -- C64 memory handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_C64MEM_H
#define VICE_C64MEM_H

#include "mem.h"
#include "types.h"

#ifndef C64_RAM_SIZE
#define C64_RAM_SIZE 0x10000
#endif

#define C64_KERNAL_ROM_SIZE  0x2000
#define C64_BASIC_ROM_SIZE   0x2000
#define C64_CHARGEN_ROM_SIZE 0x1000

int c64_mem_init_resources(void);
int c64_mem_init_cmdline_options(void);

void mem_set_vbank(int new_vbank);

uint8_t ram_read(uint16_t addr);
void ram_store(uint16_t addr, uint8_t value);
void ram_hi_store(uint16_t addr, uint8_t value);

uint8_t chargen_read(uint16_t addr);
void chargen_store(uint16_t addr, uint8_t value);

void colorram_store(uint16_t addr, uint8_t value);
uint8_t colorram_read(uint16_t addr);

void mem_pla_config_changed(void);
void mem_set_tape_sense(int sense);
void mem_set_tape_write_in(int val);
void mem_set_tape_motor_in(int val);

extern uint8_t mem_chargen_rom[C64_CHARGEN_ROM_SIZE];

void mem_set_write_hook(int config, int page, store_func_t *f);
void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func);
void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr);
void mem_read_limit_set(unsigned int base, unsigned int index, uint32_t limit);

void mem_store_without_ultimax(uint16_t addr, uint8_t value);
uint8_t mem_read_without_ultimax(uint16_t addr);
void mem_store_without_romlh(uint16_t addr, uint8_t value);

void store_bank_io(uint16_t addr, uint8_t byte);
uint8_t read_bank_io(uint16_t addr);

void c64_mem_init(void);

int c64_mem_ui_init_early(void);
int c64_mem_ui_init(void);
void c64_mem_ui_shutdown(void);

uint8_t vsid_io_read(uint16_t addr);
void vsid_io_store(uint16_t addr, uint8_t val);

#endif
