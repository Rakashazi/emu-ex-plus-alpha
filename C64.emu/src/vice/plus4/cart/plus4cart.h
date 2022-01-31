/*
 * plus4cart.h -- Plus4 cartridge handling.
 *
 * Written by
 *  Tibor Biczo <crown@axelero.hu>
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_PLUS4CART_H
#define VICE_PLUS4CART_H

#include "types.h"

/* Cartridge ROM limit = 4MB */
#define PLUS4CART_ROM_LIMIT (1024 * 1024 * 4)
/* Cartridge RAM limit = 32kB */
#define PLUS4CART_RAM_LIMIT (32 * 1024)
/* maximum size of a full "all inclusive" cartridge image */
#define PLUS4CART_IMAGE_LIMIT (PLUS4CART_ROM_LIMIT + PLUS4CART_RAM_LIMIT)

#define PLUS4_C1LO_ROM_SIZE (16 * 0x400)
#define PLUS4_C1HI_ROM_SIZE (16 * 0x400)

/* expansion port access functions */
extern uint8_t plus4cart_c1lo_read(uint16_t addr);
extern uint8_t plus4cart_c1hi_read(uint16_t addr);

/* FIXME: these live in plus4-generic.c */
/* FIXME: we need this because of a reference in plus4mem.c that should be
          replaced by a better, indirect, access mechanism */
extern uint8_t extromlo2[PLUS4_C1LO_ROM_SIZE];
extern uint8_t extromhi2[PLUS4_C1HI_ROM_SIZE];

#endif /* _PLUS4CART_H */
