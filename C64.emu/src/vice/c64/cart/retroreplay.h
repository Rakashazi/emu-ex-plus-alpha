/*
 * retroreplay.h - Cartridge handling, Retro Replay cart.
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifdef CARTRIDGE_INCLUDE_PRIVATE_API

#ifndef VICE_RETROREPLAY_H
#define VICE_RETROREPLAY_H

#include <stdio.h>

#include "types.h"

struct snapshot_s;

uint8_t retroreplay_roml_read(uint16_t addr);
void retroreplay_roml_store(uint16_t addr, uint8_t value);
uint8_t retroreplay_a000_bfff_read(uint16_t addr);
void retroreplay_a000_bfff_store(uint16_t addr, uint8_t value);
int retroreplay_roml_no_ultimax_store(uint16_t addr, uint8_t value);
uint8_t retroreplay_romh_read(uint16_t addr);
void retroreplay_romh_store(uint16_t addr, uint8_t value);
int retroreplay_peek_mem(export_t *export, uint16_t addr, uint8_t *value);
void retroreplay_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

void retroreplay_freeze(void);
int retroreplay_freeze_allowed(void);

void retroreplay_config_init(void);
void retroreplay_reset(void);
void retroreplay_config_setup(uint8_t *rawcart);
int retroreplay_bin_attach(const char *filename, uint8_t *rawcart);
int retroreplay_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename, uint8_t revision);
int retroreplay_bin_save(const char *filename);
int retroreplay_crt_save(const char *filename);
int retroreplay_flush_image(void);
void retroreplay_detach(void);
void retroreplay_powerup(void);

int retroreplay_cart_enabled(void);

int retroreplay_cmdline_options_init(void);
int retroreplay_resources_init(void);
void retroreplay_resources_shutdown(void);

int retroreplay_snapshot_write_module(struct snapshot_s *s);
int retroreplay_snapshot_read_module(struct snapshot_s *s);

#endif  /* VICE_RETROREPLAY_H */
#endif  /* CARTRIDGE_INCLUDE_PRIVATE_API */

#ifndef VICE_RETROREPLAY_PUBLIC_H
#define VICE_RETROREPLAY_PUBLIC_H

#define RR_REV_RETRO_REPLAY  0
#define RR_REV_NORDIC_REPLAY 1

#endif  /* VICE_RETROREPLAY_PUBLIC_H */
