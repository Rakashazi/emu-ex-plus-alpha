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

int rrnetmk3_cart_enabled(void);
void rrnetmk3_config_init(void);
int rrnetmk3_roml_read(uint16_t addr);
int rrnetmk3_roml_store(uint16_t addr, uint8_t byte);
int rrnetmk3_peek_mem(export_t *export, uint16_t addr, uint8_t *value);

void rrnetmk3_config_setup(uint8_t *rawcart);

int rrnetmk3_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int rrnetmk3_bin_attach(const char *filename, uint8_t *rawcart);
int rrnetmk3_bin_save(const char *filename);
int rrnetmk3_crt_save(const char *filename);
int rrnetmk3_flush_image(void);

int rrnetmk3_resources_init(void);
void rrnetmk3_resources_shutdown(void);
int rrnetmk3_cmdline_options_init(void);
void rrnetmk3_init(void);
void rrnetmk3_detach(void);
void rrnetmk3_reset(void);
int rrnetmk3_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

struct snapshot_s;

int rrnetmk3_snapshot_read_module(struct snapshot_s *s);
int rrnetmk3_snapshot_write_module(struct snapshot_s *s);

#endif
