/*
 * megacart.h -- VIC20 Mega-Cart emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_MEGACART_H
#define VICE_MEGACART_H

#include <stdio.h>

#include "types.h"

uint8_t megacart_ram123_read(uint16_t addr);
void megacart_ram123_store(uint16_t addr, uint8_t value);
uint8_t megacart_blk123_read(uint16_t addr);
void megacart_blk123_store(uint16_t addr, uint8_t value);
uint8_t megacart_blk5_read(uint16_t addr);
void megacart_blk5_store(uint16_t addr, uint8_t value);

void megacart_init(void);
void megacart_reset(void);
void megacart_powerup(void);

void megacart_config_setup(uint8_t *rawcart);
int megacart_bin_attach(const char *filename);

/* int megacart_bin_attach(const char *filename, uint8_t *rawcart); */

int megacart_crt_attach(FILE *fd, uint8_t *rawcart);
void megacart_detach(void);

int megacart_resources_init(void);
void megacart_resources_shutdown(void);
int megacart_cmdline_options_init(void);

struct snapshot_s;

int megacart_snapshot_write_module(struct snapshot_s *s);
int megacart_snapshot_read_module(struct snapshot_s *s);

#endif
