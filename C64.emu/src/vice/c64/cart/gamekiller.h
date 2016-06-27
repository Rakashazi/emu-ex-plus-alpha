/*
 * gamekiller.h - Cartridge handling, Game Killer cart.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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

#ifndef VICE_GAMEKILLER_H
#define VICE_GAMEKILLER_H

#include <stdio.h>

#include "types.h"
#include "c64cart.h"

struct snapshot_s;

extern int gamekiller_peek_mem(export_t *export, WORD addr, BYTE *value);

extern void gamekiller_config_init(void);
extern void gamekiller_config_setup(BYTE *rawcart);
extern int gamekiller_bin_attach(const char *filename, BYTE *rawcart);
extern int gamekiller_crt_attach(FILE *fd, BYTE *rawcart);
extern void gamekiller_detach(void);
extern void gamekiller_freeze(void);

extern int gamekiller_snapshot_write_module(struct snapshot_s *s);
extern int gamekiller_snapshot_read_module(struct snapshot_s *s);

#endif
