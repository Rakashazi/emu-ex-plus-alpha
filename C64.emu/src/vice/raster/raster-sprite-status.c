/*
 * raster-sprite-status.c - Sprite status handling for the raster emulation.
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
#include <string.h>

#include "lib.h"
#include "raster-sprite-cache.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "raster.h"
#include "types.h"


static void raster_sprite_status_init(raster_sprite_status_t *status,
                                      unsigned int num_sprites, int sprite_offset)
{
    status->num_sprites = num_sprites;
    status->cache_init_func = raster_sprite_cache_init;

    status->draw_function = NULL;
    status->draw_partial_function = NULL;
    status->cache_function = NULL;

    if (num_sprites > 0) {
        status->sprites = lib_malloc(sizeof(*status->sprites) * num_sprites);
        status->sprite_data_1 = lib_malloc(sizeof(DWORD) * num_sprites);
        status->sprite_data_2 = lib_malloc(sizeof(DWORD) * num_sprites);
    } else {
        status->sprites = NULL;
        status->sprite_data_1 = NULL;
        status->sprite_data_2 = NULL;
    }

    raster_sprite_status_reset(status, sprite_offset);
}

static void raster_sprite_status_shutdown(raster_sprite_status_t *status,
                                          unsigned int num_sprites)
{
    if (num_sprites > 0) {
        lib_free(status->sprites);
        lib_free(status->sprite_data_1);
        lib_free(status->sprite_data_2);
    }
}

void raster_sprite_status_new(raster_t *raster, unsigned int num_sprites, int sprite_offset)
{
    raster->sprite_status = lib_malloc(sizeof(raster_sprite_status_t));
    raster_sprite_status_init(raster->sprite_status, num_sprites, sprite_offset);
}

void raster_sprite_status_destroy(raster_t *raster)
{
    if (raster->sprite_status) {
        raster_sprite_status_shutdown(raster->sprite_status,
                                      raster->sprite_status->num_sprites);
        lib_free(raster->sprite_status);
    }
}

void raster_sprite_status_reset(raster_sprite_status_t *status, int sprite_offset)
{
    unsigned int i;

    status->visible_msk = 0;
    status->dma_msk = 0;
    status->new_dma_msk = 0;

    status->mc_sprite_color_1 = 0;
    status->mc_sprite_color_2 = 0;

    memset(status->sprite_data_1, 0, sizeof(DWORD) * status->num_sprites);
    memset(status->sprite_data_2, 0, sizeof(DWORD) * status->num_sprites);

    status->sprite_data = status->sprite_data_1;
    status->new_sprite_data = status->sprite_data_2;

    for (i = 0; i < status->num_sprites; i++) {
        raster_sprite_reset(&status->sprites[i], sprite_offset);
    }
}

void raster_sprite_status_set_draw_function(raster_sprite_status_t *status,
                                            raster_sprite_status_draw_function_t function)
{
    status->draw_function = function;
}

void raster_sprite_status_set_cache_function(raster_sprite_status_t *status,
                                             raster_sprite_status_cache_function_t function)
{
    status->cache_function = function;
}

void raster_sprite_status_set_draw_partial_function(raster_sprite_status_t *status,
                                                    raster_sprite_status_draw_partial_function_t function)
{
    status->draw_partial_function = function;
}
