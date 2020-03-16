/*
 * tapecart.h: tapecart emulation.
 *
 * Written by
 *  Ingo Korb <ingo@akana.de>
 *
 * based on sense-dongle.h by
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

#ifndef VICE_TAPECART_H
#define VICE_TAPECART_H

#include <stdint.h>

#define TAPECART_FLASH_SIZE    (2 * 1024 * 1024)
#define TAPECART_LOADER_SIZE   171
#define TAPECART_FILENAME_SIZE 16

/* all the important data from a TCRT file */
/* (aka "everything a real tapecart stores in non-volatile memory") */
typedef struct tapecart_memory_s {
    uint8_t  flash[TAPECART_FLASH_SIZE];
    uint8_t  loader[TAPECART_LOADER_SIZE];
    uint8_t  filename[TAPECART_FILENAME_SIZE];
    uint16_t data_offset;
    uint16_t data_length;
    uint16_t call_address;
    int      changed;
} tapecart_memory_t;

extern int tapecart_resources_init(void);
extern int tapecart_cmdline_options_init(void);
extern int tapecart_attach_tcrt(const char *filename, void *unused);
extern int tapecart_flush_tcrt(void);
extern void tapecart_exit(void);

int tapecart_is_valid(const char *filename);

#endif
