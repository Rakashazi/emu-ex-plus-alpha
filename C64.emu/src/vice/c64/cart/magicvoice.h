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

extern void magicvoice_reset(void);
extern int magicvoice_cart_enabled(void);

extern int magicvoice_a000_bfff_read(WORD addr, BYTE *value);
extern int magicvoice_roml_read(WORD addr, BYTE *value);
extern int magicvoice_romh_read(WORD addr, BYTE *value);
extern int magicvoice_ultimax_read(WORD addr, BYTE *value);
extern int magicvoice_romh_phi1_read(WORD addr, BYTE *value);
extern int magicvoice_romh_phi2_read(WORD addr, BYTE *value);
extern int magicvoice_peek_mem(WORD addr, BYTE *value);

extern void magicvoice_passthrough_changed(export_t *export);

extern void magicvoice_init(void);
extern void magicvoice_shutdown(void);

extern void magicvoice_config_init(export_t *export);
extern void magicvoice_config_setup(BYTE *rawcart);
extern void magicvoice_setup_context(struct machine_context_s *machine_context);

extern int magicvoice_resources_init(void);
extern void magicvoice_resources_shutdown(void);

extern int magicvoice_cmdline_options_init(void);

extern int magicvoice_bin_attach(const char *filename, BYTE *rawcart);
extern int magicvoice_crt_attach(FILE *fd, BYTE *rawcart);
extern int magicvoice_enable(void);
extern void magicvoice_detach(void);
extern const char *magicvoice_get_file_name(void);
extern int magicvoice_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern void magicvoice_sound_chip_init(void);

struct snapshot_s;
extern int magicvoice_snapshot_read_module(struct snapshot_s *s);
extern int magicvoice_snapshot_write_module(struct snapshot_s *s);

#endif
