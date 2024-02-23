/*
 * ram.h - RAM stuff.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifndef VICE_RAM_H
#define VICE_RAM_H

#include "types.h"

#define RAM_INIT_RANDOM_CHANCE_MAX      10000  /* 100% */
#define RAM_INIT_RANDOM_CHANCE_DEFAULT     10  /* 0.1% */

typedef struct _RAMINITPARAM {
    int start_value;/* first value of the base pattern (byte value) */
    int value_invert; /* number of bytes until start value is inverted */
    int value_offset; /* offset of first pattern in bytes */

    int pattern_invert; /* invert base pattern after this many bytes */
    int pattern_invert_value; /* invert base pattern with this byte */

    int random_start; /* length of random pattern in bytes */ /* FIXME: bad name */
    int random_repeat; /* repeat random pattern after this many bytes */

    int random_chance; /* global random chance */
} RAMINITPARAM;

int ram_resources_init(void);
int ram_cmdline_options_init(void);

void ram_init(uint8_t *memram, unsigned int ramsize);
void ram_init_with_pattern(uint8_t *memram, unsigned int ramsize, RAMINITPARAM *ramparam);
void ram_init_print_pattern(char *s, int len, char *eol);

#endif
