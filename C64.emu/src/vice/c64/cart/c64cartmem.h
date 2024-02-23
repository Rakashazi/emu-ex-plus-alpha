/*
 * c64cartmem.h -- C64 cartridge emulation, memory handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_C64CARTMEM_H
#define VICE_C64CARTMEM_H

#include "types.h"

/*
    this interface is used by: vicii-fetch.c, vicii.c, c64meminit.c, c64mem.c, c64memsc.c
*/

/* expansion port memory read/write hooks */
uint8_t roml_read(uint16_t addr);
void roml_store(uint16_t addr, uint8_t value);
uint8_t romh_read(uint16_t addr);
uint8_t ultimax_romh_read_hirom(uint16_t addr);
void romh_store(uint16_t addr, uint8_t value);
void roml_no_ultimax_store(uint16_t addr, uint8_t value);
void raml_no_ultimax_store(uint16_t addr, uint8_t value);
void romh_no_ultimax_store(uint16_t addr, uint8_t value);
void ramh_no_ultimax_store(uint16_t addr, uint8_t value);

uint8_t ultimax_0800_0fff_read(uint16_t addr);
void ultimax_0800_0fff_store(uint16_t addr, uint8_t value);
uint8_t ultimax_1000_7fff_read(uint16_t addr);
void ultimax_1000_7fff_store(uint16_t addr, uint8_t value);
uint8_t ultimax_a000_bfff_read(uint16_t addr);
void ultimax_a000_bfff_store(uint16_t addr, uint8_t value);
uint8_t ultimax_c000_cfff_read(uint16_t addr);
void ultimax_c000_cfff_store(uint16_t addr, uint8_t value);
uint8_t ultimax_d000_dfff_read(uint16_t addr);
void ultimax_d000_dfff_store(uint16_t addr, uint8_t value);

/* VIC-II reads. the _ptr functions are for the old vic implementation (x64) */
uint8_t *ultimax_romh_phi1_ptr(uint16_t addr);
uint8_t *ultimax_romh_phi2_ptr(uint16_t addr);
int ultimax_romh_phi1_read(uint16_t addr, uint8_t *value);
int ultimax_romh_phi2_read(uint16_t addr, uint8_t *value);

#endif
