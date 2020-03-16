/*
 * sidcart.h - SID cart emulation.
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

#ifndef VICE_SIDCART_H
#define VICE_SIDCART_H

#include "snapshot.h"

#define SIDCART_CLOCK_C64     0
#define SIDCART_CLOCK_NATIVE  1

extern int sidcartjoy_enabled;

extern int sidcart_cmdline_options_init(void);
extern int sidcart_resources_init(void);

extern int sidcart_enabled(void);

extern int sidcart_address;
extern int sidcart_clock;

extern void sidcart_sound_chip_init(void);

extern void sidcart_detach(void);

extern int sidcart_snapshot_write_module(snapshot_t *s);
extern int sidcart_snapshot_read_module(snapshot_t *s);

#endif
