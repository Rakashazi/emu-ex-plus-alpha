/*
 * snapshot64.h - Cartridge handling, Snapshot64 cart.
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

#ifndef VICE_SNAPSHOT64_H
#define VICE_SNAPSHOT64_H

#include "types.h"

extern BYTE snapshot64_roml_read(WORD addr);
extern BYTE snapshot64_romh_read(WORD addr);

extern void snapshot64_freeze(void);

extern void snapshot64_config_init(void);
extern void snapshot64_config_setup(BYTE *rawcart);
extern int snapshot64_bin_attach(const char *filename, BYTE *rawcart);
extern int snapshot64_crt_attach(FILE *f, BYTE *rawcart);

extern void snapshot64_detach(void);

struct snapshot_s;

extern int snapshot64_snapshot_write_module(struct snapshot_s *s);
extern int snapshot64_snapshot_read_module(struct snapshot_s *s);

#endif
