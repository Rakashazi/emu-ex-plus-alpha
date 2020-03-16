/*
 * debugcart.c - debug "cartridge" used for automatic regression testing
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

#include "archdep.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "lib.h"
#include "resources.h"
#include "machine.h"
#include "maincpu.h"

#include "debugcart.h"

static int debugcart_enabled = 0;

/* ------------------------------------------------------------------------- */

/* a prototype is needed */
static void debugcart_store(uint16_t addr, uint8_t value);

/* This is not a real cartridge, it is only used for debugging purposes */
static io_source_t debugcart_device = {
    CARTRIDGE_NAME_DEBUGCART, /* name of the device */
    IO_DETACH_RESOURCE,       /* use resource to detach the device when involved in a read-collision */
    "DebugCartEnable",        /* resource to set to '0' */
    0x910f, 0x910f, 0xff,     /* range for the device, reg:$910f */
    0,                        /* read is never valid, device is write only */
    debugcart_store,          /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    NULL,                     /* nothing to dump */
    CARTRIDGE_DEBUGCART,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0                         /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *debugcart_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_DEBUGCART, 0, 0, &debugcart_device, NULL, CARTRIDGE_DEBUGCART
};

/* ------------------------------------------------------------------------- */

static void debugcart_store(uint16_t addr, uint8_t value)
{
    fprintf(stdout, "DBGCART: exit(%d) cycles elapsed: %u\n",
            (int)value, maincpu_clk);

    archdep_vice_exit(value);
}

/* ------------------------------------------------------------------------- */

static int debugcart_enable(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    debugcart_list_item = io_source_register(&debugcart_device);
    return 0;
}

static void debugcart_disable(void)
{
    export_remove(&export_res);
    io_source_unregister(debugcart_list_item);
    debugcart_list_item = NULL;
}

static int set_debugcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((val) && (!debugcart_enabled)) {
        if (debugcart_enable() < 0) {
            return -1;
        }
        debugcart_enabled = 1;
    } else if ((!val) && (debugcart_enabled)) {
        debugcart_disable();
        debugcart_enabled = 0;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_i[] = {
    { "DebugCartEnable", 0, RES_EVENT_STRICT, 0,
      &debugcart_enabled, set_debugcart_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int debugcart_resources_init(void)
{
    return resources_register_int(resources_i);
}

void debugcart_resources_shutdown(void)
{
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cart_cmdline_options[] =
{
    { "-debugcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DebugCartEnable", (resource_value_t)1,
      NULL, "Enable Debug cartridge" },
    { "+debugcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DebugCartEnable", (resource_value_t)0,
      NULL, "Disable Debug cartridge" },
    CMDLINE_LIST_END
};

int debugcart_cmdline_options_init(void)
{
    if (cmdline_register_options(cart_cmdline_options) < 0) {
          return -1;
    }
    return 0;
}
