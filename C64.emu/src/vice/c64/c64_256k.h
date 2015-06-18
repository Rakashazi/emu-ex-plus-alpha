/*
 * c64_256k.h - 256K EXPANSION emulation.
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

#ifndef VICE_C64_256K_H
#define VICE_C64_256K_H

#include "types.h"

extern int c64_256k_start;
extern int c64_256k_enabled;

extern int c64_256k_resources_init(void);
extern void c64_256k_resources_shutdown(void);
extern int c64_256k_cmdline_options_init(void);

extern void c64_256k_init(void);
extern void c64_256k_reset(void);
extern void c64_256k_cia_set_vbank(int ciabank);
extern void c64_256k_shutdown(void);

extern void c64_256k_ram_segment0_store(WORD addr, BYTE value);
extern void c64_256k_ram_segment1_store(WORD addr, BYTE value);
extern void c64_256k_ram_segment2_store(WORD addr, BYTE value);
extern void c64_256k_ram_segment3_store(WORD addr, BYTE value);
extern BYTE c64_256k_ram_segment0_read(WORD addr);
extern BYTE c64_256k_ram_segment1_read(WORD addr);
extern BYTE c64_256k_ram_segment2_read(WORD addr);
extern BYTE c64_256k_ram_segment3_read(WORD addr);

extern int set_c64_256k_enabled(int value);

#endif
