/*
 * tpi.h - IEEE488 interface for the C64.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_C64TPI_H
#define VICE_C64TPI_H

#include "types.h"

struct machine_context_s;

int tpi_cart_enabled(void);

void tpi_config_init(export_t *export);
void tpi_config_setup(uint8_t *rawcart);
void tpi_detach(void);
int tpi_enable(void);
int tpi_disable(void);

int tpi_cmdline_options_init(void);
int tpi_resources_init(void);
void tpi_resources_shutdown(void);

int tpi_roml_read(uint16_t addr, uint8_t *value);
int tpi_peek_mem(uint16_t addr, uint8_t *value);

void tpi_passthrough_changed(export_t *export);

void tpi_setup_context(struct machine_context_s *machine_context);
int tpi_bin_attach(const char *filename, uint8_t *rawcart);
int tpi_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
const char *tpi_get_file_name(void);
int tpi_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

void tpi_reset(void);
void tpi_init(void);
void tpi_shutdown(void);

struct snapshot_s;

int tpi_snapshot_read_module(struct snapshot_s *s);
int tpi_snapshot_write_module(struct snapshot_s *s);

#endif
