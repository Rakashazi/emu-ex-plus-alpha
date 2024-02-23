/*
 * magicvoice.h - Speech Cartridge
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_MAGICVOICE_H
#define VICE_MAGICVOICE_H

#include "types.h"
#include "sound.h"

struct machine_context_s;

void magicvoice_reset(void);
int magicvoice_cart_enabled(void);

int magicvoice_a000_bfff_read(uint16_t addr, uint8_t *value);
int magicvoice_roml_read(uint16_t addr, uint8_t *value);
int magicvoice_romh_read(uint16_t addr, uint8_t *value);
int magicvoice_ultimax_read(uint16_t addr, uint8_t *value);
int magicvoice_romh_phi1_read(uint16_t addr, uint8_t *value);
int magicvoice_romh_phi2_read(uint16_t addr, uint8_t *value);
int magicvoice_peek_mem(uint16_t addr, uint8_t *value);

void magicvoice_passthrough_changed(export_t *export);

void magicvoice_init(void);
void magicvoice_shutdown(void);

void magicvoice_config_init(export_t *export);
void magicvoice_config_setup(uint8_t *rawcart);
void magicvoice_setup_context(struct machine_context_s *machine_context);

int magicvoice_resources_init(void);
void magicvoice_resources_shutdown(void);

int magicvoice_cmdline_options_init(void);

int magicvoice_bin_attach(const char *filename, uint8_t *rawcart);
int magicvoice_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
int magicvoice_enable(void);
int magicvoice_disable(void);
void magicvoice_detach(void);
const char *magicvoice_get_file_name(void);
int magicvoice_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

void magicvoice_sound_chip_init(void);

struct snapshot_s;

int magicvoice_snapshot_read_module(struct snapshot_s *s);
int magicvoice_snapshot_write_module(struct snapshot_s *s);

#endif
