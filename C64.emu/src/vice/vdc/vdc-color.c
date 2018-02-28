/*
 * vdc-color.c - Colors for the VDC emulation.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "vdctypes.h"
#include "vdc-color.h"
#include "vdc-resources.h"
#include "video.h"

/* base saturation of all colors except the grey tones */
#define VDC_SATURATION   (128.0f)

/* phase shift of all colors */
#define VDC_PHASE          0.0f

#define VDCRGB(x) (((x)>>16) & 0xff), (((x)>>8) & 0xff), (((x)>>0) & 0xff)
static video_cbm_color_t vdc_colors[VDC_NUM_COLORS] =
{
    { VDCRGB(0x000000), "Black"       },
    { VDCRGB(0x555555), "Medium Gray" },
    { VDCRGB(0x0000AA), "Blue"        },
    { VDCRGB(0x5555FF), "Light Blue"  },
    { VDCRGB(0x00AA00), "Green"       },
    { VDCRGB(0x55FF55), "Light Green" },
    { VDCRGB(0x00AAAA), "Cyan"        },
    { VDCRGB(0x55FFFF), "Light Cyan"  },
    { VDCRGB(0xAA0000), "Red"         },
    { VDCRGB(0xFF5555), "Light Red"   },
    { VDCRGB(0xAA00AA), "Purple"      },
    { VDCRGB(0xFF55FF), "Light Purple"},
    { VDCRGB(0xAA5500), "Brown"       }, /* "brown fix", aka "dark yellow" 0xAAAA00 */
    { VDCRGB(0xFFFF55), "Yellow"      },
    { VDCRGB(0xAAAAAA), "Light Gray"  },
    { VDCRGB(0xFFFFFF), "White"       },
};
#undef VDCRGB

static video_cbm_palette_t vdc_palette =
{
    VDC_NUM_COLORS,
    vdc_colors,
    VDC_SATURATION,
    VDC_PHASE,
    CBM_PALETTE_RGB
};

int vdc_color_update_palette(struct video_canvas_s *canvas)
{
    video_color_palette_internal(canvas, &vdc_palette);
    return 0;
}
