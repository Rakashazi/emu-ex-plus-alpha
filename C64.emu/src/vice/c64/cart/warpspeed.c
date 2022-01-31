/*
 * warpspeed.c - Cartridge handling, Warpspeed cart.
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

/* #define DEBUG_WARPSPEED */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "warpspeed.h"
#include "crt.h"

#ifdef DEBUG_WARPSPEED
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
    Warpspeed

    - 16k ROM
    - uses full io1/io2

    the cartridge uses two j/k flipflops to enable/disable the ROM. simply said
    it works like this:

    io1
    - read: ROM (offset $1e00)
    - write: enable rom at 8000

    io2
    - read: ROM (offset $1f00)
    - write: disable rom at 8000

    however, in reality its like this (for writing):

    to enable the cartridge:
    - write access in IO1
    - write access outside of IO2
    (two write accesses in IO1 will work)

    to disable the cartridge:
    - write access outside of IO1
    - write access in IO2
    (two write accesses in IO2 will work)

    to toggle the ROM enabled state:
    - write access to IO1
    - write access to IO2

    the software uses "indexed INC abs" to do the switching, which is why the
    simplified version also works.
*/

/* some prototypes are needed */
static uint8_t warpspeed_io1_read(uint16_t addr);
static void warpspeed_io1_store(uint16_t addr, uint8_t value);
static uint8_t warpspeed_io2_read(uint16_t addr);
static void warpspeed_io2_store(uint16_t addr, uint8_t value);
static int warpspeed_dump(void);

static io_source_t warpspeed_io1_device = {
    CARTRIDGE_NAME_WARPSPEED, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, regs:$de00-$deff */
    1,                        /* read is always valid */
    warpspeed_io1_store,      /* store function */
    NULL,                     /* NO poke function */
    warpspeed_io1_read,       /* read function */
    warpspeed_io1_read,       /* peek function */
    warpspeed_dump,           /* device state information dump function */
    CARTRIDGE_WARPSPEED,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0                         /* insertion order, gets filled in by the registration function */
};

static io_source_t warpspeed_io2_device = {
    CARTRIDGE_NAME_WARPSPEED, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,     /* range for the device, regs:$df00-$dfff */
    1,                        /* read is always valid */
    warpspeed_io2_store,      /* store function */
    NULL,                     /* NO poke function */
    warpspeed_io2_read,       /* read function */
    warpspeed_io2_read,       /* peek function */
    warpspeed_dump,           /* device state information dump function */
    CARTRIDGE_WARPSPEED,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0                         /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *warpspeed_io1_list_item = NULL;
static io_source_list_t *warpspeed_io2_list_item = NULL;

/* ---------------------------------------------------------------------*/

static uint8_t warpspeed_io1_read(uint16_t addr)
{
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static uint8_t warpspeed_io2_read(uint16_t addr)
{
    return roml_banks[0x1f00 + (addr & 0xff)];
}

#define LASTWRITE_IO1   0
#define LASTWRITE_IO2   1
#define LASTWRITE_NOTIO 2

static int warpspeed_enabled = 0;
static int warpspeed_lastwrite = 0;

/* FIXME: this should be called on *any other* write access. we cant really do
          that right now with the cartridge system :( fortunately the warpspeed
          cartridge software does not rely on this, so it works anyway */
#if 0
void warpspeed_no_io_store(uint16_t addr, uint8_t value)
{
    /* if last write did go to IO1, enable the cartridge */
    if (warpspeed_lastwrite == LASTWRITE_IO1) {
        DBG(("warpspeed_io1_store %04x %02x - lastwrite: IO1 rom: enabling\n", addr, value));
        warpspeed_enabled = 1;
        cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    }
    warpspeed_lastwrite = LASTWRITE_NOTIO;
}
#endif

static void warpspeed_io1_store(uint16_t addr, uint8_t value)
{
    /* if last write did go to IO1, enable the cartridge */
    if (warpspeed_lastwrite == LASTWRITE_IO1) {
        DBG(("warpspeed_io1_store de%02x %02x - lastwrite: IO1 rom: enabling\n", addr, value));
        warpspeed_enabled = 1;
        cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    }
#ifdef DEBUG_WARPSPEED
    else {
        DBG(("warpspeed_io1_store de%02x %02x - lastwrite: not IO1 rom: %s (no change)\n",
            addr, value, warpspeed_enabled ? "enabled" : "disabled"));
    }
#endif
    warpspeed_lastwrite = LASTWRITE_IO1;
}

static void warpspeed_io2_store(uint16_t addr, uint8_t value)
{
    /* if last write did not go to IO1, disable the cartridge */
    if (warpspeed_lastwrite != LASTWRITE_IO1) {
        DBG(("warpspeed_io2_store df%02x %02x - lastwrite: not IO1 rom: disabling\n", addr, value));
        warpspeed_enabled = 0;
    } else if (warpspeed_lastwrite == LASTWRITE_IO1) {
        /* if last write did go to IO1, then toggle the cartridge enabled status */
        warpspeed_enabled ^= 1;
        DBG(("warpspeed_io2_store df%02x %02x - lastwrite: IO1 rom: toggling (now: %s)\n",
             addr, value, warpspeed_enabled ? "enabled" : "disabled"));
    }
    if (warpspeed_enabled) {
        cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    } else {
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_RAM);
    }
    warpspeed_lastwrite = LASTWRITE_IO2;
}

static int warpspeed_dump(void)
{
    mon_out("$8000-$9FFF ROM: %s\n", (warpspeed_enabled) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static const export_resource_t export_res_warpspeed = {
    CARTRIDGE_NAME_WARPSPEED, 1, 1, &warpspeed_io1_device, &warpspeed_io2_device, CARTRIDGE_WARPSPEED
};

/* ---------------------------------------------------------------------*/
void warpspeed_reset(void)
{
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    warpspeed_enabled = 1;
    warpspeed_lastwrite = 0;
}

void warpspeed_config_init(void)
{
    warpspeed_reset();
}

void warpspeed_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    warpspeed_reset();
}

static int warpspeed_common_attach(void)
{
    if (export_add(&export_res_warpspeed) < 0) {
        return -1;
    }

    warpspeed_io1_list_item = io_source_register(&warpspeed_io1_device);
    warpspeed_io2_list_item = io_source_register(&warpspeed_io2_device);

    return 0;
}

int warpspeed_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return warpspeed_common_attach();
}

int warpspeed_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.start != 0x8000 || chip.size != 0x4000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return warpspeed_common_attach();
}

void warpspeed_detach(void)
{
    export_remove(&export_res_warpspeed);
    io_source_unregister(warpspeed_io1_list_item);
    io_source_unregister(warpspeed_io2_list_item);
    warpspeed_io1_list_item = NULL;
    warpspeed_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTWARP snapshot module format:

   type  | name     | version | description
   ----------------------------------------
   BYTE  | ROM 8000 |   0.1   | ROM at $8000 flag
   ARRAY | ROML     |   0.0+  | 8192 BYTES of ROML data
   ARRAY | ROMH     |   0.0+  | 8192 BYTES of ROMH data
 */

static const char snap_module_name[] = "CARTWARP";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int warpspeed_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)warpspeed_enabled) < 0
        || SMW_BA(m, roml_banks, 0x2000) < 0
        || SMW_BA(m, romh_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int warpspeed_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &warpspeed_enabled) < 0) {
            goto fail;
        }
    } else {
        warpspeed_enabled = 0;
    }

    if (0
        || SMR_BA(m, roml_banks, 0x2000) < 0
        || SMR_BA(m, romh_banks, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return warpspeed_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
