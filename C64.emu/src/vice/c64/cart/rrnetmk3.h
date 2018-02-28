/*
 * rrnetmk3.h - Cartridge handling, RR-Net MK3 cart.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_RRNETMK3_H
#define VICE_RRNETMK3_H

#include "types.h"
#include "c64cart.h"

extern int rrnetmk3_cart_enabled(void);
extern void rrnetmk3_config_init(void);
extern int rrnetmk3_roml_read(WORD addr);
extern int rrnetmk3_roml_store(WORD addr, BYTE byte);
extern int rrnetmk3_peek_mem(export_t *export, WORD addr, BYTE *value);

extern void rrnetmk3_config_setup(BYTE *rawcart);

extern int rrnetmk3_crt_attach(FILE *fd, BYTE *rawcart, const char *filename);
extern int rrnetmk3_bin_attach(const char *filename, BYTE *rawcart);
extern int rrnetmk3_bin_save(const char *filename);
extern int rrnetmk3_crt_save(const char *filename);
extern int rrnetmk3_flush_image(void);

extern int rrnetmk3_resources_init(void);
extern void rrnetmk3_resources_shutdown(void);
extern int rrnetmk3_cmdline_options_init(void);
extern void rrnetmk3_init(void);
extern void rrnetmk3_detach(void);
extern void rrnetmk3_reset(void);
extern int rrnetmk3_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

struct snapshot_s;
extern int rrnetmk3_snapshot_read_module(struct snapshot_s *s);
extern int rrnetmk3_snapshot_write_module(struct snapshot_s *s);

#endif
