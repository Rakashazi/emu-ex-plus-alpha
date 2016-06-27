/*
 * c64mem.h -- C64 memory handling.
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

#define C64_BASIC_CHECKSUM         15702

#define C64_KERNAL_CHECKSUM_R01    54525
#define C64_KERNAL_CHECKSUM_R02    50955
#define C64_KERNAL_CHECKSUM_R03    50954
#define C64_KERNAL_CHECKSUM_R03swe 50633
#define C64_KERNAL_CHECKSUM_R43    50955
#define C64_KERNAL_CHECKSUM_R64    49680

/* the value located at 0xff80 */
#define C64_KERNAL_ID_R01    0xaa
#define C64_KERNAL_ID_R02    0x00
#define C64_KERNAL_ID_R03    0x03
#define C64_KERNAL_ID_R03swe 0x03
#define C64_KERNAL_ID_R43    0x43
#define C64_KERNAL_ID_R64    0x64

extern int c64_mem_init_resources(void);
extern int c64_mem_init_cmdline_options(void);

extern void mem_set_vbank(int new_vbank);

extern BYTE ram_read(WORD addr);
extern void ram_store(WORD addr, BYTE value);
extern void ram_hi_store(WORD addr, BYTE value);

extern BYTE chargen_read(WORD addr);
extern void chargen_store(WORD addr, BYTE value);

extern void colorram_store(WORD addr, BYTE value);
extern BYTE colorram_read(WORD addr);

extern void mem_pla_config_changed(void);
extern void mem_set_tape_sense(int sense);

extern BYTE mem_chargen_rom[C64_CHARGEN_ROM_SIZE];

extern void mem_set_write_hook(int config, int page, store_func_t *f);
extern void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func);
extern void mem_read_base_set(unsigned int base, unsigned int index, BYTE *mem_ptr);

extern void mem_store_without_ultimax(WORD addr, BYTE value);
extern BYTE mem_read_without_ultimax(WORD addr);
extern void mem_store_without_romlh(WORD addr, BYTE value);

extern void store_bank_io(WORD addr, BYTE byte);
extern BYTE read_bank_io(WORD addr);

extern void c64_mem_init(void);

extern int c64_mem_ui_init_early(void);
extern int c64_mem_ui_init(void);
extern void c64_mem_ui_shutdown(void);

extern BYTE vsid_io_read(WORD addr);
extern void vsid_io_store(WORD addr, BYTE val);

#endif
