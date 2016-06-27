/*
 * raster-line-changes.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "raster-cache.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "raster.h"
#include "viewport.h"


static int update_for_minor_changes(raster_t *raster,
                                    unsigned int *changed_start,
                                    unsigned int *changed_end)
{
    raster_cache_t *cache;
    unsigned int video_mode;
    unsigned int changed_start_char, changed_end_char;
    int needs_update;

    video_mode = raster_line_get_real_mode(raster);

    cache = &(raster->cache)[raster->current_line];

    changed_start_char = raster->geometry->text_size.width;
    changed_end_char = 0;

    needs_update = raster_modes_fill_cache(raster->modes,
                                           video_mode,
                                           cache,
                                           &changed_start_char,
                                           &changed_end_char,
                                           0);
    if (needs_update) {
        raster_modes_draw_line_cached(raster->modes,
                                      video_mode,
                                      cache,
                                      changed_start_char,
                                      changed_end_char);

        /* Convert from character to pixel coordinates. */
        *changed_start = raster->geometry->gfx_position.x + raster->xsmooth
                         + raster->geometry->char_pixel_width * changed_start_char;

        *changed_end = raster->geometry->gfx_position.x + raster->xsmooth
                       + raster->geometry->char_pixel_width * (changed_end_char + 1) - 1;
    }

    /* FIXME: Why always doing so?  */
    raster_line_draw_borders(raster);

    return needs_update;
}

void raster_line_changes_init(raster_t *raster)
{
    raster->line_changes = update_for_minor_changes;
}
