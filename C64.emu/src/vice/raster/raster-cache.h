/*
 * raster-cache.h - Raster line cache.
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

#ifndef VICE_RASTER_CACHE_H
#define VICE_RASTER_CACHE_H

#include <string.h>

#include "raster-sprite-cache.h"
#include "types.h"

/* Yeah, static allocation sucks.  But it's faster, and we are not wasting
   much space anyway.  */
#define RASTER_CACHE_MAX_TEXTCOLS 0x100
#define RASTER_CACHE_MAX_SPRITES  8
#define RASTER_CACHE_GFX_MSK_SIZE 0x100

/* This defines the screen cache.  It includes the sprite cache too.  */
struct raster_cache_s {
    /* Number of line shown (referred to drawable area) */
    int n;

    /* If nonzero, it means that the cache entry is invalid.  */
    int is_dirty;

    /* This is needed in the VIC-II for the area between the end of the left
       border and the start of the graphics, when the X smooth scroll
       register is > 0.  */
    BYTE xsmooth_color;
    BYTE idle_background_color;

    /* X smooth scroll offset.  */
    int xsmooth;

    /* Video mode.  */
    unsigned int video_mode;

    /* Blank mode flag.  */
    int blank;

    /* This defines the borders.  */
    int display_xstart, display_xstop;

    /* Number of columns enabled on this line.  */
    unsigned int numcols;

    /* Number of sprites on this line.  */
    unsigned int numsprites;

    /* Bit mask for the sprites that are visible on this line.  */
    unsigned int sprmask;

    /* Sprite cache.  */
    raster_sprite_cache_t sprites[RASTER_CACHE_MAX_SPRITES];
    BYTE *gfx_msk;

    /* Sprite-sprite and sprite-background collisions that were detected on
       this line.  */
    BYTE sprite_sprite_collisions;
    BYTE sprite_background_collisions;

    /* Character memory pointer.  */
    BYTE *chargen_ptr;

    /* Character row counter.  */
    unsigned int ycounter;

    /* Flags for open left/right borders.  */
    int open_right_border, open_left_border;

    /* Color information.  */
    unsigned int border_color;
    BYTE background_data[RASTER_CACHE_MAX_TEXTCOLS];

    /* Bitmap representation of the graphics in foreground.  */
    BYTE foreground_data[RASTER_CACHE_MAX_TEXTCOLS];

    /* The following are generic and are used differently by the video
       emulators.  */
    BYTE color_data_1[RASTER_CACHE_MAX_TEXTCOLS];
    BYTE color_data_2[RASTER_CACHE_MAX_TEXTCOLS];
    BYTE color_data_3[RASTER_CACHE_MAX_TEXTCOLS];
};
typedef struct raster_cache_s raster_cache_t;

struct raster_sprite_status_s;

extern void raster_cache_new(raster_cache_t *cache,
                             struct raster_sprite_status_s *status);
extern void raster_cache_destroy(raster_cache_t *cache,
                                 struct raster_sprite_status_s *status);
extern void raster_cache_realloc(raster_cache_t **cache,
                                 unsigned int screen_height);

#endif
