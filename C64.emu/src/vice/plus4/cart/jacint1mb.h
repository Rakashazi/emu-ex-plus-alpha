
/*
 * jacint1mb.h - 1MB Cartridge handling
 *
 * Written by
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

#ifndef VICE_1MBJACINT_H
#define VICE_1MBJACINT_H

#include "types.h"

extern uint8_t jacint1mb_c1lo_read(uint16_t addr);

extern void jacint1mb_config_setup(uint8_t *rawcart);
extern int jacint1mb_bin_attach(const char *filename, uint8_t *rawcart);
extern int jacint1mb_crt_attach(FILE *fd, uint8_t *rawcart);

extern void jacint1mb_detach(void);

extern void jacint1mb_reset(void);

extern int jacint1mb_snapshot_write_module(snapshot_t *s);
extern int jacint1mb_snapshot_read_module(snapshot_t *s);

#endif
