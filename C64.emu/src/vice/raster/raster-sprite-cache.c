/*
 * raster-sprite-cache.c - Cache for a sprite raster line.
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
#include "raster-sprite-cache.h"


void raster_sprite_cache_init(raster_sprite_cache_t *sc)
{
    sc->c1 = sc->c2 = sc->c3 = 0;
    sc->data = 0;
    sc->x_expanded = 0;
    sc->x = 0;
    sc->visible = 0;
    sc->in_background = 0;
    sc->multicolor = 0;
}

raster_sprite_cache_t *raster_sprite_cache_new(void)
{
    raster_sprite_cache_t *new_cache;

    new_cache = lib_malloc(sizeof(raster_sprite_cache_t));
    raster_sprite_cache_init(new_cache);

    return new_cache;
}
