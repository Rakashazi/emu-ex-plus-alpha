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
#include "render1x1.h"
#include "render1x1pal.h"
#include "render1x1ntsc.h"
#include "render1x2crt.h"
#include "render2x2crt.h"
#include "render2x2ntsc.h"
#include "render2x2pal.h"
#include "render2x4crt.h"
#include "renderyuv.h"
#include "types.h"
#include "video-render.h"
#include "video-sound.h"
#include "video.h"

static void (*render_1x2_func)(video_render_config_t *, const BYTE *, BYTE *,
                               unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               int);

static void (*render_2x2_func)(video_render_config_t *, const BYTE *, BYTE *,
                               unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               const unsigned int, const unsigned int,
                               int);

static void (*render_pal_func)(video_render_config_t *, BYTE *, BYTE *,
                               int, int, int, int,
                               int, int, int, int, int, viewport_t *);

static void (*render_crt_func)(video_render_config_t *, BYTE *, BYTE *,
                               int, int, int, int,
                               int, int, int, int, int, viewport_t *);

void video_render_initconfig(video_render_config_t *config)
{
    int i;

    config->rendermode = VIDEO_RENDER_NULL;
    config->doublescan = 0;

    for (i = 0; i < 256; i++) {
        config->color_tables.physical_colors[i] = 0;
    }
}

void video_render_setphysicalcolor(video_render_config_t *config, int index,
                                   DWORD color, int depth)
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

void video_render_main(video_render_config_t *config, BYTE *src, BYTE *trg,
                       int width, int height, int xs, int ys, int xt, int yt,
                       int pitchs, int pitcht, int depth, viewport_t *viewport)
{
    const video_render_color_tables_t *colortab;
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
    colortab = &config->color_tables;

    switch (rendermode) {
        case VIDEO_RENDER_NULL:
            return;
            break;

        case VIDEO_RENDER_PAL_1X1:
        case VIDEO_RENDER_PAL_2X2:
            (*render_pal_func)(config, src, trg, width, height, xs, ys, xt, yt,
                               pitchs, pitcht, depth, viewport);
            return;

        case VIDEO_RENDER_CRT_1X1:
        case VIDEO_RENDER_CRT_1X2:
        case VIDEO_RENDER_CRT_2X2:
        case VIDEO_RENDER_CRT_2X4:
            (*render_crt_func)(config, src, trg, width, height, xs, ys, xt, yt,
                               pitchs, pitcht, depth, viewport);
            return;

        case VIDEO_RENDER_RGB_1X1:
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
            break;

        case VIDEO_RENDER_RGB_1X2:
            (*render_1x2_func)(config, src, trg, width, height,
                               xs, ys, xt, yt, pitchs, pitcht, depth);
            return;

        case VIDEO_RENDER_RGB_2X2:
            (*render_2x2_func)(config, src, trg, width, height,
                               xs, ys, xt, yt, pitchs, pitcht, depth);
            return;
    }
    if (rendermode_error != rendermode) {
        log_error(LOG_DEFAULT, "video_render_main: unsupported rendermode (%d)", rendermode);
    }
    rendermode_error = rendermode;
}

void video_render_1x2func_set(void (*func)(video_render_config_t *,
                                           const BYTE *, BYTE *,
                                           unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           int))
{
    render_1x2_func = func;
}

void video_render_2x2func_set(void (*func)(video_render_config_t *,
                                           const BYTE *, BYTE *,
                                           unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           const unsigned int, const unsigned int,
                                           int))
{
    render_2x2_func = func;
}

void video_render_palfunc_set(void (*func)(video_render_config_t *,
                                           BYTE *, BYTE *, int, int, int, int,
                                           int, int, int, int, int, viewport_t *))
{
    render_pal_func = func;
}

void video_render_crtfunc_set(void (*func)(video_render_config_t *,
                                           BYTE *, BYTE *, int, int, int, int,
                                           int, int, int, int, int, viewport_t *))
{
    render_crt_func = func;
}

/*******************************************************************************
  YUV rendering
  - used by XAW and AmigaOS ports

  FIXME: - several sub renderers are missing
         - the prototype of this function should be changed to be the same
           as the one of video_render_main()
*******************************************************************************/

#ifdef HAVE_XVIDEO
/* Render YUV 4:2:2 and 4:1:1 formats. */
void render_yuv_image(viewport_t *viewport,
                      int true_pal_mode,
                      int pal_blur,
                      int pal_scanline_shade,
                      fourcc_t format,
                      image_t* image,
                      unsigned char* src,
                      int src_pitch,
                      video_render_config_t *config,
                      int src_x, int src_y,
                      unsigned int src_w, unsigned int src_h,
                      int dest_x, int dest_y)
{
    int planar;
    int shift_y0, shift_u, shift_v, shift_y1;
    int plane_y, plane_u, plane_v;
    int double_scan = config->doublescan;

    switch (format.id) {
        case FOURCC_UYVY:
            planar = 0;
            plane_y = plane_u = plane_v = 0;
#ifdef WORDS_BIGENDIAN
            shift_y0 = 16; shift_u = 24; shift_v = 8; shift_y1 = 0;
#else
            shift_y0 = 8; shift_u = 0; shift_v = 16; shift_y1 = 24;
#endif
            break;
        case FOURCC_YUY2:
            planar = 0;
            plane_y = plane_u = plane_v = 0;
#ifdef WORDS_BIGENDIAN
            shift_y0 = 24; shift_u = 16; shift_v = 0; shift_y1 = 8;
#else
            shift_y0 = 0; shift_u = 8; shift_v = 24; shift_y1 = 16;
#endif
            break;
        case FOURCC_YVYU:
            planar = 0;
            plane_y = plane_u = plane_v = 0;
#ifdef WORDS_BIGENDIAN
            shift_y0 = 24; shift_u = 0; shift_v = 16; shift_y1 = 8;
#else
            shift_y0 = 0; shift_u = 24; shift_v = 8; shift_y1 = 16;
#endif
            break;
        case FOURCC_YV12:
            planar = 1;
            shift_y0 = shift_u = shift_v = shift_y1 = 0;
            plane_y = 0; plane_u = 2; plane_v = 1;
            break;
        case FOURCC_I420:
        case FOURCC_IYUV:
            planar = 1;
            shift_y0 = shift_u = shift_v = shift_y1 = 0;
            plane_y = 0; plane_u = 1; plane_v = 2;
            break;
        default:
            return;
    }

    switch (config->rendermode) {
        case VIDEO_RENDER_NULL:
            return;
            break;

        case VIDEO_RENDER_PAL_1X1:
            if (planar) {
                if (!true_pal_mode) {
                    /* planar, 1x1, no filter */
                    renderyuv_4_1_1(image, plane_y, plane_u, plane_v, src,
                                    src_pitch, config->color_tables.yuv_table, src_x,
                                    src_y, src_w, src_h, dest_x, dest_y,
                                    &config->color_tables.yuv_updated);
                } else {
                    /* planar, 1x1, pal filter */
                    renderyuv_4_1_1_pal(image, plane_y, plane_u, plane_v,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y,
                                        pal_blur, &config->color_tables.yuv_updated);
/* FIXME: implement renderyuv_4_1_1_ntsc */
                }
                return;
            } else {
                if (!true_pal_mode) {
                    /* not planar, 1x1, no filter */
                    renderyuv_4_2_2(image, shift_y0,
                                    shift_u, shift_v,
                                    shift_y1, src, src_pitch,
                                    config->color_tables.yuv_table,
                                    src_x, src_y, src_w,
                                    src_h, dest_x, dest_y,
                                    &config->color_tables.yuv_updated);
                    return;
                } else {
                    if (viewport->crt_type) {
                        /* not planar, 1x1, pal filter */
                        switch (format.id) {
                            case FOURCC_UYVY:
                                render_UYVY_1x1_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], config
                                    );
                                return;
                            case FOURCC_YUY2:
                                render_YUY2_1x1_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], config
                                    );
                                return;
                            case FOURCC_YVYU:
                                render_YVYU_1x1_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], config
                                    );
                                return;
                        }
                    } else {
                        /* not planar, 1x1, ntsc filter */
                        switch (format.id) {
                            case FOURCC_UYVY:
                                render_UYVY_1x1_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0]
                                    );
                                return;
                            case FOURCC_YUY2:
                                render_YUY2_1x1_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0]
                                    );
                                return;
                            case FOURCC_YVYU:
                                render_YVYU_1x1_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0]
                                    );
                                return;
                        }
                    }
                }
            }
            break;
        case VIDEO_RENDER_PAL_2X2:
            if (planar) {
                if (!true_pal_mode) {
                    /* planar, 2x2, no filter */
                    renderyuv_2x_4_1_1(image, plane_y, plane_u, plane_v,
                                       src, src_pitch, config->color_tables.yuv_table,
                                       src_x, src_y, src_w, src_h, dest_x, dest_y,
                                       double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
                } else {
                    /* planar, 2x2, pal filter */
                    renderyuv_2x_4_1_1_pal(image, plane_y, plane_u, plane_v,
                                           src, src_pitch, config->color_tables.yuv_table,
                                           src_x, src_y, src_w, src_h, dest_x, dest_y,
                                           pal_blur, pal_scanline_shade, &config->color_tables.yuv_updated);
/* FIXME: implement renderyuv_2x_4_1_1_ntsc */
                }
                return;
            } else {
                if (!true_pal_mode) {
                    /* not planar, 2x2, no filter */
                    renderyuv_2x_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                       src, src_pitch, config->color_tables.yuv_table,
                                       src_x, src_y, src_w, src_h, dest_x, dest_y,
                                       double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
                    return;
                } else {
                    src_w *= 2;
                    src_h *= 2;
                    dest_y *= 2;
                    if (viewport->crt_type) {
                        /* not planar, 2x2, pal filter */
                        switch (format.id) {
                            case FOURCC_UYVY:
                                render_UYVY_2x2_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                            case FOURCC_YUY2:
                                render_YUY2_2x2_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                            case FOURCC_YVYU:
                                render_YVYU_2x2_pal(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                        }
                    } else {
                        /* not planar, 2x2, ntsc filter */
                        switch (format.id) {
                            case FOURCC_UYVY:
                                render_UYVY_2x2_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                            case FOURCC_YUY2:
                                render_YUY2_2x2_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                            case FOURCC_YVYU:
                                render_YVYU_2x2_ntsc(
                                    &config->color_tables, src, image->data + image->offsets[0],
                                    src_w, src_h, src_x, src_y,
                                    dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                    );
                                return;
                        }
                    }
                }
            }
            break;

        case VIDEO_RENDER_CRT_1X1:
            if (planar) {
                /* planar, 1x1, no filter */
                renderyuv_4_1_1(image, plane_y, plane_u, plane_v, src,
                                src_pitch, config->color_tables.yuv_table, src_x,
                                src_y, src_w, src_h, dest_x, dest_y,
                                &config->color_tables.yuv_updated);
                return;
            } else {
                /* not planar, 1x1, no filter */
                renderyuv_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                src, src_pitch, config->color_tables.yuv_table,
                                src_x, src_y, src_w, src_h, dest_x, dest_y, &config->color_tables.yuv_updated);
                return;
            }
/* FIXME: implement renderyuv_4_1_1_crt, render_UYVY_1x1_crt, render_YUY2_1x1_crt, render_YVYU_1x1_crt */
#if 0
            if (planar) {
                if (!true_pal_mode) {
                    /* planar, 1x1, no filter */
                    renderyuv_4_1_1(image, plane_y, plane_u, plane_v,
                                    src, src_pitch, config->color_tables.yuv_table,
                                    src_x, src_y, src_w, src_h, dest_x, dest_y);
                } else {
                    /* planar, 1x1, crt filter */
                    renderyuv_4_1_1_crt(image, plane_y, plane_u, plane_v,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y,
                                        pal_blur);
                }
                return;
            } else {
                if (!true_pal_mode) {
                    /* not planar, 1x1, no filter */
                    renderyuv_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                    src, src_pitch, config->color_tables.yuv_table,
                                    src_x, src_y, src_w, src_h, dest_x, dest_y);
                    return;
                } else {
                    /* not planar, 1x1, crt filter */
                    switch (format.id) {
                        case FOURCC_UYVY:
                            render_UYVY_1x1_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], config
                                );
                            return;
                        case FOURCC_YUY2:
                            render_YUY2_1x1_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], config
                                );
                            return;
                        case FOURCC_YVYU:
                            render_YVYU_1x1_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], config
                                );
                            return;
                    }
                }
            }
#endif
            break;
        case VIDEO_RENDER_CRT_1X2:
            if (planar) {
/* FIXME: implement renderyuv_1x2_4_1_1, renderyuv_1x2_4_1_1_crt */
#if 0
                if (!true_pal_mode) {
                    /* planar, 1x2, no filter */
                    renderyuv_1x2_4_1_1(image, plane_y, plane_u, plane_v,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y);
                } else {
                    /* planar, 1x2, crt filter */
                    renderyuv_1x2_4_1_1_crt(image, plane_y, plane_u, plane_v,
                                            src, src_pitch, config->color_tables.yuv_table,
                                            src_x, src_y, src_w, src_h, dest_x, dest_y,
                                            pal_blur);
                }
                return;
#endif
            } else {
/* FIXME: implement renderyuv_1x2_4_2_2 */
#if 0
                if (!true_pal_mode) {
                    /* not planar, 1x2, no filter */
                    renderyuv_1x2_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y);
                    return;
                } else {
#endif
                /* not planar, 1x2, crt filter */
                src_h *= 2;
                dest_y *= 2;
                switch (format.id) {
                    case FOURCC_UYVY:
                        render_UYVY_1x2_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
                    case FOURCC_YUY2:
                        render_YUY2_1x2_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
                    case FOURCC_YVYU:
                        render_YVYU_1x2_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
                }
            }
#if 0
    }
#endif
            break;
        case VIDEO_RENDER_CRT_2X2:
            if (planar) {
/* FIXME: implement renderyuv_2x_4_1_1_crt */
#if 0
                if (!true_pal_mode) {
                    /* planar, 2x2, no filter */
                    renderyuv_2x_4_1_1(image, plane_y, plane_u, plane_v,
                                       src, src_pitch, config->color_tables.yuv_table,
                                       src_x, src_y, src_w, src_h, dest_x, dest_y,
                                       double_scan, pal_scanline_shade);
                } else {
                    /* planar, 2x2, crt filter */
                    renderyuv_2x_4_1_1_crt(image, plane_y, plane_u, plane_v,
                                           src, src_pitch, config->color_tables.yuv_table,
                                           src_x, src_y, src_w, src_h, dest_x, dest_y,
                                           pal_blur, double_scan, pal_scanline_shade);
                }
                return;
#else
                /* planar, 2x2, no filter */
                renderyuv_2x_4_1_1(image, plane_y, plane_u, plane_v,
                                   src, src_pitch, config->color_tables.yuv_table,
                                   src_x, src_y, src_w, src_h, dest_x, dest_y,
                                   double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
                return;
#endif
            } else {
                if (!true_pal_mode) {
                    /* not planar, 2x2, no filter */
                    renderyuv_2x_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                       src, src_pitch, config->color_tables.yuv_table,
                                       src_x, src_y, src_w, src_h, dest_x, dest_y,
                                       double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
                    return;
                } else {
                    /* not planar, 2x2, crt filter */
                    src_w *= 2;
                    src_h *= 2;
                    dest_y *= 2;
                    switch (format.id) {
                        case FOURCC_UYVY:
                            render_UYVY_2x2_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                );
                            return;
                        case FOURCC_YUY2:
                            render_YUY2_2x2_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                );
                            return;
                        case FOURCC_YVYU:
                            render_YVYU_2x2_crt(
                                &config->color_tables, src, image->data + image->offsets[0],
                                src_w, src_h, src_x, src_y,
                                dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                                );
                            return;
                    }
                }
            }
            break;
        case VIDEO_RENDER_CRT_2X4:
            if (planar) {
/* FIXME: implement renderyuv_2x4_4_1_1, renderyuv_2x4_4_1_1_crt */
#if 0
                if (!true_pal_mode) {
                    /* planar, 2x4, no filter */
                    renderyuv_2x4_4_1_1(image, plane_y, plane_u, plane_v,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y);
                } else {
                    /* planar, 2x4, crt filter */
                    renderyuv_2x4_4_1_1_crt(image, plane_y, plane_u, plane_v,
                                            src, src_pitch, config->color_tables.yuv_table,
                                            src_x, src_y, src_w, src_h, dest_x, dest_y,
                                            pal_blur);
                }
                return;
#endif
            } else {
/* FIXME: implement renderyuv_2x4_4_2_2 */
#if 0
                if (!true_pal_mode) {
                    /* not planar, 2x4, no filter */
                    renderyuv_2x4_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                        src, src_pitch, config->color_tables.yuv_table,
                                        src_x, src_y, src_w, src_h, dest_x, dest_y);
                    return;
                } else {
#endif
                /* not planar, 2x4, crt filter */
                src_w *= 2;
                src_h *= 4;
                dest_y *= 4;
                switch (format.id) {
                    case FOURCC_UYVY:
                        render_UYVY_2x4_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
                    case FOURCC_YUY2:
                        render_YUY2_2x4_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
                    case FOURCC_YVYU:
                        render_YVYU_2x4_crt(
                            &config->color_tables, src, image->data + image->offsets[0],
                            src_w, src_h, src_x, src_y,
                            dest_x, dest_y, src_pitch, image->pitches[0], viewport, config
                            );
                        return;
#if 0
                }
#endif
                }
            }
            break;

        case VIDEO_RENDER_RGB_1X1:
            if (planar) {
                /* planar, 1x1, no filter */
                renderyuv_4_1_1(image, plane_y, plane_u, plane_v,
                                src, src_pitch, config->color_tables.yuv_table,
                                src_x, src_y, src_w, src_h, dest_x, dest_y, &config->color_tables.yuv_updated);
            } else {
                /* not planar, 1x1, no filter */
                renderyuv_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                src, src_pitch, config->color_tables.yuv_table,
                                src_x, src_y, src_w, src_h, dest_x, dest_y, &config->color_tables.yuv_updated);
            }
            return;

        case VIDEO_RENDER_RGB_1X2:
/* FIXME: implement renderyuv_1x2_4_1_1, renderyuv_1x2_4_2_2 */
#if 0
            if (planar) {
                /* planar, 1x2, no filter */
                renderyuv_1x2_4_1_1(image, plane_y, plane_u, plane_v,
                                    src, src_pitch, config->color_tables.yuv_table,
                                    src_x, src_y, src_w, src_h, dest_x, dest_y);
            } else {
                /* not planar, 1x2, no filter */
                renderyuv_1x2_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                    src, src_pitch, config->color_tables.yuv_table,
                                    src_x, src_y, src_w, src_h, dest_x, dest_y);
            }
            return;
#endif
            break;
        case VIDEO_RENDER_RGB_2X2:
            if (planar) {
                /* planar, 2x2, no filter */
                renderyuv_2x_4_1_1(image, plane_y, plane_u, plane_v,
                                   src, src_pitch, config->color_tables.yuv_table,
                                   src_x, src_y, src_w, src_h, dest_x, dest_y,
                                   double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
            } else {
                /* not planar, 2x2, no filter */
                renderyuv_2x_4_2_2(image, shift_y0, shift_u, shift_v, shift_y1,
                                   src, src_pitch, config->color_tables.yuv_table,
                                   src_x, src_y, src_w, src_h, dest_x, dest_y,
                                   double_scan, pal_scanline_shade, &config->color_tables.yuv_updated);
            }
            return;
    }

    if (rendermode_error != config->rendermode) {
        log_error(LOG_DEFAULT, "render_yuv_image: unsupported rendermode (%d)", config->rendermode);
    }
    rendermode_error = config->rendermode;
}

void video_render_update_palette(struct video_canvas_s *canvas)
{
    video_color_update_palette(canvas);
}
#endif
