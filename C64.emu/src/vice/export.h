/*
 * export.h - Expansion port and devices handling.
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

#ifndef VICE_EXPORT_H
#define VICE_EXPORT_H

#include "cartio.h"

typedef struct export_resource_s {
    const char *name;
    unsigned int game;
    unsigned int exrom;
    io_source_t *io1;
    io_source_t *io2;
    unsigned int cartid;
} export_resource_t;

typedef struct export_list_s {
    struct export_list_s *previous;
    export_resource_t *device;
    struct export_list_s *next;
} export_list_t;

/* returns head of list if param is NULL, else the next item */
extern export_list_t *export_query_list(export_list_t *item);
extern void export_dump(void);

extern int export_add(const export_resource_t *export_res);
extern int export_remove(const export_resource_t *export_res);

extern int export_resources_init(void);

#endif
