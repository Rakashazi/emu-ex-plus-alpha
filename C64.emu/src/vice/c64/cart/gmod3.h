/*
 * gmod3.h - Cartridge handling, GMod3 cart.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_GMOD3_H
#define VICE_GMOD3_H

#include <stdio.h>

#include "types.h"

struct snapshot_s;

uint8_t gmod3_roml_read(uint16_t addr);
uint8_t gmod3_romh_read(uint16_t addr);
int gmod3_romh_phi1_read(uint16_t addr, uint8_t *value);
int gmod3_romh_phi2_read(uint16_t addr, uint8_t *value);

int gmod3_peek_mem(export_t *export, uint16_t addr, uint8_t *value);
void gmod3_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

void gmod3_config_init(void);
void gmod3_reset(void);
void gmod3_config_setup(uint8_t *rawcart);
int gmod3_bin_attach(const char *filename, uint8_t *rawcart);
int gmod3_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int gmod3_bin_save(const char *filename);
int gmod3_crt_save(const char *filename);
int gmod3_flush_image(void);
void gmod3_detach(void);

int gmod3_cmdline_options_init(void);
int gmod3_resources_init(void);
void gmod3_resources_shutdown(void);

int gmod3_snapshot_write_module(struct snapshot_s *s);
int gmod3_snapshot_read_module(struct snapshot_s *s);

#endif
