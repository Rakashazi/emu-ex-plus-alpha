/*
 * supersnapshot.h - Cartridge handling, Super Snapshot cart.
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

#ifndef VICE_SUPERSNAPSHOT_H
#define VICE_SUPERSNAPSHOT_H

#include "types.h"

extern BYTE supersnapshot_v5_roml_read(WORD addr);
extern void supersnapshot_v5_roml_store(WORD addr, BYTE value);
extern void supersnapshot_v5_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern void supersnapshot_v5_freeze(void);

extern void supersnapshot_v5_config_init(void);
extern void supersnapshot_v5_config_setup(BYTE *rawcart);
extern int supersnapshot_v5_bin_attach(const char *filename, BYTE *rawcart);
extern int supersnapshot_v5_crt_attach(FILE *fd, BYTE *rawcart);
extern void supersnapshot_v5_detach(void);

extern int supersnapshot_v5_resources_init(void);
extern void supersnapshot_v5_resources_shutdown(void);
extern int supersnapshot_v5_cmdline_options_init(void);

struct snapshot_s;

extern int supersnapshot_v5_snapshot_write_module(struct snapshot_s *s);
extern int supersnapshot_v5_snapshot_read_module(struct snapshot_s *s);

#endif
