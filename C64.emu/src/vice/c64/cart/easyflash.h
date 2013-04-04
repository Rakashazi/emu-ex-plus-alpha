/*
 * easyflash.h - Cartridge handling of the easyflash cart.
 *
 * Written by
 *  ALeX Kazik <alx@kazik.de>
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

#ifndef VICE_EASYFLASH_H
#define VICE_EASYFLASH_H

#include "types.h"

extern int easyflash_resources_init(void);
extern void easyflash_resources_shutdown(void);
extern int easyflash_cmdline_options_init(void);

extern BYTE easyflash_roml_read(WORD addr);
extern void easyflash_roml_store(WORD addr, BYTE value);
extern BYTE easyflash_romh_read(WORD addr);
extern void easyflash_romh_store(WORD addr, BYTE value);
extern void easyflash_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern void easyflash_config_init(void);
extern void easyflash_config_setup(BYTE *rawcart);
extern int easyflash_bin_attach(const char *filename, BYTE *rawcart);
extern int easyflash_crt_attach(FILE *fd, BYTE *rawcart, const char *filename);
extern void easyflash_detach(void);
extern int easyflash_bin_save(const char *filename);
extern int easyflash_crt_save(const char *filename);
extern int easyflash_flush_image(void);

struct snapshot_s;

extern int easyflash_snapshot_write_module(struct snapshot_s *s);
extern int easyflash_snapshot_read_module(struct snapshot_s *s);

#endif
