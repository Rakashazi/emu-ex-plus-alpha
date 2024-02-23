/*
 * gmod2c128.h - Cartridge handling, GMod2-C128 cart.
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

#ifndef VICE_GMOD2C128_H
#define VICE_GMOD2C128_H

#include <stdio.h>

#include "types.h"

struct snapshot_s;

uint8_t c128gmod2_roml_read(uint16_t addr);
void c128gmod2_roml_store(uint16_t addr, uint8_t value);

void c128gmod2_config_init(void);
void c128gmod2_reset(void);
void c128gmod2_config_setup(uint8_t *rawcart);
int c128gmod2_bin_attach(const char *filename, uint8_t *rawcart);
int c128gmod2_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int c128gmod2_bin_save(const char *filename);
int c128gmod2_crt_save(const char *filename);
int c128gmod2_flush_image(void);
void c128gmod2_detach(void);

int c128gmod2_eeprom_save(const char *filename);
int c128gmod2_flush_eeprom(void);
int c128gmod2_can_save_eeprom(void);
int c128gmod2_can_flush_eeprom(void);

int c128gmod2_cmdline_options_init(void);
int c128gmod2_resources_init(void);
void c128gmod2_resources_shutdown(void);

/* FIXME: snapshot stuff is not implemented yet */
int c128gmod2_snapshot_write_module(struct snapshot_s *s);
int c128gmod2_snapshot_read_module(struct snapshot_s *s);

#endif
