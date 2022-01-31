/*
 * actionreplay3.c - Cartridge handling, Action Replay III cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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
#include <string.h>

#include "actionreplay.h"
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
#include "crt.h"

#include "actionreplay3.h"


/*
    Action Replay 3

    - 16k ROM, 2*8kb banks

    io1:

    bit 3 - exrom
    bit 2 - disable
    bit 1 - unused
    bit 0 - bank
*/

/* #define DEBUGAR */

#ifdef DEBUGAR
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int ar_active = 0;
static int ar_reg = 0;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t actionreplay3_io1_peek(uint16_t addr);
static void actionreplay3_io1_store(uint16_t addr, uint8_t value);
static uint8_t actionreplay3_io2_peek(uint16_t addr);
static uint8_t actionreplay3_io2_read(uint16_t addr);
static int actionreplay3_dump(void);

static io_source_t actionreplay3_io1_device = {
    CARTRIDGE_NAME_ACTION_REPLAY3, /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,          /* range for the device */
    0,                             /* read is never valid */
    actionreplay3_io1_store,       /* store function */
    NULL,                          /* NO poke function */
    NULL,                          /* NO read function */
    actionreplay3_io1_peek,        /* peek function */
    actionreplay3_dump,            /* device state information dump function */
    CARTRIDGE_ACTION_REPLAY3,      /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0                              /* insertion order, gets filled in by the registration function */
};

static io_source_t actionreplay3_io2_device = {
    CARTRIDGE_NAME_ACTION_REPLAY3, /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,          /* range for the device */
    1,                             /* read is always valid */
    NULL,                          /* NO store function */
    NULL,                          /* NO poke function */
    actionreplay3_io2_read,        /* read function */
    actionreplay3_io2_peek,        /* peek function */
    actionreplay3_dump,            /* device state information dump function */
    CARTRIDGE_ACTION_REPLAY3,      /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0                              /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *actionreplay3_io1_list_item = NULL;
static io_source_list_t *actionreplay3_io2_list_item = NULL;

/* ---------------------------------------------------------------------*/

static void actionreplay3_io1_store(uint16_t addr, uint8_t value)
{
    int exrom, bank, conf;

    ar_reg = value;

    exrom = (value >> 3) & 1;
    bank = value & 1;
    conf = (bank << CMODE_BANK_SHIFT) | ((exrom ^ 1) << 1);

    if (ar_active) {
        cart_config_changed_slotmain((uint8_t)conf, (uint8_t)conf, CMODE_WRITE);
        if (value & 4) {
            ar_active = 0;
        }
    }
}

static uint8_t actionreplay3_io2_read(uint16_t addr)
{
    actionreplay3_io2_device.io_source_valid = 0;

    if (!ar_active) {
        return 0;
    }

    actionreplay3_io2_device.io_source_valid = 1;

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
    }

    actionreplay3_io2_device.io_source_valid = 0;

    return 0;
}

static uint8_t actionreplay3_io1_peek(uint16_t addr)
{
    return ar_reg;
}

static uint8_t actionreplay3_io2_peek(uint16_t addr)
{
    if (!ar_active) {
        return 0;
    }

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
    }

    return 0;
}

static int actionreplay3_dump(void)
{
    mon_out("EXROM line: %d, bank: %d, cart state: %s\n",
            ar_reg & 8,
            ar_reg & 1,
            (ar_reg & 4) ? "Disabled" : "Enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t actionreplay3_roml_read(uint16_t addr)
{
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

uint8_t actionreplay3_romh_read(uint16_t addr)
{
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

/* ---------------------------------------------------------------------*/

void actionreplay3_freeze(void)
{
    DBG(("AR3: freeze\n"));
    ar_active = 1;
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ);
}

void actionreplay3_config_init(void)
{
    DBG(("AR3: config init\n"));
    ar_active = 1;
    cart_config_changed_slotmain(CMODE_8KGAME | (1 << CMODE_BANK_SHIFT), CMODE_8KGAME | (1 << CMODE_BANK_SHIFT), CMODE_READ);
}

void actionreplay3_reset(void)
{
    DBG(("AR3: reset\n"));
    ar_active = 1;
}

void actionreplay3_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x4000);
    cart_config_changed_slotmain(0 | (1 << CMODE_BANK_SHIFT), 0 | (1 << CMODE_BANK_SHIFT), CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static const export_resource_t export_res = {
    CARTRIDGE_NAME_ACTION_REPLAY3, 0, 1, &actionreplay3_io1_device, &actionreplay3_io2_device, CARTRIDGE_ACTION_REPLAY3
};

static int actionreplay3_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    actionreplay3_io1_list_item = io_source_register(&actionreplay3_io1_device);
    actionreplay3_io2_list_item = io_source_register(&actionreplay3_io2_device);

    return 0;
}

int actionreplay3_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return actionreplay3_common_attach();
}

int actionreplay3_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 1; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 1 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return actionreplay3_common_attach();
}

void actionreplay3_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(actionreplay3_io1_list_item);
    io_source_unregister(actionreplay3_io2_list_item);
    actionreplay3_io1_list_item = NULL;
    actionreplay3_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTAR3 snapshot module format:

   type  | name     | description
   ------------------------------
   BYTE  | active   | cartridge active flag
   BYTE  | register | cartridge register
   ARRAY | ROML     | 16768 BYTES of ROML data
 */

static const char snap_module_name[] = "CARTAR3";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int actionreplay3_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)ar_active) < 0)
        || (SMW_B(m, (uint8_t)ar_reg) < 0)
        || (SMW_BA(m, roml_banks, 0x4000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int actionreplay3_snapshot_read_module(snapshot_t *s)
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

    if (0
        || (SMR_B_INT(m, &ar_active) < 0)
        || (SMR_B_INT(m, &ar_reg) < 0)
        || (SMR_BA(m, roml_banks, 0x4000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return actionreplay3_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
