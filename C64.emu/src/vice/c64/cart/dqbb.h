/*
 * dqbb.h - DOUBLE QUICK BROWN BOX emulation.
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

#ifndef VICE_DQBB_H
#define VICE_DQBB_H

#include "types.h"

int dqbb_cart_enabled(void);

int dqbb_resources_init(void);
void dqbb_resources_shutdown(void);
int dqbb_cmdline_options_init(void);
void dqbb_reset(void);
void dqbb_detach(void);
void dqbb_init_config(void);
int dqbb_enable(void);
int dqbb_disable(void);
void dqbb_config_setup(uint8_t *rawcart);

uint8_t dqbb_roml_read(uint16_t addr);
void dqbb_roml_store(uint16_t addr, uint8_t byte);
uint8_t dqbb_romh_read(uint16_t addr);
void dqbb_romh_store(uint16_t addr, uint8_t byte);
int dqbb_peek_mem(uint16_t addr, uint8_t *value);
void dqbb_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

const char *dqbb_get_file_name(void);
int dqbb_bin_attach(const char *filename, uint8_t *rawcart);
int dqbb_bin_save(const char *filename);
int dqbb_flush_image(void);
void dqbb_powerup(void);

struct snapshot_s;

int dqbb_snapshot_read_module(struct snapshot_s *s);
int dqbb_snapshot_write_module(struct snapshot_s *s);

#endif
