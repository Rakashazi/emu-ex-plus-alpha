/*
 * cs8900io.h - CS8900 I/O for TFE and RRNET (clockport) carts.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifndef VICE_TFE_H
#define VICE_TFE_H

#ifdef HAVE_RAWNET

#include <stdint.h>

#include "types.h"

int cs8900io_cart_enabled(void);

void cs8900io_init(void);
int cs8900io_resources_init(void);
void cs8900io_resources_shutdown(void);
int cs8900io_cmdline_options_init(void);

void cs8900io_reset(void);
void cs8900io_detach(void);
int cs8900io_enable(const char *owner);
int cs8900io_disable(void);

void cs8900io_store(uint16_t io_address, uint8_t byte);
uint8_t cs8900io_read(uint16_t io_address);
uint8_t cs8900io_peek(uint16_t io_address);
int cs8900io_dump(void);

#endif
#endif
