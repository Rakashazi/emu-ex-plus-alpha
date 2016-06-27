/*
 * video-render-pal.c - Implementation of framebuffer to physical screen copy
 *
 * Written by
 *  John Selck <graham@cruise.de>
 *  Dag Lem <resid@nimrod.no>
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

#include "log.h"
#include "machine.h"
#include "render1x1.h"
#include "render1x1pal.h"
#include "render1x1ntsc.h"
#include "render1x2.h"
#include "render1x2crt.h"
#include "render2x2.h"
#include "render2x2pal.h"
#include "render2x2ntsc.h"
#include "renderscale2x.h"
#include "resources.h"
#include "types.h"
#include "video-render.h"
#include "video.h"

#ifdef DINGOO_NATIVE
#include "render1x1_dingoo.h"
#endif

static void video_render_pal_main(video_render_config_t *config,
                                  BYTE *src, BYTE *trg,
                                  int width, int height, int xs, int ys, int xt,
                                  int yt, int pitchs, int pitcht, int depth,
                                  viewport_t *viewport)
{
#ifdef DINGOO_NATIVE
    render_16_1x1_04_dingoo(&config->color_tables, src, trg, width, height, xs, ys, xt, yt, pitchs, pitcht);
#else
    video_render_color_tables_t *colortab;
    int doublescan, delayloop, rendermode, scale2x, video;

    video = viewport->crt_type;

    rendermode = config->rendermode;
    doublescan = config->doublescan;
    colortab = &config->color_tables;
    scale2x = config->scale2x;

    delayloop = (config->filter == VIDEO_FILTER_CRT);

    /*
    if (config->external_palette)
        delayloop = 0;
    */

    if ((rendermode == VIDEO_RENDER_PAL_1X1
         || rendermode == VIDEO_RENDER_PAL_2X2)
        && config->video_resources.pal_scanlineshade <= 0) {
        doublescan = 0;
    }

    switch (rendermode) {
        case VIDEO_RENDER_NULL:
            break;

        case VIDEO_RENDER_PAL_1X1:
            if (delayloop && depth != 8) {
                if (video) {
                    switch (depth) {
                        case 16:
                            render_16_1x1_pal(colortab, src, trg, width, height,
                                              xs, ys, xt, yt, pitchs, pitcht, config);
                            return;
                        case 24:
                            render_24_1x1_pal(colortab, src, trg, width, height,
                                              xs, ys, xt, yt, pitchs, pitcht, config);
                            return;
                        case 32:
                            render_32_1x1_pal(colortab, src, trg, width, height,
                                              xs, ys, xt, yt, pitchs, pitcht, config);
                            return;
                    }
                } else {
                    switch (depth) {
                        case 16:
                            render_16_1x1_ntsc(colortab, src, trg, width, height,
                                               xs, ys, xt, yt, pitchs, pitcht);
                            return;
                        case 24:
                            render_24_1x1_ntsc(colortab, src, trg, width, height,
                                               xs, ys, xt, yt, pitchs, pitcht);
                            return;
                        case 32:
                            render_32_1x1_ntsc(colortab, src, trg, width, height,
                                               xs, ys, xt, yt, pitchs, pitcht);
                            return;
                    }
                }
            } else {
                switch (depth) {
                    case 8:
                        render_08_1x1_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 16:
                        render_16_1x1_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 24:
                        render_24_1x1_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 32:
                        render_32_1x1_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht);
                        return;
                }
            }
            return;
        case VIDEO_RENDER_PAL_2X2:
            if (delayloop && depth != 8) {
                switch (video) {
                    case 0: /* NTSC */
                        switch (depth) {
                            case 16:
                                render_16_2x2_ntsc(colortab, src, trg, width, height,
                                                   xs, ys, xt, yt, pitchs, pitcht,
                                                   viewport, config);
                                return;
                            case 24:
                                render_24_2x2_ntsc(colortab, src, trg, width, height,
                                                   xs, ys, xt, yt, pitchs, pitcht,
                                                   viewport, config);
                                return;
                            case 32:
                                render_32_2x2_ntsc(colortab, src, trg, width, height,
                                                   xs, ys, xt, yt, pitchs, pitcht,
                                                   viewport, config);
                                return;
                        }
                        break;
                    case 1: /* PAL */
                        switch (depth) {
                            case 16:
                                render_16_2x2_pal(colortab, src, trg, width, height,
                                                  xs, ys, xt, yt, pitchs, pitcht,
                                                  viewport, config);
                                return;
                            case 24:
                                render_24_2x2_pal(colortab, src, trg, width, height,
                                                  xs, ys, xt, yt, pitchs, pitcht,
                                                  viewport, config);
                                return;
                            case 32:
                                render_32_2x2_pal(colortab, src, trg, width, height,
                                                  xs, ys, xt, yt, pitchs, pitcht,
                                                  viewport, config);
                                return;
                        }
                        break;
                }
            } else if (scale2x) {
                switch (depth) {
                    case 8:
                        render_08_scale2x(colortab, src, trg, width, height,
                                          xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 16:
                        render_16_scale2x(colortab, src, trg, width, height,
                                          xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 24:
                        render_24_scale2x(colortab, src, trg, width, height,
                                          xs, ys, xt, yt, pitchs, pitcht);
                        return;
                    case 32:
                        render_32_scale2x(colortab, src, trg, width, height,
                                          xs, ys, xt, yt, pitchs, pitcht);
                        return;
                }
            } else {
                switch (depth) {
                    case 8:
                        render_08_2x2_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
                        return;
                    case 16:
                        render_16_2x2_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
                        return;
                    case 24:
                        render_24_2x2_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
                        return;
                    case 32:
                        render_32_2x2_04(colortab, src, trg, width, height,
                                         xs, ys, xt, yt, pitchs, pitcht, doublescan, config);
                        return;
                }
            }
    }
    log_debug("video_render_pal_main unsupported rendermode (%d)\n", rendermode);
#endif
}

void video_render_pal_init(void)
{
    video_render_palfunc_set(video_render_pal_main);
}
