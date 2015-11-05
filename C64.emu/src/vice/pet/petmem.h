/*
 * petmem.h - PET memory handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_PETMEM_H
#define VICE_PETMEM_H

#include <stdio.h>

#include "mem.h"
#include "types.h"

#define PET_RAM_SIZE            0x8000
#define PET_ROM_SIZE            0x8000
#define PET_CHARGEN_ROM_SIZE    0x4000

#define PET_KERNAL1_CHECKSUM    3236
#define PET_KERNAL2_CHECKSUM    31896
#define PET_KERNAL4_CHECKSUM    53017

#define PET_EDIT1G_CHECKSUM     51858
#define PET_EDIT2G_CHECKSUM     64959
#define PET_EDIT2B_CHECKSUM     1514
#define PET_EDIT4G40_CHECKSUM   14162
#define PET_EDIT4B40_CHECKSUM   27250
#define PET_EDIT4B80_CHECKSUM   21166

extern BYTE mem_chargen_rom[PET_CHARGEN_ROM_SIZE];
extern BYTE mem_rom[PET_ROM_SIZE];
extern BYTE mem_6809rom[];

struct petres_s;
struct petinfo_s;

extern int pet_mem_init_resources(void);
extern int pet_mem_init_cmdline_options(void);

extern void mem_initialize_memory(void);
extern void get_mem_access_tables(read_func_ptr_t **read, store_func_ptr_t **write, BYTE ***base, int **limit);
extern int petmem_get_screen_columns(void);
extern void petmem_check_info(struct petres_s *pi);

extern void petmem_reset(void);
extern int petmem_superpet_diag(void);
extern void petmem_set_vidmem(void);

extern int petmem_dump(FILE *fp);
extern int petmem_undump(FILE *fp);

extern int petmem_set_conf_info(struct petinfo_s *pi);

extern int spet_ramen;
extern int spet_bank;
extern void set_spet_bank(int bank);
extern int spet_ctrlwp;
extern int spet_diag;
extern int spet_ramwp;
extern int spet_flat_mode;
extern int spet_firq_disabled;

struct dongle6702_s {
    int val;
    int prevodd;
    int wantodd;
    int shift[8];
};

extern struct dongle6702_s dongle6702;

extern BYTE petmem_map_reg;
extern BYTE petmem_ramON;
extern BYTE petmem_2001_buf_ef[];

extern read_func_t mem6809_read;
extern store_func_t mem6809_store;
extern void mem6809_store16(WORD addr, WORD value);
extern WORD mem6809_read16(WORD addr);
#ifdef H6309
extern void mem6809_store32(WORD addr, DWORD value);
extern DWORD mem6809_read32(WORD addr);
#endif
extern void mem_initialize_memory_6809(void);
extern void ramsel_changed(void);

extern int superpet_sync(void);

#endif
