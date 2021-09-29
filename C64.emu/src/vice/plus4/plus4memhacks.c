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
#include "types.h"


static const char *hack_desc[] = {
    "None",
    "256KiB CSORY",
    "256KiB Hannes",
    "1MiB Hannes",
    "4MiB Hannes"
};


static int memory_hack = 0;

static int set_memory_hack(int val, void *param)
{
    int ramsize;

    if (val == memory_hack) {
        return 0;
    }

    /* check if new memory hack is a valid one */
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

    /* disable active memory hack */
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

    /* enable new memory hack */
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

int plus4_memory_hacks_ram_inject(uint16_t addr, uint8_t value)
{
    switch (memory_hack) {
        case MEMORY_HACK_C256K:
            cs256k_ram_inject(addr, value);
            break;
        case MEMORY_HACK_H256K:
        case MEMORY_HACK_H1024K:
        case MEMORY_HACK_H4096K:
            h256k_ram_inject(addr, value);
            break;
        case MEMORY_HACK_NONE:
        default:
            return 0;
            break;
    }
    return 1;
}

static const resource_int_t resources_int[] = {
    { "MemoryHack", MEMORY_HACK_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &memory_hack, set_memory_hack, NULL },
    RESOURCE_INT_LIST_END
};


/** \brief  Get description of \a hack
 *
 * \param[in]   hack    Plus4 hack ID
 *
 * \return  description of hack, to be used in UI or debug code
 */
const char *plus4_memory_hacks_desc(int hack)
{
    if (hack < 0 || hack >= (sizeof hack_desc / sizeof hack_desc[0])) {
        return "Invalid";
    }
    return hack_desc[hack];
}



int plus4_memory_hacks_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-memoryexphack", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MemoryHack", NULL,
      "<device>", "Set the 'memory expansion hack' device (0: None, 1: CSORY 256KiB, 2: HANNES 256KiB, 3: HANNES 1MiB, 4: HANNES 4MiB)" },
    CMDLINE_LIST_END
};

int plus4_memory_hacks_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
