/*
 * vic20model.c - Plus4 model detection and setting.
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

#include "vic20-resources.h"
#include "vic20cart.h"
#include "vic20mem.h"
#include "vic20model.h"
#include "machine.h"
#include "resources.h"
#include "types.h"

struct model_s {
    int video;   /* machine video timing */
    int ramblocks;
};

enum {
    BLOCK_0 = 1,
    BLOCK_1 = 1 << 1,
    BLOCK_2 = 1 << 2,
    BLOCK_3 = 1 << 3,
    BLOCK_5 = 1 << 5
};

static struct model_s vic20models[] = {
    { MACHINE_SYNC_PAL,  NO_EXTRA_RAM},
    { MACHINE_SYNC_NTSC, NO_EXTRA_RAM},
    { MACHINE_SYNC_NTSC, BLOCK_1 | BLOCK_2}, /* SuperVIC */
};

/* ------------------------------------------------------------------------- */

static int vic20model_get_temp(int video, int ramblocks)
{
    int i;

    for (i = 0; i < VIC20MODEL_NUM; ++i) {
        if ((vic20models[i].video == video)
            && (vic20models[i].ramblocks == ramblocks)) {
            return i;
        }
    }

    return VIC20MODEL_UNKNOWN;
}

int vic20model_get(void)
{
    int video, ramblocks, block0, block1, block2, block3, block5;

    if ((resources_get_int("MachineVideoStandard", &video) < 0)
        || (resources_get_int("RamBlock0", &block0) < 0)
        || (resources_get_int("RamBlock1", &block1) < 0)
        || (resources_get_int("RamBlock2", &block2) < 0)
        || (resources_get_int("RamBlock3", &block3) < 0)
        || (resources_get_int("RamBlock5", &block5) < 0)) {
        return -1;
    }
    ramblocks = (block0 ? BLOCK_0 : 0);
    ramblocks |= (block1 ? BLOCK_1 : 0);
    ramblocks |= (block2 ? BLOCK_2 : 0);
    ramblocks |= (block3 ? BLOCK_3 : 0);
    ramblocks |= (block5 ? BLOCK_5 : 0);
    return vic20model_get_temp(video, ramblocks);
}

#if 0
static void vic20model_set_temp(int model, int *vic_model, int *ramblocks)
{
    int old_model;

    old_model = vic20model_get_temp(*vic_model, *ramblocks);

    if ((model == old_model) || (model == VIC20MODEL_UNKNOWN)) {
        return;
    }

    *vic_model = vic20models[model].video;
    *ramblocks = vic20models[model].ramblocks;
}
#endif

void vic20model_set(int model)
{
    int old_model, blocks;

    old_model = vic20model_get();

    if ((model == old_model) || (model == VIC20MODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("MachineVideoStandard", vic20models[model].video);
    blocks = vic20models[model].ramblocks;
    resources_set_int("RamBlock0", blocks & BLOCK_0 ? 1 : 0);
    resources_set_int("RamBlock1", blocks & BLOCK_1 ? 1 : 0);
    resources_set_int("RamBlock2", blocks & BLOCK_2 ? 1 : 0);
    resources_set_int("RamBlock3", blocks & BLOCK_3 ? 1 : 0);
    resources_set_int("RamBlock5", blocks & BLOCK_5 ? 1 : 0);
}
