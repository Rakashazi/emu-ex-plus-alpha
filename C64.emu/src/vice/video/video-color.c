/*
 * video-color.c - Video implementation of YUV, YCbCr and RGB colors
 *
 * Written by
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

/*

  formulas for color space conversion:

  RGB to YCbCr:

  Y  = 0.2989*R + 0.5866*G + 0.1145*B
  Cb = B - Y
  Cr = R - Y

  YCbCr to RGB:

  G = Y - (0.1145/0.5866)*Cb - (0.2989/0.5866)*Cr
  B = Cb + Y
  R = Cr + Y

  YCbCr to YUV:

  U = 0.493111*Cb
  V = 0.877283*Cr

  YUV is the PAL colorspace. It's just a slightly modified YCbCr for
  better broadcasting.

*/

#define MATH_PI 3.141592653589793238462643383279
/* 5028841971 6939937510 5820974944 5923078164 0628620899 8628034825
   3421170679 */

typedef struct video_ycbcr_color_s {
    float y;
    float cb;
    float cr;
} video_ycbcr_color_t;

typedef struct video_ycbcr_palette_s {
    unsigned int num_entries;
    video_ycbcr_color_t *entries;
} video_ycbcr_palette_t;

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

/* conversion of VIC/VIC-II/TED colors to YCbCr */

static void video_convert_cbm_to_ycbcr(const video_cbm_color_t *src,
                                       float basesat, float phase,
                                       video_ycbcr_color_t *dst)
{
    dst->y = src->luminance;

    /* chrominance (U and V) of color */

    dst->cb = (float)(basesat * cos((src->angle + phase) * (MATH_PI / 180.0)));
    dst->cr = (float)(basesat * sin((src->angle + phase) * (MATH_PI / 180.0)));

    /* convert UV to CbCr */

    dst->cb /= 0.493111f;
    dst->cr /= 0.877283f;

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

/* conversion of YCbCr to RGB */

static void video_convert_ycbcr_to_rgb(video_ycbcr_color_t *src, float sat,
                                       float bri, float con, float gam, float tin,
                                       palette_entry_t *dst)
{
    float rf, bf, gf;
    float cb, cr, y;
    double factor;
    int r, g, b;

    cb = src->cb;
    cr = src->cr;
    y = src->y;

    cr += tin; /* apply tint */

    /* apply saturation */
    cb *= sat;
    cr *= sat;

    /* convert YCbCr to RGB */
    bf = cb + y;
    rf = cr + y;
    gf = y - (0.1145f / 0.5866f) * cb - (0.2989f / 0.5866f) * cr;

    factor = pow(255.0f, 1.0f - gam);
    rf = video_gamma(rf, factor, gam, bri, con);
    gf = video_gamma(gf, factor, gam, bri, con);
    bf = video_gamma(bf, factor, gam, bri, con);

    /* convert to int and clip to 8 bit boundaries */

    r = (int)rf;
    g = (int)gf;
    b = (int)bf;

    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    dst->dither = 0;
    dst->red = (BYTE)r;
    dst->green = (BYTE)g;
    dst->blue = (BYTE)b;
    dst->name = NULL;
}

/* conversion of RGB to YCbCr */

static void video_convert_rgb_to_ycbcr(const palette_entry_t *src,
                                       video_ycbcr_color_t *dst)
{
    /* convert RGB to YCbCr */

    dst->y = 0.2989f * src->red + 0.5866f * src->green + 0.1145f * src->blue;
    dst->cb = -0.168736f * src->red - 0.331264f * src->green + 0.5f * src->blue;
    dst->cr = 0.5f * src->red - 0.418688f * src->green - 0.081312f * src->blue;
}

/* FIXME: handle gamma for CRT emulation (CGA and Monochrom video) too */
static float video_get_gamma(video_resources_t *video_resources)
{
    int video;
    float mgam, vgam;

    resources_get_int("MachineVideoStandard", &video);
    if ((video == MACHINE_SYNC_PAL) || (video == MACHINE_SYNC_PALN)) {
        vgam = 2.8f;
    } else {
        vgam = 2.2f;
    }

    mgam = ((float)(video_resources->color_gamma)) / 1000.0f;
    return mgam / vgam;
}

/* gammatable calculation */
static void video_calc_gammatable(video_resources_t *video_resources)
{
    int i;
    float bri, con, gam, scn, v;
    double factor;
    DWORD vi;

    bri = ((float)(video_resources->color_brightness - 1000))
          * (128.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast   )) / 1000.0f;
    gam = video_get_gamma(video_resources);
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

static void video_calc_ycbcrtable(video_resources_t *video_resources,
                                  const video_ycbcr_palette_t *p, video_render_color_tables_t *color_tab)
{
    video_ycbcr_color_t *primary;
    unsigned int i, lf, hf;
    float sat, tin, bri, con, gam;
    float yf, uf, vf;
    int y, u, v;
    double factor, len;

    lf = 64 * video_resources->pal_blur / 1000;
    hf = 255 - (lf << 1);
    sat = ((float)(video_resources->color_saturation)) * (256.0f / 1000.0f);
    tin = (((float)(video_resources->color_tint)) * (50.0f / 2000.0f)) - 25.0f;
    bri = ((float)(video_resources->color_brightness - 1000)) * (112.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast   )) / 1000.0f;
    gam = video_get_gamma(video_resources);

    factor = pow(256.0f, 1.0f - gam);
    for (i = 0; i < p->num_entries; i++) {
        SDWORD val;

        /* create primary table */
        primary = &p->entries[i];
        val = (SDWORD)(primary->y * 256.0f);
        color_tab->ytablel[i] = val * lf;
        color_tab->ytableh[i] = val * hf;
        color_tab->cbtable[i] = (SDWORD)((primary->cb) * sat);
        color_tab->cutable[i] = (SDWORD)(0.493111 * primary->cb * 256.0);
        /* tint, add to cr in odd lines */
        val = (SDWORD)(tin);
        color_tab->crtable[i] = (SDWORD)((primary->cr + val) * sat);
        color_tab->cvtable[i] = (SDWORD)(0.877283 * (primary->cr + val) * 256.0);

        yf = (float)(video_gamma(primary->y, factor, gam, bri, con) * 224.0 / 256.0 + 16.5);
        uf = (float)(0.493111 * primary->cb * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
        vf = (float)(0.877283 * (primary->cr + tin) * sat * con * 224.0 / 256.0 / 256.0 + 128.5);
        y = (int)yf;
        u = (int)uf;
        v = (int)vf;

        /* sanity check: cbtable and crtable must be kept in 16 bit range or we 
                         might get overflows in eg the CRT renderer */
        len = sqrt(((double)color_tab->cbtable[i] * (double)color_tab->cbtable[i]) +
                   ((double)color_tab->crtable[i] * (double)color_tab->crtable[i]));
        if (len >= (double)0x10000) {
            log_error(LOG_DEFAULT, 
                "video_calc_ycbcrtable: color %d cbcr vector too long, use lower base saturation.", i);
        }

        if (y < 16) {
            y = 16;
        } else if (y > 240) {
            y = 240;
        }
        if (u < 16) {
            u = 16;
        } else if (u > 240) {
            u = 240;
        }
        if (v < 16) {
            v = 16;
        } else if (v > 240) {
            v = 240;
        }
        /* YCbCr to YUV, scale [0, 256] to [0, 255] */
        color_tab->yuv_table[i] = (y << 16) | (u << 8) | v;
    }
    color_tab->yuv_updated = 0;
}

static void video_calc_ycbcrtable_oddlines(video_resources_t *video_resources,
                                           const video_ycbcr_palette_t *p, video_render_color_tables_t *color_tab)
{
    video_ycbcr_color_t *primary;
    unsigned int i;
    float sat, tin;

    sat = ((float)(video_resources->color_saturation)) * (256.0f / 1000.0f);
    tin = (((float)(video_resources->color_tint)) * (50.0f / 2000.0f)) - 25.0f;

    for (i = 0; i < p->num_entries; i++) {
        SDWORD val;

        /* create primary table */
        primary = &p->entries[i];
        color_tab->cbtable_odd[i] = -(SDWORD)((primary->cb) * sat);
        color_tab->cutable_odd[i] = -(SDWORD)(0.493111 * primary->cb * 256);
        /* tint, substract from cr in odd lines */
        val = (SDWORD)(tin);
        color_tab->crtable_odd[i] = -(SDWORD)((primary->cr - val) * sat);
        color_tab->cvtable_odd[i] = -(SDWORD)(0.877283 * (primary->cr - val) * 256);
    }
}

/* Convert an RGB palette to YCbCr. */
static void video_palette_to_ycbcr(const palette_t *p,
                                   video_ycbcr_palette_t* ycbcr)
{
    unsigned int i;

    for (i = 0; i < p->num_entries; i++) {
        video_convert_rgb_to_ycbcr(&p->entries[i], &ycbcr->entries[i]);
    }
}

/* Convert an RGB palette to YCbCr. */
static void video_palette_to_ycbcr_oddlines(const palette_t *p, video_ycbcr_palette_t* ycbcr)
{
    unsigned int i;

    for (i = 0; i < p->num_entries; i++) {
        video_convert_rgb_to_ycbcr(&p->entries[i], &ycbcr->entries[i]);
        ycbcr->entries[i].cr = -ycbcr->entries[i].cr;
        ycbcr->entries[i].cb = -ycbcr->entries[i].cb;
    }
}

/* Convert the internal videochip palette to YCbCr. */
static void video_cbm_palette_to_ycbcr(const video_cbm_palette_t *p, video_ycbcr_palette_t* ycbcr)
{
    unsigned int i;

    if (p->type == CBM_PALETTE_RGB) {
        /* special case for handling chips that output RGB (such as the VDC),
           admittedly slightly ugly but simple and effective :) */
        palette_entry_t src;
        for (i = 0; i < p->num_entries; i++) {
            src.red = (BYTE)p->entries[i].luminance;
            src.green = (BYTE)p->entries[i].angle;
            src.blue = p->entries[i].direction;
            video_convert_rgb_to_ycbcr(&src, &ycbcr->entries[i]);
        }
    } else {
        for (i = 0; i < p->num_entries; i++) {
            video_convert_cbm_to_ycbcr(&p->entries[i], p->saturation, p->phase, &ycbcr->entries[i]);
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
                                                const video_cbm_palette_t *p, video_ycbcr_palette_t* ycbcr)
{
    unsigned int i;
    float offs = (((float)(video_resources->pal_oddlines_phase)) / (2000.0f / 90.0f)) + (180.0f - 45.0f);

    for (i = 0; i < p->num_entries; i++) {
        video_convert_cbm_to_ycbcr(&p->entries[i], p->saturation, (p->phase + offs), &ycbcr->entries[i]);
    }
}

/*
    Calculate a RGB palette out of VIC/VIC-II/TED colors (in ycbcr format),
    apply saturation, brightness, contrast, tint and gamma settings.

    this palette will be used for screenshots and by renderers if CRT emulation
    is disabled.
*/
static palette_t *video_calc_palette(struct video_canvas_s *canvas, const video_ycbcr_palette_t *p)
{
    palette_t *prgb;
    unsigned int i;
    float sat, bri, con, gam, tin;
    video_resources_t *video_resources = &(canvas->videoconfig->video_resources);

    DBG(("video_calc_palette"));

    sat = ((float)(video_resources->color_saturation)) / 1000.0f;
    bri = ((float)(video_resources->color_brightness - 1000)) * (128.0f / 1000.0f);
    con = ((float)(video_resources->color_contrast)) / 1000.0f;
    gam = video_get_gamma(video_resources);
    tin = (((float)(video_resources->color_tint)) / (2000.0f / 50.0f)) - 25.0f;

    /* create RGB palette with the base colors of the video chip */
    prgb = palette_create(p->num_entries, NULL);
    if (prgb == NULL) {
        return NULL;
    }

    for (i = 0; i < p->num_entries; i++) {
        video_convert_ycbcr_to_rgb(&p->entries[i], sat, bri, con, gam, tin,
                                   &prgb->entries[i]);
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

    video_resources = &(canvas->videoconfig->video_resources);

    if (canvas->videoconfig->external_palette) {
        palette = video_load_palette(canvas->videoconfig->cbm_palette,
                                     canvas->videoconfig->external_palette_name);

        if (!palette) {
            return -1;
        }

        video_calc_gammatable(video_resources);
        ycbcr = video_ycbcr_palette_create(palette->num_entries);
        video_palette_to_ycbcr(palette, ycbcr);
        video_calc_ycbcrtable(video_resources, ycbcr, &canvas->videoconfig->color_tables);
        if (canvas->videoconfig->filter == VIDEO_FILTER_CRT) {
            palette_free(palette);
            palette = video_calc_palette(canvas, ycbcr);
        }
        /* additional table for odd lines */
        video_palette_to_ycbcr_oddlines(palette, ycbcr);
        video_calc_ycbcrtable_oddlines(video_resources, ycbcr, &canvas->videoconfig->color_tables);
    } else {
        video_calc_gammatable(video_resources);
        ycbcr = video_ycbcr_palette_create(canvas->videoconfig->cbm_palette->num_entries);
        video_cbm_palette_to_ycbcr(canvas->videoconfig->cbm_palette, ycbcr);
        video_calc_ycbcrtable(video_resources, ycbcr, &canvas->videoconfig->color_tables);
        palette = video_calc_palette(canvas, ycbcr);
        /* additional table for odd lines */
        video_cbm_palette_to_ycbcr_oddlines(video_resources, canvas->videoconfig->cbm_palette, ycbcr);
        video_calc_ycbcrtable_oddlines(video_resources, ycbcr, &canvas->videoconfig->color_tables);
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
    video_calc_gammatable(&(videoconfig->video_resources));
}
