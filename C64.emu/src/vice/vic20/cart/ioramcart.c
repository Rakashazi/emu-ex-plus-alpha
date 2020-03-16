/*
 * ioramcart.c - VIC20 RAM in I/O space emulation.
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

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "ioramcart.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"

static uint8_t ram_io2[0x400];
static uint8_t ram_io3[0x400];

static int ram_io2_enabled = 0;
static int ram_io3_enabled = 0;

/* ---------------------------------------------------------------------*/

static uint8_t ram_io2_read(uint16_t addr)
{
    return ram_io2[addr & 0x3ff];
}

static uint8_t ram_io3_read(uint16_t addr)
{
    return ram_io3[addr & 0x3ff];
}

static void ram_io2_store(uint16_t addr, uint8_t val)
{
    ram_io2[addr & 0x3ff] = val;
}

static void ram_io3_store(uint16_t addr, uint8_t val)
{
    ram_io3[addr & 0x3ff] = val;
}

/* ---------------------------------------------------------------------*/

static io_source_t ram_io2_device = {
    CARTRIDGE_VIC20_NAME_IO2_RAM, /* name of the device */
    IO_DETACH_RESOURCE,           /* use resource to detach the device when involved in a read-collision */
    "IO2RAM",                     /* resource to set to '0' */
    0x9800, 0x9bff, 0x3ff,        /* range for the device, regs:$9800-$9bff */
    1,                            /* read is always valid */
    ram_io2_store,                /* store function */
    NULL,                         /* NO poke function */
    ram_io2_read,                 /* read function */
    ram_io2_read,                 /* peek function */
    NULL,                         /* nothing to dump */
    CARTRIDGE_VIC20_IO2_RAM,      /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

static io_source_t ram_io3_device = {
    CARTRIDGE_VIC20_NAME_IO3_RAM, /* name of the device */
    IO_DETACH_RESOURCE,           /* use resource to detach the device when involved in a read-collision */
    "IO3RAM",                     /* resource to set to '0' */
    0x9c00, 0x9fff, 0x3ff,        /* range for the device, regs:$9c00-$9fff */
    1,                            /* read is always valid */
    ram_io3_store,                /* store function */
    NULL,                         /* NO poke function */
    ram_io3_read,                 /* read function */
    ram_io3_read,                 /* peek function */
    NULL,                         /* nothing to dump */
    CARTRIDGE_VIC20_IO3_RAM,      /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *ram_io2_list_item = NULL;
static io_source_list_t *ram_io3_list_item = NULL;

static const export_resource_t export_res_io2 = {
    CARTRIDGE_VIC20_NAME_IO2_RAM, 0, 0, &ram_io2_device, NULL, CARTRIDGE_VIC20_IO2_RAM
};

static const export_resource_t export_res_io3 = {
    CARTRIDGE_VIC20_NAME_IO2_RAM, 0, 0, NULL, &ram_io3_device, CARTRIDGE_VIC20_IO3_RAM
};

/* ---------------------------------------------------------------------*/

static int set_ram_io2_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!ram_io2_enabled && val) {
        if (export_add(&export_res_io2) < 0) {
            return -1;
        }
        ram_io2_list_item = io_source_register(&ram_io2_device);
    } else if (ram_io2_enabled && !val) {
        export_remove(&export_res_io2);
        io_source_unregister(ram_io2_list_item);
        ram_io2_list_item = NULL;
    }
    ram_io2_enabled = val;
    return 0;
}

static int set_ram_io3_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!ram_io3_enabled && val) {
        if (export_add(&export_res_io3) < 0) {
            return -1;
        }
        ram_io3_list_item = io_source_register(&ram_io3_device);
    } else if (ram_io3_enabled && !val) {
        export_remove(&export_res_io3);
        io_source_unregister(ram_io3_list_item);
        ram_io3_list_item = NULL;
    }
    ram_io3_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "IO2RAM", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ram_io2_enabled, set_ram_io2_enabled, NULL },
    { "IO3RAM", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ram_io3_enabled, set_ram_io3_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int ioramcart_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-io2ram", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IO2RAM", (resource_value_t)1,
      NULL, "Enable I/O-2 RAM" },
    { "+io2ram", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IO2RAM", (resource_value_t)0,
      NULL, "Disable I/O-2 RAM" },
    { "-io3ram", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IO3RAM", (resource_value_t)1,
      NULL, "Enable I/O-3 RAM" },
    { "+io3ram", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IO3RAM", (resource_value_t)0,
      NULL, "Disable I/O-3 RAM" },
    CMDLINE_LIST_END
};

int ioramcart_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

void ioramcart_io2_detach(void)
{
    set_ram_io2_enabled(0, NULL);
}

void ioramcart_io3_detach(void)
{
    set_ram_io3_enabled(0, NULL);
}

/* ---------------------------------------------------------------------*/

/* IO2RAMCART snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | RAM  | 1024 BYTES of RAM data
 */

/* IO3RAMCART snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | RAM  | 1024 BYTES of RAM data
 */

static char snap_io2_module_name[] = "IO2RAMCART";
static char snap_io3_module_name[] = "IO3RAMCART";
#define IORAMCART_DUMP_VER_MAJOR   0
#define IORAMCART_DUMP_VER_MINOR   0

int ioramcart_io2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_io2_module_name, IORAMCART_DUMP_VER_MAJOR, IORAMCART_DUMP_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, ram_io2, 0x400) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int ioramcart_io2_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_io2_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, IORAMCART_DUMP_VER_MAJOR, IORAMCART_DUMP_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_BA(m, ram_io2, 0x400) < 0) {
        goto fail;
    }

    snapshot_module_close(m);
    return set_ram_io2_enabled(1, NULL);

fail:
    snapshot_module_close(m);
    return -1;
}

int ioramcart_io3_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_io3_module_name, IORAMCART_DUMP_VER_MAJOR, IORAMCART_DUMP_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, ram_io3, 0x400) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int ioramcart_io3_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_io3_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, IORAMCART_DUMP_VER_MAJOR, IORAMCART_DUMP_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_BA(m, ram_io3, 0x400) < 0) {
        goto fail;
    }

    snapshot_module_close(m);
    return set_ram_io3_enabled(1, NULL);

fail:
    snapshot_module_close(m);
    return -1;
}
