/*
 * c64dtvmodel.c - DTV model detection and setting.
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

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "c64dtv-resources.h"
#include "c64dtvmem.h"
#include "c64dtvmodel.h"
#include "machine.h"
#include "resources.h"
#include "types.h"

struct model_s {
    int video;        /* machine video timing */
    int asic;         /* asic revision */
    int hummeradc;    /* hummer adc present */
};

/*
ASIC revisions:

Rev 1: (used in the US 2004 production shipped to QVC and KB Toys)
  NTSC Only
  DMA,(no blitter)
  SRAM Support Only
  Two Crystals
  No Illegal Opcodes
  Extended High Color Video Modes
Rev1  Bugs:
  CLUT Chroma15 Mirrored over 16-255's Chroma
  Counter runs backwards in DMA
  Modulus applied 8 times at end of line
  F7 Key not mapped

Rev 2: (used in the first production of PAL DTV's for Toy Lobster)
  PAL and NTSC
  Better Timing
  SDRAM Support
  DMA and Blitter
  Burst and Skip CPU Modes
  Most Illegal Opcodes support
  More Extended Video Modes
  Extended 6510 control port
Rev 2 Bugs:
  Black Pixels Every 4 in Blitter Transparent Mode
  F7 Key not mapped

Rev 3: (used in Hummer and the next productions runs of PAL C64 DTV's)
  Same features of Rev 2 with blitter bug fixed.
*/

static struct model_s dtvmodels[] = {
    { MACHINE_SYNC_PAL,  REV_2, IS_DTV    }, /* DTV v2 (pal) */
    { MACHINE_SYNC_NTSC, REV_2, IS_DTV    }, /* DTV v2 (ntsc) */
    { MACHINE_SYNC_PAL,  REV_3, IS_DTV    }, /* DTV v3 (pal) */
    { MACHINE_SYNC_NTSC, REV_3, IS_DTV    }, /* DTV v3 (ntsc) */
    { MACHINE_SYNC_NTSC, REV_3, IS_HUMMER }, /* Hummer (ntsc) */
};

/* ------------------------------------------------------------------------- */
static int dtvmodel_get_temp(int video, int asic, int hummeradc)
{
    int i;

    for (i = 0; i < DTVMODEL_NUM; ++i) {
        if ((dtvmodels[i].video == video)
            && (dtvmodels[i].asic == asic)
            && (dtvmodels[i].hummeradc == hummeradc)) {
            return i;
        }
    }

    return DTVMODEL_UNKNOWN;
}

int dtvmodel_get(void)
{
    int video, asic, hummeradc;

    if ((resources_get_int("MachineVideoStandard", &video) < 0)
        || (resources_get_int("DtvRevision", &asic) < 0)
        || (resources_get_int("HummerADC", &hummeradc) < 0)) {
        return -1;
    }

    return dtvmodel_get_temp(video, asic, hummeradc);
}

#if 0
static void dtvmodel_set_temp(int model, int *vic_model, int *asic, int *hummeradc)
{
    int old_model;

    old_model = dtvmodel_get_temp(*vic_model, *asic, *hummeradc);

    if ((model == old_model) || (model == DTVMODEL_UNKNOWN)) {
        return;
    }

    *vic_model = dtvmodels[model].video;
    *asic = dtvmodels[model].asic;
    *hummeradc = dtvmodels[model].hummeradc;
}
#endif

void dtvmodel_set(int model)
{
    int old_model;

    old_model = dtvmodel_get();

    if ((model == old_model) || (model == DTVMODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("MachineVideoStandard", dtvmodels[model].video);
    resources_set_int("DtvRevision", dtvmodels[model].asic);
    resources_set_int("HummerADC", dtvmodels[model].hummeradc);
}
