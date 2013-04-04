/*
 * render1x2crt.c - 1x2 CRT renderers
 *
 * Written by
 *  groepaz <groepaz@gmx.net> based on the pal renderers written by
 *  John Selck <graham@cruise.de>
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

#include "render1x2.h"
#include "render1x2crt.h"
#include "types.h"
#include "video-color.h"

/*
    this is the simpliest possible CRT emulation, meaning blur and scanlines only.

    TODO: use RGB color space
*/

static inline
void yuv_to_rgb(SDWORD y, SDWORD u, SDWORD v, SWORD *red, SWORD *grn, SWORD *blu)
{
#ifdef _MSC_VER
# pragma warning( push )
# pragma warning( disable: 4244 )
#endif

    *red = (y + v) >> 16;
    *blu = (y + u) >> 16;
    *grn = (y - ((50 * u + 130 * v) >> 8)) >> 16;

#ifdef _MSC_VER
# pragma warning( pop )
#endif
}

/* Often required function that stores gamma-corrected pixel to current line,
 * averages the current rgb with the contents of previous non-scanline-line,
 * stores the gamma-corrected scanline, and updates the prevline rgb buffer.
 * The variants 4, 3, 2 refer to pixel width of output. */

static inline
void store_line_and_scanline_2(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade, /* ignored by RGB modes */
    const SDWORD y, const SDWORD u, const SDWORD v, const SDWORD y2, const SDWORD u2, const SDWORD v2)
{
    SWORD red, grn, blu;
    WORD *tmp1, *tmp2;
    yuv_to_rgb(y, u, v, &red, &grn, &blu);

    tmp1 = (WORD *) scanline;
    tmp2 = (WORD *) line;

    *tmp1++ = (WORD) (gamma_red_fac[512 + red + prevline[0]]
                      | gamma_grn_fac[512 + grn + prevline[1]]
                      | gamma_blu_fac[512 + blu + prevline[2]]);

    *tmp2++ = (WORD) (gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu]);

    prevline[0] = red;
    prevline[1] = grn;
    prevline[2] = blu;

    yuv_to_rgb(y2, u2, v2, &red, &grn, &blu);

    *tmp1 = (WORD) (gamma_red_fac[512 + red + prevline[3]]
                    | gamma_grn_fac[512 + grn + prevline[4]]
                    | gamma_blu_fac[512 + blu + prevline[5]]);

    *tmp2 = (WORD) (gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu]);

    prevline[3] = red;
    prevline[4] = grn;
    prevline[5] = blu;
}

static inline
void store_line_and_scanline_3(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade, /* ignored by RGB modes */
    const SDWORD y, const SDWORD u, const SDWORD v, const SDWORD y2, const SDWORD u2, const SDWORD v2)
{
    DWORD tmp1, tmp2;
    SWORD red, grn, blu;
    yuv_to_rgb(y, u, v, &red, &grn, &blu);

    tmp1 = gamma_red_fac[512 + red + prevline[0]]
           | gamma_grn_fac[512 + grn + prevline[1]]
           | gamma_blu_fac[512 + blu + prevline[2]];
    tmp2 = gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu];
    scanline[0] = (BYTE) tmp1;
    tmp1 >>= 8;
    scanline[1] = (BYTE) tmp1;
    tmp1 >>= 8;
    scanline[2] = (BYTE) tmp1;

    line[0] = (BYTE) tmp2;
    tmp2 >>= 8;
    line[1] = (BYTE) tmp2;
    tmp2 >>= 8;
    line[2] = (BYTE) tmp2;

    prevline[0] = red;
    prevline[1] = grn;
    prevline[2] = blu;

    yuv_to_rgb(y2, u2, v2, &red, &grn, &blu);

    tmp1 = gamma_red_fac[512 + red + prevline[0]]
           | gamma_grn_fac[512 + grn + prevline[1]]
           | gamma_blu_fac[512 + blu + prevline[2]];
    tmp2 = gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu];
    scanline[3] = (BYTE) tmp1;
    tmp1 >>= 8;
    scanline[4] = (BYTE) tmp1;
    tmp1 >>= 8;
    scanline[5] = (BYTE) tmp1;

    line[3] = (BYTE) tmp2;
    tmp2 >>= 8;
    line[4] = (BYTE) tmp2;
    tmp2 >>= 8;
    line[5] = (BYTE) tmp2;

    prevline[3] = red;
    prevline[4] = grn;
    prevline[5] = blu;
}

static inline
void store_line_and_scanline_4(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade, /* ignored by RGB modes */
    const SDWORD y, const SDWORD u, const SDWORD v, const SDWORD y2, const SDWORD u2, const SDWORD v2)
{
    SWORD red, grn, blu;
    DWORD *tmp1, *tmp2;
    yuv_to_rgb(y, u, v, &red, &grn, &blu);

    tmp1 = (DWORD *) scanline;
    tmp2 = (DWORD *) line;
    *tmp1++ = gamma_red_fac[512 + red + prevline[0]]
              | gamma_grn_fac[512 + grn + prevline[1]]
              | gamma_blu_fac[512 + blu + prevline[2]]
              | alpha;
    *tmp2++ = gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu]
              | alpha;

    prevline[0] = red;
    prevline[1] = grn;
    prevline[2] = blu;

    yuv_to_rgb(y2, u2, v2, &red, &grn, &blu);

    *tmp1 = gamma_red_fac[512 + red + prevline[3]]
            | gamma_grn_fac[512 + grn + prevline[4]]
            | gamma_blu_fac[512 + blu + prevline[5]]
            | alpha;
    *tmp2 = gamma_red[256 + red] | gamma_grn[256 + grn] | gamma_blu[256 + blu]
            | alpha;

    prevline[3] = red;
    prevline[4] = grn;
    prevline[5] = blu;
}

static inline
void store_line_and_scanline_UYVY(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade,
    SDWORD y1_, SDWORD u1, SDWORD v1, SDWORD y2_, SDWORD u2, SDWORD v2)
{
#ifdef _MSC_VER
# pragma warning( push )
# pragma warning( disable: 4244 )
#endif
    BYTE y1 = (BYTE)((y1_ >> 16) & 0xFFu);
    BYTE y2 = (BYTE)((y2_ >> 16) & 0xFFu);

    u1 = (u1 + u2) >> 17;
    v1 = (v1 + v2) >> 17;

    line[0] = (BYTE)(u1 + 128);
    line[1] = y1;
    line[2] = (BYTE)(v1 + 128);
    line[3] = y2;

    y1 = (y1 * shade) >> 8;
    y2 = (y2 * shade) >> 8;
    u1 = 128 + ((u1 * shade) >> 8);
    v1 = 128 + ((v1 * shade) >> 8);

    scanline[0] = (u1 + prevline[1]) >> 1;
    scanline[1] = (y1 + prevline[0]) >> 1;
    scanline[2] = (v1 + prevline[2]) >> 1;
    scanline[3] = (y2 + prevline[3]) >> 1;

    prevline[0] = y1;
    prevline[1] = u1;
    prevline[2] = v1;
    prevline[3] = y2;
/*
    prevline[4] = u1;
    prevline[5] = v1;
*/
#ifdef _MSC_VER
# pragma warning( pop )
#endif
}

static inline
void store_line_and_scanline_YUY2(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade,
    SDWORD y1_, SDWORD u1, SDWORD v1, SDWORD y2_, SDWORD u2, SDWORD v2)
{
#ifdef _MSC_VER
# pragma warning( push )
# pragma warning( disable: 4244 )
#endif

    BYTE y1 = (BYTE)((y1_ >> 16) & 0xFFu);
    BYTE y2 = (BYTE)((y2_ >> 16) & 0xFFu);

    u1 = (u1 + u2) >> 17;
    v1 = (v1 + v2) >> 17;

    line[0] = y1;
    line[1] = u1 + 128;
    line[2] = y2;
    line[3] = v1 + 128;

    y1 = (y1 * shade) >> 8;
    y2 = (y2 * shade) >> 8;
    u1 = 128 + ((u1 * shade) >> 8);
    v1 = 128 + ((v1 * shade) >> 8);

    scanline[0] = (y1 + prevline[0]) >> 1;
    scanline[1] = (u1 + prevline[1]) >> 1;
    scanline[2] = (y2 + prevline[3]) >> 1;
    scanline[3] = (v1 + prevline[2]) >> 1;

    prevline[0] = y1;
    prevline[1] = u1;
    prevline[2] = v1;
    prevline[3] = y2;
/*
    prevline[4] = u1;
    prevline[5] = v1;
*/

#ifdef _MSC_VER
# pragma warning( pop )
#endif
}

static inline
void store_line_and_scanline_YVYU(
    BYTE *const line, BYTE *const scanline,
    SWORD *const prevline, const int shade,
    SDWORD y1_, SDWORD u1, SDWORD v1, SDWORD y2_, SDWORD u2, SDWORD v2)
{
#ifdef _MSC_VER
# pragma warning( push )
# pragma warning( disable: 4244 )
#endif

    BYTE y1 = (BYTE)((y1_ >> 16) & 0xFFu);
    BYTE y2 = (BYTE)((y2_ >> 16) & 0xFFu);

    u1 = (u1 + u2) >> 17;
    v1 = (v1 + v2) >> 17;

    line[0] = y1;
    line[1] = v1 + 128;
    line[2] = y2;
    line[3] = u1 + 128;

    y1 = (y1 * shade) >> 8;
    y2 = (y2 * shade) >> 8;
    u1 = 128 + ((u1 * shade) >> 8);
    v1 = 128 + ((v1 * shade) >> 8);

    scanline[0] = (y1 + prevline[0]) >> 1;
    scanline[1] = (v1 + prevline[2]) >> 1;
    scanline[2] = (y2 + prevline[3]) >> 1;
    scanline[3] = (u1 + prevline[1]) >> 1;

    prevline[0] = y1;
    prevline[1] = u1;
    prevline[2] = v1;
    prevline[3] = y2;
/*
    prevline[4] = u1;
    prevline[5] = v1;
*/

#ifdef _MSC_VER
# pragma warning( pop )
#endif
}


static inline
void get_yuv_from_video(
    const SDWORD unew, const SDWORD vnew,
    const int off_flip,
    SDWORD *const u, SDWORD *const v)
{
    *u = (unew) * off_flip;
    *v = (vnew) * off_flip;
}

static inline
void render_generic_1x2_crt(video_render_color_tables_t *color_tab,
                            const BYTE *src, BYTE *trg,
                            unsigned int width, const unsigned int height,
                            unsigned int xs, const unsigned int ys,
                            unsigned int xt, const unsigned int yt,
                            const unsigned int pitchs, const unsigned int pitcht,
                            viewport_t *viewport, unsigned int pixelstride,
                            void (*store_func)(
                                BYTE *const line, BYTE *const scanline,
                                SWORD *const prevline, const int shade,
                                SDWORD l1, SDWORD u1, SDWORD v1, SDWORD l2, SDWORD u2, SDWORD v2),
                            const int write_interpolated_pixels, video_render_config_t *config)
{
    SWORD *prevrgblineptr;
    const SDWORD *ytablel = color_tab->ytablel;
    const SDWORD *ytableh = color_tab->ytableh;
    const BYTE *tmpsrc;
    BYTE *tmptrg, *tmptrgscanline;
    SDWORD *cbtable, *crtable;
    DWORD x, y, wfirst, wlast, yys;
    SDWORD l, u, unew, v, vnew, off_flip, shade;
    SDWORD l2 = 0;
    SDWORD u2 = 0;
    SDWORD v2 = 0;
    int first_line = viewport->first_line * 2;
    int last_line = (viewport->last_line * 2) + 1;

    src = src + pitchs * ys + xs - 2;
    trg = trg + pitcht * yt + xt * pixelstride;
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
/*    width >>= 1; */

    /* That's all initialization we need for full lines. Unfortunately, for
     * scanlines we also need to calculate the RGB color of the previous
     * full line, and that requires initialization from 2 full lines above our
     * rendering target. We just won't render the scanline above the target row,
     * so you need to call us with 1 line before the desired rectangle, and
     * for one full line after it! */

    /* Calculate odd line shading */
    shade = (int) ((float) config->video_resources.pal_scanlineshade / 1000.0f * 256.f);
    off_flip = 1 << 6;

    /* height & 1 == 0. */
    for (y = yys; y < yys + height + 1; y += 2) {
        /* when we are dealing with the last line, the rules change:
         * we no longer write the main output to screen, we just put it into
         * the scanline. */
        if (y == yys + height) {
            /* no place to put scanline in: we are outside viewport or still
             * doing the first iteration (y == yys), height == 0 */
            if (y == yys || y <= (unsigned int)first_line || y > (unsigned int)(last_line + 1)) {
                break;
            }
            tmptrg = &color_tab->rgbscratchbuffer[0];
            tmptrgscanline = trg - pitcht;
            if (y == (unsigned int)(last_line + 1)) {
                /* src would point after the source area, so rewind one line */
                src -= pitchs;
            }
        } else {
            /* pixel data to surface */
            tmptrg = trg;
            /* write scanline data to previous line if possible,
             * otherwise we dump it to the scratch region... We must never
             * render the scanline for the first row, because prevlinergb is not
             * yet initialized and scanline data would be bogus! */
            tmptrgscanline = y != yys && y > (unsigned int)first_line && y <= (unsigned int)last_line
                             ? trg - pitcht
                             : &color_tab->rgbscratchbuffer[0];
        }

        /* current source image for YUV xform */
        tmpsrc = src;

        cbtable = write_interpolated_pixels ? color_tab->cbtable : color_tab->cutable;
        crtable = write_interpolated_pixels ? color_tab->crtable : color_tab->cvtable;

        l = ytablel[tmpsrc[1]] + ytableh[tmpsrc[2]] + ytablel[tmpsrc[3]];
        unew = cbtable[tmpsrc[0]] + cbtable[tmpsrc[1]] + cbtable[tmpsrc[2]] + cbtable[tmpsrc[3]];
        vnew = crtable[tmpsrc[0]] + crtable[tmpsrc[1]] + crtable[tmpsrc[2]] + crtable[tmpsrc[3]];
        get_yuv_from_video(unew, vnew, off_flip, &u, &v);
        unew -= cbtable[tmpsrc[0]];
        vnew -= crtable[tmpsrc[0]];
        tmpsrc += 1;

        /* actual line */
        prevrgblineptr = &color_tab->prevrgbline[0];
        if (wfirst) {
            l2 = ytablel[tmpsrc[1]] + ytableh[tmpsrc[2]] + ytablel[tmpsrc[3]];
            unew += cbtable[tmpsrc[3]];
            vnew += crtable[tmpsrc[3]];
            get_yuv_from_video(unew, vnew, off_flip, &u2, &v2);
            unew -= cbtable[tmpsrc[0]];
            vnew -= crtable[tmpsrc[0]];
            tmpsrc += 1;
#if 0
            if (write_interpolated_pixels) {
                store_func(tmptrg, tmptrgscanline, prevrgblineptr, shade, (l + l2) >> 1, (u + u2) >> 1, (v + v2) >> 1);
                tmptrgscanline += pixelstride;
                tmptrg += pixelstride;
                prevrgblineptr += 3;
            }
#endif
            l = l2;
            u = u2;
            v = v2;
        }
        for (x = 0; x < width; x++) {
#if 0
            store_func(tmptrg, tmptrgscanline, prevrgblineptr, shade, l, u, v);
            tmptrgscanline += pixelstride;
            tmptrg += pixelstride;
            prevrgblineptr += 3;
#endif
            l2 = ytablel[tmpsrc[1]] + ytableh[tmpsrc[2]] + ytablel[tmpsrc[3]];
            unew += cbtable[tmpsrc[3]];
            vnew += crtable[tmpsrc[3]];
            get_yuv_from_video(unew, vnew, off_flip, &u2, &v2);
            unew -= cbtable[tmpsrc[0]];
            vnew -= crtable[tmpsrc[0]];
            tmpsrc += 1;
#if 0
            if (write_interpolated_pixels) {
                store_func(tmptrg, tmptrgscanline, prevrgblineptr, shade, (l + l2) >> 1, (u + u2) >> 1, (v + v2) >> 1);
                tmptrgscanline += pixelstride;
                tmptrg += pixelstride;
                prevrgblineptr += 3;
            }
#endif
            x++;
            if (x == width) {
                break;
            }
#if 1
            store_func(tmptrg, tmptrgscanline, prevrgblineptr, shade, l, u, v, l2, u2, v2);
            tmptrgscanline += pixelstride * 2;
            tmptrg += pixelstride * 2;
            prevrgblineptr += 6;
#endif
            l2 = ytablel[tmpsrc[1]] + ytableh[tmpsrc[2]] + ytablel[tmpsrc[3]];
            unew += cbtable[tmpsrc[3]];
            vnew += crtable[tmpsrc[3]];
            get_yuv_from_video(unew, vnew, off_flip, &u2, &v2);
            unew -= cbtable[tmpsrc[0]];
            vnew -= crtable[tmpsrc[0]];
            tmpsrc += 1;

            l = l2;
            u = u2;
            v = v2;
        }
        if (wlast) {
            store_func(tmptrg, tmptrgscanline, prevrgblineptr, shade, l, u, v, l2, u2, v2);
        }

        src += pitchs;
        trg += pitcht * 2;
    }
}

void render_UYVY_1x2_crt(video_render_color_tables_t *color_tab,
                         const BYTE *src, BYTE *trg,
                         unsigned int width, const unsigned int height,
                         const unsigned int xs, const unsigned int ys,
                         const unsigned int xt, const unsigned int yt,
                         const unsigned int pitchs, const unsigned int pitcht,
                         viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           2, store_line_and_scanline_UYVY, 0, config);
}

void render_YUY2_1x2_crt(video_render_color_tables_t *color_tab,
                         const BYTE *src, BYTE *trg,
                         unsigned int width, const unsigned int height,
                         const unsigned int xs, const unsigned int ys,
                         const unsigned int xt, const unsigned int yt,
                         const unsigned int pitchs, const unsigned int pitcht,
                         viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           2, store_line_and_scanline_YUY2, 0, config);
}

void render_YVYU_1x2_crt(video_render_color_tables_t *color_tab,
                         const BYTE *src, BYTE *trg,
                         unsigned int width, const unsigned int height,
                         const unsigned int xs, const unsigned int ys,
                         const unsigned int xt, const unsigned int yt,
                         const unsigned int pitchs, const unsigned int pitcht,
                         viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           2, store_line_and_scanline_YVYU, 0, config);
}

void render_16_1x2_crt(video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht,
                       viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           2, store_line_and_scanline_2, 1, config);
}

void render_24_1x2_crt(video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht,
                       viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           3, store_line_and_scanline_3, 1, config);
}

void render_32_1x2_crt(video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht,
                       viewport_t *viewport, video_render_config_t *config)
{
    render_generic_1x2_crt(color_tab, src, trg, width, height, xs, ys,
                           xt, yt, pitchs, pitcht, viewport,
                           4, store_line_and_scanline_4, 1, config);
}
