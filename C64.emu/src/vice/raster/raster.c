/*
 * raster.c - Raster-based video chip emulation helper.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "raster-cache.h"
#include "raster-canvas.h"
#include "raster-changes.h"
#include "raster-modes.h"
#include "raster-resources.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "raster.h"
#include "screenshot.h"
#include "types.h"
#include "util.h"
#include "video.h"
#include "videoarch.h"
#include "viewport.h"


static int raster_calc_frame_buffer_width(raster_t *raster)
{
    return raster->geometry->screen_size.width
           + raster->geometry->extra_offscreen_border_left
           + raster->geometry->extra_offscreen_border_right;
}

static int raster_draw_buffer_alloc(video_canvas_t *canvas,
                                    unsigned int fb_width,
                                    unsigned int fb_height,
                                    unsigned int *fb_pitch)
{
    if (canvas->video_draw_buffer_callback) {
        return canvas->video_draw_buffer_callback->draw_buffer_alloc(canvas, &canvas->draw_buffer->draw_buffer, fb_width, fb_height, fb_pitch);
    }

    /* FIXME: Allocate one more line to prevent access violations by the
       scale2x render.  */
    canvas->draw_buffer->draw_buffer = lib_malloc(fb_width * (fb_height + 1));
    *fb_pitch = fb_width;
    return 0;
}

static void raster_draw_buffer_free(video_canvas_t *canvas)
{
    if (canvas->video_draw_buffer_callback) {
        canvas->video_draw_buffer_callback->draw_buffer_free(canvas, canvas->draw_buffer->draw_buffer);
        return;
    }

    lib_free(canvas->draw_buffer->draw_buffer);
    canvas->draw_buffer->draw_buffer = NULL;
}

static void raster_draw_buffer_clear(video_canvas_t *canvas, BYTE value,
                                     unsigned int fb_width,
                                     unsigned int fb_height,
                                     unsigned int fb_pitch)
{
    if (canvas->video_draw_buffer_callback) {
        canvas->video_draw_buffer_callback->draw_buffer_clear(canvas, canvas->draw_buffer->draw_buffer, value, fb_width, fb_height, fb_pitch);
        return;
    }

    memset(canvas->draw_buffer->draw_buffer, value, fb_width * fb_height);
}

void raster_draw_buffer_ptr_update(raster_t *raster)
{
    raster->draw_buffer_ptr =
        raster->canvas->draw_buffer->draw_buffer
        /* lines 0+ are displayed in the lower border on NTSC VIC-II */
        + ((raster->current_line < raster->geometry->first_displayed_line
            && raster->geometry->screen_size.height <= raster->geometry->last_displayed_line ?
            raster->geometry->screen_size.height : 0
            ) + raster->current_line
           ) * raster_calc_frame_buffer_width(raster)
        + raster->geometry->extra_offscreen_border_left;
}

static int raster_realize_frame_buffer(raster_t *raster)
{
    unsigned int fb_width, fb_height, fb_pitch;

    raster_draw_buffer_free(raster->canvas);

    fb_width = raster_calc_frame_buffer_width(raster);
    fb_height = raster->geometry->screen_size.height > raster->geometry->last_displayed_line ?
                raster->geometry->screen_size.height
                : raster->geometry->last_displayed_line + 1; /* allocate extra space for the */
    /* lower part of the visible lower border (lines 0+) on NTSC VIC-II */

    if (fb_width > 0 && fb_height > 0) {
        if (raster_draw_buffer_alloc(raster->canvas, fb_width, fb_height, &fb_pitch)) {
            return -1;
        }

        raster->canvas->draw_buffer->draw_buffer_width = fb_width;
        raster->canvas->draw_buffer->draw_buffer_height = fb_height;
        raster->canvas->draw_buffer->draw_buffer_pitch = fb_pitch;

        raster_draw_buffer_clear(raster->canvas, 0, fb_width, fb_height,
                                 fb_pitch);
    }

    raster->fake_draw_buffer_line = lib_realloc(raster->fake_draw_buffer_line,
                                                fb_width);

    memset(raster->fake_draw_buffer_line, 0, fb_width);

    return 0;
}

/* called from raster_realize() */
static int realize_canvas(raster_t *raster)
{
    video_canvas_t *new_canvas;

    raster->intialized = 1;

    if (!video_disabled_mode) {
        new_canvas = video_canvas_create(raster->canvas,
                                         &raster->canvas->draw_buffer->canvas_width,
                                         &raster->canvas->draw_buffer->canvas_height, 1);

        if (new_canvas == NULL) {
            return -1;
        }

        raster->canvas = new_canvas;

#if defined(USE_SDLUI) || defined(USE_SDLUI2)
        /* A hack to allow raster_force_repaint() calls for SDL UI & vkbd */
        raster->canvas->parent_raster = raster;
#endif

        video_canvas_create_set(raster->canvas);
    }

    if (raster_realize_frame_buffer(raster) < 0) {
        return -1;
    }

    /* The canvas might give us something different from what we
       requested. FIXME: Only do this if really something changed. */
    video_viewport_resize(raster->canvas, 1);
    return 0;
}

static int perform_mode_change(raster_t *raster)
{
    if (!video_disabled_mode
        && raster->canvas && raster->canvas->palette != NULL) {
        if (video_canvas_set_palette(raster->canvas, raster->canvas->palette) < 0) {
            return -1;
        }
    }
    raster_force_repaint(raster);

    video_viewport_resize(raster->canvas, 1);

    return 0;
}

/* FIXME: dead code */
#if 0
inline static void draw_blank(raster_t *raster,
                              unsigned int start,
                              unsigned int end)
{
    memset(raster->draw_buffer_ptr + start,
           raster->border_color, end - start + 1);
}

/* Draw the borders.  */
inline static void draw_borders(raster_t *raster)
{
    if (!raster->open_left_border) {
        draw_blank(raster, 0, raster->display_xstart - 1);
    }
    if (!raster->open_right_border) {
        draw_blank(raster,
                   raster->display_xstop,
                   raster->geometry->screen_size.width - 1);
    }
}
#endif

int raster_init(raster_t *raster,
                unsigned int num_modes)
{
    raster->intialized = 0;

    raster->modes = lib_malloc(sizeof(raster_modes_t));
    raster_modes_init(raster->modes, num_modes);
    raster_canvas_init(raster);
    raster_changes_init(raster);

    raster_reset(raster);

    raster->display_xstart = raster->display_xstop = 0;
    raster->display_ystart = raster->display_ystop = 0;

    raster->cache = NULL;
    raster->cache_enabled = 0;
    raster->dont_cache = 1;
    raster->dont_cache_all = 0;
    raster->num_cached_lines = 0;

    raster->fake_draw_buffer_line = NULL;

    raster->can_disable_border = 0;
    raster->border_disable = 0;

    raster->border_color = 0;
    raster->background_color = 0;
    raster->idle_background_color = 0;
    raster->xsmooth_color = 0;

    memset(raster->gfx_msk, 0, RASTER_GFX_MSK_SIZE);
    memset(raster->zero_gfx_msk, 0, RASTER_GFX_MSK_SIZE);

    video_viewport_get(raster->canvas, &raster->viewport, &raster->geometry);

    raster->canvas->initialized = 1;
    raster_set_canvas_refresh(raster, 1);

    return 0;
}

void raster_reset(raster_t *raster)
{
    raster_changes_remove_all(raster->changes->background);
    raster_changes_remove_all(raster->changes->foreground);
    raster_changes_remove_all(raster->changes->border);
    raster_changes_remove_all(raster->changes->sprites);
    raster_changes_remove_all(raster->changes->next_line);
    raster->changes->have_on_this_line = 0;

    raster->current_line = 0;

    raster->xsmooth = raster->ysmooth = 0;
    raster->sprite_xsmooth = 0;
    raster->xsmooth_shift_left = 0;
    raster->xsmooth_shift_right = 0;
    raster->sprite_xsmooth_shift_right = 0;
    raster->skip_frame = 0;

    raster->blank_off = 0;
    raster->blank_enabled = 0;
    raster->blank_this_line = 0;
    raster->open_right_border = 0;
    raster->open_left_border = 0;
    raster->border_disable = 0;
    raster->blank = 0;

    raster->draw_idle_state = 0;
    raster->ycounter = 0;
    raster->video_mode = 0;
    raster->last_video_mode = -1;
}

typedef struct raster_list_t {
    raster_t *raster;
    struct raster_list_t *next;
} raster_list_t;

static raster_list_t *ActiveRasters = NULL;

/* seemingly dead code */
#if 0
raster_t *raster_new(unsigned int num_modes,
                     unsigned int num_sprites)
{
    raster_t *new;

    new = lib_malloc(sizeof(raster_t));
    raster_init(new, num_modes, num_sprites);

    return new;
}
#endif

void raster_mode_change(void)
{
    raster_list_t *rasters = ActiveRasters;

    while (rasters != NULL) {
        perform_mode_change(rasters->raster);
        rasters = rasters->next;
    }
}


void raster_new_cache(raster_t *raster, unsigned int screen_height)
{
    unsigned int i;

    for (i = 0; i < screen_height; i++) {
        raster_cache_new(&(raster->cache)[i], raster->sprite_status);
    }
}

static void raster_destroy_cache(raster_t *raster, unsigned int screen_height)
{
    unsigned int i;

    if (raster->cache == NULL) {
        return;
    }

    for (i = 0; i < screen_height; i++) {
        raster_cache_destroy(&(raster->cache)[i], raster->sprite_status);
    }
}

void raster_set_geometry(raster_t *raster,
                         unsigned int canvas_width, unsigned int canvas_height,
                         unsigned int screen_width, unsigned int screen_height,
                         unsigned int gfx_width, unsigned int gfx_height,
                         unsigned int text_width, unsigned int text_height,
                         unsigned int gfx_position_x,
                         unsigned int gfx_position_y,
                         int gfx_area_moves,
                         unsigned int first_displayed_line,
                         unsigned int last_displayed_line,
                         unsigned int extra_offscreen_border_left,
                         unsigned int extra_offscreen_border_right)
{
    geometry_t *geometry;

    geometry = raster->geometry;
    if (screen_height != geometry->screen_size.height
        || raster->cache == NULL) {
        raster_destroy_cache(raster, geometry->screen_size.height);
        raster_cache_realloc(&(raster->cache), screen_height);
        raster_new_cache(raster, screen_height);
    }

    geometry->first_displayed_line = first_displayed_line;
    geometry->last_displayed_line = last_displayed_line;

    if (geometry->screen_size.width != screen_width
        || geometry->screen_size.height != screen_height
        || geometry->extra_offscreen_border_left != extra_offscreen_border_left
        || geometry->extra_offscreen_border_right
        != extra_offscreen_border_right) {
        geometry->screen_size.width = screen_width;
        geometry->screen_size.height = screen_height;
        geometry->extra_offscreen_border_left = extra_offscreen_border_left;
        geometry->extra_offscreen_border_right = extra_offscreen_border_right;
        raster_realize_frame_buffer(raster);
    }

    geometry->gfx_size.width = gfx_width;
    geometry->gfx_size.height = gfx_height;
    geometry->text_size.width = text_width;
    geometry->text_size.height = text_height;
    if (!geometry->char_pixel_width) { /* Default to 8 if not defined */
        geometry->char_pixel_width = 8;
    }

    geometry->gfx_position.x = gfx_position_x;
    geometry->gfx_position.y = gfx_position_y;

    geometry->gfx_area_moves = gfx_area_moves;

    raster->canvas->draw_buffer->visible_width = canvas_width;
    raster->canvas->draw_buffer->visible_height = canvas_height;
}

static int raster_realize_init_done = 0;

/* called from init_raster() in the videochip code */
int raster_realize(raster_t *raster)
{
    raster_list_t *rlist;

    if (realize_canvas(raster) < 0) {
        return -1;
    }

    if (raster_realize_init_done == 0) {
        ActiveRasters = NULL;
    }
    raster_realize_init_done++;

    video_canvas_refresh_all(raster->canvas);

    rlist = lib_malloc(sizeof(raster_list_t));
    rlist->raster = raster;
    rlist->next = NULL;

    if (ActiveRasters == NULL) {
        ActiveRasters = rlist;
    } else {
        raster_list_t *rasters = ActiveRasters;

        while (rasters->next != NULL) {
            rasters = rasters->next;
        }
        rasters->next = rlist;
    }

    return 0;
}

static void raster_destroy_raster(raster_t *raster)
{
    raster_list_t *rlist, *tmplist;

    rlist = ActiveRasters;
    tmplist = NULL;

    while (rlist != NULL && rlist->raster != raster) {
        tmplist = rlist;
        rlist = rlist->next;
    }

    if (rlist != NULL) {
        if (tmplist == NULL) {
            ActiveRasters = rlist->next;
        } else {
            tmplist->next = rlist->next;
        }
        lib_free(rlist);
    }

    if (raster_realize_init_done > 0) {
        raster_realize_init_done--;
        if (raster_realize_init_done == 0) {
            ActiveRasters = NULL;
        }
    }
}

void raster_force_repaint(raster_t *raster)
{
    raster->dont_cache = 1;
    raster->num_cached_lines = 0;
}

void raster_set_title(raster_t *raster, const char *name)
{
    char *title;

    title = util_concat("VICE: ", name, " emulator", NULL);
    video_viewport_title_set(raster->canvas, title);

    lib_free(title);
}

void raster_skip_frame(raster_t *raster, int skip)
{
    raster->skip_frame = skip;
}

void raster_enable_cache(raster_t *raster, int enable)
{
    raster->cache_enabled = enable;
    raster_force_repaint(raster);
}

void raster_set_canvas_refresh(raster_t *raster, int enable)
{
    raster->canvas->viewport->update_canvas = enable;
}

void raster_screenshot(raster_t *raster, screenshot_t *screenshot)
{
    screenshot->palette = raster->canvas->palette;
    screenshot->max_width = raster->geometry->screen_size.width;
    screenshot->max_height = raster->geometry->screen_size.height;
    screenshot->x_offset = raster->geometry->extra_offscreen_border_left;
    screenshot->size_width = 1;
    screenshot->size_height = 1;
    screenshot->dpi_x = 100;
    screenshot->dpi_y = 100;
    screenshot->first_displayed_line = raster->geometry->first_displayed_line;
    screenshot->last_displayed_line = raster->geometry->last_displayed_line;
    screenshot->first_displayed_col
        = raster->geometry->extra_offscreen_border_left
          + raster->canvas->viewport->first_x;
    screenshot->draw_buffer = raster->canvas->draw_buffer->draw_buffer;
    screenshot->draw_buffer_line_size
        = raster->canvas->draw_buffer->draw_buffer_width;
}

void raster_async_refresh(raster_t *raster, struct canvas_refresh_s *ref)
{
    ref->draw_buffer = raster->canvas->draw_buffer->draw_buffer;
    ref->draw_buffer_line_size
        = raster->canvas->draw_buffer->draw_buffer_width;
#ifdef __OS2__
    ref->bufh = raster->canvas->draw_buffer->draw_buffer_height;
#endif
    ref->x = raster->geometry->extra_offscreen_border_left
             + raster->canvas->viewport->first_x;
    ref->y = raster->geometry->first_displayed_line;

    ref->x *= raster->canvas->videoconfig->scalex;
    ref->y *= raster->canvas->videoconfig->scaley;
}

void raster_shutdown(raster_t *raster)
{
    if (raster->canvas) {
        raster_draw_buffer_free(raster->canvas);
    }

    if (raster->cache) {
        raster_destroy_cache(raster, raster->geometry->screen_size.height);
        lib_free(raster->cache);
    }

    if (raster->modes) {
        raster_modes_shutdown(raster->modes);
        lib_free(raster->modes);
    }

    raster_changes_shutdown(raster);

    lib_free(raster->fake_draw_buffer_line);
    raster_canvas_shutdown(raster);


    video_color_palette_free(raster->canvas->palette);
    video_canvas_destroy(raster->canvas);
    raster_resources_chip_shutdown(raster);
    raster_destroy_raster(raster);
}
