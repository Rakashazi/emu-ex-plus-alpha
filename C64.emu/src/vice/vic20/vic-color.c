/*
 * vic-color.c - Colors for the VIC-I emulation.
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

#include "log.h"
#include "machine.h"
#include "resources.h"
#include "vic-color.h"
#include "vic.h"
#include "victypes.h"
#include "video.h"


/* base saturation of all colors */

/* must stay below 64 to not result in overflows in the CRT renderer (and maybe
   elsewhere). especially the NTSC mode seems to be an edge case */
#define VIC_SATURATION    63.0f

/* phase shift of all colors */

#define VIC_PHASE         -9.5f

/* chroma angles in UV space */

#define ANGLE_RED        112.5f
#define ANGLE_GRN       -135.0f
#define ANGLE_BLU          0.0f
#define ANGLE_ORN        -45.0f /* negative orange (orange is at +135.0 degree) */

/*
    In theory it would make sense that there are only 5 brightness
    levels like on VICII, however for some reason in practice it
    looks differently. Measuring the actual luma signal level
    would clear things up.

    http://sourceforge.net/tracker/?func=detail&atid=1057619&aid=3542105&group_id=223021
*/

static video_cbm_color_t vic_colors_pal[VIC_NUM_COLORS] =
{
    {   0.0f, ANGLE_ORN, -0, "Black"       },
    { 256.0f, ANGLE_ORN, -0, "White"       },
    {  51.0f, ANGLE_RED,  1, "Red"         },
    { 157.0f, ANGLE_RED, -1, "Cyan"        },
    {  75.0f, ANGLE_GRN, -1, "Purple"      },
    { 132.0f, ANGLE_GRN,  1, "Green"       },
    {  47.0f, ANGLE_BLU,  1, "Blue"        },
    { 183.0f, ANGLE_BLU, -1, "Yellow"      },
    {  85.0f, ANGLE_ORN, -1, "Orange"      },
    { 161.0f, ANGLE_ORN, -1, "Light Orange"},
    { 144.0f, ANGLE_RED,  1, "Pink"        },
    { 208.0f, ANGLE_RED, -1, "Light Cyan"  },
    { 158.0f, ANGLE_GRN, -1, "Light Purple"},
    { 191.0f, ANGLE_GRN,  1, "Light Green" },
    { 129.0f, ANGLE_BLU,  1, "Light Blue"  },
    { 234.0f, ANGLE_BLU, -1, "Light Yellow"}
};

/* FIXME: the following is hand-tuned to somehow match mikes/tokras palette. it
          is not necessarily correct and should get backed up by measurements. */
static video_cbm_color_t vic_colors_ntsc[VIC_NUM_COLORS] =
{
    {   0.0f, ANGLE_ORN, -0, "Black"       },
    { 256.0f, ANGLE_ORN, -0, "White"       },
    {  51.0f, ANGLE_RED,  1, "Red"         },
    { 157.0f, ANGLE_RED, -1, "Cyan"        },
    {  75.0f, ANGLE_GRN + 35.0f, -1, "Purple"      },
    { 132.0f, ANGLE_GRN + 45.0f,  1, "Green"       },
    {  47.0f, ANGLE_BLU,  1, "Blue"        },
    { 183.0f, ANGLE_BLU, -1, "Yellow"      },
    {  85.0f, ANGLE_ORN, -1, "Orange"      },
    { 161.0f, ANGLE_ORN - 30.0f, -1, "Light Orange"},
    { 144.0f, ANGLE_RED - 30.0f,  1, "Pink"        },
    { 208.0f, ANGLE_RED, -1, "Light Cyan"  },
    { 158.0f, ANGLE_GRN, -1, "Light Purple"},
    { 191.0f, ANGLE_GRN + 30.0f,  1, "Light Green" },
    { 129.0f, ANGLE_BLU,  1, "Light Blue"  },
    { 234.0f, ANGLE_BLU, -1, "Light Yellow"}
};

static video_cbm_palette_t vic_palette =
{
    VIC_NUM_COLORS,
    vic_colors_pal,
    VIC_SATURATION,
    VIC_PHASE,
    CBM_PALETTE_YUV
};

int vic_color_update_palette(struct video_canvas_s *canvas)
{
    int sync;

    if (resources_get_int("MachineVideoStandard", &sync) < 0) {
        sync = MACHINE_SYNC_PAL;
    }

    switch (sync) {
        case MACHINE_SYNC_PAL: /* VIC_MODEL_PALG */
            vic_palette.entries = vic_colors_pal;
            break;
        case MACHINE_SYNC_NTSC: /* VIC_MODEL_NTSCM */
            vic_palette.entries = vic_colors_ntsc;
            break;
        default:
            vic_palette.entries = vic_colors_pal; /* FIXME */
            log_error(LOG_DEFAULT, "unknown VIC type.");
            break;
    }

    video_color_palette_internal(canvas, &vic_palette);
    return 0;
}
