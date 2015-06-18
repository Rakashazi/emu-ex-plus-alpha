/*
 * c64-memory-hacks.c - C64-256K/PLUS60K/PLUS256K EXPANSION HACK control.
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

#include "c64-memory-hacks.h"
#include "c64_256k.h"
#include "cmdline.h"
#include "plus256k.h"
#include "plus60k.h"
#include "resources.h"
#include "translate.h"
#include "types.h"

static int memory_hack = 0;

static int set_memory_hack(int value, void *param)
{
    if (value == memory_hack) {
        return 0;
    }

    switch (value) {
        case MEMORY_HACK_NONE:
        case MEMORY_HACK_C64_256K:
        case MEMORY_HACK_PLUS60K:
        case MEMORY_HACK_PLUS256K:
            break;
        default:
            return -1;
    }

    switch (memory_hack) {
        case MEMORY_HACK_C64_256K:
            set_c64_256k_enabled(0);
            break;
        case MEMORY_HACK_PLUS60K:
            set_plus60k_enabled(0);
            break;
        case MEMORY_HACK_PLUS256K:
            set_plus256k_enabled(0);
            break;
        case MEMORY_HACK_NONE:
        default:
            break;
    }

    switch (value) {
        case MEMORY_HACK_C64_256K:
            set_c64_256k_enabled(1);
            break;
        case MEMORY_HACK_PLUS60K:
            set_plus60k_enabled(1);
            break;
        case MEMORY_HACK_PLUS256K:
            set_plus256k_enabled(1);
            break;
        case MEMORY_HACK_NONE:
            break;
        default:
            return -1;
            break;
    }

    memory_hack = value;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "MemoryHack", MEMORY_HACK_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &memory_hack, set_memory_hack, NULL },
    { NULL }
};

int memory_hacks_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-memoryexphack", SET_RESOURCE, 1,
      NULL, NULL, "MemoryHack", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_DEVICE, IDCLS_SET_C64_MEMORY_HACK,
      NULL, NULL },
    { NULL }
};

int memory_hacks_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
