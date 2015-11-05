/*
 * megacart.h -- VIC20 Mega-Cart emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_MEGACART_H
#define VICE_MEGACART_H

#include <stdio.h>

#include "types.h"

extern BYTE megacart_ram123_read(WORD addr);
extern void megacart_ram123_store(WORD addr, BYTE value);
extern BYTE megacart_blk123_read(WORD addr);
extern void megacart_blk123_store(WORD addr, BYTE value);
extern BYTE megacart_blk5_read(WORD addr);
extern void megacart_blk5_store(WORD addr, BYTE value);

extern void megacart_init(void);
extern void megacart_reset(void);

extern void megacart_config_setup(BYTE *rawcart);
extern int megacart_bin_attach(const char *filename);
/* extern int megacart_bin_attach(const char *filename, BYTE *rawcart); */
/* extern int megacart_crt_attach(FILE *fd, BYTE *rawcart); */
extern void megacart_detach(void);

extern int megacart_resources_init(void);
extern void megacart_resources_shutdown(void);
extern int megacart_cmdline_options_init(void);

struct snapshot_s;

extern int megacart_snapshot_write_module(struct snapshot_s *s);
extern int megacart_snapshot_read_module(struct snapshot_s *s);

#endif
