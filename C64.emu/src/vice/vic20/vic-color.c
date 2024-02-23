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

/* uncomment for the old "idealized" Pepto colors */
/* #define PEPTO_COLORS */

/* uncomment for the new "idealized" Pepto colors, aka "colodore" */
/* #define COLODORE_COLORS */

/* uncomment for the colors measured by Tobias */
#define TOBIAS_COLORS

/* uncomment to use seperate palettes for odd/even lines */
#define SEPERATE_ODD_EVEN_COLORS

/******************************************************************************/

/* base saturation of all colors */

/* must stay below 64 to not result in overflows in the CRT renderer (and maybe
   elsewhere). especially the NTSC mode seems to be an edge case */
#define VIC_SATURATION    48.0f

/* phase shift of all colors */

#define VIC_PHASE         -9.5f

/* chroma angles in UV space */

#define ANGLE_RED        112.5f
/* #define ANGLE_GRN       -135.0f */ /* old pepto */
#define ANGLE_GRN       -112.5f /* new pepto ("colodore") */
#define ANGLE_BLU          0.0f
#define ANGLE_ORN        -45.0f /* negative orange (orange is at +135.0 degree) */

#ifdef PEPTO_COLORS
#define LUMA0      0.0f
#define LUMA1    256.0f
#define LUMA2     51.0f
#define LUMA3    157.0f
#define LUMA4     75.0f
#define LUMA5    132.0f
#define LUMA6     47.0f
#define LUMA7    183.0f
#define LUMA8     85.0f
#define LUMA9    161.0f
#define LUMA10   144.0f
#define LUMA11   208.0f
#define LUMA12   158.0f
#define LUMA13   191.0f
#define LUMA14   129.0f
#define LUMA15   234.0f
#endif

/* new pepto ("colodore") */
#define LUMA0      0.0f
#define LUMA1    256.0f
#define LUMA2     64.0f
#define LUMA3    192.0f
#define LUMA4     96.0f
#define LUMA5    160.0f
#define LUMA6     56.0f
#define LUMA7    208.0f
#define LUMA8    112.0f
#define LUMA9    184.0f
#define LUMA10   168.0f
#define LUMA11   224.0f
#define LUMA12   184.0f
#define LUMA13   216.0f
#define LUMA14   152.0f
#define LUMA15   240.0f

#ifdef PEPTO_COLORS
static video_cbm_color_t vic_colors_pal[VIC_NUM_COLORS] =
{
    { LUMA0 , ANGLE_ORN, VIC_SATURATION, -0, "Black"       },
    { LUMA1 , ANGLE_ORN, VIC_SATURATION, -0, "White"       },
    { LUMA2 , ANGLE_RED, VIC_SATURATION,  1, "Red"         },
    { LUMA3 , ANGLE_RED, VIC_SATURATION, -1, "Cyan"        },
    { LUMA4 , ANGLE_GRN, VIC_SATURATION, -1, "Purple"      },
    { LUMA5 , ANGLE_GRN, VIC_SATURATION,  1, "Green"       },
    { LUMA6 , ANGLE_BLU, VIC_SATURATION,  1, "Blue"        },
    { LUMA7 , ANGLE_BLU, VIC_SATURATION, -1, "Yellow"      },
    { LUMA8 , ANGLE_ORN, VIC_SATURATION, -1, "Orange"      },
    { LUMA9 , ANGLE_ORN, VIC_SATURATION, -1, "Light Orange"},
    { LUMA10, ANGLE_RED, VIC_SATURATION,  1, "Pink"        },
    { LUMA11, ANGLE_RED, VIC_SATURATION, -1, "Light Cyan"  },
    { LUMA12, ANGLE_GRN, VIC_SATURATION, -1, "Light Purple"},
    { LUMA13, ANGLE_GRN, VIC_SATURATION,  1, "Light Green" },
    { LUMA14, ANGLE_BLU, VIC_SATURATION,  1, "Light Blue"  },
    { LUMA15, ANGLE_BLU, VIC_SATURATION, -1, "Light Yellow"}
};

static video_cbm_palette_t vic_palette_pal =
{
    VIC_NUM_COLORS,
    vic_colors_pal,
    NULL, NULL,
    VIC_PHASE,
    CBM_PALETTE_YUV
};
#endif

/* FIXME: the following is hand-tuned to somehow match the reference pictures. it
          is not necessarily correct and should get backed up by measurements. */

/* "The 'normal' colors (2-8) have a phase error on NTSC compared to their lighter
   variations (9-15) - this is due to how the color signal is generated on the chip. */

#define NTSC_PHASE_ERROR    30.0f

static video_cbm_color_t vic_colors_ntsc[VIC_NUM_COLORS] =
{
    { LUMA0 , ANGLE_ORN,                    VIC_SATURATION, -0, "Black"       },
    { LUMA1 , ANGLE_ORN,                    VIC_SATURATION, -0, "White"       },

    { LUMA2 , ANGLE_RED + NTSC_PHASE_ERROR, VIC_SATURATION,  1, "Red"         },
    { LUMA3 , ANGLE_RED + NTSC_PHASE_ERROR, VIC_SATURATION, -1, "Cyan"        },
    { LUMA4 , ANGLE_GRN + NTSC_PHASE_ERROR, VIC_SATURATION, -1, "Purple"      },
    { LUMA5 , ANGLE_GRN + NTSC_PHASE_ERROR, VIC_SATURATION,  1, "Green"       },
    { LUMA6 , ANGLE_BLU + NTSC_PHASE_ERROR, VIC_SATURATION,  1, "Blue"        },
    { LUMA7 , ANGLE_BLU + NTSC_PHASE_ERROR, VIC_SATURATION, -1, "Yellow"      },
    { LUMA8 , ANGLE_ORN + NTSC_PHASE_ERROR, VIC_SATURATION, -1, "Orange"      },

    { LUMA9 , ANGLE_ORN,                    VIC_SATURATION, -1, "Light Orange"},
    { LUMA10, ANGLE_RED,                    VIC_SATURATION,  1, "Pink"        },
    { LUMA11, ANGLE_RED,                    VIC_SATURATION, -1, "Light Cyan"  },
    { LUMA12, ANGLE_GRN,                    VIC_SATURATION, -1, "Light Purple"},
    { LUMA13, ANGLE_GRN,                    VIC_SATURATION,  1, "Light Green" },
    { LUMA14, ANGLE_BLU,                    VIC_SATURATION,  1, "Light Blue"  },
    { LUMA15, ANGLE_BLU,                    VIC_SATURATION, -1, "Light Yellow"}
};

static video_cbm_palette_t vic_palette_ntsc =
{
    VIC_NUM_COLORS,
    vic_colors_ntsc,
    NULL, NULL,
    VIC_PHASE,
    CBM_PALETTE_YUV
};

/******************************************************************************/

#ifdef TOBIAS_COLORS
static video_cbm_color_t vic_colors_6561_101[VIC_NUM_COLORS] =
{
    { 0.000 * 256.0,           0.00        , 0.000 * 256.0, -0, "Black"        },
    { 1.000 * 256.0,           0.00        , 0.000 * 256.0, -0, "White"        },
    { 0.295 * 256.0,         102.13        , 0.272 * 256.0,  1, "Red"          },
    { 0.738 * 256.0,         282.00 - 180.0, 0.317 * 256.0, -1, "Cyan"         },
    { 0.428 * 256.0,          54.83 - 180.0, 0.274 * 256.0, -1, "Purple"       },
    { 0.650 * 256.0,         238.95        , 0.304 * 256.0,  1, "Green"        },
    { 0.273 * 256.0,         355.48 - 360.0, 0.223 * 256.0,  1, "Blue"         },
    { 0.815 * 256.0,         174.35 - 180.0, 0.268 * 256.0, -1, "Yellow"       },
    { 0.480 * 256.0,         126.75 - 180.0, 0.280 * 256.0, -1, "Orange"       },
    { 0.758 * 256.0,         126.75 - 180.0, 0.140 * 256.0, -1, "Light Orange" },
    { 0.705 * 256.0,         102.13        , 0.136 * 256.0,  1, "Pink"         },
    { 0.888 * 256.0,         282.00 - 180.0, 0.158 * 256.0, -1, "Light Cyan"   },
    { 0.748 * 256.0,          54.83 - 180.0, 0.137 * 256.0, -1, "Light Purple" },
    { 0.845 * 256.0,         238.95 - 360.0, 0.152 * 256.0,  1, "Light Green"  },
    { 0.660 * 256.0,         355.48 - 360.0, 0.112 * 256.0,  1, "Light Blue"   },
    { 0.943 * 256.0,         174.35 - 180.0, 0.134 * 256.0, -1, "Light Yellow" }
};

static video_cbm_color_t vic_colors_6561_101_odd[VIC_NUM_COLORS] =
{
    { 0.0000 * 256.0,           0.00        , 0.000 * 256.0, -0, "Black"        },
    { 1.0000 * 256.0,           0.00        , 0.000 * 256.0, -0, "White"        },
    { 0.2950 * 256.0,          94.25        , 0.257 * 256.0,  1, "Red"          },
    { 0.7375 * 256.0,         267.25 - 180.0, 0.302 * 256.0, -1, "Cyan"         },
    { 0.4275 * 256.0,          43.20 - 180.0, 0.267 * 256.0, -1, "Purple"       },
    { 0.6500 * 256.0,         223.20        , 0.312 * 256.0,  1, "Green"        },
    { 0.2725 * 256.0,         339.60 - 360.0, 0.253 * 256.0,  1, "Blue"         },
    { 0.8150 * 256.0,         168.60 - 180.0, 0.313 * 256.0, -1, "Yellow"       },
    { 0.4800 * 256.0,         120.25 - 180.0, 0.280 * 256.0, -1, "Orange"       },
    { 0.7575 * 256.0,         150.25 - 180.0, 0.140 * 256.0, -1, "Light Orange" },
    { 0.7050 * 256.0,         124.25        , 0.128 * 256.0,  1, "Pink"         },
    { 0.8875 * 256.0,         297.25 - 180.0, 0.151 * 256.0, -1, "Light Cyan"   },
    { 0.7475 * 256.0,          73.20 - 180.0, 0.133 * 256.0, -1, "Light Purple" },
    { 0.8450 * 256.0,         253.20 - 360.0, 0.156 * 256.0,  1, "Light Green"  },
    { 0.6600 * 256.0,         369.60 - 360.0, 0.127 * 256.0,  1, "Light Blue"   },
    { 0.9425 * 256.0,         198.60 - 180.0, 0.157 * 256.0, -1, "Light Yellow" }
};

static video_cbm_color_t vic_colors_6561_101_even[VIC_NUM_COLORS] =
{
    { 0.000 * 256.0,           0.00        , 0.000 * 256.0, -0, "Black"        },
    { 1.000 * 256.0,           0.00        , 0.000 * 256.0, -0, "White"        },
    { 0.295 * 256.0,         110.00        , 0.287 * 256.0,  1, "Red"          },
    { 0.738 * 256.0,         296.75 - 180.0, 0.332 * 256.0, -1, "Cyan"         },
    { 0.428 * 256.0,          66.45 - 180.0, 0.282 * 256.0, -1, "Purple"       },
    { 0.650 * 256.0,         254.70        , 0.297 * 256.0,  1, "Green"        },
    { 0.273 * 256.0,         371.35 - 360.0, 0.193 * 256.0,  1, "Blue"         },
    { 0.815 * 256.0,         180.10 - 180.0, 0.223 * 256.0, -1, "Yellow"       },
    { 0.480 * 256.0,         133.25 - 180.0, 0.280 * 256.0, -1, "Orange"       },
    { 0.758 * 256.0,         103.25 - 180.0, 0.140 * 256.0, -1, "Light Orange" },
    { 0.705 * 256.0,          80.00        , 0.143 * 256.0,  1, "Pink"         },
    { 0.888 * 256.0,         266.75 - 180.0, 0.166 * 256.0, -1, "Light Cyan"   },
    { 0.748 * 256.0,          36.45 - 180.0, 0.141 * 256.0, -1, "Light Purple" },
    { 0.845 * 256.0,         224.70 - 360.0, 0.148 * 256.0,  1, "Light Green"  },
    { 0.660 * 256.0,         341.35 - 360.0, 0.097 * 256.0,  1, "Light Blue"   },
    { 0.943 * 256.0,         150.10 - 180.0, 0.112 * 256.0, -1, "Light Yellow" }
};

static video_cbm_palette_t vic_palette_6561_101 =
{
    VIC_NUM_COLORS,
    vic_colors_6561_101,
#ifdef SEPERATE_ODD_EVEN_COLORS
    vic_colors_6561_101_odd,
    vic_colors_6561_101_even,
#else
    NULL, NULL,
#endif
    0.0,
    CBM_PALETTE_YUV
};
#endif

/******************************************************************************/

int vic_color_update_palette(struct video_canvas_s *canvas)
{
    video_cbm_palette_t *cp;
    int sync;

    /* first setup the fallback default */
#if defined(PEPTO_COLORS) || defined (COLODORE_COLORS)
    cp = &vic_palette_pal;
#endif
#ifdef TOBIAS_COLORS
    cp = &vic_palette_6561_101;
#endif

    if (resources_get_int("MachineVideoStandard", &sync) < 0) {
        sync = MACHINE_SYNC_PAL;
    }

    switch (sync) {
        case MACHINE_SYNC_PAL: /* VIC_MODEL_PALG */
#if defined(PEPTO_COLORS) || defined (COLODORE_COLORS)
            cp = &vic_palette_pal;
#endif
#ifdef TOBIAS_COLORS
            cp = &vic_palette_6561_101;
#endif
            break;
        case MACHINE_SYNC_NTSC: /* VIC_MODEL_NTSCM */
            cp = &vic_palette_ntsc;
            break;
        default:
            log_error(LOG_DEFAULT, "unknown VIC type.");
            break;
    }

    video_color_palette_internal(canvas, cp);
    return 0;
}
