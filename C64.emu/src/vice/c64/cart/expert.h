/*
 * expert.h - Cartridge handling, Expert cart.
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

#ifndef VICE_EXPERT_H
#define VICE_EXPERT_H

#include <stdio.h>

#include "types.h"

uint8_t expert_roml_read(uint16_t addr);
void expert_roml_store(uint16_t addr, uint8_t value);
void expert_raml_store(uint16_t addr, uint8_t value);
uint8_t expert_romh_read(uint16_t addr);
int expert_romh_phi1_read(uint16_t addr, uint8_t *value);
int expert_romh_phi2_read(uint16_t addr, uint8_t *value);
int expert_peek_mem(uint16_t addr, uint8_t *value);

void expert_reset(void);
void expert_freeze(void);

void expert_config_init(void);
void expert_config_setup(uint8_t *rawcart);
int expert_bin_attach(const char *filename, uint8_t *rawcart);
int expert_bin_save(const char *filename);
int expert_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int expert_crt_save(const char *filename);
int expert_flush_image(void);

void expert_detach(void);
int expert_enable(void);
int expert_disable(void);
void expert_powerup(void);

int expert_freeze_allowed(void);
int expert_cart_enabled(void);
void expert_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

struct snapshot_s;

int expert_snapshot_write_module(struct snapshot_s *s);
int expert_snapshot_read_module(struct snapshot_s *s);

int expert_resources_init(void);
void expert_resources_shutdown(void);
int expert_cmdline_options_init(void);

const char *expert_get_file_name(void);

#endif /* VICE_EXPERT_H */
#endif /* CARTRIDGE_INCLUDE_PRIVATE_API */

#ifndef VICE_EXPERT_PUBLIC_H
#define VICE_EXPERT_PUBLIC_H

/* Expert cartridge has three modes: */
#define EXPERT_MODE_OFF 0
#define EXPERT_MODE_PRG 1
#define EXPERT_MODE_ON 2
#define EXPERT_MODE_DEFAULT EXPERT_MODE_PRG

#endif /* VICE_EXPERT_PUBLIC_H */

