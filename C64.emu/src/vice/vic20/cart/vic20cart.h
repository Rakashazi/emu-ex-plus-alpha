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

void reset_try_flags(void);
int try_cartridge_attach(int c);

#define TRY_RESOURCE_CARTTYPE (1 << 0)
#define TRY_RESOURCE_CARTNAME (1 << 1)
#define TRY_RESOURCE_CARTRESET (1 << 2)

/* Cartridge ROM limit = 1MB */
#define VIC20CART_ROM_LIMIT (1024 * 1024)
/* Cartridge RAM limit = 64kB */
#define VIC20CART_RAM_LIMIT (64 * 1024)
/* maximum size of a full "all inclusive" cartridge image */
#define VIC20CART_IMAGE_LIMIT (VIC20CART_ROM_LIMIT)

extern int cartridge_is_from_snapshot;

struct snapshot_s;
/* FIXME: rename functions to cartridge_... and remove these prototypes (they are in cartridge.h) */
int vic20cart_snapshot_write_module(struct snapshot_s *s);
int vic20cart_snapshot_read_module(struct snapshot_s *s);

/* used internally, don't call from UI or other non cart related code */
void cartridge_attach(int type, uint8_t *rawcart);
void cartridge_detach(int type);

#endif
