/*
 * vic20-ieee488.c - VIC20 specific IEEE488 emulation.
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

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "uiapi.h"
#include "vic20-ieee488.h"
#include "vic20ieeevia.h"

/* Flag: Do we enable the VIC-1112 IEEE488 interface?  */
static int ieee488_enabled;

/* ---------------------------------------------------------------------*/

static BYTE ieee488_read(WORD address)
{
    if (address & 0x10) {
        return ieeevia2_read(address);
    }
    return ieeevia1_read(address);
}

static void ieee488_store(WORD address, BYTE value)
{
    if (address & 0x10) {
        ieeevia2_store(address, value);
    } else {
        ieeevia1_store(address, value);
    }
}

/* ---------------------------------------------------------------------*/

static io_source_t ieee488_device = {
    "IEEE488",
    IO_DETACH_RESOURCE,
    "IEEE488",
    0x9800, 0x9bff, 0x3ff,
    1, /* read is always valid */
    ieee488_store,
    ieee488_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_VIC20_IEEE488,
    0,
    0
};

static io_source_list_t *ieee488_list_item = NULL;

/* ---------------------------------------------------------------------*/

static int set_ieee488_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!ieee488_enabled && val) {
        ieee488_list_item = io_source_register(&ieee488_device);
        ieee488_enabled = 1;
    } else if (ieee488_enabled && !val) {
        io_source_unregister(ieee488_list_item);
        ieee488_list_item = NULL;
        ieee488_enabled = 0;
    }

    ui_update_menus();

    return 0;
}

static const resource_int_t resources_int[] = {
    { "IEEE488", 0, RES_EVENT_SAME, NULL,
      &ieee488_enabled, set_ieee488_enabled, NULL },
    { NULL }
};

int vic20_ieee488_resources_init(void)
{
    return resources_register_int(resources_int);
}

static cmdline_option_t const cmdline_options[] =
{
    { "-ieee488", SET_RESOURCE, 0,
      NULL, NULL, "IEEE488", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_VIC1112_IEEE488,
      NULL, NULL },
    { "+ieee488", SET_RESOURCE, 0,
      NULL, NULL, "IEEE488", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_VIC1112_IEEE488,
      NULL, NULL },
    { NULL}
};

int vic20_ieee488_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTIEEE"

/* FIXME: implement snapshot support */
int vic20_ieee488_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int vic20_ieee488_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}
