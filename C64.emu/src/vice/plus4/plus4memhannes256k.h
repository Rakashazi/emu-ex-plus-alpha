/*
 * plus4memhannes256k.h - HANNES 256K EXPANSION emulation.
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

#ifndef VICE_H256K_H
#define VICE_H256K_H

#include "types.h"

#define H256K_DISABLED  0
#define H256K_256K      1
#define H256K_1024K     2
#define H256K_4096K     3

extern int h256k_enabled;

extern void h256k_init(void);
extern void h256k_reset(void);
extern void h256k_shutdown(void);

extern BYTE h256k_reg_read(WORD addr);
extern void h256k_reg_store(WORD addr, BYTE value);
extern void h256k_store(WORD addr, BYTE value);
extern BYTE h256k_read(WORD addr);

extern int set_h256k_enabled(int val);

#endif
