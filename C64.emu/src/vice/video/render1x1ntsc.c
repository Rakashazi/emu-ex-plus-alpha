/*
 * render1x1ntsc.c - 1x1 NTSC renderers
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

#include "render1x1ntsc.h"
#include "types.h"
#include "video-color.h"

/*
    right now this is basically the PAL renderer without delay line emulation
*/

/*
    YIQ->RGB (Sony CXA2025AS US decoder matrix)

    R = Y + (1.630 * I + 0.317 * Q)
    G = Y - (0.378 * I + 0.466 * Q)
    B = Y - (1.089 * I - 1.677 * Q)
*/
static inline
void yuv_to_rgb(int32_t y, int32_t u, int32_t v,
                int32_t *red, int32_t *grn, int32_t *blu)
{
    *red = (y + ((209 * u +  41 * v) >> 7)) >> 15;
    *grn = (y - (( 48 * u +  69 * v) >> 7)) >> 15;
    *blu = (y - ((139 * u - 215 * v) >> 7)) >> 15;
}

static inline
void store_pixel_4(video_render_color_tables_t *color_tab, uint8_t *trg, int32_t y1, int32_t u1, int32_t v1, int32_t y2, int32_t u2, int32_t v2)
{
    uint32_t *tmp;
    int32_t red;
    int32_t grn;
    int32_t blu;

    yuv_to_rgb(y1, u1, v1, &red, &grn, &blu);
    tmp = (uint32_t *) trg;
    tmp[0] = color_tab->gamma_red[256 + red]
             | color_tab->gamma_grn[256 + grn]
             | color_tab->gamma_blu[256 + blu]
             | color_tab->alpha;

    yuv_to_rgb(y2, u2, v2, &red, &grn, &blu);
    tmp[1] = color_tab->gamma_red[256 + red]
             | color_tab->gamma_grn[256 + grn]
             | color_tab->gamma_blu[256 + blu]
             | color_tab->alpha;
}

/* NTSC 1x1 renderers */
static inline void
render_generic_1x1_ntsc(video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                        unsigned int width, const unsigned int height,
                        unsigned int xs, const unsigned int ys,
                        unsigned int xt, const unsigned int yt,
                        const unsigned int pitchs, const unsigned int pitcht,
                        const unsigned int pixelstride,
                        int yuvtarget)
{
    const int32_t *cbtable;
    const int32_t *crtable;
    const int32_t *ytablel = color_tab->ytablel;
    const int32_t *ytableh = color_tab->ytableh;
    const uint8_t *tmpsrc;
    uint8_t *tmptrg;
    unsigned int x, y;
    int32_t l1, l2, u1, u2, v1, v2, unew, vnew;
    uint8_t cl0, cl1, cl2, cl3;
    int off_flip;

    /* ensure starting on even coords */
    if ((xt & 1) && xs > 0) {
        xs--;
        xt--;
        width++;
    }

    src = src + pitchs * ys + xs - 2;
    trg = trg + pitcht * yt + (xt >> 1) * pixelstride;

    width >>= 1;

    off_flip = 1 << 6;

    for (y = ys; y < height + ys; y++) {
        tmpsrc = src;
        tmptrg = trg;

        cbtable = yuvtarget ? color_tab->cutable : color_tab->cbtable;
        crtable = yuvtarget ? color_tab->cvtable : color_tab->crtable;

        /* one scanline */
        for (x = 0; x < width; x++) {
            cl0 = tmpsrc[0];
            cl1 = tmpsrc[1];
            cl2 = tmpsrc[2];
            cl3 = tmpsrc[3];
            tmpsrc += 1;
            l1 = ytablel[cl1] + ytableh[cl2] + ytablel[cl3];
            unew = cbtable[cl0] + cbtable[cl1] + cbtable[cl2] + cbtable[cl3];
            vnew = crtable[cl0] + crtable[cl1] + crtable[cl2] + crtable[cl3];
            u1 = (unew) * off_flip;
            v1 = (vnew) * off_flip;

            cl0 = tmpsrc[0];
            cl1 = tmpsrc[1];
            cl2 = tmpsrc[2];
            cl3 = tmpsrc[3];
            tmpsrc += 1;
            l2 = ytablel[cl1] + ytableh[cl2] + ytablel[cl3];
            unew = cbtable[cl0] + cbtable[cl1] + cbtable[cl2] + cbtable[cl3];
            vnew = crtable[cl0] + crtable[cl1] + crtable[cl2] + crtable[cl3];
            u2 = (unew) * off_flip;
            v2 = (vnew) * off_flip;

            store_pixel_4(color_tab, tmptrg, l1, u1, v1, l2, u2, v2);
            tmptrg += pixelstride;
        }

        src += pitchs;
        trg += pitcht;
    }
}

void
render_32_1x1_ntsc(video_render_color_tables_t *color_tab,
                   const uint8_t *src, uint8_t *trg,
                   const unsigned int width, const unsigned int height,
                   const unsigned int xs, const unsigned int ys,
                   const unsigned int xt, const unsigned int yt,
                   const unsigned int pitchs, const unsigned int pitcht)
{
    render_generic_1x1_ntsc(color_tab, src, trg, width, height, xs, ys, xt, yt,
                            pitchs, pitcht,
                            8, 0);
}
