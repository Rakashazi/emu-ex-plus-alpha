/*
 * ramcart.h - RAMCART emulation.
 *
 * Written by
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_RAMCART_H
#define VICE_RAMCART_H

#include "types.h"

extern void ramcart_init(void);
extern int ramcart_resources_init(void);
extern void ramcart_resources_shutdown(void);
extern int ramcart_cmdline_options_init(void);

extern void ramcart_init_config(void);
extern void ramcart_config_setup(BYTE *rawcart);
extern void ramcart_reset(void);
extern void ramcart_detach(void);
extern int ramcart_enable(void);

extern BYTE ramcart_roml_read(WORD addr);
extern void ramcart_roml_store(WORD addr, BYTE byte);
extern int ramcart_peek_mem(WORD addr, BYTE *value);
extern void ramcart_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern int ramcart_cart_enabled(void);
extern const char *ramcart_get_file_name(void);
extern int ramcart_bin_attach(const char *filename, BYTE *rawcart);
extern int ramcart_bin_save(const char *filename);
extern int ramcart_flush_image(void);

struct snapshot_s;
extern int ramcart_snapshot_read_module(struct snapshot_s *s);
extern int ramcart_snapshot_write_module(struct snapshot_s *s);

#endif
