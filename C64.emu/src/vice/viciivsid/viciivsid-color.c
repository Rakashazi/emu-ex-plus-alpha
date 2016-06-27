/*
 * viciivsid-color.c - Colors for the MOS 6569 (VIC-II) emulation.
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

#include "viciitypes.h"
#include "vicii-color.h"
#include "vicii-resources.h"
#include "video.h"


/* base saturation of all colors except the grey tones */

#define VICII_SATURATION 48.0f

/* phase shift of all colors */

#define VICII_PHASE -4.5f

/* chroma angles in UV space */

#define ANGLE_RED        112.5f
#define ANGLE_GRN       -135.0f
#define ANGLE_BLU          0.0f
#define ANGLE_ORN        -45.0f /* negative orange (orange is at +135.0 degree)
*/
#define ANGLE_BRN        157.5f

/* new luminances */

#define LUMN0     0.0f
#define LUMN1    56.0f
#define LUMN2    74.0f
#define LUMN3    92.0f
#define LUMN4   117.0f
#define LUMN5   128.0f
#define LUMN6   163.0f
#define LUMN7   199.0f
#define LUMN8   256.0f

/* the wellknown vic-ii palette used for 99% of all vic-ii chips */

static video_cbm_color_t vicii_colors[VICII_NUM_COLORS] =
{
    { LUMN0, ANGLE_ORN, -0, "Black"       },
    { LUMN8, ANGLE_BRN,  0, "White"       },
    { LUMN2, ANGLE_RED,  1, "Red"         },
    { LUMN6, ANGLE_RED, -1, "Cyan"        },
    { LUMN3, ANGLE_GRN, -1, "Purple"      },
    { LUMN5, ANGLE_GRN,  1, "Green"       },
    { LUMN1, ANGLE_BLU,  1, "Blue"        },
    { LUMN7, ANGLE_BLU, -1, "Yellow"      },
    { LUMN3, ANGLE_ORN, -1, "Orange"      },
    { LUMN1, ANGLE_BRN,  1, "Brown"       },
    { LUMN5, ANGLE_RED,  1, "Light Red"   },
    { LUMN2, ANGLE_RED, -0, "Dark Grey"   },
    { LUMN4, ANGLE_GRN, -0, "Medium Grey" },
    { LUMN7, ANGLE_GRN,  1, "Light Green" },
    { LUMN4, ANGLE_BLU,  1, "Light Blue"  },
    { LUMN6, ANGLE_BLU, -0, "Light Grey"  }
};

static video_cbm_palette_t vicii_palette =
{
    VICII_NUM_COLORS,
    vicii_colors,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

int vicii_color_update_palette(struct video_canvas_s *canvas)
{
    video_cbm_palette_t *cp = &vicii_palette;

    video_color_palette_internal(canvas, cp);
    return 0;
}
