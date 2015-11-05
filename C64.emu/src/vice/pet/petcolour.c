/*
 * petcolour.c
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
 *
 *  This implements the 2-chip colour board for the Universal PET
 *  mainboard (i.e. model 4032) as designed by Steve Gray:
 *  http://www.6502.org/users/sjgray/projects/colourpet/index.html
 *  The hardware is a work-in-progress.
 *
 *  It is not compatible with other display extensions
 *  (DWW hires, HRE hires).
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

#include "crtc.h"
#include "crtc-color.h"
#include "crtctypes.h"
#include "pet.h"
#include "petcolour.h"
#include "petmem.h"
#include "pets.h"
#include "pet-resources.h"
#include "video.h"

/*
 * These "draw" functions don't really draw anything in the usual sense
 * of the word.  They look at the already drawn bits (generated from the
 * character set) and, assuming it is the usual monochrome, replace them
 * by coloured pixels.
 */
static void DRAW_rgbi(BYTE *p, int xstart, int xend, int scr_rel, int ymod8)
{
    if (ymod8 < 8 && xstart < xend) {
        BYTE *colour_ptr = crtc.screen_base + COLOUR_MEMORY_START;
        int i;

#if DEBUG_GFX
        printf("DRAW_rgbi: xstart=%d, xend=%d, ymod8=%d, scr_rel=%04x\n", xstart, xend, ymod8, scr_rel);
#endif
        for (i = xstart; i < xend; i++) {
            BYTE colour = colour_ptr[scr_rel & crtc.vaddr_mask];
            BYTE bg = (colour >> 4) & 0x0F;
            BYTE fg =  colour       & 0x0F;

            int pixel;
            for (pixel = 0; pixel < 8; pixel++) {
                int on = *p;

                if (on) {
                    *p++ = fg;
                } else {
                    *p++ = bg;
                }
            }
            scr_rel++;
        }
    }
}

static void DRAW_analog(BYTE *p, int xstart, int xend, int scr_rel, int ymod8)
{
    if (ymod8 < 8 && xstart < xend) {
        BYTE *colour_ptr = crtc.screen_base + COLOUR_MEMORY_START;
        int i;

#if DEBUG_GFX
        printf("DRAW_analog: xstart=%d, xend=%d, ymod8=%d, scr_rel=%04x\n", xstart, xend, ymod8, scr_rel);
#endif
        for (i = xstart; i < xend; i++) {
            BYTE colour = colour_ptr[scr_rel & crtc.vaddr_mask];

            int pixel;
            for (pixel = 0; pixel < 8; pixel++) {
                int on = *p;

                if (on) {
                    *p++ = colour;
                } else {
                    *p++ = pet_colour_analog_bg;
                }
            }
            scr_rel++;
        }
    }
}

#define RGBI_NUM_COLORS          16
#define ANALOG_NUM_COLORS       256

/* base saturation of all colors except the grey tones */
#define RGBI_SATURATION   (128.0f)

/* phase shift of all colors */
#define RGBI_PHASE          0.0f

#define MAKE_RGB(x) (((x)>>16) & 0xff), (((x)>>8) & 0xff), (((x)>>0) & 0xff)
static video_cbm_color_t rgbi_colors[RGBI_NUM_COLORS] =
{
    { MAKE_RGB(0x000000), "Black"       },
    { MAKE_RGB(0x555555), "Medium Gray" },
    { MAKE_RGB(0x0000AA), "Blue"        },
    { MAKE_RGB(0x5555FF), "Light Blue"  },
    { MAKE_RGB(0x00AA00), "Green"       },
    { MAKE_RGB(0x55FF55), "Light Green" },
    { MAKE_RGB(0x00AAAA), "Cyan"        },
    { MAKE_RGB(0x55FFFF), "Light Cyan"  },
    { MAKE_RGB(0xAA0000), "Red"         },
    { MAKE_RGB(0xFF5555), "Light Red"   },
    { MAKE_RGB(0xAA00AA), "Purple"      },
    { MAKE_RGB(0xFF55FF), "Light Purple"},
    { MAKE_RGB(0xAA5500), "Brown"       }, /* "brown fix", aka "dark yellow" 0xAAAA00 */
    { MAKE_RGB(0xFFFF55), "Yellow"      },
    { MAKE_RGB(0xAAAAAA), "Light Gray"  },
    { MAKE_RGB(0xFFFFFF), "White"       },
};
#undef MAKE_RGB

static video_cbm_palette_t rgbi_palette =
{
    RGBI_NUM_COLORS,
    rgbi_colors,
    RGBI_SATURATION,
    RGBI_PHASE,
    CBM_PALETTE_RGB
};

static video_cbm_color_t analog_colors[ANALOG_NUM_COLORS];

static video_cbm_palette_t analog_palette =
{
    ANALOG_NUM_COLORS,
    analog_colors,
    RGBI_SATURATION,
    RGBI_PHASE,
    CBM_PALETTE_RGB
};

static void init_analog_palette(void)
{
    int i;

    if (analog_colors[0].name != NULL) {
        return;
    }

    for (i = 0; i < ANALOG_NUM_COLORS; i++) {
        int r = (i >> 5) & 0x07;
        int g = (i >> 2) & 0x07;
        int b = (i >> 0) & 0x03;

        analog_colors[i].luminance = (float)((r << 5) | (r << 2) | (r >> 1));
        analog_colors[i].angle =     (float)((g << 5) | (g << 2) | (g >> 1));
        analog_colors[i].direction = (b << 6) | (b << 4) | (b << 2) | b;
        analog_colors[i].name = "Analog";
    }
}

int petcolour_set_type(int val)
{
    switch (val) {
        case PET_COLOUR_TYPE_OFF:
            crtc_color_update_palette(crtc.raster.canvas);
            crtc_set_hires_draw_callback(NULL);
            break;
        case PET_COLOUR_TYPE_RGBI:
            video_color_palette_internal(crtc.raster.canvas, &rgbi_palette);
            crtc_set_hires_draw_callback(DRAW_rgbi);
            break;
        case PET_COLOUR_TYPE_ANALOG:
            init_analog_palette();
            video_color_palette_internal(crtc.raster.canvas, &analog_palette);
            crtc_set_hires_draw_callback(DRAW_analog);
            break;
        default:
            return -1;
    }
    petmem_set_vidmem();

    return 0;
}

/*
 * Have an init function that can be called in a defined order,
 * after initializing the CRTC, so that we can install the proper
 * palette and hires callback again.
 */
void petcolour_init(void)
{
    petcolour_set_type(pet_colour_type);
}
