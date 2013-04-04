/*
 * raster-line-changes-sprite.c
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

#include "raster-line.h"
#include "raster-modes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "raster.h"
#include "viewport.h"


inline static void update_cached_sprite_collisions(raster_t *raster)
{
    if (raster->sprite_status != NULL
        && raster->sprite_status->cache_function != NULL) {
        raster->sprite_status->cache_function(
            &(raster->cache[raster->current_line]));
    }
}

inline static int fill_sprite_cache(raster_t *raster, raster_cache_t *cache, unsigned int *xs, unsigned int *xe)
{
    raster_sprite_t *sprite;
    raster_sprite_cache_t *sprite_cache;
    raster_sprite_status_t *sprite_status;
    unsigned int xs_return, xe_return;
    int sxe, sxs, sxe1, sxs1;
    int rr, r, msk;
    unsigned int i;
    unsigned int num_sprites;

    xs_return = raster->geometry->screen_size.width;
    xe_return = 0;

    rr = 0;

    sprite_status = raster->sprite_status;
    num_sprites = sprite_status->num_sprites;

    cache->numsprites = num_sprites;
    cache->sprmask = 0;

    for (msk = 1, i = 0; i < num_sprites; i++, msk <<= 1) {
        sprite = sprite_status->sprites + i;
        sprite_cache = cache->sprites + i;
        r = 0;

        if (sprite_status->dma_msk & msk) {
            DWORD data;

            data = sprite_status->sprite_data[i];

            cache->sprmask |= msk;
            sxe = sprite->x + (sprite->x_expanded ? 48 : 24);
            sxs = sprite->x;

            if (sprite->x != sprite_cache->x) {
                if (sprite_cache->visible) {
                    sxe1 = (sprite_cache->x + (sprite_cache->x_expanded ? 48 : 24));
                    sxs1 = sprite_cache->x;
                    if (sxs1 < sxs) {
                        sxs = sxs1;
                    }
                    if (sxe1 > sxe) {
                        sxe = sxe1;
                    }
                }
                sprite_cache->x = sprite->x;
                r = 1;
            }

            if (!sprite_cache->visible) {
                sprite_cache->visible = 1;
                r = 1;
            }

            if (sprite->x_expanded != sprite_cache->x_expanded) {
                sprite_cache->x_expanded = sprite->x_expanded;
                r = 1;
            }

            if (sprite->multicolor != sprite_cache->multicolor) {
                sprite_cache->multicolor = sprite->multicolor;
                r = 1;
            }

            if (sprite_status->mc_sprite_color_1 != sprite_cache->c1) {
                sprite_cache->c1 = sprite_status->mc_sprite_color_1;
                r = 1;
            }

            if (sprite_status->mc_sprite_color_2 != sprite_cache->c2) {
                sprite_cache->c2 = sprite_status->mc_sprite_color_2;
                r = 1;
            }

            if (sprite->color != sprite_cache->c3) {
                sprite_cache->c3 = sprite->color;
                r = 1;
            }

            if (sprite->in_background != sprite_cache->in_background) {
                sprite_cache->in_background = sprite->in_background;
                r = 1;
            }

            if (sprite_cache->data != data) {
                sprite_cache->data = data;
                r = 1;
            }

            if (r) {
                unsigned int cxs = 0, cxe = 0;

                if (sxs > 0) {
                    cxs = (unsigned int)sxs;
                }
                if (sxe > 0) {
                    cxe = (unsigned int)sxe;
                }

                xs_return = MIN(xs_return, cxs);
                xe_return = MAX(xe_return, cxe);
                rr = 1;
            }
        } else {
            if (sprite_cache->visible) {
                unsigned int cxs = 0, cxe = 0;

                sprite_cache->visible = 0;
                sxe = sprite_cache->x + (sprite_cache->x_expanded ? 24 : 48);

                if (sprite_cache->x > 0) {
                    cxs = sprite_cache->x;
                }
                if (sxe > 0) {
                    cxe = sxe;
                }

                xs_return = MIN(xs_return, cxs);
                xe_return = MAX(xe_return, cxe);
                rr = 1;
            }
        }
    }

    if (xe_return >= raster->geometry->screen_size.width) {
        *xe = raster->geometry->screen_size.width - 1;
    } else {
        *xe = xe_return;
    }

    *xs = xs_return;

    return rr;
}

static void draw_sprites_when_cache_enabled(raster_t *raster,
                                            raster_cache_t *cache)
{
    if (raster->sprite_status == NULL || raster->sprite_status->draw_function == NULL) {
        return;
    }

    raster->sprite_status->draw_function(raster->draw_buffer_ptr,
                                         cache->gfx_msk);
    cache->sprite_sprite_collisions
        = raster->sprite_status->sprite_sprite_collisions;
    cache->sprite_background_collisions
        = raster->sprite_status->sprite_background_collisions;
}

static int update_for_minor_changes_sprite(raster_t *raster,
                                           unsigned int *changed_start,
                                           unsigned int *changed_end)
{
    raster_cache_t *cache;
    unsigned int video_mode;
    unsigned int sprite_changed_start, sprite_changed_end;
    unsigned int changed_start_char, changed_end_char;
    int sprites_need_update;
    int needs_update;

    video_mode = raster_line_get_real_mode(raster);

    cache = &(raster->cache)[raster->current_line];

    changed_start_char = raster->geometry->text_size.width;
    changed_end_char = 0;

    sprites_need_update = fill_sprite_cache(raster, cache,
                                            &sprite_changed_start,
                                            &sprite_changed_end);
    /* If sprites have changed, do not bother trying to reduce the amount
       of recalculated data, but simply redraw everything.  */
    needs_update = raster_modes_fill_cache(raster->modes,
                                           video_mode,
                                           cache,
                                           &changed_start_char,
                                           &changed_end_char,
                                           sprites_need_update);

    if (needs_update) {
        raster_modes_draw_line_cached(raster->modes,
                                      video_mode,
                                      cache,
                                      changed_start_char,
                                      changed_end_char);

        /* Fill the space between the border and the graphics with the
           background color (necessary if xsmooth is > 0).  */
        raster_line_fill_xsmooth_region(raster);

#if 0
        if (raster->sprite_status != NULL) {
#endif
        /* FIXME: Could be optimized better.  */
        draw_sprites_when_cache_enabled(raster, cache);
        raster_line_draw_borders(raster);
#if 0
    } else {
        if (raster->xsmooth > 0) {
            /* If xsmooth > 0, drawing the graphics might have corrupted
               part of the border... fix it here.  */
            if (!raster->open_right_border) {
                raster_line_draw_blank(raster,
                                       raster->geometry->gfx_position.x
                                       + raster->geometry->gfx_size.width,
                                       raster->geometry->gfx_position.x
                                       + raster->geometry->gfx_size.width
                                       + 8);
            }
        }
    }
#endif
        /* Calculate the interval in pixel coordinates.  */
#if 0
        *changed_start = raster->geometry->gfx_position.x;
#else
        *changed_start = raster->geometry->gfx_position.x + raster->xsmooth
                         + 8 * changed_start_char;
#endif
        *changed_end = raster->geometry->gfx_position.x + raster->xsmooth
                       + 8 * (changed_end_char + 1) - 1;

        if (sprites_need_update) {
            /* FIXME: wrong.  */
            if (raster->open_left_border) {
                *changed_start = 0;
            }
            if (raster->open_right_border) {
                *changed_end = raster->geometry->screen_size.width - 1;
            }

            /* Even if we have recalculated the whole line, we will
               refresh only the part that has actually changed when
               writing to the window.  */
            *changed_start = MIN(*changed_start, sprite_changed_start);
            *changed_end = MAX(*changed_end, sprite_changed_end);

            /* The borders have not changed, so do not repaint them even
               if there are sprites under them.  */
            *changed_start = MAX((int)(*changed_start), raster->display_xstart);
            *changed_end = MIN((int)(*changed_end), raster->display_xstop);
        }
    } else {
        update_cached_sprite_collisions(raster);
    }

    if (!sprites_need_update) {
        raster->sprite_status->sprite_sprite_collisions
            = cache->sprite_sprite_collisions;
        raster->sprite_status->sprite_background_collisions
            = cache->sprite_background_collisions;
    }
    return needs_update;
}

void raster_line_changes_sprite_init(raster_t *raster)
{
    raster->line_changes = update_for_minor_changes_sprite;
    raster->draw_sprites_when_cache_enabled = draw_sprites_when_cache_enabled;
    raster->fill_sprite_cache = fill_sprite_cache;
}
