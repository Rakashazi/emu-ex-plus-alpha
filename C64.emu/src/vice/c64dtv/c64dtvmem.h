/*
 * c64dtvmem.h - C64DTV memory (2MB RAM/ROM) implementation
 *
 * Written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 * Based on code by
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

#ifndef VICE_C64DTVMEM_H
#define VICE_C64DTVMEM_H

#include "types.h"

int c64dtvmem_resources_init(void);
void c64dtvmem_resources_shutdown(void);
int c64dtvmem_cmdline_options_init(void);
void c64dtv_init(void);
void c64dtvmem_init_config(void);
void c64dtvmem_reset(void);
void c64dtvmem_shutdown(void);
/* void c64dtv_dma_irq_init(void); */

uint8_t c64dtv_mapper_read(uint16_t addr);
void c64dtv_mapper_store(uint16_t addr, uint8_t value);

uint8_t c64dtv_palette_read(uint16_t addr);
void c64dtv_palette_store(uint16_t addr, uint8_t value);

uint8_t c64dtv_dmablit_read(uint16_t addr);
void c64dtv_dmablit_store(uint16_t addr, uint8_t value);

uint8_t c64io1_read(uint16_t addr);
void c64io1_store(uint16_t addr, uint8_t value);
uint8_t c64io2_read(uint16_t addr);
void c64io2_store(uint16_t addr, uint8_t value);

extern uint8_t c64dtvmem_memmapper[0x2];

#endif
