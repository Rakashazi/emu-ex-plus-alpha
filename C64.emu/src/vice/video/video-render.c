/*
 * video-render.c - Implementation of framebuffer to physical screen copy
 *
 * Written by
 *  John Selck <graham@cruise.de>
 *  Dag Lem <resid@nimrod.no>
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

#include "log.h"
#include "types.h"
#include "video-render.h"
#include "video-sound.h"
#include "video.h"

static render_pal_ntsc_func_t  render_pal_ntsc_func  = video_render_pal_ntsc_main;
static render_rgbi_func_t render_rgbi_func = video_render_rgbi_main;
static render_crt_mono_func_t render_crt_mono_func = video_render_crt_mono_main;

void video_render_initconfig(video_render_config_t *config)
{
    int i;

    config->rendermode = VIDEO_RENDER_NULL;
    config->doublescan = 0;

    for (i = 0; i < 256; i++) {
        config->color_tables.physical_colors[i] = 0;
    }
}

/* called from archdep code */
void video_render_setphysicalcolor(video_render_config_t *config, int index,
                                   uint32_t color, int depth)
{
    /* duplicated colours are used by the double size 8/16 bpp renderers. */
    switch (depth) {
        case 8:
            color &= 0x000000FF;
            color = color | (color << 8);
            break;
        case 16:
            color &= 0x0000FFFF;
            color = color | (color << 16);
            break;
    }
    config->color_tables.physical_colors[index] = color;
}

static int rendermode_error = -1;

void video_render_main(video_render_config_t *config, uint8_t *src, uint8_t *trg,
                       int width, int height, int xs, int ys, int xt, int yt,
                       int pitchs, int pitcht, viewport_t *viewport)
{
    int rendermode;

#if 0
    log_debug("w:%i h:%i xs:%i ys:%i xt:%i yt:%i ps:%i pt:%i d%i",
              width, height, xs, ys, xt, yt, pitchs, pitcht, depth);

#endif
    if (width <= 0) {
        return; /* some render routines don't like invalid width */
    }

    video_sound_update(config, src, width, height, xs, ys, pitchs, viewport);

    rendermode = config->rendermode;

    switch (rendermode) {
        case VIDEO_RENDER_NULL:
            return;
            break;

        case VIDEO_RENDER_PAL_NTSC_1X1:
        case VIDEO_RENDER_PAL_NTSC_2X2:
            render_pal_ntsc_func(config, src, trg, width, height, xs, ys, xt, yt, pitchs, pitcht, viewport->crt_type, viewport->first_line, viewport->last_line);
            return;

        case VIDEO_RENDER_CRT_MONO_1X1:
        case VIDEO_RENDER_CRT_MONO_1X2:
        case VIDEO_RENDER_CRT_MONO_2X2:
        case VIDEO_RENDER_CRT_MONO_2X4:
            render_crt_mono_func(config, src, trg, width, height, xs, ys, xt, yt, pitchs, pitcht, viewport->first_line, viewport->last_line);
            return;

        case VIDEO_RENDER_RGBI_1X1:
        case VIDEO_RENDER_RGBI_1X2:
        case VIDEO_RENDER_RGBI_2X2:
        case VIDEO_RENDER_RGBI_2X4:
            render_rgbi_func(config, src, trg, width, height, xs, ys, xt, yt, pitchs, pitcht, viewport->first_line, viewport->last_line);
            return;
    }
    if (rendermode_error != rendermode) {
        log_error(LOG_DEFAULT, "video_render_main: unsupported rendermode (%d)", rendermode);
    }
    rendermode_error = rendermode;
}

void video_render_palntscfunc_set(render_pal_ntsc_func_t func)
{
    render_pal_ntsc_func = func;
}

void video_render_crtmonofunc_set(render_crt_mono_func_t func)
{
    render_crt_mono_func = func;
}

void video_render_rgbifunc_set(render_rgbi_func_t func)
{
    render_rgbi_func = func;
}
