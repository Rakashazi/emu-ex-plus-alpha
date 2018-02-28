/*
 * ultimem.h -- UltiMem emulation.
 *
 * Written by
 *  Marko Makela <marko.makela@iki.fi>
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

#ifndef VICE_VIC_UM_H
#define VICE_VIC_UM_H

#include <stdio.h>

#include "types.h"

extern BYTE vic_um_ram123_read(WORD addr);
extern void vic_um_ram123_store(WORD addr, BYTE value);
extern BYTE vic_um_blk1_read(WORD addr);
extern void vic_um_blk1_store(WORD addr, BYTE value);
extern BYTE vic_um_blk23_read(WORD addr);
extern void vic_um_blk23_store(WORD addr, BYTE value);
extern BYTE vic_um_blk5_read(WORD addr);
extern void vic_um_blk5_store(WORD addr, BYTE value);

extern void vic_um_init(void);
extern void vic_um_reset(void);

extern void vic_um_config_setup(BYTE *rawcart);
extern int vic_um_bin_attach(const char *filename);
extern void vic_um_detach(void);

extern int vic_um_resources_init(void);
extern void vic_um_resources_shutdown(void);
extern int vic_um_cmdline_options_init(void);

struct snapshot_s;

extern int vic_um_snapshot_write_module(struct snapshot_s *s);
extern int vic_um_snapshot_read_module(struct snapshot_s *s);

#endif
