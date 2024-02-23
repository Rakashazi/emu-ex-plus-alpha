/*
 * ide64.h - Cartridge handling, IDE64 cart.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#ifndef VICE_IDE64_H
#define VICE_IDE64_H

#include "types.h"

extern char *ide64_image_file;

int ide64_resources_init(void);
int ide64_resources_shutdown(void);
int ide64_cmdline_options_init(void);

void ide64_reset(void);

void ide64_config_init(void);
void ide64_config_setup(uint8_t *rawcart);
int ide64_bin_attach(const char *filename, uint8_t *rawcart);
int ide64_crt_attach(FILE *fd, uint8_t *rawcart);
void ide64_detach(void);

uint8_t ide64_rom_read(uint16_t addr);
uint8_t ide64_ram_read(uint16_t addr);
void ide64_rom_store(uint16_t addr, uint8_t value);
void ide64_ram_store(uint16_t addr, uint8_t value);
void ide64_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

struct snapshot_s;

int ide64_snapshot_read_module(struct snapshot_s *s);
int ide64_snapshot_write_module(struct snapshot_s *s);

/* values to be used with IDE64Version resource */
#define IDE64_VERSION_3   0
#define IDE64_VERSION_4_1 1
#define IDE64_VERSION_4_2 2

/* device numbers for IDE64 resources */
#define IDE64_DEVICE_MIN    1
#define IDE64_DEVICE_MAX    4
#define IDE64_DEVICE_COUNT  (IDE64_DEVICE_MAX - IDE64_DEVICE_MIN + 1)

/* limits for the geometry resources (inclusive) */
#define IDE64_CYLINDERS_MIN     1
#define IDE64_CYLINDERS_MAX 65535
#define IDE64_HEADS_MIN         1
#define IDE64_HEADS_MAX        16
#define IDE64_SECTORS_MIN       1
#define IDE64_SECTORS_MAX      63

#endif
