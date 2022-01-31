/*
 * crt.h - CRT image handling.
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

#ifndef VICE_CRT_H
#define VICE_CRT_H

#include "types.h"

typedef struct crt_header_s {
    uint16_t version;         /* version */
    uint16_t type;            /* type of cartridge */
    uint8_t subtype;          /* subtype/hardware revision of cartridge */
    int exrom;                /* exrom line status */
    int game;                 /* game line status */
    char name[32 + 1];        /* name of cartridge */
} crt_header_t;

typedef struct crt_chip_header_s {
    uint32_t skip;               /* bytes to skip after ROM */
    uint16_t type;                /* chip type */
    uint16_t bank;                /* bank number */
    uint16_t start;               /* start address of ROM */
    uint16_t size;                /* size of ROM in bytes */
} crt_chip_header_t;

FILE *crt_open(const char *filename, crt_header_t *header);
extern int crt_getid(const char *filename);
extern int crt_read_chip_header(crt_chip_header_t *header, FILE *fd);
extern int crt_read_chip(uint8_t *rawcart, int offset, crt_chip_header_t *chip, FILE *fd);
extern FILE *crt_create(const char *filename, int type, int exrom, int game, const char *name);
extern int crt_write_chip(uint8_t *data, crt_chip_header_t *header, FILE *fd);
/* create v1.1 header with sub type */
extern FILE *crt_create_v11(const char *filename, int type, int subtype, int exrom, int game, const char *name);

#endif
