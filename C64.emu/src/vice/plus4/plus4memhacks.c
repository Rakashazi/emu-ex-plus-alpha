/*
 * plus4memhacks.c - Plus4 memory hacks control.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "plus4memcsory256k.h"
#include "plus4memhacks.h"
#include "plus4memhannes256k.h"
#include "resources.h"
#include "translate.h"
#include "types.h"

static int memory_hack = 0;

static int set_memory_hack(int val, void *param)
{
    int ramsize;

    if (val == memory_hack) {
        return 0;
    }

    switch (val) {
        case MEMORY_HACK_NONE:
        case MEMORY_HACK_C256K:
        case MEMORY_HACK_H256K:
        case MEMORY_HACK_H1024K:
        case MEMORY_HACK_H4096K:
            break;
        default:
            return -1;
    }

    switch (memory_hack) {
        case MEMORY_HACK_NONE:
            break;
        case MEMORY_HACK_C256K:
            set_cs256k_enabled(0);
            break;
        case MEMORY_HACK_H256K:
        case MEMORY_HACK_H1024K:
        case MEMORY_HACK_H4096K:
            set_h256k_enabled(H256K_DISABLED);
            break;
    }

    memory_hack = val;

    switch (memory_hack) {
        case MEMORY_HACK_NONE:
            resources_get_int("RamSize", &ramsize);
            if (ramsize > 64) {
                resources_set_int("RamSize", 64);
            }
            break;
        case MEMORY_HACK_C256K:
            set_cs256k_enabled(1);
            resources_set_int("RamSize", 256);
            break;
        case MEMORY_HACK_H256K:
            set_h256k_enabled(H256K_256K);
            resources_set_int("RamSize", 256);
            break;
        case MEMORY_HACK_H1024K:
            set_h256k_enabled(H256K_1024K);
            resources_set_int("RamSize", 1024);
            break;
        case MEMORY_HACK_H4096K:
            set_h256k_enabled(H256K_4096K);
            resources_set_int("RamSize", 4096);
            break;
    }

    return 0;
}

static const resource_int_t resources_int[] = {
    { "MemoryHack", MEMORY_HACK_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &memory_hack, set_memory_hack, NULL },
    { NULL }
};

int plus4_memory_hacks_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-memoryexphack", SET_RESOURCE, 1,
      NULL, NULL, "MemoryHack", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_DEVICE, IDCLS_SET_PLUS4_MEMORY_HACK,
      NULL, NULL },
    { NULL }
};

int plus4_memory_hacks_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
