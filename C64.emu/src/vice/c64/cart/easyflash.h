/*
 * easyflash.h - Cartridge handling of the easyflash cart.
 *
 * Written by
 *  ALeX Kazik <alx@kazik.de>
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

#ifndef VICE_EASYFLASH_H
#define VICE_EASYFLASH_H

#include "types.h"

int easyflash_resources_init(void);
void easyflash_resources_shutdown(void);
int easyflash_cmdline_options_init(void);

uint8_t easyflash_roml_read(uint16_t addr);
void easyflash_roml_store(uint16_t addr, uint8_t value);
uint8_t easyflash_romh_read(uint16_t addr);
void easyflash_romh_store(uint16_t addr, uint8_t value);
void easyflash_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

void easyflash_config_init(void);
void easyflash_config_setup(uint8_t *rawcart);
int easyflash_bin_attach(const char *filename, uint8_t *rawcart);
int easyflash_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
void easyflash_detach(void);
int easyflash_bin_save(const char *filename);
int easyflash_crt_save(const char *filename);
int easyflash_flush_image(void);
void easyflash_powerup(void);

struct snapshot_s;

int easyflash_snapshot_write_module(struct snapshot_s *s);
int easyflash_snapshot_read_module(struct snapshot_s *s);

#endif
