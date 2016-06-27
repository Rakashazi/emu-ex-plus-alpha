/*
 * raster-changes.c - Handling of changes within a raster line.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include "lib.h"
#include "raster-changes.h"
#include "raster.h"


void raster_changes_init(raster_t *raster)
{
    raster->changes = lib_calloc(1, sizeof(raster_changes_all_t));

    raster->changes->background = lib_calloc(1, sizeof(raster_changes_t));
    raster->changes->foreground = lib_calloc(1, sizeof(raster_changes_t));
    raster->changes->border = lib_calloc(1, sizeof(raster_changes_t));
    raster->changes->sprites = lib_calloc(1, sizeof(raster_changes_t));
    raster->changes->next_line = lib_calloc(1, sizeof(raster_changes_t));
}

void raster_changes_shutdown(raster_t *raster)
{
    if (raster->changes) {
        lib_free(raster->changes->background);
        lib_free(raster->changes->foreground);
        lib_free(raster->changes->border);
        lib_free(raster->changes->sprites);
        lib_free(raster->changes->next_line);

        lib_free(raster->changes);
    }
}

#if 0
raster_changes_t *raster_changes_new(void)
{
    raster_changes_t *new_changes;

    new_changes = lib_malloc(sizeof(raster_changes_t));
    raster_changes_init(new_changes);

    return new_changes;
}
#endif
