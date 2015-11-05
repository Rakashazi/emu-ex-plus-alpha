/*
 * ted-color.c - Colors for the TED emulation.
 *
 * Written by
 *  John Selck <graham@cruise.de>
 *
 * Research about the YUV values by
 *  Philip Timmermann <pepto@pepto.de>
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

#include "tedtypes.h"
#include "ted-color.h"
#include "ted-resources.h"
#include "video.h"


/* base saturation of all colors except the grey tones */

/* must stay below 64 to not result in overflows in the CRT renderer (and maybe
   elsewhere) */
#define TED_SATURATION  48.0f

/* phase shift of all colors */

#define TED_PHASE       -4.5f

/* chroma angles in UV space */

#define ANGLE_RED        112.5f
#define ANGLE_GRN       -135.0f
#define ANGLE_BLU          0.0f
#define ANGLE_ORN        -45.0f /* negative orange (orange is at +135.0 degree) */
#define ANGLE_BRN        157.5f

#define ANGLE_YLG       -168.75f
#define ANGLE_PNK         78.75f
#define ANGLE_BLG       -101.25f
#define ANGLE_LBL        -22.5f
#define ANGLE_DBL         11.25f
#define ANGLE_LGN       -157.5f

/* luminances */

static float ted_luminances[8] =
{
    40.0f,
    56.0f,
    64.0f,
    80.0f,
    128.0f,
    160.0f,
    192.0f,
    256.0f
};

/* the base ted palette without luminances */

static video_cbm_color_t ted_colors[16] =
{
    { 0.0f, ANGLE_ORN, -0, "Black"       },
    { 0.0f, ANGLE_BRN,  0, "White"       },
    { 0.0f, ANGLE_RED,  1, "Red"         },
    { 0.0f, ANGLE_RED, -1, "Cyan"        },
    { 0.0f, ANGLE_GRN, -1, "Purple"      },
    { 0.0f, ANGLE_GRN,  1, "Green"       },
    { 0.0f, ANGLE_BLU,  1, "Blue"        },
    { 0.0f, ANGLE_BLU, -1, "Yellow"      },
    { 0.0f, ANGLE_ORN, -1, "Orange"      },
    { 0.0f, ANGLE_BRN,  1, "Brown"       },
    { 0.0f, ANGLE_YLG,  1, "Yellow-Green"},
    { 0.0f, ANGLE_PNK,  1, "Pink"        },
    { 0.0f, ANGLE_BLG,  1, "Blue-Green"  },
    { 0.0f, ANGLE_LBL,  1, "Light Blue"  },
    { 0.0f, ANGLE_DBL,  1, "Dark Blue"   },
    { 0.0f, ANGLE_LGN,  1, "Light Green" }
};

static video_cbm_color_t ted_colors_with_lum[TED_NUM_COLORS];

static video_cbm_palette_t ted_palette =
{
    TED_NUM_COLORS,
    ted_colors_with_lum,
    TED_SATURATION,
    TED_PHASE,
    CBM_PALETTE_YUV
};

int ted_color_update_palette(struct video_canvas_s *canvas)
{
    int col, lum, cl;
    float tedlum;
    video_cbm_color_t *vc;

    cl = 0;
    for (lum = 0; lum < 8; lum++) {
        tedlum = ted_luminances[lum] * 0.867f;
        for (col = 0; col < 16; col++) {
            vc = &ted_colors_with_lum[cl];
            if (col) {
                vc->luminance = tedlum;
            } else {
                vc->luminance = 0.0f;
            }
            vc->angle = ted_colors[col].angle;
            vc->direction = ted_colors[col].direction;
            vc->name = ted_colors[col].name;
            cl++;
        }
    }

    video_color_palette_internal(canvas, &ted_palette);
    return 0;
}
