/*
 * raster-line.c - Raster-based video chip emulation helper.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * DTV sections written by
 *  Daniel Kahlin <daniel@kahlin.net>
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
#include <string.h>

#include "raster-cache.h"
#include "raster-canvas.h"
#include "raster-changes.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "raster.h"
#include "viewport.h"


unsigned int raster_line_get_real_mode(raster_t *raster)
{
    if (raster->draw_idle_state) {
        return raster_modes_get_idle_mode(raster->modes);
    } else {
        return raster->video_mode;
    }
}

/* Increase `area' so that it also includes [xs; xe] at line y.  */
inline static void add_line_to_area(raster_canvas_area_t *area, unsigned int y,
                                    unsigned int xs, unsigned int xe)
{
    if (area->is_null) {
        area->ys = area->ye = y;
        area->xs = xs;
        area->xe = xe;
        area->is_null = 0;
    } else {
        area->xs = MIN(xs, area->xs);
        area->xe = MAX(xe, area->xe);
        area->ys = MIN(y, area->ys);
        area->ye = MAX(y, area->ye);
    }
}

inline void raster_line_draw_blank(raster_t *raster, unsigned int start,
                                   unsigned int end)
{
    memset(raster->draw_buffer_ptr + start,
           raster->border_color, end - start + 1);
}

/* This kludge updates the sprite-sprite collisions without writing to the
   real frame buffer.  We might write a function that actually checks for
   collisions only, but we are lazy.  */
inline static void update_sprite_collisions(raster_t *raster)
{
    BYTE *fake_draw_buffer_ptr;

    if (raster->sprite_status == NULL || raster->sprite_status->draw_function == NULL) {
        return;
    }

    fake_draw_buffer_ptr = raster->fake_draw_buffer_line
                           + raster->geometry->extra_offscreen_border_left;

    raster->sprite_status->draw_function(fake_draw_buffer_ptr,
                                         raster->zero_gfx_msk);
}

/* map the current line so that lines 0+ in the lower border on NTSC */
/* VIC-II are put correctly to the lower frame buffer area */
inline static int map_current_line_to_area(raster_t *raster)
{
    return (raster->current_line < raster->geometry->first_displayed_line
            && raster->geometry->screen_size.height <= raster->geometry->last_displayed_line ?
            raster->geometry->screen_size.height + raster->current_line
            : raster->current_line);
}

inline static void handle_blank_line_cached(raster_t *raster)
{
    if (raster->dont_cache
        || raster->cache[raster->current_line].is_dirty
        || raster->border_color
        != raster->cache[raster->current_line].border_color
        || !raster->cache[raster->current_line].blank) {
        /* Even when the actual caching is disabled, redraw blank lines
           only if it is really necessary to do so.  */
        raster->cache[raster->current_line].border_color
            = raster->border_color;
        raster->cache[raster->current_line].blank = 1;
        raster->cache[raster->current_line].is_dirty = 0;

        raster_line_draw_blank(raster, 0,
                               raster->geometry->screen_size.width - 1);
        add_line_to_area(raster->update_area,
                         map_current_line_to_area(raster),
                         0, raster->geometry->screen_size.width - 1);
    }
}

static void handle_blank_line(raster_t *raster)
{
    if (raster->changes->have_on_this_line) {
        raster_changes_t *border_changes;
        unsigned int i, xs;

        raster_changes_apply_all(raster->changes->background);
        raster_changes_apply_all(raster->changes->foreground);
        raster_changes_apply_all(raster->changes->sprites);

        border_changes = raster->changes->border;

        /* Only draw a blank line if there are border changes. If there
           are no border changes just check for dirty cache.  */
        if (border_changes->count > 0) {
            for (xs = i = 0; i < border_changes->count; i++) {
                unsigned int xe;

                xe = border_changes->actions[i].where;

                if (xs < xe) {
                    raster_line_draw_blank(raster, xs, xe);
                    xs = xe;
                }

                raster_changes_apply(border_changes, i);
            }

            if (xs < raster->geometry->screen_size.width - 1) {
                raster_line_draw_blank(raster, xs, raster->geometry->screen_size.width - 1);
            }

            raster->cache[raster->current_line].border_color = 0xFF;
            raster->cache[raster->current_line].blank = 1;

            raster_changes_remove_all(border_changes);

            add_line_to_area(raster->update_area, map_current_line_to_area(raster),
                             0, raster->geometry->screen_size.width - 1);
        } else {
            handle_blank_line_cached(raster);
        }
        raster->changes->have_on_this_line = 0;
    } else {
        handle_blank_line_cached(raster);
    }

    update_sprite_collisions(raster);
}

inline static void draw_sprites(raster_t *raster)
{
    if (raster->sprite_status != NULL
        && raster->sprite_status->draw_function != NULL) {
        raster->sprite_status->draw_function(raster->draw_buffer_ptr,
                                             raster->gfx_msk);
    }
}

inline static void draw_sprites_partial(raster_t *raster, int xs, int xe)
{
    if (raster->sprite_status != NULL
        && raster->sprite_status->draw_partial_function != NULL) {
        if (raster->sprite_xsmooth_shift_right > 0) {
            raster->sprite_status->draw_partial_function(raster->draw_buffer_ptr,
                                                         raster->zero_gfx_msk, xs, xe);
        } else {
            raster->sprite_status->draw_partial_function(raster->draw_buffer_ptr,
                                                         raster->gfx_msk, xs, xe);
        }
        raster->sprite_xsmooth_shift_right = 0;
    }
}

void raster_line_draw_borders(raster_t *raster)
{
    if (!raster->border_disable) {
        if (!raster->open_left_border) {
            raster_line_draw_blank(raster, 0, raster->display_xstart - 1);
        }
        if (!raster->open_right_border) {
            raster_line_draw_blank(raster, raster->display_xstop,
                                   raster->geometry->screen_size.width - 1);
        }
    }
}

void raster_line_fill_xsmooth_region(raster_t *raster)
{
    if (raster->xsmooth != 0) {
        memset(raster->draw_buffer_ptr + raster->geometry->gfx_position.x,
               raster->xsmooth_color, raster->xsmooth);
    }
}

inline static int update_for_minor_changes(raster_t *raster,
                                           unsigned int *changed_start,
                                           unsigned int *changed_end)
{
    return raster->line_changes(raster, changed_start, changed_end);
}

inline static void fill_background(raster_t *raster)
{
    raster_line_fill_xsmooth_region(raster);

    if (raster->open_left_border || raster->border_disable) {
        if (raster->draw_idle_state) {
            memset(raster->draw_buffer_ptr, raster->idle_background_color,
                   (raster->geometry->gfx_position.x + raster->xsmooth));
        } else {
            memset(raster->draw_buffer_ptr, raster->xsmooth_color,
                   (raster->geometry->gfx_position.x + raster->xsmooth));
        }
    }

    if (raster->open_right_border || raster->border_disable) {
        if (!raster->can_disable_border) {
            if (raster->draw_idle_state) {
                memset(raster->draw_buffer_ptr +
                       raster->geometry->gfx_position.x
                       + raster->geometry->gfx_size.width
                       + raster->xsmooth,
                       raster->idle_background_color,
                       raster->geometry->screen_size.width
                       - raster->geometry->gfx_position.x
                       - raster->geometry->gfx_size.width
                       - raster->xsmooth);
            } else {
                memset(raster->draw_buffer_ptr +
                       raster->geometry->gfx_position.x
                       + raster->geometry->gfx_size.width
                       + raster->xsmooth,
                       raster->xsmooth_color,
                       raster->geometry->screen_size.width
                       - raster->geometry->gfx_position.x
                       - raster->geometry->gfx_size.width
                       - raster->xsmooth);
            }
        } else {
            int len = raster->geometry->screen_size.width
                      - raster->geometry->gfx_position.x
                      - raster->geometry->gfx_size.width
                      - raster->xsmooth;
            if (len > 0) {
                if (raster->draw_idle_state) {
                    memset(raster->draw_buffer_ptr +
                           raster->geometry->gfx_position.x
                           + raster->geometry->gfx_size.width
                           + raster->xsmooth,
                           raster->idle_background_color,
                           len);
                } else {
                    memset(raster->draw_buffer_ptr +
                           raster->geometry->gfx_position.x
                           + raster->geometry->gfx_size.width
                           + raster->xsmooth,
                           raster->xsmooth_color,
                           len);
                }
            }
        }
    }
}

static void handle_visible_line_with_cache(raster_t *raster)
{
    int needs_update;
    unsigned int changed_start, changed_end;
    raster_cache_t *cache;

    cache = &raster->cache[raster->current_line];

    /* Check for "major" changes first.  If there is any, just write straight
       to the cache without any comparisons and redraw the whole line.  */
    /* check_for_major_changes_and_update() is embedded here because of some
       VAC++ bug.  */
    {
        unsigned int video_mode;
        int line;

        video_mode = raster_line_get_real_mode(raster);

        line = raster->current_line - raster->geometry->gfx_position.y - raster->ysmooth - 1;

        if (cache->is_dirty
            || raster->dont_cache || raster->dont_cache_all
            || cache->n != line
            || cache->xsmooth != raster->xsmooth
            || cache->video_mode != video_mode
            || cache->blank
            || cache->ycounter != raster->ycounter
            || cache->border_color != raster->border_color
            || cache->display_xstart != raster->display_xstart
            || cache->display_xstop != raster->display_xstop
            || (cache->open_right_border && !raster->open_right_border)
            || (cache->open_left_border && !raster->open_left_border)
            || cache->xsmooth_color != raster->xsmooth_color
            || cache->idle_background_color
            != raster->idle_background_color) {
            unsigned int changed_start_char, changed_end_char;

            cache->n = line;
            cache->xsmooth = raster->xsmooth;
            cache->video_mode = video_mode;
            cache->blank = 0;
            cache->ycounter = raster->ycounter;
            cache->border_color = raster->border_color;
            cache->display_xstart = raster->display_xstart;
            cache->display_xstop = raster->display_xstop;
            cache->open_right_border = raster->open_right_border;
            cache->open_left_border = raster->open_left_border;
            cache->xsmooth_color = raster->xsmooth_color;
            cache->idle_background_color = raster->idle_background_color;

            /* Fill the space between the border and the graphics with the
             background color (necessary if `xsmooth' is != 0).  */

            fill_background(raster);

            if (raster->sprite_status != NULL) {
                (raster->fill_sprite_cache)(raster, cache,
                                            &changed_start_char,
                                            &changed_end_char);
            }

            raster_modes_fill_cache(raster->modes,
                                    video_mode,
                                    cache,
                                    &changed_start_char,
                                    &changed_end_char, 1);

            /* [ `changed_start' ; `changed_end' ] now covers the whole line, as
             we have called fill_cache() with `1' as the last parameter (no
             check).  */
            raster_modes_draw_line_cached(raster->modes, video_mode,
                                          cache,
                                          changed_start_char,
                                          changed_end_char);

            if (raster->sprite_status != NULL) {
                (raster->draw_sprites_when_cache_enabled)(raster, cache);
            }

            changed_start = 0;
            changed_end = raster->geometry->screen_size.width - 1;

            raster_line_draw_borders(raster);

            needs_update = 1;
        } else {
            needs_update = 0;
        }
    }

    if (!needs_update) {
        /* There are no `major' changes: try to do some optimization.  */
        needs_update = update_for_minor_changes(raster,
                                                &changed_start,
                                                &changed_end);
    }

    if (needs_update) {
        add_line_to_area(raster->update_area, map_current_line_to_area(raster),
                         changed_start, changed_end);
    }

    cache->is_dirty = 0;
}

static void handle_visible_line_without_cache(raster_t *raster)
{
    geometry_t *geometry;
    raster_cache_t *cache;

    geometry = raster->geometry;

    /* If screen is scrolled to the right, we need to fill with the
       background color the blank part on the left.  */

    fill_background(raster);

    /* Draw the graphics and sprites.  */
    raster_modes_draw_line(raster->modes, raster_line_get_real_mode(raster));
    draw_sprites(raster);
    raster_line_draw_borders(raster);

    cache = &raster->cache[raster->current_line];

    if (raster->dont_cache || raster->dont_cache_all
        || (raster->sprite_status != NULL && raster->sprite_status->dma_msk != 0)
        || cache->is_dirty
        || cache->blank
        || cache->border_color != raster->border_color
        /* FIXME: Done differently in another place.  */
        || cache->open_right_border != raster->open_right_border
        || cache->open_left_border != raster->open_left_border
        || cache->idle_background_color != raster->idle_background_color
        || cache->xsmooth_color != raster->xsmooth_color) {
        cache->blank = 0;
        cache->is_dirty = 0;
        cache->border_color = raster->border_color;
        cache->open_right_border = raster->open_right_border;
        cache->open_left_border = raster->open_left_border;
        cache->xsmooth_color = raster->xsmooth_color;
        cache->idle_background_color = raster->idle_background_color;

        add_line_to_area(raster->update_area, map_current_line_to_area(raster),
                         0, raster->geometry->screen_size.width - 1);
    } else {
        /* Still do some minimal caching anyway.  */
        /* Only update the part between the borders.  */
        add_line_to_area(raster->update_area, map_current_line_to_area(raster),
                         geometry->gfx_position.x,
                         geometry->gfx_position.x
                         + geometry->gfx_size.width - 1);
    }
}

static void handle_visible_line_with_changes(raster_t *raster)
{
    unsigned int i;
    int xs, xstop, old_draw_idle_state, old_video_mode;
    geometry_t *geometry;
    raster_changes_all_t *changes;

    geometry = raster->geometry;
    changes = raster->changes;

    /* Idle state is changed in both background and foreground. As background
       changes may change the value, save the original value and restore it
       later before processing the foreground changes.  */
    old_draw_idle_state = raster->draw_idle_state;
    old_video_mode = raster->video_mode;

    /* Draw the background.  */
    for (xs = i = 0; i < changes->background->count; i++) {
        int xe = changes->background->actions[i].where;

        if (xs < xe) {
            raster_modes_draw_background(raster->modes,
                                         raster_line_get_real_mode(raster),
                                         xs,
                                         xe - 1);
            xs = xe;
        }
        raster_changes_apply(changes->background, i);
    }
    if (xs <= (int)geometry->screen_size.width - 1) {
        raster_modes_draw_background(raster->modes,
                                     raster_line_get_real_mode(raster),
                                     xs,
                                     geometry->screen_size.width - 1);
    }

    raster->draw_idle_state = old_draw_idle_state;
    raster->video_mode = old_video_mode;

    /* Draw the foreground graphics.  */
    for (xs = i = 0; i < changes->foreground->count; i++) {
        int xe = changes->foreground->actions[i].where;

        if (xs < xe) {
            raster_modes_draw_foreground(raster->modes,
                                         raster_line_get_real_mode(raster),
                                         xs,
                                         xe - 1);
            xs = xe;
        }
        raster->xsmooth_shift_left = 0;
        raster_changes_apply(changes->foreground, i);
    }
    if (xs <= (int)geometry->text_size.width - 1) {
        raster_modes_draw_foreground(raster->modes,
                                     raster_line_get_real_mode(raster),
                                     xs,
                                     geometry->text_size.width - 1);
    }

    raster->xsmooth_shift_left = 0;

/*
    Draw the sprites.

    NOTE: make sure not to draw more than the actually visible part of the line,
    because also only that part will get overdrawn by the border color and
    excessive pixels will show up as artefacts in renderers which rely on the
    offscreen area properly being updated (such as Scale2x and CRT emulation).
*/
#if 0
    draw_sprites(raster);
#else
    xs = 0;
    for (i = 0; i < changes->sprites->count; i++) {
        int xe = changes->sprites->actions[i].where;

        if (xe >= (int)geometry->screen_size.width) {
            xe = geometry->screen_size.width - 1;
        }
        if (xs < xe) {
            draw_sprites_partial(raster, xs, xe - 1);
            xs = xe;
        }
        raster_changes_apply(changes->sprites, i);
    }
    if (xs <= (int)(geometry->screen_size.width - 1)) {
        draw_sprites_partial(raster, xs, geometry->screen_size.width - 1);
    }
#endif

    /* If this is really a blanked line, draw border over all of the line
       considering border changes */
    if (raster->can_disable_border && ((raster->blank_this_line || raster->blank_enabled) && !raster->open_left_border)) {
        for (xs = i = 0; i < changes->border->count; i++) {
            int xe = changes->border->actions[i].where;

            if (xs < xe) {
                if (!raster->border_disable) {
                    raster_line_draw_blank(raster, xs, xe - 1);
                }
                xs = xe;
            }
            raster_changes_apply(changes->border, i);
        }
        if (!raster->border_disable) {
            if (xs <= (int)geometry->screen_size.width - 1) {
                raster_line_draw_blank(raster, xs, geometry->screen_size.width - 1);
            }
        }
    } else {
        /* Draw left border.  */
        xstop = raster->display_xstart - 1;
        if (!raster->open_left_border) {
            for (xs = i = 0;
                 (i < changes->border->count && changes->border->actions[i].where <= xstop);
                 i++) {
                int xe = changes->border->actions[i].where;

                if (xs < xe) {
                    if (!raster->border_disable) {
                        raster_line_draw_blank(raster, xs, xe - 1);
                    }
                    xs = xe;
                }
                raster_changes_apply(changes->border, i);
            }
            if ((!raster->border_disable) && (xs <= xstop)) {
                raster_line_draw_blank(raster, xs, xstop);
            }
        } else {
            for (i = 0;
                 (i < changes->border->count && changes->border->actions[i].where <= xstop);
                 i++) {
                raster_changes_apply(changes->border, i);
            }
        }

        /* Draw right border.  */
        if (!raster->open_right_border) {
            for (;
                 (i < changes->border->count && (changes->border->actions[i].where <= raster->display_xstop));
                 i++) {
                raster_changes_apply(changes->border, i);
            }
            for (xs = raster->display_xstop;
                 i < changes->border->count;
                 i++) {
                int xe = changes->border->actions[i].where;

                if (xs < xe) {
                    if (!raster->border_disable) {
                        raster_line_draw_blank(raster, xs, xe - 1);
                    }
                    xs = xe;
                }
                raster_changes_apply(changes->border, i);
            }
            if (!raster->border_disable) {
                if (xs <= (int)geometry->screen_size.width - 1) {
                    raster_line_draw_blank(raster, xs, geometry->screen_size.width - 1);
                }
            }
        } else {
            for (i = 0; i < changes->border->count; i++) {
                raster_changes_apply(changes->border, i);
            }
        }
    }

    raster_changes_remove_all(changes->foreground);
    raster_changes_remove_all(changes->background);
    raster_changes_remove_all(changes->border);
    raster_changes_remove_all(changes->sprites);
    raster->changes->have_on_this_line = 0;

    /* Do not cache this line at all.  */
    raster->cache[raster->current_line].is_dirty = 1;

    add_line_to_area(raster->update_area, map_current_line_to_area(raster),
                     0, raster->geometry->screen_size.width - 1);
}

inline static void handle_visible_line(raster_t *raster)
{
    if (raster->changes->have_on_this_line) {
        handle_visible_line_with_changes(raster);
    } else {
        if (raster->cache_enabled
            && !raster->open_left_border
            && !raster->open_right_border) {     /* FIXME: shortcut! */
            handle_visible_line_with_cache(raster);
        } else {
            handle_visible_line_without_cache(raster);
        }
    }

    if (raster->draw_idle_state) {
        raster->xsmooth_color = raster->idle_background_color;
    }
}

void raster_line_emulate(raster_t *raster)
{
    raster_draw_buffer_ptr_update(raster);

    /* Emulate the vertical blank flip-flops.  (Well, sort of.)  */
    if (raster->current_line == raster->display_ystart && (!raster->blank || raster->blank_off)) {
        raster->blank_enabled = 0;
    }
    if (raster->current_line == raster->display_ystop) {
        raster->blank_enabled = 1;
    }

    if ((raster->current_line >= raster->geometry->first_displayed_line
         && raster->current_line <= raster->geometry->last_displayed_line)
        /* handle the case when lines 0+ are displayed in the lower border */
        || (raster->current_line <= raster->geometry->last_displayed_line - raster->geometry->screen_size.height
            && raster->geometry->screen_size.height <= raster->geometry->last_displayed_line)
        ) {
        /* handle lines with no border or with changes that may affect
           the border as visible lines */
        if (raster->can_disable_border && (raster->border_disable || raster->changes->have_on_this_line)) {
            handle_visible_line(raster);
        } else {
            if ((raster->blank_this_line || raster->blank_enabled)
                && !raster->open_left_border) {
                handle_blank_line(raster);
            } else {
                handle_visible_line(raster);
            }
        }

        if (++raster->num_cached_lines == (1
                                           + raster->geometry->last_displayed_line
                                           - raster->geometry->first_displayed_line)) {
            raster->dont_cache = 0;
            raster->num_cached_lines = 0;
        }

#if 0
        /* this is a fix for the pal emulation bug at the left/right edges */
        /* hacked, but other solutions would cause changes in many places in
           the code */
        memset(raster->draw_buffer_ptr - 4,
               *raster->draw_buffer_ptr, 4);
        memset(raster->draw_buffer_ptr + raster->geometry->screen_size.width,
               *raster->draw_buffer_ptr, 4);
#endif
    } else {
        update_sprite_collisions(raster);

        if (raster->changes->have_on_this_line) {
            raster_changes_apply_all(raster->changes->background);
            raster_changes_apply_all(raster->changes->foreground);
            raster_changes_apply_all(raster->changes->border);
            raster_changes_apply_all(raster->changes->sprites);
            raster->changes->have_on_this_line = 0;
        }
    }

    raster->current_line++;

    if (raster->current_line == raster->geometry->screen_size.height) {
        raster->current_line = 0;
        /* not end of frame on NTSC VIC-II where lines 0+ are */
        /* displayed in the lower border */
        if (raster->geometry->screen_size.height > raster->geometry->last_displayed_line) {
            raster_canvas_handle_end_of_frame(raster);
        }
    }

    /* end of frame on NTSC VIC-II */
    if (raster->geometry->screen_size.height <= raster->geometry->last_displayed_line
        && raster->current_line == raster->geometry->last_displayed_line - raster->geometry->screen_size.height + 1) {
        raster_canvas_handle_end_of_frame(raster);
    }

    raster_changes_apply_all(raster->changes->next_line);

    /* Handle open borders.  */
    raster->open_left_border = raster->open_right_border;
    raster->open_right_border = 0;

    if (raster->sprite_status != NULL) {
        raster->sprite_status->dma_msk = raster->sprite_status->new_dma_msk;
    }

    raster->blank_this_line = 0;
}
