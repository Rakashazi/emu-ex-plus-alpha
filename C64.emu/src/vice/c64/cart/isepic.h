/*
 * isepic.h - ISEPIC cart emulation.
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

#ifndef VICE_ISEPIC_H
#define VICE_ISEPIC_H

#include "types.h"

int isepic_cart_enabled(void);
int isepic_cart_active(void);
int isepic_freeze_allowed(void);
void isepic_freeze(void);
void isepic_reset(void);
void isepic_config_init(void);
void isepic_config_setup(uint8_t *rawcart);
void isepic_powerup(void);

int isepic_resources_init(void);
void isepic_resources_shutdown(void);
int isepic_cmdline_options_init(void);

uint8_t isepic_romh_read(uint16_t addr);
void isepic_romh_store(uint16_t addr, uint8_t byte);
uint8_t isepic_page_read(uint16_t addr);
void isepic_page_store(uint16_t addr, uint8_t byte);
int isepic_romh_phi1_read(uint16_t addr, uint8_t *value);
int isepic_romh_phi2_read(uint16_t addr, uint8_t *value);
int isepic_peek_mem(uint16_t addr, uint8_t *value);
void isepic_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

int isepic_bin_attach(const char *filename, uint8_t *rawcart);
int isepic_bin_save(const char *filename);
int isepic_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int isepic_crt_save(const char *filename);
int isepic_flush_image(void);
void isepic_detach(void);
int isepic_enable(void);
int isepic_disable(void);

const char *isepic_get_file_name(void);

struct snapshot_s;

int isepic_snapshot_read_module(struct snapshot_s *s);
int isepic_snapshot_write_module(struct snapshot_s *s);

#endif
