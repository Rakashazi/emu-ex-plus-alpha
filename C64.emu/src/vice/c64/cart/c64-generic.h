/*
 * c64-generic.h - Cartridge handling, generic carts.
 *
 * Written by
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

#ifndef VICE_C64_GENERIC_H
#define VICE_C64_GENERIC_H

#include <stdio.h>

#include "types.h"

struct snapshot_s;

extern void generic_8kb_config_init(void);
extern void generic_16kb_config_init(void);
extern void generic_ultimax_config_init(void);
extern void generic_8kb_config_setup(BYTE *rawcart);
extern void generic_16kb_config_setup(BYTE *rawcart);
extern void generic_ultimax_config_setup(BYTE *rawcart);
extern int generic_8kb_bin_attach(const char *filename, BYTE *rawcart);
extern int generic_16kb_bin_attach(const char *filename, BYTE *rawcart);
extern int generic_ultimax_bin_attach(const char *filename, BYTE *rawcart);
extern int generic_crt_attach(FILE *fd, BYTE *rawcart);
extern void generic_8kb_detach(void);
extern void generic_16kb_detach(void);
extern void generic_ultimax_detach(void);

extern BYTE generic_roml_read(WORD addr);
extern void generic_roml_store(WORD addr, BYTE value);
extern BYTE generic_romh_read(WORD addr);
extern int generic_romh_phi1_read(WORD addr, BYTE *value);
extern int generic_romh_phi2_read(WORD addr, BYTE *value);
extern int generic_peek_mem(export_t *export, WORD addr, BYTE *value);
extern void generic_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern int generic_snapshot_write_module(struct snapshot_s *s, int type);
extern int generic_snapshot_read_module(struct snapshot_s *s, int type);

#endif
