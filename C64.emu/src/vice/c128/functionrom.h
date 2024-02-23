/*
 * functionrom.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_FUNCTIONROM_H
#define VICE_FUNCTIONROM_H

#include "types.h"

extern uint8_t int_function_rom[];

uint8_t internal_function_rom_read(uint16_t addr);
void internal_function_rom_store(uint16_t addr, uint8_t value);
void internal_function_top_shared_store(uint16_t addr, uint8_t value);
uint8_t internal_function_rom_peek(uint16_t addr);

int functionrom_resources_init(void);
void functionrom_resources_shutdown(void);
int functionrom_cmdline_options_init(void);

enum {
    INT_FUNCTION_NONE = 0,
    INT_FUNCTION_ROM,
    INT_FUNCTION_RAM,
    INT_FUNCTION_RTC
};

#endif
