/*
 * pethre.h - PET Hi-Res Emulator Emulation
 *
 * Written by
 *  Olaf 'Rhialto' Seibert <rhialto@falu.nl>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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

#ifndef VICE_PETHRE_H
#define VICE_PETHRE_H

#include "types.h"
#include "mem.h"

/* ------------------------------------------------------------------------- */

extern int pethre_enabled;

struct snapshot_s;

extern int pethre_init_resources(void);
extern int pethre_init_cmdline_options(void);
extern int pethre_resources_init(void);
extern void pethre_resources_shutdown(void);
extern int pethre_cmdline_options_init(void);

extern void pethre_init(void);
extern void pethre_reset(void);
extern void pethre_shutdown(void);
extern int e888_dump(void);

extern int pethre_snapshot_read_module(struct snapshot_s *);
extern int pethre_snapshot_write_module(struct snapshot_s *);

extern void crtc_store_hre(WORD addr, BYTE value);

extern void pethre_powerup(void);

#endif
