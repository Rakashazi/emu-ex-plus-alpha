/*
 * plus4cart.h -- Plus4 cartridge handling.
 *
 * Written by
 *  Tibor Biczo <crown@axelero.hu>
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

extern int plus4cart_load_func_lo(const char *rom_name);
extern int plus4cart_load_func_hi(const char *rom_name);
extern int plus4cart_load_c1lo(const char *rom_name);
extern int plus4cart_load_c1hi(const char *rom_name);
extern int plus4cart_load_c2lo(const char *rom_name);
extern int plus4cart_load_c2hi(const char *rom_name);
extern void plus4cart_detach_cartridges(void);

#endif /* _PLUS4CART_H */
