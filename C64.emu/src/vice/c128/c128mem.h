/*
 * c128mem.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#define C128_KERNAL_ROM_SIZE        0x2000
#define C128_BASIC_ROM_SIZE         0x8000
#define C128_EDITOR_ROM_SIZE        0x1000
#define C128_Z80BIOS_ROM_SIZE       0x1000
#define C128_CHARGEN_ROM_SIZE       0x2000

#define C128_BASIC_ROM_IMAGELO_SIZE 0x4000
#define C128_BASIC_ROM_IMAGEHI_SIZE 0x4000
#define C128_KERNAL_ROM_IMAGE_SIZE  0x4000

#define C128_KERNAL64_ROM_SIZE      0x2000
#define C128_BASIC64_ROM_SIZE       0x2000

#define C128_BASIC_CHECKSUM_85      38592
#define C128_BASIC_CHECKSUM_86      2496
#define C128_EDITOR_CHECKSUM_R01    56682
#define C128_EDITOR_CHECKSUM_R01SWE 9364
#define C128_EDITOR_CHECKSUM_R01GER 9619
#define C128_KERNAL_CHECKSUM_R01    22353
#define C128_KERNAL_CHECKSUM_R01SWE 24139
#define C128_KERNAL_CHECKSUM_R01GER 22098

extern int c128_mem_init_resources(void);
extern int c128_mem_init_cmdline_options(void);

extern void mem_update_config(int config);
extern void mem_set_machine_type(unsigned type);
extern void mem_set_ram_config(BYTE value);
extern void mem_set_ram_bank(BYTE value);
extern void mem_set_vbank(int new_vbank);
extern void mem_set_tape_sense(int sense);
extern void mem_pla_config_changed(void);

extern void mem_set_write_hook(int config, int page, store_func_t *f);
extern void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func);
extern void mem_read_base_set(unsigned int base, unsigned int index, BYTE *mem_ptr);

extern BYTE ram_read(WORD addr);
extern void ram_store(WORD addr, BYTE value);

extern BYTE one_read(WORD addr);
extern void one_store(WORD addr, BYTE value);

extern void colorram_store(WORD addr, BYTE value);
extern BYTE colorram_read(WORD addr);

extern BYTE d7xx_read(WORD addr);
extern void d7xx_store(WORD addr, BYTE value);

extern BYTE lo_read(WORD addr);
extern void lo_store(WORD addr, BYTE value);

extern BYTE hi_read(WORD addr);
extern void hi_store(WORD addr, BYTE value);

extern BYTE top_shared_read(WORD addr);
extern void top_shared_store(WORD addr, BYTE value);

extern BYTE editor_read(WORD addr);
extern void editor_store(WORD addr, BYTE value);

extern BYTE basic_read(WORD addr);
extern void basic_store(WORD addr, BYTE value);
extern BYTE kernal_read(WORD addr);
extern void kernal_store(WORD addr, BYTE value);
extern BYTE chargen_read(WORD addr);
extern void chargen_store(WORD addr, BYTE value);

extern BYTE basic_lo_read(WORD addr);
extern void basic_lo_store(WORD addr, BYTE value);
extern BYTE basic_hi_read(WORD addr);
extern void basic_hi_store(WORD addr, BYTE value);

extern BYTE *ram_bank;

extern BYTE mem_chargen_rom[C128_CHARGEN_ROM_SIZE];

extern BYTE c128_c64io_d000_read(WORD addr);
extern void c128_c64io_d000_store(WORD addr, BYTE value);
extern BYTE c128_c64io_d100_read(WORD addr);
extern void c128_c64io_d100_store(WORD addr, BYTE value);
extern BYTE c128_c64io_d200_read(WORD addr);
extern void c128_c64io_d200_store(WORD addr, BYTE value);
extern BYTE c128_c64io_d300_read(WORD addr);
extern void c128_c64io_d300_store(WORD addr, BYTE value);
extern BYTE c128_c64io_d400_read(WORD addr);
extern void c128_c64io_d400_store(WORD addr, BYTE value);
extern BYTE c128_d5xx_read(WORD addr);
extern void c128_d5xx_store(WORD addr, BYTE value);
extern BYTE c128_mmu_read(WORD addr);
extern void c128_mmu_store(WORD addr, BYTE value);
extern BYTE c128_vdc_read(WORD addr);
extern void c128_vdc_store(WORD addr, BYTE value);
extern BYTE c128_c64io_d700_read(WORD addr);
extern void c128_c64io_d700_store(WORD addr, BYTE value);
extern BYTE c128_colorram_read(WORD addr);
extern void c128_colorram_store(WORD addr, BYTE value);
extern BYTE c128_cia1_read(WORD addr);
extern void c128_cia1_store(WORD addr, BYTE value);
extern BYTE c128_cia2_read(WORD addr);
extern void c128_cia2_store(WORD addr, BYTE value);
extern BYTE c128_c64io_de00_read(WORD addr);
extern void c128_c64io_de00_store(WORD addr, BYTE value);
extern BYTE c128_c64io_df00_read(WORD addr);
extern void c128_c64io_df00_store(WORD addr, BYTE value);

#endif
