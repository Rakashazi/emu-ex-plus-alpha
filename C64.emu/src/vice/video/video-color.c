/*
 * video-color.c - Video implementation of YUV, YIQ, YCbCr and RGB colors
 *
 * Written by
 *  John Selck <graham@cruise.de>
 *  Groepaz <groepaz@gmx.net>
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

/* #define DEBUG_VIDEO */

#ifdef DEBUG_VIDEO
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>       /* needed for pow function */

#include "lib.h"
#include "log.h"
#include "machine.h"
#include "palette.h"
#include "resources.h"
#include "viewport.h"
#include "video-canvas.h"
#include "video-color.h"
#include "video.h"
#include "videoarch.h"

DWORD gamma_red[256 * 3];
DWORD gamma_grn[256 * 3];
DWORD gamma_blu[256 * 3];

DWORD gamma_red_fac[256 * 3 * 2];
DWORD gamma_grn_fac[256 * 3 * 2];
DWORD gamma_blu_fac[256 * 3 * 2];

DWORD alpha = 0;

static DWORD color_red[256];
static DWORD color_grn[256];
static DWORD color_blu[256];

#define RMIN(x,min) (((x) < (min)) ? (min) : (x))
#define RMAX(x,max) (((x) > (max)) ? (max) : (x))
#define RMINMAX(x,min,max) RMIN(RMAX(x,max),min)

#define MATH_PI 3.141592653589793238462643383279

typedef struct video_ycbcr_color_s {
    float y;
    float cb;
    float cr;
} video_ycbcr_color_t;

typedef struct video_ycbcr_palette_s {
    unsigned int num_entries;
    video_ycbcr_color_t *entries;
} video_ycbcr_palette_t;

void video_render_setrawrgb(unsigned int index, DWORD r, DWORD g, DWORD b)
{
    color_red[index] = r;
    color_grn[index] = g;
    color_blu[index] = b;
}

void video_render_setrawalpha(DWORD a)
{
    alpha = a;
}

static video_ycbcr_palette_t *video_ycbcr_palette_create(unsigned int num_entries)
{
    video_ycbcr_palette_t *p;

    p = lib_malloc(sizeof(video_ycbcr_palette_t));

    p->num_entries = num_entries;
    p->entries = lib_calloc(num_entries, sizeof(video_ycbcr_color_t));

    return p;
}

static void video_ycbcr_palette_free(video_ycbcr_palette_t *p)
{
    if (p == NULL) {
        return;
    }

    lib_free(p->entries);
    lib_free(p);
}

/*******************************************************************************
 * color space conversion functions
 ******************************************************************************/

/*
  formulas for color space conversion:

  RGB to YCbCr:

  y  =  0.299   * R + 0.587   * G + 0.114   * B
  Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B
  Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B

  Y  = 0.2989   * R + 0.5866  * G + 0.1145  * B
  Cb = B - Y
  Cr = R - Y

  RGB to YUV

  Y =  0,299 * R + 0,587 * G + 0,114 * B
  U = -0,147 * R - 0,289 * G + 0,436 * B   (B-Y;Cr)
  V =  0,615 * R - 0,515 * G - 0,100 * B   (R-Y;Cb)

  U =  0.493 * (B - Y)
  V =  0.877 * (R - Y)

  RGB to YIQ

  Y =  0,299 * R + 0,587 * G + 0,114 * B
  I =  0,596 * R - 0,274 * G + 0,322 * B
  Q =  0,212 * R - 0,523 * G - 0,311 * B

  I = -0.27 * (B - Y) + 0.74 * (R - Y)
  Q =  0.41 * (B - Y) + 0.48 * (R - Y)

  YCbCr to RGB:

  R = Y + Cr
  G = Y - (0.1145/0.5866) * Cb - (0.2989/0.5866) * Cr
  G = Y - 0.195192 * Cb - 0.509546 * Cr
  B = Y + Cb

  YCbCr to YUV:

  U = 0.493111 * Cb
  V = 0.877283 * Cr

  YCbCr to YIQ:

  I = -0.27 * Cb + 0.74 * Cr
  Q =  0.41 * Cb + 0.48 * Cr

  YUV to RGB

  R = Y + V
  G = Y - (0.1953 * U + 0.5078 * V)
  B = Y + U

  YUV to YCbCr

  Cb = U / 0.493111
  Cr = V / 0.877283

  YIQ to YUV

  U = -0.546512701 * I + 0.842540416 * Q
  V =  0.830415704 * I + 0.546859122 * Q

  YIQ to YCbCr

  Cb = (-0.546512701 * I + 0.842540416 * Q) / 0.493111
  Cr = ( 0.830415704 * I + 0.546859122 * Q) / 0.877283

  YIQ->RGB (Sony CXA2025AS US decoder matrix)

  R = Y + 1,630 * I + 0,317 * Q
  G = Y - 0,378 * I - 0,466 * Q
  B = Y - 1,089 * I + 1,677 * Q

  YUV is the PAL colorspace. It's just a slightly modified YCbCr for
  better broadcasting.

  YIQ is the NTSC colorspace, its very similar - basically 33 degrees rotated.

  the PAL renderer does a bit of cheating and uses YCbCr instead of YUV for
  speed reasons. the NTSC renderer uses YIQ as defined by the SONY matrix.
*/

#ifdef DEBUG_VIDEO
static inline void video_convert_ycbcr_to_yuv(video_ycbcr_color_t *src, video_ycbcr_color_t *dst)
{
    dst->y = src->y;
    dst->cb = src->cb * 0.493111f;
    dst->cr = src->cr * 0.877283f;
}

static inline void video_convert_ycbcr_to_yiq(video_ycbcr_color_t *src, video_ycbcr_color_t *dst)
{
    float cb = src->cb;
    float cr = src->cr;
    dst->y = src->y;
    dst->cb = -0.27f * cb + 0.74f * cr;
    dst->cr =  0.41f * cb + 0.48f * cr;
}
#endif

static inline void video_convert_yuv_to_ycbcr(video_ycbcr_color_t *src, video_ycbcr_color_t *dst)
{
    dst->y = src->y;
    dst->cb = src->cb / 0.493111f;
    dst->cr = src->cr / 0.877283f;
}

#if 0 /* currently unused */
static inline void video_convert_yiq_to_ycbcr(video_ycbcr_color_t *src, video_ycbcr_color_t *dst)
{
    float i = src->cb;
    float q = src->cr;
    dst->y = src->y;
    dst->cb = (-0.546512701f * i + 0.842540416f * q) / 0.493111f;
    dst->cr = ( 0.830415704f * i + 0.546859122f * q) / 0.877283f;
}
#endif

/*
 YPbPr (ITU-R BT.601)
 ==========================================================
 R' =     Y +                    1.402 * Pr
 G' =     Y - 0.344136 * Pb - 0.714136 * Pr
 B' =     Y +    1.772 * Pb
 ..........................................................
 Y' in [0; 1]
 Pb in [-0.5; 0.5]
 Pr in [-0.5; 0.5]
 R', G', B' in [0; 1]
*/
static inline void video_convert_ycbcr_to_rgb(const video_ycbcr_color_t *src, int *r, int *g, int *b)
{
    float rf, gf, bf;
#if 0
    bf = src->cb + src->y;
    rf = src->cr + src->y;
    gf = src->y - (0.1145f / 0.5866f) * src->cb - (0.2989f / 0.5866f) * src->cr;
#else
    /* gives better reverse-conversion results than the above */
    rf = src->y +                          1.402f * src->cr;
    gf = src->y - 0.344136f * src->cb - 0.714136f * src->cr;
    bf = src->y +    1.772f * src->cb;
#endif
    *r = (int)RMINMAX(rf, 0, 255);
    *g = (int)RMINMAX(gf, 0, 255);
    *b = (int)RMINMAX(bf, 0, 255);
}

static inline void video_convert_yiq_to_rgb(video_ycbcr_color_t *src, int *r, int *g, int *b)
{
    float rf, gf, bf;

    /* SONY matrix */
    rf = src->y + 1.630f * src->cb + 0.317f * src->cr;
    gf = src->y - 0.378f * src->cb - 0.466f * src->cr;
    bf = src->y - 1.089f * src->cb + 1.677f * src->cr;

    *r = (int)RMINMAX(rf, 0, 255);
    *g = (int)RMINMAX(gf, 0, 255);
    *b = (int)RMINMAX(bf, 0, 255);
}

static inline void video_convert_rgb_to_yiq(const palette_entry_t *src, video_ycbcr_color_t *dst)
{
    /* inverse of SONY matrix */
    dst->y  = 0.23485876230514607f * src->red + 0.6335007388077467f  * src->green + 0.13164049888710716f * src->blue;
    dst->cb = 0.4409594767911895f  * src->red - 0.27984362502847304f * src->green - 0.16111585176271648f * src->blue; /* I */
    dst->cr = 0.14630060102591497f * src->red - 0.5594814826856017f  * src->green + 0.4131808816596867f  * src->blue; /* Q */
}

/*
 YPbPr (ITU-R BT.601)
 ========================================================
 Y' =     + 0.299    * R' + 0.587    * G' + 0.114    * B'
 Pb =     - 0.168736 * R' - 0.331264 * G' + 0.5      * B'
 Pr =     + 0.5      * R' - 0.418688 * G' - 0.081312 * B'
 ........................................................
 R', G', B' in [0; 1]
 Y' in [0; 1]
 Pb in [-0.5; 0.5]
 Pr in [-0.5; 0.5]
*/
static inline void video_convert_rgb_to_ycbcr(const palette_entry_t *src, video_ycbcr_color_t *dst)
{
    /* convert RGB to YCbCr */
    dst->y  =  0.2989f   * src->red + 0.5866f   * src->green + 0.1145f   * src->blue;
    dst->cb = -0.168736f * src->red - 0.331264f * src->green + 0.5f      * src->blue;
    dst->cr =       0.5f * src->red - 0.418688f * src->green - 0.081312f * src->blue;
}

/******************************************************************************/
/* conversion of VIC/VIC-II/TED colors to YCbCr

   used by video_cbm_palette_to_ycbcr (internal palette/CRT emu)
*/

#define INT_LUMA_ADJ (1.00f)
#define INT_SAT_ADJ (1.75f)

static void video_convert_cbm_to_ycbcr(const video_cbm_color_t *src,
                                       float basesat, float phase,
                                       video_ycbcr_color_t *dst, int video)
{
    /* DBG(("video_convert_cbm_to_ycbcr sat:%f phase:%f", basesat, phase)); */

    dst->y = src->luminance;
    /* FIXME: this is a hack to adjust the output of the internal color generator somewhat
              to align with the pepto palette when all neutral parameters are used */
    dst->y /= INT_LUMA_ADJ;
    basesat /= INT_SAT_ADJ;

    /* chrominance (U and V) of color */
    if (video) {
        /* PAL */
        dst->cb = (float)(basesat * cos((src->angle + phase) * (MATH_PI / 180.0))); /* U */
        dst->cr = (float)(basesat * sin((src->angle + phase) * (MATH_PI / 180.0))); /* V */

        /* convert UV to CbCr */
        video_convert_yuv_to_ycbcr(dst, dst);
    } else {
        /* NTSC */
        dst->cb = (float)(basesat * sin((src->angle + phase - (100.0f / 3.0f)) * (MATH_PI / 180.0))); /* I */
        dst->cr = (float)(basesat * cos((src->angle + phase - (100.0f / 3.0f)) * (MATH_PI / 180.0))); /* Q */
    }

    /* direction of color vector (-1 = inverted vector, 0 = grey vector) */

    if (src->direction == 0) {
        dst->cb = 0.0f;
        dst->cr = 0.0f;
    }
    if (src->direction < 0) {
        dst->cb = -dst->cb;
        dst->cr = -dst->cr;
    }
}

/* gamma correction */

static float video_gamma(float value, double factor, float gamma, float bri, float con)
{
    float ret;

    value += bri;
    value *= con;

    if (value <= 0.0f) {
        return 0.0f;
    }

    ret = (float)(factor * pow(value, gamma));

    if (ret < 0.0f) {
        ret = 0.0f;
    }

    return ret;
}

#ifdef DEBUG_VIDEO
static void video_convert_renderer_to_rgb(video_ycbcr_color_t *src,
                                       palette_entry_t *dst, int video)
{
    int r, g, b;

    /* DBG(("video_convert_renderer_to_rgb")); */

    if (video) {
        /* PAL */
        video_convert_ycbcr_to_rgb(src, &r, &g, &b);
    } else {
        /* NTSC */
        video_convert_yiq_to_rgb(src, &r, &g, &b);
    }

    dst->dither = 0;
    dst->red = (BYTE)r;
    dst->green = (BYTE)g;
    dst->blue = (BYTE)b;
    dst->name = NULL;
}
#endif

/* conversion of YCbCr to RGB
   used by video_calc_palette (internal palette) (not used by CRT emu)
 */
static void video_convert_renderer_to_rgb_gamma(video_ycbcr_color_t *src, float sat,
                                       float bri, float con, float gam, float tin,
                                       palette_entry_t *dst, int video)
{
    float rf, bf, gf;
    double factor;
    int r, g, b;
    video_ycbcr_color_t tmp;

    /* DBG(("video_convert_renderer_to_rgb_gamma")); */

    tmp.cb = src->cb;
    tmp.cr = src->cr;
    tmp.y = src->y;

    if (video) {
        tmp.cr += tin; /* apply tint */
        /* apply saturation */
        tmp.cb *= sat;
        tmp.cr *= sat;

        video_convert_ycbcr_to_rgb(&tmp, &r, &g, &b);
    } else {
        /* FIXME: tint for ntsc */
        tmp.cr += tin; /* apply tint */
        /* apply saturation */
        tmp.cb *= sat;
        tmp.cr *= sat;

        video_convert_yiq_to_rgb(&tmp, &r, &g, &b);
    }

    /* do gamma correction */
    factor = pow(255.0f, 1.0f - gam);
    rf = video_gamma((float)r, factor, gam, bri, con);
    gf = video_gamma((float)g, factor, gam, bri, con);
    bf = video_gamma((float)b, factor, gam, bri, con);

    /* convert to int and clip to 8 bit boundaries */

    r = (int)rf;
    g = (int)gf;
    b = (int)bf;

    dst->dither = 0;
    dst->red = (BYTE)RMAX(r,255);
    dst->green = (BYTE)RMAX(g,255);
    dst->blue = (BYTE)RMAX(b,255);
    dst->name = NULL;
}

/* conversion of RGB to YCbCr
 * (custom palette, RGB color modes)
 */

static void video_convert_rgb_to_renderer(const palette_entry_t *src,
                                       video_ycbcr_color_t *dst, int video)
{
    /* DBG(("video_convert_rgb_to_renderer")); */

    if (video) {
        /* PAL */
        video_convert_rgb_to_ycbcr(src, dst);
    } else {
        /* NTSC */
        video_convert_rgb_to_yiq(src, dst);
    }
}

/* FIXME: handle gamma for CRT emulation (CGA and Monochrom video) too */
static float video_get_gamma(video_resources_t *video_resources, int video)
{
    float mgam, vgam;

    if (video) {
        vgam = 2.8f; /* PAL gamma */
    } else {
        vgam = 2.2f; /* NTSC gamma */
    }

    mgam = ((float)(video_resources->color_gamma)) / 1000.0f;
    return mgam / vgam;
}

/* gammatable calculation */
static void video_calc_gammatable(video_resources_t *video_resources, int video)
{
    int i;
    float bri, con, gam, scn, v;
    double factor;
    DWORD vi;

    bri = ((float)(video_resources->color_brightness - 1000))
          * (128.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast   )) / 1000.0f;
    gam = video_get_gamma(video_resources, video);
    scn = ((float)(video_resources->pal_scanlineshade)) / 1000.0f;

    factor = pow(255.0f, 1.0f - gam);
    for (i = 0; i < (256 * 3); i++) {
        v = video_gamma((float)(i - 256), factor, gam, bri, con);

        vi = (DWORD)v;
        if (vi > 255) {
            vi = 255;
        }
        gamma_red[i] = color_red[vi];
        gamma_grn[i] = color_grn[vi];
        gamma_blu[i] = color_blu[vi];

        vi = (DWORD)(v * scn);
        if (vi > 255) {
            vi = 255;
        }
        gamma_red_fac[i * 2] = color_red[vi];
        gamma_grn_fac[i * 2] = color_grn[vi];
        gamma_blu_fac[i * 2] = color_blu[vi];
        v = video_gamma((float)(i - 256) + 0.5f, factor, gam, bri, con);
        vi = (DWORD)(v * scn);
        if (vi > 255) {
            vi = 255;
        }
        gamma_red_fac[i * 2 + 1] = color_red[vi];
        gamma_grn_fac[i * 2 + 1] = color_grn[vi];
        gamma_blu_fac[i * 2 + 1] = color_blu[vi];
    }
}

/* ycbcr table calculation */
/* called by video_color_update_palette, internal and external palette */
static void video_calc_ycbcrtable(video_resources_t *video_resources,
                                  const video_ycbcr_palette_t *p, video_render_color_tables_t *color_tab, int video)
{
    video_ycbcr_color_t *primary;
    unsigned int i, lf, hf;
    float sat, tin, bri, con, gam;
    float yf, uf, vf;
    int y, u, v;
    double factor, len;
#ifdef DEBUG_VIDEO
    palette_entry_t temp;
#endif

    DBG(("video_calc_ycbcrtable"));

    lf = 64 * video_resources->pal_blur / 1000;
    hf = 255 - (lf << 1);
    sat = ((float)(video_resources->color_saturation)) * (256.0f / 1000.0f);
    tin = (((float)(video_resources->color_tint)) * (50.0f / 2000.0f)) - 25.0f;
    bri = ((float)(video_resources->color_brightness - 1000)) * (112.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast   )) / 1000.0f;
    gam = video_get_gamma(video_resources, video);

    factor = pow(256.0f, 1.0f - gam);

    DBG((" sat:%d bri:%d con:%d gam:%d tin:%d", (int)sat, (int)bri, (int)con, (int)gam, (int)tin));

    for (i = 0; i < p->num_entries; i++) {
        SDWORD val;

        /* create primary table */
        primary = &p->entries[i];
        if (video) {
            val = (SDWORD)(primary->y * 256.0f);
            color_tab->ytablel[i] = val * lf;
            color_tab->ytableh[i] = val * hf;
            /* tint, add to cr in odd lines */
            val = (SDWORD)(tin);
            color_tab->cbtable[i] = (SDWORD)((primary->cb) * sat);
            color_tab->crtable[i] = (SDWORD)((primary->cr + val) * sat);
            color_tab->cutable[i] = (SDWORD)(0.493111f * primary->cb * 256.0); /* convert Cb to U */
            color_tab->cvtable[i] = (SDWORD)(0.877283f * (primary->cr + val) * 256.0); /* convert Cr to V */
        } else {
            /* for NTSC use one bit less for the fraction in the tables to avoid
               integer overflows in the CRT renderer */
            val = (SDWORD)(primary->y * 128.0f);
            color_tab->ytablel[i] = (val * lf);
            color_tab->ytableh[i] = (val * hf);
            /* FIXME: tint for NTSC */
            val = (SDWORD)(tin);
            color_tab->cbtable[i] = (SDWORD)((primary->cb) * sat) >> 1;
            color_tab->crtable[i] = (SDWORD)((primary->cr + val) * sat) >> 1;
            /* FIXME: convert IQ to UV (used by YUV renderers) */
            color_tab->cutable[i] = (SDWORD)(primary->cb * 256.0);
            color_tab->cvtable[i] = (SDWORD)((primary->cr + val) * 256.0);
        }

#ifdef DEBUG_VIDEO
        video_convert_renderer_to_rgb(primary, &temp, video);
        DBG((" %2d  'Cb':%4d 'Cr':%4d     'Cr':%6d 'Cb':%6d     R:%4d G:%4d B:%4d", i,
            (int)primary->cb, (int)primary->cr,
            (int)color_tab->cbtable[i], (int)color_tab->crtable[i],
            temp.red, temp.green, temp.blue
        ));
#endif

        yf = (float)(video_gamma(primary->y, factor, gam, bri, con) * 224.0 / 256.0 + 16.5);
        if (video) {
            /* PAL: convert CbCr to UV */
            uf = (float)(0.493111f * primary->cb * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
            vf = (float)(0.877283f * (primary->cr + tin) * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
        } else {
            /* FIXME: convert IQ to UV (used by YUV renderers) */
            uf = (float)(0.493111f * primary->cb * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
            vf = (float)(0.877283f * (primary->cr + tin) * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
        }

        /* sanity check: cbtable and crtable must be kept in 16 bit range or we 
                         might get overflows in eg the CRT renderer */
        /* FIXME: we need to check more and clamp the values accordingly - it is
                  still possible to "overdrive" the renderer in NTSC mode */
        len = sqrt(((double)color_tab->cbtable[i] * (double)color_tab->cbtable[i]) +
                   ((double)color_tab->crtable[i] * (double)color_tab->crtable[i]));
        if (len >= (double)0x10000) {
            log_error(LOG_DEFAULT, 
                "video_calc_ycbcrtable: color %d cbcr vector too long, use lower base saturation.", i);
        }

        y = (int)RMINMAX(yf, 16, 240);
        u = (int)RMINMAX(uf, 16, 240);
        v = (int)RMINMAX(vf, 16, 240);

        /* YCbCr to YUV, scale [0, 256] to [0, 255] (used by YUV renderers) */
        color_tab->yuv_table[i] = (y << 16) | (u << 8) | v;
    }
    color_tab->yuv_updated = 0;
}

/* FIXME: this is a hack to adjust the output of the internal color generator somewhat
          to align with the pepto palette when all neutral parameters are used */
#define CRT_SAT_MUL (1.5f+0.25f)

static void video_calc_ycbcrtable_oddlines(video_resources_t *video_resources,
                                           const video_ycbcr_palette_t *p, video_render_color_tables_t *color_tab, int video)
{
    video_ycbcr_color_t *primary;
    unsigned int i;
    float sat, tin;

    DBG(("video_calc_ycbcrtable_oddlines"));

    sat = ((float)(video_resources->color_saturation)) * (256.0f / 1000.0f) * CRT_SAT_MUL;
    tin = (((float)(video_resources->color_tint)) * (50.0f / 2000.0f)) - 25.0f;

    for (i = 0; i < p->num_entries; i++) {
        SDWORD val;

        /* create primary table */
        primary = &p->entries[i];
        /* tint, substract from cr in odd lines */
        val = (SDWORD)(tin);

        color_tab->cbtable_odd[i] = -(SDWORD)((primary->cb) * sat);
        color_tab->crtable_odd[i] = -(SDWORD)((primary->cr - val) * sat);
        color_tab->cutable_odd[i] = -(SDWORD)(0.493111f * primary->cb * 256); /* PAL: convert Cb to U */
        color_tab->cvtable_odd[i] = -(SDWORD)(0.877283f * (primary->cr - val) * 256); /* PAL: convert Cr to V */
    }
}

/* Convert an RGB palette to YCbCr. (used when custom palette is loaded) */
static void video_palette_to_ycbcr(const palette_t *p, video_ycbcr_palette_t* ycbcr, int video)
{
    unsigned int i;
#ifdef DEBUG_VIDEO
    int r, g, b;
    palette_entry_t temp;
#endif
    DBG(("video_palette_to_ycbcr"));

    for (i = 0; i < p->num_entries; i++) {
        if (video) {
            video_convert_rgb_to_renderer(&p->entries[i], &ycbcr->entries[i], video);
#ifdef DEBUG_VIDEO
            video_convert_renderer_to_rgb(&ycbcr->entries[i], &temp, video);
            DBG((" %2d R:%3d G:%3d B:%3d    Y:%4d Cb:%4d Cr:%4d    R:%3d G:%3d B:%3d", i,
                p->entries[i].red, p->entries[i].green, p->entries[i].blue,
                (int)ycbcr->entries[i].y, (int)ycbcr->entries[i].cb, (int)ycbcr->entries[i].cr,
                temp.red, temp.green, temp.blue
            ));
#endif
        } else {
            video_convert_rgb_to_yiq(&p->entries[i], &ycbcr->entries[i]);
#ifdef DEBUG_VIDEO
            video_convert_yiq_to_rgb(&ycbcr->entries[i], &r, &g, &b);
            DBG((" %2d R:%3d G:%3d B:%3d    Y:%4d I:%4d Q:%4d    R:%3d G:%3d B:%3d", i,
                p->entries[i].red, p->entries[i].green, p->entries[i].blue,
                (int)ycbcr->entries[i].y, (int)ycbcr->entries[i].cb, (int)ycbcr->entries[i].cr,
                r, g, b
            ));
#endif
        }
    }
}

/* Convert an RGB palette to YCbCr. (used when custom palette is loaded) */
static void video_palette_to_ycbcr_oddlines(const palette_t *p, video_ycbcr_palette_t* ycbcr, int video)
{
    unsigned int i;
    DBG(("video_palette_to_ycbcr_oddlines"));

    for (i = 0; i < p->num_entries; i++) {
        video_convert_rgb_to_renderer(&p->entries[i], &ycbcr->entries[i], video);
        ycbcr->entries[i].cr = -ycbcr->entries[i].cr;
        ycbcr->entries[i].cb = -ycbcr->entries[i].cb;
    }
}

/* Convert the internal videochip palette to YCbCr. */
static void video_cbm_palette_to_ycbcr(const video_cbm_palette_t *p, video_ycbcr_palette_t* ycbcr, int video)
{
    unsigned int i;
#ifdef DEBUG_VIDEO
    int cb, cr;
#endif

    DBG(("video_cbm_palette_to_ycbcr"));

    if (p->type == CBM_PALETTE_RGB) {
        /* special case for handling chips that output RGB (such as the VDC),
           admittedly slightly ugly but simple and effective :) */
        palette_entry_t src;
        for (i = 0; i < p->num_entries; i++) {
            src.red = (BYTE)p->entries[i].luminance;
            src.green = (BYTE)p->entries[i].angle;
            src.blue = p->entries[i].direction;
            video_convert_rgb_to_renderer(&src, &ycbcr->entries[i], video);
        }
    } else {
        for (i = 0; i < p->num_entries; i++) {
            video_convert_cbm_to_ycbcr(&p->entries[i], p->saturation, p->phase, &ycbcr->entries[i], video);
#ifdef DEBUG_VIDEO
            cb = ycbcr->entries[i].cb;
            cr = ycbcr->entries[i].cr;
            if (video) {
                video_convert_ycbcr_to_yuv(&ycbcr->entries[i], &ycbcr->entries[i]);
                DBG((" %2d: Luma:%4d Angle:%4d Dir:%2d    Y:%4d Cb:%4d Cr:%4d    Y:%4d U:%4d V:%4d", i,
                    (int)p->entries[i].luminance, (int)p->entries[i].angle, (int)p->entries[i].direction,
                    (int)ycbcr->entries[i].y, cb, cr,
                    (int)ycbcr->entries[i].y, (int)ycbcr->entries[i].cb, (int)ycbcr->entries[i].cr
                    ));
            } else {
                /* video_convert_ycbcr_to_yiq(&ycbcr->entries[i], &ycbcr->entries[i]); */
                DBG((" %2d: Luma:%4d Angle:%4d Dir:%2d    Y:%4d Cb:%4d Cr:%4d    Y:%4d I:%4d Q:%4d", i,
                    (int)p->entries[i].luminance, (int)p->entries[i].angle, (int)p->entries[i].direction,
                    (int)ycbcr->entries[i].y, cb, cr,
                    (int)ycbcr->entries[i].y, (int)ycbcr->entries[i].cb, (int)ycbcr->entries[i].cr
                    ));
            }
            ycbcr->entries[i].cb = cb;
            ycbcr->entries[i].cr = cr;
#endif
        }
    }
}

/*
    offsets between +45.0 and -45.0 kinda make sense (ie the colors will not
    look terribly wrong.) a "perfect" c64 setup will use +/- 0 (and thus the
    colorshifting caused by the phase differences will not be visible), however
    such thing doesn't exist in the real world, and infact gfx people are
    (ab)using this "feature" in their pictures.

    - my c64 seems to use ~ +20.0 (gpz)
*/

static void video_cbm_palette_to_ycbcr_oddlines(video_resources_t *video_resources,
                                                const video_cbm_palette_t *p, video_ycbcr_palette_t* ycbcr, int video)
{
    unsigned int i;
    float offs = (((float)(video_resources->pal_oddlines_phase)) / (2000.0f / 90.0f)) + (180.0f - 45.0f);
    DBG(("video_cbm_palette_to_ycbcr_oddlines"));

    for (i = 0; i < p->num_entries; i++) {
        video_convert_cbm_to_ycbcr(&p->entries[i], p->saturation, (p->phase + offs), &ycbcr->entries[i], video);
    }
}

/*
    Calculate a RGB palette out of VIC/VIC-II/TED colors (in ycbcr format),
    apply saturation, brightness, contrast, tint and gamma settings.

    this palette will be used for screenshots and by renderers if CRT emulation
    is disabled.
*/
static palette_t *video_calc_palette(struct video_canvas_s *canvas, const video_ycbcr_palette_t *p, int video)
{
    palette_t *prgb;
    unsigned int i;
    float sat, bri, con, gam, tin;
    video_resources_t *video_resources = &(canvas->videoconfig->video_resources);

    DBG(("video_calc_palette"));

    sat = ((float)(video_resources->color_saturation)) / 1000.0f;
    bri = ((float)(video_resources->color_brightness - 1000)) * (128.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast)) / 1000.0f;
    gam = video_get_gamma(video_resources, video);
    tin = (((float)(video_resources->color_tint)) / (2000.0f / 50.0f)) - 25.0f;

    /* create RGB palette with the base colors of the video chip */
    prgb = palette_create(p->num_entries, NULL);
    if (prgb == NULL) {
        return NULL;
    }

    DBG((" sat:%d bri:%d con:%d gam:%d tin:%d", (int)sat, (int)bri, (int)con, (int)gam, (int)tin));

    for (i = 0; i < p->num_entries; i++) {
        video_convert_renderer_to_rgb_gamma(&p->entries[i], sat, bri, con, gam, tin, &prgb->entries[i], video);
        DBG((" %2d: Y:%4d Cb:%4d Cr:%4d  R:%3d G:%3d B:%3d", i,
            (int)p->entries[i].y, (int)p->entries[i].cb, (int)p->entries[i].cr,
            (int)prgb->entries[i].red, (int)prgb->entries[i].green, (int)prgb->entries[i].blue));
    }

    return prgb;
}

/* Load RGB palette.  */
static palette_t *video_load_palette(const video_cbm_palette_t *p,
                                     const char *name)
{
    palette_t *palette;

    palette = palette_create(p->num_entries, NULL);

    if (palette == NULL) {
        return NULL;
    }

    if (!video_disabled_mode && palette_load(name, palette) < 0) {
        /* log_message(vicii.log, "Cannot load palette file `%s'.", name); */
        return NULL;
    }

    return palette;
}

/* Calculate or load a palette, depending on configuration.  */
int video_color_update_palette(struct video_canvas_s *canvas)
{
    palette_t *palette;
    video_ycbcr_palette_t *ycbcr;
    video_resources_t *video_resources;
    int video;

    DBG(("video_color_update_palette canvas: %p", canvas));

    if (canvas == NULL) {
        return 0;
    }
    canvas->videoconfig->color_tables.updated = 1;

    DBG(("video_color_update_palette cbm palette:%d extern: %d",
         canvas->videoconfig->cbm_palette ? 1 : 0, canvas->videoconfig->external_palette ? 1 : 0));

    if (canvas->videoconfig->cbm_palette == NULL) {
        return 0;
    }


    /*resources_get_int("MachineVideoStandard", &video);*/
    video = canvas->viewport->crt_type;
    video_resources = &(canvas->videoconfig->video_resources);

#if 0
    /* setup the palette params so we get 100% the same as in peptos calculations */
    video_resources->color_saturation = 1000;
    video_resources->color_brightness = 1000;
    video_resources->color_contrast = 1000;
    video_resources->color_tint = 1000;
    canvas->videoconfig->cbm_palette->saturation = 34.0081334493f;
    canvas->videoconfig->cbm_palette->phase = 0;
    DBG(("sat:%d bri:%d con:%d tin:%d",
         video_resources->color_saturation,
         video_resources->color_brightness,
         video_resources->color_contrast,
         video_resources->color_tint
    ));
#endif

    if (canvas->videoconfig->external_palette) {
        palette = video_load_palette(canvas->videoconfig->cbm_palette,
                                     canvas->videoconfig->external_palette_name);

        if (!palette) {
            return -1;
        }

        video_calc_gammatable(video_resources, video);
        ycbcr = video_ycbcr_palette_create(palette->num_entries);
        video_palette_to_ycbcr(palette, ycbcr, video);
        video_calc_ycbcrtable(video_resources, ycbcr, &canvas->videoconfig->color_tables, video);
        if (canvas->videoconfig->filter == VIDEO_FILTER_CRT) {
            palette_free(palette);
            palette = video_calc_palette(canvas, ycbcr, video);
        }
        /* additional table for odd lines */
        video_palette_to_ycbcr_oddlines(palette, ycbcr, video);
        video_calc_ycbcrtable_oddlines(video_resources, ycbcr, &canvas->videoconfig->color_tables, video);
    } else {
        video_calc_gammatable(video_resources, video);
        ycbcr = video_ycbcr_palette_create(canvas->videoconfig->cbm_palette->num_entries);
        video_cbm_palette_to_ycbcr(canvas->videoconfig->cbm_palette, ycbcr, video);
        video_calc_ycbcrtable(video_resources, ycbcr, &canvas->videoconfig->color_tables, video);
        palette = video_calc_palette(canvas, ycbcr, video);
        /* additional table for odd lines */
        video_cbm_palette_to_ycbcr_oddlines(video_resources, canvas->videoconfig->cbm_palette, ycbcr, video);
        video_calc_ycbcrtable_oddlines(video_resources, ycbcr, &canvas->videoconfig->color_tables, video);
    }

    video_ycbcr_palette_free(ycbcr);

    if (palette != NULL) {
        return video_canvas_palette_set(canvas, palette);
    }

    return -1;
}

void video_color_palette_internal(struct video_canvas_s *canvas,
                                  struct video_cbm_palette_s *cbm_palette)
{
    canvas->videoconfig->cbm_palette = cbm_palette;
    canvas->videoconfig->color_tables.updated = 0;
}

void video_color_palette_free(struct palette_s *palette)
{
    palette_free(palette);
}

/* called by archdep code for first initial setup */
void video_render_initraw(struct video_render_config_s *videoconfig)
{
    int video;
    resources_get_int("MachineVideoStandard", &video);
    video_calc_gammatable(&(videoconfig->video_resources), video);
}
