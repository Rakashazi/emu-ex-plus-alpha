/*
 * ocean.c - Cartridge handling, Ocean cart.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_OCEAN_H
#define VICE_OCEAN_H

#include <stdio.h>

#include "types.h"

extern void ocean_config_setup(BYTE *rawcart);
extern void ocean_config_init(void);
extern int ocean_bin_attach(const char *filename, BYTE *rawcart);
extern int ocean_crt_attach(FILE *fd, BYTE *rawcart);
extern void ocean_detach(void);

extern BYTE ocean_romh_read(WORD addr);

struct snapshot_s;

extern int ocean_snapshot_write_module(struct snapshot_s *s);
extern int ocean_snapshot_read_module(struct snapshot_s *s);

#endif
