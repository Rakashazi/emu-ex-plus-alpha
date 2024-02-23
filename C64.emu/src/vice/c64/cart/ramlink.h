/*
 * ramlink.h - Cartridge handling, CMD Ramlink
 *
 * Written by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#ifndef VICE_RAMLINK_H
#define VICE_RAMLINK_H

#define RL_MODE_DIRECT   0
#define RL_MODE_NORMAL   1

#include "c64cart.h"

struct snapshot_s;

void ramlink_freeze(void);
void ramlink_config_init(export_t *ex);
void ramlink_config_setup(uint8_t *rawcart);
int ramlink_bin_attach(const char *filename, uint8_t *rawcart);
void ramlink_detach(void);
int ramlink_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int ramlink_cart_enabled(void);

int ramlink_flush_ram_image(void);
const char *ramlink_get_ram_file_name(void);
int ramlink_ram_save(const char *filename);

int ramlink_can_flush_ram_image(void);
int ramlink_can_save_ram_image(void);

int ramlink_roml_read(uint16_t addr, uint8_t *value);
int ramlink_romh_read(uint16_t addr, uint8_t *value);
int ramlink_a000_bfff_read(uint16_t addr, uint8_t *value);
int ramlink_peek_mem(uint16_t addr, uint8_t *value);
int ramlink_romh_phi1_read(uint16_t addr, uint8_t *value);
int ramlink_romh_phi2_read(uint16_t addr, uint8_t *value);
int ramlink_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);
void c128ramlink_switch_mode(int mode);
int c128ramlink_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit, int mem_config);
uint8_t c128ramlink_hi_read(uint16_t addr, uint8_t *value);
uint8_t c128ramlink_roml_read(uint16_t addr, uint8_t *value);

void ramlink_passthrough_changed(export_t *ex);
int ramlink_cart_mode(void);

int ramlink_enable(void);
int ramlink_disable(void);

int ramlink_cmdline_options_init(void);
int ramlink_resources_init(void);
int ramlink_resources_shutdown(void);

int ramlink_snapshot_write_module(struct snapshot_s *s);
int ramlink_snapshot_read_module(struct snapshot_s *s);

#endif
