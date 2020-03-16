/*
 * scpu64mem.h -- C64 memory handling.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_SCPU64MEM_H
#define VICE_SCPU64MEM_H

#include "mem.h"
#include "types.h"

#define SCPU64_RAM_SIZE 0x10000
#define SCPU64_SRAM_SIZE 0x20000
#define SCPU64_KERNAL_ROM_SIZE  0x2000
#define SCPU64_CHARGEN_ROM_SIZE 0x1000
#define SCPU64_SCPU64_ROM_MINSIZE  0x10000
#define SCPU64_SCPU64_ROM_MAXSIZE  0x80000

extern uint8_t mem_sram[];
extern uint8_t mem_trap_ram[];

extern int c64_mem_init_resources(void);
extern int c64_mem_init_cmdline_options(void);

extern void mem_set_vbank(int new_vbank);

extern uint8_t ram_read(uint16_t addr);
extern void ram_store(uint16_t addr, uint8_t value);
extern uint8_t ram_read_int(uint16_t addr);
extern void ram_store_int(uint16_t addr, uint8_t value);
extern uint8_t scpu64_trap_read(uint16_t addr);
extern void scpu64_trap_store(uint16_t addr, uint8_t value);

extern uint8_t chargen_read(uint16_t addr);
extern uint8_t scpu64_kernalshadow_read(uint16_t addr);
extern uint8_t ram1_read(uint16_t addr);
extern uint8_t scpu64rom_scpu64_read(uint16_t addr);

extern void scpu64io_colorram_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_colorram_read(uint16_t addr);
extern void scpu64io_colorram_store_int(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_colorram_read_int(uint16_t addr);
extern uint8_t scpu64io_d000_read(uint16_t addr);
extern void scpu64io_d000_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d100_read(uint16_t addr);
extern void scpu64io_d100_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d200_read(uint16_t addr);
extern void scpu64io_d200_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d300_read(uint16_t addr);
extern void scpu64io_d300_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d400_read(uint16_t addr);
extern void scpu64io_d400_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d500_read(uint16_t addr);
extern void scpu64io_d500_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d600_read(uint16_t addr);
extern void scpu64io_d600_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_d700_read(uint16_t addr);
extern void scpu64io_d700_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_cia1_read(uint16_t addr);
extern void scpu64_cia1_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_cia2_read(uint16_t addr);
extern void scpu64_cia2_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_de00_read(uint16_t addr);
extern void scpu64io_de00_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64io_df00_read(uint16_t addr);
extern void scpu64io_df00_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_roml_read(uint16_t addr);
extern void scpu64_roml_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_romh_read(uint16_t addr);
extern void scpu64_romh_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_ultimax_1000_7fff_read(uint16_t addr);
extern void scpu64_ultimax_1000_7fff_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_ultimax_a000_bfff_read(uint16_t addr);
extern void scpu64_ultimax_a000_bfff_store(uint16_t addr, uint8_t value);
extern uint8_t scpu64_ultimax_c000_cfff_read(uint16_t addr);
extern void scpu64_ultimax_c000_cfff_store(uint16_t addr, uint8_t value);
extern int scpu64_interrupt_reroute(void);
extern void mem_set_simm_size(int val);
extern void mem_set_jiffy_switch(int val);
extern void mem_set_speed_switch(int val);
extern void mem_set_mirroring(int new_mirroring);
extern void mem_set_simm(int config);


extern void mem_pla_config_changed(void);
extern void mem_set_tape_sense(int sense);

extern uint8_t mem_chargen_rom[];
extern uint8_t *mem_simm_ram;

extern void mem_set_write_hook(int config, int page, store_func_t *f);
extern void mem_read_tab_set(unsigned int base, unsigned int index, read_func_ptr_t read_func);
extern void mem_read_base_set(unsigned int base, unsigned int index, uint8_t *mem_ptr);

extern void mem_store_without_ultimax(uint16_t addr, uint8_t value);
extern uint8_t mem_read_without_ultimax(uint16_t addr);
extern void mem_store_without_romlh(uint16_t addr, uint8_t value);

extern void store_bank_io(uint16_t addr, uint8_t byte);
extern uint8_t read_bank_io(uint16_t addr);

extern void mem_store2(uint32_t addr, uint8_t value);
extern uint8_t mem_read2(uint32_t addr);

extern void scpu64_mem_init(void);

extern void scpu64_hardware_reset(void);
extern void scpu64_mem_shutdown(void);

#endif
