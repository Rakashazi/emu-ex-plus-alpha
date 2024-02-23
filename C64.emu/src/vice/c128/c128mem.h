/*
 * c128mem.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on the original work in VICE 0.11.0 by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#ifndef VICE_C128MEM_H
#define VICE_C128MEM_H

#include "mem.h"
#include "types.h"

#define C128_RAM_SIZE               0x40000

#define C128_EDITOR_ROM_SIZE        0x1000  /* 0x0000 - 0x0fff in kernal image */
#define C128_Z80BIOS_ROM_SIZE       0x1000  /* 0x1000 - 0x1fff in kernal image */
#define C128_KERNAL_ROM_SIZE        0x2000  /* 0x2000 - 0x3fff in kernal image */

#define C128_BASIC_ROM_SIZE         0x8000  /* BASICLO + BASICHI */
#define C128_CHARGEN_ROM_SIZE       0x2000

#define C128_BASIC_ROM_IMAGELO_SIZE 0x4000
#define C128_BASIC_ROM_IMAGEHI_SIZE 0x4000
#define C128_KERNAL_ROM_IMAGE_SIZE  0x4000  /* Editor + Z80BIOS + Kernal */

#define C128_KERNAL64_ROM_SIZE      0x2000
#define C128_BASIC64_ROM_SIZE       0x2000

int c128_mem_init_resources(void);
int c128_mem_init_cmdline_options(void);

void mem_update_config(int config);
void mem_set_machine_type(unsigned type);
void mem_set_ram_config(uint8_t value);
void mem_set_ram_bank(uint8_t value);
void mem_set_vbank(int new_vbank);
void mem_set_tape_sense(int sense);
void mem_pla_config_changed(void);

void mem_set_write_hook(int config, int page, store_func_t *f);
void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func);
void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr);
void mem_read_limit_set(unsigned int base, unsigned int index, uint32_t limit);

uint8_t ram_read(uint16_t addr);
void ram_store(uint16_t addr, uint8_t value);
uint8_t ram_peek(uint16_t addr);

uint8_t one_read(uint16_t addr);
void one_store(uint16_t addr, uint8_t value);
uint8_t one_peek(uint16_t addr);

uint8_t z80_read_zero(uint16_t addr);
void z80_store_zero(uint16_t addr, uint8_t value);
uint8_t z80_peek_zero(uint16_t addr);

void colorram_store(uint16_t addr, uint8_t value);
uint8_t colorram_read(uint16_t addr);
uint8_t colorram_peek(uint16_t addr);

uint8_t d7xx_read(uint16_t addr);
void d7xx_store(uint16_t addr, uint8_t value);

uint8_t lo_read(uint16_t addr);
void lo_store(uint16_t addr, uint8_t value);
uint8_t lo_peek(uint16_t addr);

uint8_t hi_read(uint16_t addr);
void hi_store(uint16_t addr, uint8_t value);

uint8_t top_shared_read(uint16_t addr);
void top_shared_store(uint16_t addr, uint8_t value);
uint8_t top_shared_peek(uint16_t addr);

uint8_t editor_read(uint16_t addr);
void editor_store(uint16_t addr, uint8_t value);

uint8_t basic_read(uint16_t addr);
void basic_store(uint16_t addr, uint8_t value);
uint8_t kernal_read(uint16_t addr);
void kernal_store(uint16_t addr, uint8_t value);
uint8_t chargen_read(uint16_t addr);
void chargen_store(uint16_t addr, uint8_t value);

uint8_t basic_lo_read(uint16_t addr);
void basic_lo_store(uint16_t addr, uint8_t value);
uint8_t basic_hi_read(uint16_t addr);
void basic_hi_store(uint16_t addr, uint8_t value);

extern uint8_t *ram_bank;
extern uint8_t *dma_bank;

extern uint8_t mem_chargen_rom[C128_CHARGEN_ROM_SIZE];

uint8_t c128_c64io_d000_read(uint16_t addr);
void c128_c64io_d000_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d100_read(uint16_t addr);
void c128_c64io_d100_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d200_read(uint16_t addr);
void c128_c64io_d200_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d300_read(uint16_t addr);
void c128_c64io_d300_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d400_read(uint16_t addr);
void c128_c64io_d400_store(uint16_t addr, uint8_t value);
uint8_t c128_d5xx_read(uint16_t addr);
void c128_d5xx_store(uint16_t addr, uint8_t value);
uint8_t c128_mmu_read(uint16_t addr);
void c128_mmu_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d600_read(uint16_t addr);
void c128_c64io_d600_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_d700_read(uint16_t addr);
void c128_c64io_d700_store(uint16_t addr, uint8_t value);
uint8_t c128_colorram_read(uint16_t addr);
void c128_colorram_store(uint16_t addr, uint8_t value);
uint8_t c128_cia1_read(uint16_t addr);
void c128_cia1_store(uint16_t addr, uint8_t value);
uint8_t c128_cia2_read(uint16_t addr);
void c128_cia2_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_de00_read(uint16_t addr);
void c128_c64io_de00_store(uint16_t addr, uint8_t value);
uint8_t c128_c64io_df00_read(uint16_t addr);
void c128_c64io_df00_store(uint16_t addr, uint8_t value);

void mem_initialize_go64_memory_bank(uint8_t shared_mem);

/* add due to incompatibilities with c64mem.h */

uint8_t mem_read_without_ultimax(uint16_t addr);
void mem_store_without_ultimax(uint16_t addr, uint8_t value);
void mem_store_without_romlh(uint16_t addr, uint8_t value);
void ram_hi_store(uint16_t addr, uint8_t value);
void mem_set_tape_write_in(int val);
void mem_set_tape_motor_in(int val);
void store_bank_io(uint16_t addr, uint8_t byte);
uint8_t read_bank_io(uint16_t addr);

#endif
