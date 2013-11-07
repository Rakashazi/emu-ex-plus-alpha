/*
 * raster-canvas.c - Raster-based video chip emulation helper.
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

#include <stdio.h>

#include "lib.h"
#include "machine.h"
#include "raster-canvas.h"
#include "raster.h"
#include "video.h"
#include "videoarch.h"
#include "viewport.h"


inline static void refresh_canvas(raster_t *raster)
{
    raster_canvas_area_t *update_area;
    viewport_t *viewport;
    int x, y, xx, yy;
    int w, h;

    update_area = raster->update_area;
    viewport = raster->canvas->viewport;

    if (update_area->is_null) {
        return;
    }

    x = update_area->xs;
    y = update_area->ys;
    xx = update_area->xs - viewport->first_x;
    yy = update_area->ys - viewport->first_line;
    w = update_area->xe - update_area->xs + 1;
    h = update_area->ye - update_area->ys + 1;

    if (raster->canvas->videoconfig->filter == VIDEO_FILTER_CRT) {
        /* if pal emu is activated, more pixels have to be updated: around,
         * above and below, because of blurring and scanline effects.
         *
         * 1 line above and 1 lines below are required because the update on
         * any line affects the scanlines both above and below, so both must
         * be included in the full update rectangle.
         *
         * These coordinates are also passed to the graphics driver as the
         * updated region, so the area here must be at least as large as the
         * updated region. */
        x -= 4;
        xx -= 4;
        w += 8;
        y--;
        yy--;
        h += 2;
    }

    if (xx < 0) {
        x -= xx;
        w += xx;
        xx = 0;
    }

    if (yy < 0) {
        y -= yy;
        h += yy;
        yy = 0;
    }
    x += raster->canvas->geometry->extra_offscreen_border_left;

    xx += viewport->x_offset;
    yy += viewport->y_offset;

    if ((int)(raster->canvas->draw_buffer->canvas_height) >= yy
        && (int)(raster->canvas->draw_buffer->canvas_width) >= xx) {
        video_canvas_refresh(raster->canvas, x, y, xx, yy,
                             MIN(w, (int)(raster->canvas->draw_buffer->canvas_width - xx)),
                             MIN(h, (int)(raster->canvas->draw_buffer->canvas_height - yy)));
    }

    update_area->is_null = 1;
}

void raster_canvas_handle_end_of_frame(raster_t *raster)
{
    if (video_disabled_mode) {
        return;
    }

    if (raster->skip_frame) {
        return;
    }

    if (!raster->canvas->viewport->update_canvas) {
        return;
    }

    if (raster->dont_cache) {
        video_canvas_refresh_all(raster->canvas);
    } else {
        refresh_canvas(raster);
    }
}

void raster_canvas_init(raster_t *raster)
{
    raster->update_area = lib_malloc(sizeof(raster_canvas_area_t));

    raster->update_area->is_null = 1;
}

void raster_canvas_shutdown(raster_t *raster)
{
    lib_free(raster->update_area);
}
