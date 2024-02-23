
/*
 * plus4cart.h -- Plus4 generic cartridge handling.
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

#ifndef VICE_PLUS4GENERICCART_H
#define VICE_PLUS4GENERICCART_H

uint8_t generic_c1lo_read(uint16_t addr);
uint8_t generic_c1hi_read(uint16_t addr);

void generic_config_setup(uint8_t *rawcart);

int generic_crt_attach(FILE *fd, uint8_t *rawcart);
int generic_bin_attach(int type, const char *filename, uint8_t *rawcart);
void generic_detach(int type);

int generic_resources_init(void);
void generic_resources_shutdown(void);

int generic_cmdline_options_init(void);

int generic_snapshot_write_module(snapshot_t *s);
int generic_snapshot_read_module(snapshot_t *s);

/* FIXME: do we still need these? */
int plus4cart_load_c1lo(const char *rom_name);
int plus4cart_load_c1hi(const char *rom_name);

#endif
