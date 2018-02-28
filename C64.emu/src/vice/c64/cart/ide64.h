/*
 * ide64.h - Cartridge handling, IDE64 cart.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_IDE64_H
#define VICE_IDE64_H

#include "types.h"

extern int ide64_resources_init(void);
extern int ide64_resources_shutdown(void);
extern int ide64_cmdline_options_init(void);

extern void ide64_reset(void);

extern void ide64_config_init(void);
extern void ide64_config_setup(BYTE *rawcart);
extern int ide64_bin_attach(const char *filename, BYTE *rawcart);
extern int ide64_crt_attach(FILE *fd, BYTE *rawcart);
extern char *ide64_image_file;
extern void ide64_detach(void);

extern BYTE ide64_rom_read(WORD addr);
extern BYTE ide64_ram_read(WORD addr);
extern void ide64_rom_store(WORD addr, BYTE value);
extern void ide64_ram_store(WORD addr, BYTE value);
extern void ide64_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

struct snapshot_s;
extern int ide64_snapshot_read_module(struct snapshot_s *s);
extern int ide64_snapshot_write_module(struct snapshot_s *s);

/* values to be used with IDE64Version resource */
#define IDE64_VERSION_3 0
#define IDE64_VERSION_4_1 1
#define IDE64_VERSION_4_2 2

#endif
