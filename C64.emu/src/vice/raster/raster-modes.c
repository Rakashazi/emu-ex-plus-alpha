/*
 * raster-mode.c - Definition of drawing modes for the raster emulation.
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

#include <stdio.h>

#include "lib.h"
#include "raster-modes.h"


void raster_modes_init(raster_modes_t *modes, unsigned int num_modes)
{
    unsigned int i;

    modes->num_modes = num_modes;
    modes->idle_mode = 0;
    modes->modes = lib_malloc(sizeof(*modes->modes) * num_modes);

    for (i = 0; i < num_modes; i++) {
        raster_modes_def_t *mode;

        mode = modes->modes + i;

        mode->fill_cache = NULL;
        mode->draw_line_cached = NULL;
        mode->draw_line = NULL;
        mode->draw_background = NULL;
        mode->draw_foreground = NULL;
    }
}

void raster_modes_shutdown(raster_modes_t *modes)
{
    lib_free(modes->modes);
}

raster_modes_t *raster_modes_new(unsigned int num_modes)
{
    raster_modes_t *new_mode;

    new_mode = lib_malloc(sizeof(raster_modes_t));

    raster_modes_init(new_mode, num_modes);

    return new_mode;
}

void raster_modes_set(raster_modes_t *modes,
                      unsigned int num_mode,
                      raster_modes_fill_cache_function_t fill_cache,
                      raster_modes_draw_line_cached_function_t draw_line_cached,
                      raster_modes_draw_line_function_t draw_line,
                      raster_modes_draw_background_function_t draw_background,
                      raster_modes_draw_foreground_function_t draw_foreground)
{
    raster_modes_def_t *mode;

    mode = modes->modes + num_mode;

    mode->fill_cache = fill_cache;
    mode->draw_line_cached = draw_line_cached;
    mode->draw_line = draw_line;
    mode->draw_background = draw_background;
    mode->draw_foreground = draw_foreground;
}

int raster_modes_set_idle_mode(raster_modes_t *modes,
                               unsigned int num_mode)
{
    if (num_mode >= modes->num_modes) {
        return -1;
    }

    modes->idle_mode = num_mode;
    return 0;
}
