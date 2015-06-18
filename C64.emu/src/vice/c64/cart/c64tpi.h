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
struct export_s;

extern int tpi_cart_enabled(void);

extern void tpi_config_init(struct export_s *export);
extern void tpi_config_setup(BYTE *rawcart);
extern void tpi_detach(void);
extern int tpi_enable(void);

extern int tpi_cmdline_options_init(void);
extern int tpi_resources_init(void);
extern void tpi_resources_shutdown(void);

extern int tpi_roml_read(WORD addr, BYTE *value);
extern int tpi_peek_mem(WORD addr, BYTE *value);

extern void tpi_passthrough_changed(struct export_s *export);

extern void tpi_setup_context(struct machine_context_s *machine_context);
extern int tpi_bin_attach(const char *filename, BYTE *rawcart);
extern int tpi_crt_attach(FILE *fd, BYTE *rawcart);
extern const char *tpi_get_file_name(void);
extern int tpi_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern void tpi_reset(void);
extern void tpi_init(void);
extern void tpi_shutdown(void);

struct snapshot_s;
extern int tpi_snapshot_read_module(struct snapshot_s *s);
extern int tpi_snapshot_write_module(struct snapshot_s *s);

#endif
