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

void ramcart_init(void);
int ramcart_resources_init(void);
void ramcart_resources_shutdown(void);
int ramcart_cmdline_options_init(void);

void ramcart_init_config(void);
void ramcart_config_setup(uint8_t *rawcart);
void ramcart_reset(void);
void ramcart_detach(void);
int ramcart_enable(void);
int ramcart_disable(void);

uint8_t ramcart_roml_read(uint16_t addr);
void ramcart_roml_store(uint16_t addr, uint8_t byte);
int ramcart_peek_mem(uint16_t addr, uint8_t *value);
void ramcart_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

int ramcart_cart_enabled(void);
const char *ramcart_get_filename_by_type(void);
int ramcart_bin_attach(const char *filename, uint8_t *rawcart);
int ramcart_bin_save(const char *filename);
int ramcart_flush_image(void);

struct snapshot_s;

int ramcart_snapshot_read_module(struct snapshot_s *s);
int ramcart_snapshot_write_module(struct snapshot_s *s);

#endif
