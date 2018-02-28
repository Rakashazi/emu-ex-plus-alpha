/*
 * mem.h - Memory interface.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_MEM_H_
#define VICE_MEM_H_

#include "types.h"


typedef BYTE read_func_t(WORD addr);
typedef read_func_t *read_func_ptr_t;
typedef void store_func_t(WORD addr, BYTE value);
typedef store_func_t *store_func_ptr_t;

extern read_func_ptr_t *_mem_read_tab_ptr;
extern store_func_ptr_t *_mem_write_tab_ptr;

extern BYTE mem_ram[];
extern BYTE *mem_page_zero;
extern BYTE *mem_page_one;
extern BYTE *mem_color_ram_cpu;
extern BYTE *mem_color_ram_vicii;

extern BYTE *mem_chargen_rom_ptr;

extern void mem_initialize_memory(void);
extern void mem_powerup(void);
extern int mem_load(void);
extern void mem_get_basic_text(WORD *start, WORD *end);
extern void mem_set_basic_text(WORD start, WORD end);
extern void mem_toggle_watchpoints(int flag, void *context);
extern int mem_rom_trap_allowed(WORD addr);
extern void mem_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);
extern void mem_color_ram_to_snapshot(BYTE *color_ram);
extern void mem_color_ram_from_snapshot(BYTE *color_ram);

/*
 * DWORD addr allows injection on machines with more than 64Kb of RAM.
 * Injection should be made to follow (mostly) how load would write to
 * RAM on that machine.
 */
extern void mem_inject(DWORD addr, BYTE value);

extern read_func_t rom_read, rom_trap_read, zero_read;
extern store_func_t rom_store, rom_trap_store, zero_store;

extern read_func_t mem_read;
extern store_func_t mem_store;

/* ------------------------------------------------------------------------- */

/* Memory access functions for the monitor.  */
extern const char **mem_bank_list(void);
extern int mem_bank_from_name(const char *name);
extern BYTE mem_bank_read(int bank, WORD addr, void *context);
extern BYTE mem_bank_peek(int bank, WORD addr, void *context);
extern void mem_bank_write(int bank, WORD addr, BYTE byte, void *context);
extern void mem_get_screen_parameter(WORD *base, BYTE *rows, BYTE *columns, int *bank);

typedef struct mem_ioreg_list_s {
    const char *name;
    WORD start;
    WORD end;
    unsigned int next;
    int (*dump)(void *context, WORD address);
    void *context;
} mem_ioreg_list_t;

extern mem_ioreg_list_t *mem_ioreg_list_get(void *context);

/* Snapshots.  */
struct snapshot_s;
extern int mem_write_snapshot_module(struct snapshot_s *s, int save_roms);
extern int mem_read_snapshot_module(struct snapshot_s *s);

#endif
