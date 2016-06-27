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

#ifndef VICE_RASTER_MODES_H
#define VICE_RASTER_MODES_H

#include "raster-cache.h"

/* Fill the cache with the screen data and check for differences.  If nothing
   has changed, return 0.  Otherwise, return the smallest interval that
   contains the changed parts and return 1.  If no_check != 0, fill the cache
   without checking for differences and return 1.  */
typedef int (*raster_modes_fill_cache_function_t)
    (struct raster_cache_s *c, unsigned int *changed_start,
    unsigned int *changed_end, int no_check);

/* Draw part of one line to the buffer.  */
typedef void (*raster_modes_draw_line_cached_function_t)
    (struct raster_cache_s *c, unsigned int start, unsigned int end);

/* Draw the whole line to the buffer.  */
typedef void (*raster_modes_draw_line_function_t)
    (void);

/* Draw part of the background to the buffer.  */
typedef void (*raster_modes_draw_background_function_t)
    (unsigned int start_pixel, unsigned int end_pixel);

/* Draw part of the foreground to the buffer.  */
typedef void (*raster_modes_draw_foreground_function_t)
    (unsigned int start_char, unsigned int end_char);

struct raster_modes_def_s {
    raster_modes_fill_cache_function_t fill_cache;
    raster_modes_draw_line_cached_function_t draw_line_cached;
    raster_modes_draw_line_function_t draw_line;
    raster_modes_draw_background_function_t draw_background;
    raster_modes_draw_foreground_function_t draw_foreground;
};
typedef struct raster_modes_def_s raster_modes_def_t;

struct raster_modes_s {
    /* Number of defined modes.  */
    unsigned int num_modes;

    /* List of modes.  */
    raster_modes_def_t *modes;

    /* Mode used for idle mode.  */
    unsigned int idle_mode;
};
typedef struct raster_modes_s raster_modes_t;


extern void raster_modes_init(raster_modes_t *modes, unsigned int num_modes);
extern void raster_modes_shutdown(raster_modes_t *modes);
extern raster_modes_t *raster_modes_new(unsigned int num_modes);
extern void raster_modes_set(raster_modes_t *modes,
                             unsigned int num_mode,
                             raster_modes_fill_cache_function_t fill_cache,
                             raster_modes_draw_line_cached_function_t draw_line_cached,
                             raster_modes_draw_line_function_t draw_line,
                             raster_modes_draw_background_function_t draw_background,
                             raster_modes_draw_foreground_function_t draw_foreground);
extern int raster_modes_set_idle_mode(raster_modes_t *modes,
                                      unsigned int num_mode);

inline static int raster_modes_fill_cache(raster_modes_t *modes,
                                          unsigned int mode_num,
                                          struct raster_cache_s *c,
                                          unsigned int *changed_start,
                                          unsigned int *changed_end,
                                          int no_check)
{
    raster_modes_def_t *mode;

    mode = modes->modes + mode_num;

    return mode->fill_cache(c, changed_start, changed_end, no_check);
}

inline static void raster_modes_draw_line_cached(raster_modes_t *modes,
                                                 unsigned int mode_num,
                                                 struct raster_cache_s *c,
                                                 int start,
                                                 int end)
{
    raster_modes_def_t *mode;

    mode = modes->modes + mode_num;

    mode->draw_line_cached(c, start, end);
}

inline static void raster_modes_draw_line(raster_modes_t *modes,
                                          unsigned int mode_num)
{
    raster_modes_def_t *mode;

    mode = modes->modes + mode_num;

    mode->draw_line();
}

inline static void raster_modes_draw_background(raster_modes_t *modes,
                                                unsigned int mode_num,
                                                unsigned int start_pixel,
                                                unsigned int end_pixel)
{
    raster_modes_def_t *mode;

    mode = modes->modes + mode_num;

    mode->draw_background(start_pixel, end_pixel);
}

inline static void raster_modes_draw_foreground(raster_modes_t *modes,
                                                unsigned int mode_num,
                                                int start_char,
                                                int end_char)
{
    raster_modes_def_t *mode;

    mode = modes->modes + mode_num;

    mode->draw_foreground(start_char, end_char);
}


inline static int raster_modes_get_idle_mode(raster_modes_t *modes)
{
    return modes->idle_mode;
}

#endif /* _RASTER_MODES_H */
