/*
 * etfe.c - Short Bus ETFE emulation.
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

#ifdef HAVE_PCAP

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "cs8900io.h"
#include "lib.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "util.h"

/*
    "The Shortbus ETFE "Final Ethernet" device

    - it simply contains a cs8900a mapped to $de00, $de10 or $df00.
    - for register documentation refer to the cs8900a datasheet
*/

/* ------------------------------------------------------------------------- */
/*    resources support functions                                            */

/* Some prototypes are needed */
static BYTE shortbus_etfe_read(WORD io_address);
static BYTE shortbus_etfe_peek(WORD io_address);
static void shortbus_etfe_store(WORD io_address, BYTE byte);
static int shortbus_etfe_dump(void);

static io_source_t shortbus_etfe_device = {
    "Shortbus ETFE",
    IO_DETACH_RESOURCE,
    "SBETFE",
    0xde00, 0xde0f, 0x0f,
    0,
    shortbus_etfe_store,
    shortbus_etfe_read,
    shortbus_etfe_peek,
    shortbus_etfe_dump,
    CARTRIDGE_IDE64,
    0,
    0
};

/* current configurations */
static io_source_list_t *shortbus_etfe_list_item = NULL;

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/* This flag indicates if the IDE64 cart is active */
static int shortbus_etfe_host_active = 0;

/* This flag indicated if the expansion is active,
   real activity depends on the 'host' active flag */
static int shortbus_etfe_expansion_active = 0;

/* ETFE address */
static int shortbus_etfe_address;

static char *shortbus_etfe_address_list = NULL;

/* ---------------------------------------------------------------------*/

void shortbus_etfe_unregister(void)
{
    if (shortbus_etfe_list_item != NULL) {
        io_source_unregister(shortbus_etfe_list_item);
        shortbus_etfe_list_item = NULL;
    }
    shortbus_etfe_host_active = 0;
}

void shortbus_etfe_register(void)
{
    if (shortbus_etfe_expansion_active) {
        shortbus_etfe_list_item = io_source_register(&shortbus_etfe_device);
    }
    shortbus_etfe_host_active = 1;
}

/* ---------------------------------------------------------------------*/

static int shortbus_etfe_activate(void)
{
    return cs8900io_enable("IDE64 shortbus ETFE");
}

static int shortbus_etfe_deactivate(void)
{
    return cs8900io_disable();
}

/* ---------------------------------------------------------------------*/

static int set_shortbus_etfe_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val == shortbus_etfe_expansion_active) {
        return 0;
    }

    if (val) {
        if (shortbus_etfe_activate() < 0) {
            return -1;
        }
    } else {
        shortbus_etfe_deactivate();
    }

    if (shortbus_etfe_host_active) {
        if (val) {
            shortbus_etfe_list_item = io_source_register(&shortbus_etfe_device);
        } else {
            if (shortbus_etfe_list_item != NULL) {
                io_source_unregister(shortbus_etfe_list_item);
                shortbus_etfe_list_item = NULL;
            }
        }
    }
    shortbus_etfe_expansion_active = val;

    return 0;
}

static int set_shortbus_etfe_base(int val, void *param)
{
    int addr = val;
    int old = shortbus_etfe_expansion_active;

    if (val == shortbus_etfe_address) {
        return 0;
    }

    if (old) {
        set_shortbus_etfe_enabled(0, NULL);
    }

    switch (addr) {
        case 0xde00:
        case 0xde10:
        case 0xdf00:
            shortbus_etfe_device.start_address = (WORD)addr;
            shortbus_etfe_device.end_address = (WORD)(addr + 0xf);
            break;
        default:
            return -1;
    }

    shortbus_etfe_address = val;

    if (old) {
        set_shortbus_etfe_enabled(1, NULL);
    }
    return 0;
}

void shortbus_etfe_reset(void)
{
    cs8900io_reset();
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "SBETFE", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &shortbus_etfe_expansion_active, set_shortbus_etfe_enabled, NULL },
    { "SBETFEbase", 0xde00, RES_EVENT_NO, NULL,
      &shortbus_etfe_address, set_shortbus_etfe_base, NULL },
    RESOURCE_INT_LIST_END
};

int shortbus_etfe_resources_init(void)
{
    cs8900io_init();

    if (cs8900io_resources_init() < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void shortbus_etfe_resources_shutdown(void)
{
    cs8900io_resources_shutdown();

    if (shortbus_etfe_address_list) {
        lib_free(shortbus_etfe_address_list);
    }
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-sbetfe", SET_RESOURCE, 0,
      NULL, NULL, "SBETFE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SHORTBUS_ETFE,
      NULL, NULL },
    { "+sbtfe", SET_RESOURCE, 0,
      NULL, NULL, "SBTFE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SHORTBUS_ETFE,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-sbtfebase", SET_RESOURCE, 1,
      NULL, NULL, "SBTFEbase", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_SHORTBUS_ETFE_BASE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int shortbus_etfe_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    shortbus_etfe_address_list = lib_stralloc(". (56832: $de00, 56848: $de10, 57088: $df00)");

    base_cmdline_options[0].description = shortbus_etfe_address_list;

    if (cs8900io_cmdline_options_init() < 0) {
        return -1;
    }

    return cmdline_register_options(base_cmdline_options);
}

int shortbus_etfe_enabled(void)
{
    return shortbus_etfe_expansion_active;
}

/* ------------------------------------------------------------------------- */

/* ----- read byte from I/O range in VICE ----- */
static BYTE shortbus_etfe_read(WORD io_address)
{
    shortbus_etfe_device.io_source_valid = 1;

    return cs8900io_read(io_address);
}

/* ----- peek byte with no sideeffects from I/O range in VICE ----- */
static BYTE shortbus_etfe_peek(WORD io_address)
{
    return cs8900io_peek(io_address);
}

/* ----- write byte to I/O range of VICE ----- */
static void shortbus_etfe_store(WORD io_address, BYTE byte)
{
    cs8900io_store(io_address, byte);
}

static int shortbus_etfe_dump(void)
{
    mon_out("CS8900 mapped to $%04x ($%04x-$%04x).\n",
            shortbus_etfe_device.start_address & ~shortbus_etfe_device.address_mask,
            shortbus_etfe_device.start_address,
            shortbus_etfe_device.end_address);

    return cs8900io_dump();
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTSBETFE"

/* FIXME: implement snapshot support */
int shortbus_etfe_write_snapshot_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
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

int shortbus_etfe_read_snapshot_module(snapshot_t *s)
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

#endif /* #ifdef HAVE_PCAP */
