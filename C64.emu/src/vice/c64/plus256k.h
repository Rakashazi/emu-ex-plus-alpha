/*
 * plus256k.h - +256K EXPANSION HACK emulation.
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

#ifndef VICE_PLUS256K_H
#define VICE_PLUS256K_H

#include "snapshot.h"
#include "types.h"

extern int plus256k_enabled;

extern int plus256k_resources_init(void);
extern void plus256k_resources_shutdown(void);
extern int plus256k_cmdline_options_init(void);
extern void plus256k_init(void);
extern void plus256k_reset(void);
extern void plus256k_shutdown(void);

extern void plus256k_ram_low_store(WORD addr, BYTE value);
extern void plus256k_ram_high_store(WORD addr, BYTE value);
extern BYTE plus256k_ram_low_read(WORD addr);
extern BYTE plus256k_ram_high_read(WORD addr);

extern int set_plus256k_enabled(int value, int disable_reset);

extern int plus256k_snapshot_write(struct snapshot_s *s);
extern int plus256k_snapshot_read(struct snapshot_s *s);

#endif
