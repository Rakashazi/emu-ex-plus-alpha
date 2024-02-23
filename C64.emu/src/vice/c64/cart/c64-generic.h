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

void generic_8kb_config_init(void);
void generic_16kb_config_init(void);
void generic_ultimax_config_init(void);
void generic_8kb_config_setup(uint8_t *rawcart);
void generic_16kb_config_setup(uint8_t *rawcart);
void generic_ultimax_config_setup(uint8_t *rawcart);
int generic_8kb_bin_attach(const char *filename, uint8_t *rawcart);
int generic_16kb_bin_attach(const char *filename, uint8_t *rawcart);
int generic_ultimax_bin_attach(const char *filename, uint8_t *rawcart);
int generic_crt_attach(FILE *fd, uint8_t *rawcart);
void generic_8kb_detach(void);
void generic_16kb_detach(void);
void generic_ultimax_detach(void);

uint8_t generic_roml_read(uint16_t addr);
void generic_roml_store(uint16_t addr, uint8_t value);
uint8_t generic_romh_read(uint16_t addr);
int generic_romh_phi1_read(uint16_t addr, uint8_t *value);
int generic_romh_phi2_read(uint16_t addr, uint8_t *value);
int generic_peek_mem(export_t *export, uint16_t addr, uint8_t *value);
void generic_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

int generic_snapshot_write_module(struct snapshot_s *s, int type);
int generic_snapshot_read_module(struct snapshot_s *s, int type);

#endif
