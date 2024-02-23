/*
 * rexramfloppy.h - Cartridge handling, REX Ramfloppy cart.
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

#ifndef VICE_REXRAMFLOPPY_H
#define VICE_REXRAMFLOPPY_H

#include <stdio.h>

#include "types.h"

uint8_t rexramfloppy_roml_read(uint16_t addr);
void rexramfloppy_roml_store(uint16_t addr, uint8_t value);

void rexramfloppy_reset(void);

void rexramfloppy_config_init(void);
void rexramfloppy_config_setup(uint8_t *rawcart);
int rexramfloppy_bin_attach(const char *filename, uint8_t *rawcart);
int rexramfloppy_crt_attach(FILE *fd, uint8_t *rawcart);
void rexramfloppy_detach(void);

int rexramfloppy_resources_init(void);
void rexramfloppy_resources_shutdown(void);
int rexramfloppy_cmdline_options_init(void);

int rexramfloppy_ram_flush(void);
int rexramfloppy_ram_save(const char *filename);

int rexramfloppy_can_flush_ram(void);

struct snapshot_s;

int rexramfloppy_snapshot_write_module(struct snapshot_s *s);
int rexramfloppy_snapshot_read_module(struct snapshot_s *s);

#endif
