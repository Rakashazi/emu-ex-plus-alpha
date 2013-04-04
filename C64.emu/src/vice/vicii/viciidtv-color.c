/*
 * viciidtv-color.c - Colors for the C64 DTV (VIC-II) emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Research about the luminances on the original C64 DTV by
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

#include "viciitypes.h"
#include "vicii-color.h"
#include "vicii-resources.h"
#include "video.h"


/* base saturation of all colors except the grey tones */

#define VICII_SATURATION 48.0f

/* phase shift of all colors */

#define VICII_PHASE -4.5f

/* dtv luminances for the luma-fixed dac */

#define DTV_BLACK_LEVEL 0.318f
#define DTV_TOP_LEVEL   0.911f
#define DTV_LUMA_SCALE(a) ((a) - DTV_BLACK_LEVEL) * (256.0f / (DTV_TOP_LEVEL - DTV_BLACK_LEVEL))

/* levels in Volts obtained from simulation of the fixed dac in spice */
static float dtv_luminances[16] =
{
    DTV_LUMA_SCALE(0.318f),
    DTV_LUMA_SCALE(0.357f),
    DTV_LUMA_SCALE(0.396f),
    DTV_LUMA_SCALE(0.436f),
    DTV_LUMA_SCALE(0.476f),
    DTV_LUMA_SCALE(0.515f),
    DTV_LUMA_SCALE(0.555f),
    DTV_LUMA_SCALE(0.594f),
    DTV_LUMA_SCALE(0.635f),
    DTV_LUMA_SCALE(0.675f),
    DTV_LUMA_SCALE(0.714f),
    DTV_LUMA_SCALE(0.754f),
    DTV_LUMA_SCALE(0.793f),
    DTV_LUMA_SCALE(0.833f),
    DTV_LUMA_SCALE(0.873f),
    DTV_LUMA_SCALE(0.911f)
};

/* dtv luminances for the original broken luma-dac */

/* levels in Volts obtained from simulation of the dac in spice */
#define DTV_BLACK_LEVEL_OLD 0.362f
#define DTV_TOP_LEVEL_OLD   0.854f
#define DTV_LUMA_SCALE_OLD(a) ((a) - DTV_BLACK_LEVEL_OLD) * (256.0f / (DTV_TOP_LEVEL_OLD - DTV_BLACK_LEVEL_OLD))

static float dtv_luminances_old[16] =
{
    DTV_LUMA_SCALE_OLD(0.362f),
    DTV_LUMA_SCALE_OLD(0.370f),
    DTV_LUMA_SCALE_OLD(0.388f),
    DTV_LUMA_SCALE_OLD(0.396f),
    DTV_LUMA_SCALE_OLD(0.458f),
    DTV_LUMA_SCALE_OLD(0.466f),
    DTV_LUMA_SCALE_OLD(0.483f),
    DTV_LUMA_SCALE_OLD(0.492f),
    DTV_LUMA_SCALE_OLD(0.724f),
    DTV_LUMA_SCALE_OLD(0.733f),
    DTV_LUMA_SCALE_OLD(0.750f),
    DTV_LUMA_SCALE_OLD(0.758f),
    DTV_LUMA_SCALE_OLD(0.820f),
    DTV_LUMA_SCALE_OLD(0.828f),
    DTV_LUMA_SCALE_OLD(0.845f),
    DTV_LUMA_SCALE_OLD(0.854f)
};

/* dtv color names */
static char *dtv_color_names[16] =
{
    "Chroma0",
    "Chroma1",
    "Chroma2",
    "Chroma3",
    "Chroma4",
    "Chroma5",
    "Chroma6",
    "Chroma7",
    "Chroma8",
    "Chroma9",
    "ChromaA",
    "ChromaB",
    "ChromaC",
    "ChromaD",
    "ChromaE",
    "ChromaF"
};

static video_cbm_color_t dtv_colors_with_lum[VICIIDTV_NUM_COLORS];

static video_cbm_palette_t dtv_palette =
{
    VICIIDTV_NUM_COLORS,
    dtv_colors_with_lum,
    VICII_SATURATION,
    VICII_PHASE
};

#define DTV_STARTING_PHASE  180.0f
#define DTV_PHASE_DECREMENT 22.5f

int vicii_color_update_palette(struct video_canvas_s *canvas)
{
    int lum, col, cl;
    float *lm;
    float an;

    if (vicii_resources.new_luminances) {
        lm = dtv_luminances;
    } else {
        lm = dtv_luminances_old;
    }

    an = DTV_STARTING_PHASE;
    cl = 0;
    for (col = 0; col < 16; col++) {
        for (lum = 0; lum < 16; lum++) {
            dtv_colors_with_lum[cl].luminance = lm[lum];
            dtv_colors_with_lum[cl].angle = an;
            dtv_colors_with_lum[cl].direction = (col == 0) ? 0 : 1;
            dtv_colors_with_lum[cl].name = dtv_color_names[col];
            cl++;
        }
        an -= DTV_PHASE_DECREMENT;
        if (an < 0.0f) {
            an += 360.0f;
        }
    }

    video_color_palette_internal(canvas, &dtv_palette);
    return 0;
}
