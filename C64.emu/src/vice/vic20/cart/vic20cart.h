/*
 * vic20cart.h - VIC20 Cartridge emulation.
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

#ifndef VICE_VIC20CART_H
#define VICE_VIC20CART_H

#include "types.h"

extern void reset_try_flags(void);
extern int try_cartridge_attach(int c);

#define TRY_RESOURCE_CARTTYPE (1 << 0)
#define TRY_RESOURCE_CARTNAME (1 << 1)
#define TRY_RESOURCE_CARTRESET (1 << 2)

extern int cartridge_is_from_snapshot;

struct snapshot_s;
/* FIXME: rename functions to cartridge_... and remove these prototypes (they are in cartridge.h) */
extern int vic20cart_snapshot_write_module(struct snapshot_s *s);
extern int vic20cart_snapshot_read_module(struct snapshot_s *s);

/* used internally, don't call from UI or other non cart related code */
extern void cartridge_attach(int type, BYTE *rawcart);
extern void cartridge_detach(int type);

#endif
