/*
 * actionreplay.c - Cartridge handling, Action Replay cart.
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
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Action Replay 4.2, 5, 6 (Hardware stayed the same)

    32K rom, 4*8k pages
    8K ram

    io1 (writes)

    7    extra ROM bank selector (A15) (unused)
    6    1 = resets FREEZE-mode (turns back to normal mode)
    5    1 = enable RAM at ROML ($8000-$9FFF) &
            I/O2 ($DF00-$DFFF = $9F00-$9FFF)
    4    ROM bank selector high (A14)
    3    ROM bank selector low  (A13)
    2    1 = disable cartridge (turn off $DE00)
    1    1 = /EXROM high
    0    1 = /GAME low

    io2 (r/w)
        cart RAM (if enabled) or cart ROM
*/

static int ar_active;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE actionreplay_io1_peek(WORD addr);
static void actionreplay_io1_store(WORD addr, BYTE value);
static BYTE actionreplay_io2_read(WORD addr);
static BYTE actionreplay_io2_peek(WORD addr);
static void actionreplay_io2_store(WORD addr, BYTE value);
static int actionreplay_dump(void);

static io_source_t action_replay_io1_device = {
    CARTRIDGE_NAME_ACTION_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    actionreplay_io1_store,
    NULL,
    actionreplay_io1_peek,
    actionreplay_dump,
    CARTRIDGE_ACTION_REPLAY,
    0,
    0
};

static io_source_t action_replay_io2_device = {
    CARTRIDGE_NAME_ACTION_REPLAY,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    actionreplay_io2_store,
    actionreplay_io2_read,
    actionreplay_io2_peek,
    actionreplay_dump,
    CARTRIDGE_ACTION_REPLAY,
    0,
    0
};

static io_source_list_t *action_replay_io1_list_item = NULL;
static io_source_list_t *action_replay_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_ACTION_REPLAY, 1, 1, &action_replay_io1_device, &action_replay_io2_device, CARTRIDGE_ACTION_REPLAY
};

/* ---------------------------------------------------------------------*/

static BYTE regvalue;

static void actionreplay_io1_store(WORD addr, BYTE value)
{
    unsigned int mode = 0;

    if (ar_active) {
        regvalue = value;
        if (value & 0x40) {
            mode |= CMODE_RELEASE_FREEZE;
        }
        if (value & 0x20) {
            mode |= CMODE_EXPORT_RAM;
        }
        cart_config_changed_slotmain((BYTE)(value & 3), (BYTE)((value & 3) | (((value >> 3) & 3) << CMODE_BANK_SHIFT)), (unsigned int)(mode | CMODE_WRITE));

        if (value & 4) {
            ar_active = 0;
        }
    }
}

static BYTE actionreplay_io1_peek(WORD addr)
{
    return regvalue;
}

static BYTE actionreplay_io2_read(WORD addr)
{
    action_replay_io2_device.io_source_valid = 0;

    if (!ar_active) {
        return 0;
    }

    action_replay_io2_device.io_source_valid = 1;

    if (export_ram) {
        return export_ram0[0x1f00 + (addr & 0xff)];
    }

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
        case 2:
            return roml_banks[(addr & 0x1fff) + 0x4000];
        case 3:
            return roml_banks[(addr & 0x1fff) + 0x6000];
    }

    action_replay_io2_device.io_source_valid = 0;

    return 0;
}

static BYTE actionreplay_io2_peek(WORD addr)
{
    if (!ar_active) {
        return 0;
    }

    if (export_ram) {
        return export_ram0[0x1f00 + (addr & 0xff)];
    }

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
        case 2:
            return roml_banks[(addr & 0x1fff) + 0x4000];
        case 3:
            return roml_banks[(addr & 0x1fff) + 0x6000];
    }

    return 0;
}

static void actionreplay_io2_store(WORD addr, BYTE value)
{
    if (ar_active) {
        if (export_ram) {
            export_ram0[0x1f00 + (addr & 0xff)] = value;
        }
    }
}

static int actionreplay_dump(void)
{
    mon_out("$8000-$9FFF/$DF00-$DFFF: %s, ROM bank: %d, EXROM line: %s, GAME line: %s, cart state: %s\n",
            (regvalue & 0x20) ? "RAM" : "ROM",
            (regvalue & 0x18) >> 3,
            (regvalue & 2) ? "high" : "low",
            (regvalue & 1) ? "low" : "high",
            (regvalue & 4) ? "disabled" : "enabled");
    return 0;
}


/* ---------------------------------------------------------------------*/

BYTE actionreplay_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[addr & 0x1fff];
    }

    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void actionreplay_roml_store(WORD addr, BYTE value)
{
    if (export_ram) {
        export_ram0[addr & 0x1fff] = value;
    }
}

/* ---------------------------------------------------------------------*/

void actionreplay_freeze(void)
{
    ar_active = 1;
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_EXPORT_RAM);
}

void actionreplay_config_init(void)
{
    ar_active = 1;
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

void actionreplay_reset(void)
{
    ar_active = 1;
}

void actionreplay_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x8000);
    memcpy(romh_banks, rawcart, 0x8000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int actionreplay_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    action_replay_io1_list_item = io_source_register(&action_replay_io1_device);
    action_replay_io2_list_item = io_source_register(&action_replay_io2_device);

    return 0;
}

int actionreplay_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return actionreplay_common_attach();
}

int actionreplay_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 3; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 3 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return actionreplay_common_attach();
}

void actionreplay_detach(void)
{
    io_source_unregister(action_replay_io1_list_item);
    io_source_unregister(action_replay_io2_list_item);
    action_replay_io1_list_item = NULL;
    action_replay_io2_list_item = NULL;
    c64export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTAR"

int actionreplay_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)ar_active) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, romh_banks, 0x8000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int actionreplay_snapshot_read_module(snapshot_t *s)
{
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

    if (0
        || (SMR_B_INT(m, &ar_active) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, romh_banks, 0x8000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return actionreplay_common_attach();
}
