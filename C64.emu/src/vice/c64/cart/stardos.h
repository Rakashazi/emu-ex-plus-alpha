/*
 * stardos.h - Cartridge handling, StarDOS cart.
 *
 * (w)2008 Groepaz/Hitmen
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

#ifndef VICE_STARDOS_H
#define VICE_STARDOS_H

#include <stdio.h>

#include "types.h"
#include "c64cart.h"

struct snapshot_s;

uint8_t stardos_roml_read(uint16_t addr);
uint8_t stardos_romh_read(uint16_t addr);
int stardos_romh_phi1_read(uint16_t addr, uint8_t *value);
int stardos_romh_phi2_read(uint16_t addr, uint8_t *value);
int stardos_peek_mem(export_t *export, uint16_t addr, uint8_t *value);

void stardos_config_init(void);
void stardos_reset(void);
void stardos_config_setup(uint8_t *rawcart);
int stardos_bin_attach(const char *filename, uint8_t *rawcart);
int stardos_crt_attach(FILE *fd, uint8_t *rawcart);
void stardos_detach(void);

int stardos_snapshot_write_module(struct snapshot_s *s);
int stardos_snapshot_read_module(struct snapshot_s *s);

#endif
