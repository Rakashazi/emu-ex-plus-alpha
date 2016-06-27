/*
 * capture.h - Cartridge handling, Capture cart.
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

#ifndef VICE_CAPTURE_H
#define VICE_CAPTURE_H

#include <stdio.h>

#include "types.h"

#include "c64cart.h"

struct snapshot_s;

extern BYTE capture_romh_read(WORD addr);
extern void capture_romh_store(WORD addr, BYTE value);
extern BYTE capture_1000_7fff_read(WORD addr);
extern void capture_1000_7fff_store(WORD addr, BYTE value);
extern int capture_romh_phi1_read(WORD addr, BYTE *value);
extern int capture_romh_phi2_read(WORD addr, BYTE *value);
extern int capture_peek_mem(export_t *export, WORD addr, BYTE *value);

extern void capture_freeze(void);

extern void capture_config_init(void);
extern void capture_reset(void);
extern void capture_config_setup(BYTE *rawcart);
extern int capture_bin_attach(const char *filename, BYTE *rawcart);
extern int capture_crt_attach(FILE *fd, BYTE *rawcart);
extern void capture_detach(void);

extern int capture_snapshot_write_module(struct snapshot_s *s);
extern int capture_snapshot_read_module(struct snapshot_s *s);

#endif
