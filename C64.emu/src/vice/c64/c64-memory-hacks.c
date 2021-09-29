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
#include "mem.h"
#include "plus256k.h"
#include "plus60k.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "ui.h"

static int memory_hack = 0;

/** \brief  Set memory hack
 *
 * Pauses emulation when switching to another memory expansion hack to avoid
 * having a running CPU access invalid memory. Unpauses emulation if the
 * emulation wasn't already paused before switching memory expansion.
 *
 * \param[in]   value   memory hack ID
 * \param[in]   param   extra data (unused)
 *
 * \return  0 on succes, -1 on failure
 */
static int set_memory_hack(int value, void *param)
{
    if (value == memory_hack) {
        return 0;
    }

    /* check if the new memory hack is a valid one */
    switch (value) {
        case MEMORY_HACK_NONE:
        case MEMORY_HACK_C64_256K:
        case MEMORY_HACK_PLUS60K:
        case MEMORY_HACK_PLUS256K:
            break;
        default:
            return -1;
    }

    /* disable already active memory hack */
    switch (memory_hack) {
        case MEMORY_HACK_C64_256K:
            set_c64_256k_enabled(0, 0);
            break;
        case MEMORY_HACK_PLUS60K:
            set_plus60k_enabled(0, 0);
            break;
        case MEMORY_HACK_PLUS256K:
            set_plus256k_enabled(0, 0);
            break;
        case MEMORY_HACK_NONE:
        default:
            break;
    }

    /* enable new memory hack */
    switch (value) {
        case MEMORY_HACK_C64_256K:
            set_c64_256k_enabled(1, 0);
            break;
        case MEMORY_HACK_PLUS60K:
            set_plus60k_enabled(1, 0);
            break;
        case MEMORY_HACK_PLUS256K:
            set_plus256k_enabled(1, 0);
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

int memory_hacks_ram_inject(uint16_t addr, uint8_t value)
{
    switch (memory_hack) {
        case MEMORY_HACK_C64_256K:
            c64_256k_ram_inject(addr, value);
            break;
        case MEMORY_HACK_PLUS60K:
            plus60k_ram_inject(addr, value);
            break;
        case MEMORY_HACK_PLUS256K:
            plus256k_ram_inject(addr, value);
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

int memory_hacks_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-memoryexphack", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MemoryHack", NULL,
      "<device>", "Set the 'memory expansion hack' device (0: None, 1: C64 256K, 2: +60K, 3: +256K)" },
    CMDLINE_LIST_END
};

int memory_hacks_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* C64MEMHACKS snapshot module format:

   type | name  | description
   --------------------------
   BYTE | hacks | which memory hack is active
 */

static char snap_module_name[] = "C64MEMHACKS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int memhacks_snapshot_write_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)memory_hack) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    switch (memory_hack) {
        default:
        case MEMORY_HACK_NONE:
            break;
        case MEMORY_HACK_C64_256K:
            if (c64_256k_snapshot_write(s) < 0) {
                return -1;
            }
            break;
        case MEMORY_HACK_PLUS60K:
            if (plus60k_snapshot_write(s) < 0) {
                return -1;
            }
            break;
        case MEMORY_HACK_PLUS256K:
            if (plus256k_snapshot_write(s) < 0) {
                return -1;
            }
            break;
    }
    return 0;
}

int memhacks_snapshot_read_modules(struct snapshot_s *s)
{
    snapshot_module_t *m;
    uint8_t vmajor, vminor;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* do not accept higher versions than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* disable all hacks (without reset) before loading snapshot if needed */
    if (memory_hack != MEMORY_HACK_NONE) {
        set_c64_256k_enabled(0, 1);
        set_plus60k_enabled(0, 1);
        set_plus256k_enabled(0, 1);

        /* restore default c64 memory config */
        mem_initialize_memory();
    }

    if (SMR_B_INT(m, &memory_hack) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    switch (memory_hack) {
        default:
        case MEMORY_HACK_NONE:
            break;
        case MEMORY_HACK_C64_256K:
            if (c64_256k_snapshot_read(s) < 0) {
                goto fail;
            }
            break;
        case MEMORY_HACK_PLUS60K:
            if (plus60k_snapshot_read(s) < 0) {
                goto fail;
            }
            break;
        case MEMORY_HACK_PLUS256K:
            if (plus256k_snapshot_read(s) < 0) {
                goto fail;
            }
            break;
    }

    /* set new memory config */
    if (memory_hack != MEMORY_HACK_NONE) {
        mem_initialize_memory();
    }

    return 0;
    
fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    memory_hack = MEMORY_HACK_NONE;
    return -1;
}
