/*
 * vicii-color.c - Colors for the MOS 6569 (VIC-II) emulation.
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
#include "vicii.h"
#include "viciitypes.h"
#include "vicii-color.h"
#include "vicii-resources.h"
#include "video.h"


/* base saturation of all colors except the grey tones */

/* must stay below 64 to not result in overflows in the CRT renderer (and maybe
   elsewhere) */
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

/* default dithering */

/*
static char vicii_color_dither[16] =
{
    0x00,0x0E,0x04,0x0C,
    0x08,0x04,0x04,0x0C,
    0x04,0x04,0x08,0x04,
    0x08,0x08,0x08,0x0C
};
*/

/*
  6567R56A (NTSC old)
  Voltage (ripple)
1  560 (495-640)
2  840 (780-920)
3 1180 (1100-1240)
4 1500 (1440-1600)
5 1825 (1775-1900)

  6567R8 (NTSC)
  Voltage (ripple)
1  590 (570-630)
2  860 (840-880)
3  950 (920-980)
4 1030 (1010-1050)
5 1160 (1140-1200)
6 1210 (1190-1230)
7 1380 (1360-1400)
8 1560 (1540-1580)
9 1825

   6569R1 (PAL)
   Voltage (ripple)
1   630 (590-680)
2   900 (860-960)
3  1260 (1200-1320)
4  1560 (1500-1620)
5  1850

  6569R3 (PAL)       6569R4 (PAL)       6569R5 (PAL)
  Voltage (ripple)
1 700 (680-720)       500 (480-520)     540 (520-560)
2 1020 (990-1030)     760 (740-780)     810 (790-830)
3 1090 (1070-1110)    840 (820-860)     900 (880-920)
4 1180 (1160-1200)    920 (900-940)     980 (960-1000)
5 1300 (1280-1320)   1050 (1030-1070)   1110 (1090-1130)
6 1340 (1320-1360)   1100 (1080-1120)   1150 (1130-1170)
7 1480 (1460-1500)   1300 (1280-1320)   1340 (1320-1360)
8 1620 (1600-1640)   1500 (1480-1520)   1520 (1500-1540)
9 1850               1875               1850

FIXME: measurements missing for: 8565 (PAL), 6562 (NTSC), 6572 (PAL-N)

 */

/******************************************************************************/

#define LUMA(x,y,z) ((((float)x-y)*256.0f)/(z-y))

static video_cbm_color_t vicii_colors_6567r56a[VICII_NUM_COLORS] =
{
    { LUMA( 560,560,1825), ANGLE_ORN, -0, "Black"       },
    { LUMA(1825,560,1825), ANGLE_BRN,  0, "White"       },
    { LUMA( 840,560,1825), ANGLE_RED,  1, "Red"         },
    { LUMA(1500,560,1825), ANGLE_RED, -1, "Cyan"        },
    { LUMA(1180,560,1825), ANGLE_GRN, -1, "Purple"      },
    { LUMA(1180,560,1825), ANGLE_GRN,  1, "Green"       },
    { LUMA( 840,560,1825), ANGLE_BLU,  1, "Blue"        },
    { LUMA(1500,560,1825), ANGLE_BLU, -1, "Yellow"      },
    { LUMA(1180,560,1825), ANGLE_ORN, -1, "Orange"      },
    { LUMA( 840,560,1825), ANGLE_BRN,  1, "Brown"       },
    { LUMA(1180,560,1825), ANGLE_RED,  1, "Light Red"   },
    { LUMA( 840,560,1825), ANGLE_RED, -0, "Dark Grey"   },
    { LUMA(1180,560,1825), ANGLE_GRN, -0, "Medium Grey" },
    { LUMA(1500,560,1825), ANGLE_GRN,  1, "Light Green" },
    { LUMA(1180,560,1825), ANGLE_BLU,  1, "Light Blue"  },
    { LUMA(1500,560,1825), ANGLE_BLU, -0, "Light Grey"  }
};

static video_cbm_color_t vicii_colors_6567r8[VICII_NUM_COLORS] =
{
    { LUMA( 590,590,1825), ANGLE_ORN, -0, "Black"       },
    { LUMA(1825,590,1825), ANGLE_BRN,  0, "White"       },
    { LUMA( 950,590,1825), ANGLE_RED,  1, "Red"         },
    { LUMA(1380,590,1825), ANGLE_RED, -1, "Cyan"        },
    { LUMA(1030,590,1825), ANGLE_GRN, -1, "Purple"      },
    { LUMA(1210,590,1825), ANGLE_GRN,  1, "Green"       },
    { LUMA( 860,590,1825), ANGLE_BLU,  1, "Blue"        },
    { LUMA(1560,590,1825), ANGLE_BLU, -1, "Yellow"      },
    { LUMA(1030,590,1825), ANGLE_ORN, -1, "Orange"      },
    { LUMA( 860,590,1825), ANGLE_BRN,  1, "Brown"       },
    { LUMA(1210,590,1825), ANGLE_RED,  1, "Light Red"   },
    { LUMA( 950,590,1825), ANGLE_RED, -0, "Dark Grey"   },
    { LUMA(1160,590,1825), ANGLE_GRN, -0, "Medium Grey" },
    { LUMA(1560,590,1825), ANGLE_GRN,  1, "Light Green" },
    { LUMA(1160,590,1825), ANGLE_BLU,  1, "Light Blue"  },
    { LUMA(1380,590,1825), ANGLE_BLU, -0, "Light Grey"  }
};

static video_cbm_color_t vicii_colors_6569r1[VICII_NUM_COLORS] =
{
    { LUMA( 630,630,1850), ANGLE_ORN, -0, "Black"       },
    { LUMA(1850,630,1850), ANGLE_BRN,  0, "White"       },
    { LUMA( 900,630,1850), ANGLE_RED,  1, "Red"         },
    { LUMA(1560,630,1850), ANGLE_RED, -1, "Cyan"        },
    { LUMA(1260,630,1850), ANGLE_GRN, -1, "Purple"      },
    { LUMA(1260,630,1850), ANGLE_GRN,  1, "Green"       },
    { LUMA( 900,630,1850), ANGLE_BLU,  1, "Blue"        },
    { LUMA(1560,630,1850), ANGLE_BLU, -1, "Yellow"      },
    { LUMA(1260,630,1850), ANGLE_ORN, -1, "Orange"      },
    { LUMA( 900,630,1850), ANGLE_BRN,  1, "Brown"       },
    { LUMA(1260,630,1850), ANGLE_RED,  1, "Light Red"   },
    { LUMA( 900,630,1850), ANGLE_RED, -0, "Dark Grey"   },
    { LUMA(1260,630,1850), ANGLE_GRN, -0, "Medium Grey" },
    { LUMA(1560,630,1850), ANGLE_GRN,  1, "Light Green" },
    { LUMA(1260,630,1850), ANGLE_BLU,  1, "Light Blue"  },
    { LUMA(1560,630,1850), ANGLE_BLU, -0, "Light Grey"  }
};

static video_cbm_color_t vicii_colors_6569r3[VICII_NUM_COLORS] =
{
    { LUMA( 700,700,1850), ANGLE_ORN, -0, "Black"       },
    { LUMA(1850,700,1850), ANGLE_BRN,  0, "White"       },
    { LUMA(1090,700,1850), ANGLE_RED,  1, "Red"         },
    { LUMA(1480,700,1850), ANGLE_RED, -1, "Cyan"        },
    { LUMA(1180,700,1850), ANGLE_GRN, -1, "Purple"      },
    { LUMA(1340,700,1850), ANGLE_GRN,  1, "Green"       },
    { LUMA(1020,700,1850), ANGLE_BLU,  1, "Blue"        },
    { LUMA(1620,700,1850), ANGLE_BLU, -1, "Yellow"      },
    { LUMA(1180,700,1850), ANGLE_ORN, -1, "Orange"      },
    { LUMA(1020,700,1850), ANGLE_BRN,  1, "Brown"       },
    { LUMA(1340,700,1850), ANGLE_RED,  1, "Light Red"   },
    { LUMA(1090,700,1850), ANGLE_RED, -0, "Dark Grey"   },
    { LUMA(1300,700,1850), ANGLE_GRN, -0, "Medium Grey" },
    { LUMA(1620,700,1850), ANGLE_GRN,  1, "Light Green" },
    { LUMA(1300,700,1850), ANGLE_BLU,  1, "Light Blue"  },
    { LUMA(1480,700,1850), ANGLE_BLU, -0, "Light Grey"  }
};

/******************************************************************************/

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

/* old luminances */
#define LUMO0     0.0f
#define LUMO1    56.0f
#define LUMO2   128.0f
#define LUMO3   191.0f
#define LUMO4   256.0f

/* very old vic-ii palette with less luminances */
#if 0
static video_cbm_color_t vicii_colors_old[VICII_NUM_COLORS] =
{
    { LUMO0, ANGLE_ORN, -0, "Black"       },
    { LUMO4, ANGLE_BRN,  0, "White"       },
    { LUMO1, ANGLE_RED,  1, "Red"         },
    { LUMO3, ANGLE_RED, -1, "Cyan"        },
    { LUMO2, ANGLE_GRN, -1, "Purple"      },
    { LUMO2, ANGLE_GRN,  1, "Green"       },
    { LUMO1, ANGLE_BLU,  1, "Blue"        },
    { LUMO3, ANGLE_BLU, -1, "Yellow"      },
    { LUMO2, ANGLE_ORN, -1, "Orange"      },
    { LUMO1, ANGLE_BRN,  1, "Brown"       },
    { LUMO2, ANGLE_RED,  1, "Light Red"   },
    { LUMO1, ANGLE_RED, -0, "Dark Grey"   },
    { LUMO2, ANGLE_GRN, -0, "Medium Grey" },
    { LUMO3, ANGLE_GRN,  1, "Light Green" },
    { LUMO2, ANGLE_BLU,  1, "Light Blue"  },
    { LUMO3, ANGLE_BLU, -0, "Light Grey"  }
};
#endif
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

/******************************************************************************/

static video_cbm_palette_t vicii_palette_6567r56a =
{
    VICII_NUM_COLORS,
    vicii_colors_6567r56a,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

static video_cbm_palette_t vicii_palette_6567r8 =
{
    VICII_NUM_COLORS,
    vicii_colors_6567r8,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

static video_cbm_palette_t vicii_palette_6569r1 =
{
    VICII_NUM_COLORS,
    vicii_colors_6569r1,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

static video_cbm_palette_t vicii_palette_6569r3 =
{
    VICII_NUM_COLORS,
    vicii_colors_6569r3,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

#if 0
static video_cbm_palette_t vicii_palette_old =
{
    VICII_NUM_COLORS,
    vicii_colors_old,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};
#endif
static video_cbm_palette_t vicii_palette =
{
    VICII_NUM_COLORS,
    vicii_colors,
    VICII_SATURATION,
    VICII_PHASE,
    CBM_PALETTE_YUV
};

/******************************************************************************/

int vicii_color_update_palette(struct video_canvas_s *canvas)
{
    video_cbm_palette_t *cp;
    switch (vicii_resources.model) {
#if 1 /* comment out if you want pepto-style lumas for testing */
        case VICII_MODEL_6567R56A:      /* NTSC, 64 cycle, ? luma, "old" */
            cp = &vicii_palette_6567r56a;
            break;
        case VICII_MODEL_6567:          /* NTSC, 65 cycle, 9 luma, "old" */
            cp = &vicii_palette_6567r8;
            break;
        case VICII_MODEL_6569R1:        /* PAL, 63 cycle, 5 luma, "old" */
            cp = &vicii_palette_6569r1;
            break;
        case VICII_MODEL_6569:          /* PAL, 63 cycle, 9 luma, "old" */
            cp = &vicii_palette_6569r3;
            break;
        case VICII_MODEL_6572:          /* PAL-N, 65 cycle, ? luma, "?" */
            cp = &vicii_palette; /* FIXME: measurement missing */
            break;
        case VICII_MODEL_8565:          /* PAL, 63 cycle, 9 luma, "new" */
            cp = &vicii_palette; /* FIXME: measurement missing */
            break;
        case VICII_MODEL_8562:          /* NTSC, 65 cycle, 9 luma, "new" */
            cp = &vicii_palette; /* FIXME: measurement missing */
            break;
#endif
        default:
            cp = &vicii_palette; /* FIXME */
            log_error(LOG_DEFAULT, "vicii_color_update_palette: unknown VICII type.");
            break;
    }

    video_color_palette_internal(canvas, cp);
    return 0;
}
