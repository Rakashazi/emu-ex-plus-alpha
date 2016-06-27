/*
 * magicformel.h - Cartridge handling, Magic Formel cart.
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

#ifndef VICE_MAGICFORMEL_H
#define VICE_MAGICFORMEL_H

#include <stdio.h>

#include "types.h"
#include "c64cart.h"

struct snapshot_s;

extern BYTE magicformel_romh_read(WORD addr);
extern BYTE magicformel_romh_read_hirom(WORD addr);
extern int magicformel_romh_phi1_read(WORD addr, BYTE *value);
extern int magicformel_romh_phi2_read(WORD addr, BYTE *value);
extern int magicformel_peek_mem(export_t *export, WORD addr, BYTE *value);

extern void magicformel_freeze(void);
extern void magicformel_config_init(void);
extern void magicformel_reset(void);
extern void magicformel_config_setup(BYTE *rawcart);
extern int magicformel_bin_attach(const char *filename, BYTE *rawcart);
extern int magicformel_crt_attach(FILE *fd, BYTE *rawcart);
extern void magicformel_detach(void);

extern int magicformel_snapshot_write_module(struct snapshot_s *s);
extern int magicformel_snapshot_read_module(struct snapshot_s *s);

#endif
