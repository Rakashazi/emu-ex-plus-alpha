/*
 * blackbox9.c - Cartridge handling, Black BOX 9 cart.
 *
 * Written by
 *  Kamil Zbrog <kamil.zbrog@gmail.com>
 * Based on code written by
 *  Artur Sidor <ff8@poczta.wp.pl>
 * Based on code from VICE written by
 *  ALeX Kazik <alx@kazik.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Andreas Boose <viceteam@t-online.de>
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
#include "blackbox9.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"
#include "log.h"
#include "vicii-phi1.h"

/*
    Black BOX 9

    - 32k ROM, 2*16k (16K Game mode)
      - starts from bank 1 in ultimax mode
    - cart ROM mirror is visible in IO1

    A register is mapped to IO1, which is changed by accessing an address in
    the IO space, the address bits are mapped like this:

    bit 7   -   bank    (inverted on write)
    bit 6   -   exrom
    bit 0   -   game

    GUESS: The register will be disabled (until reset) when the following
           conditions are met:

    - read from io1
    - addr bit 7 is 0
    - addr bit 6 is 1
    - addr bit 0 is 1
*/

/* #define BB9DEBUG */

#ifdef BB9DEBUG
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

/* some prototypes are needed */
static uint8_t blackbox9_io1_peek(uint16_t addr);
static uint8_t blackbox9_io1_read(uint16_t addr);
static void blackbox9_io1_store(uint16_t addr, uint8_t);
static int blackbox9_dump(void);

static io_source_t io1_device = {
    CARTRIDGE_NAME_BLACKBOX9, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, regs:$de00-$deff */
    1,                        /* read is always valid */
    blackbox9_io1_store,      /* store function */
    NULL,                     /* NO poke function */
    blackbox9_io1_read,       /* read function */
    blackbox9_io1_peek,       /* peek function */
    blackbox9_dump,           /* device state information dump function */
    CARTRIDGE_BLACKBOX9,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_list_t *io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_BLACKBOX9, 1, 1, &io1_device, NULL, CARTRIDGE_BLACKBOX9
};

/* ---------------------------------------------------------------------*/

static int currbank = 0;
static int currmode = 0;
static int cart_enabled = 0;

static uint8_t blackbox9_io1_peek(uint16_t addr)
{
    return roml_banks[0x1e00 + (roml_bank << 13) + (addr & 0xff)];
}

static uint8_t blackbox9_io1_read(uint16_t addr)
{
    if (cart_enabled) {
        currbank = (addr >> 7) & 1;
        currmode = ((addr >> 5) & 2) | ((addr & 1) ^ 1);
        /* weird hack to prevent ultimax mode in the powerup screen */
        if ((currmode == CMODE_ULTIMAX) && ((addr & 0x40) == 0x40) && ((addr & 0x01) == 0x00)) {
            DBG(("ultimax mode ignored"));
        } else {
            cart_config_changed_slotmain(CMODE_RAM, (currbank << CMODE_BANK_SHIFT) | currmode, CMODE_READ);
        }
        if (addr == 0x41) {
            cart_enabled = 0;
            DBG(("disabling cart"));
        }
#if 0
        if ((addr != 0x41) && ((currbank == 0) && (currmode == 3))) {
            DBG(("IO1 read  %04x value:%02x  bank:%d mode:%d",
                 addr, roml_banks[0x1e00 + (roml_bank << 13) + (addr & 0xff)], currbank, currmode));
        }
#endif
        return roml_banks[0x1e00 + (roml_bank << 13) + (addr & 0xff)];
    }
    return vicii_read_phi1();
}

void blackbox9_io1_store(uint16_t addr, uint8_t val)
{
    if (cart_enabled) {
        currbank = ((addr >> 7) & 1) ^ 1;
        currmode = ((addr >> 5) & 2) | ((addr & 1) ^ 1);
        DBG(("IO1 store %04x val:%02x bank:%d mode:%d", addr, val, currbank, currmode));
        cart_config_changed_slotmain(CMODE_RAM, (currbank << CMODE_BANK_SHIFT) | currmode, CMODE_WRITE);
    }
}

static int blackbox9_dump(void)
{
    mon_out("current mode: %s\n", cart_config_string(currmode));
    mon_out("IO  at $DE00-$DEFF: %s\n", cart_enabled ? "enabled" : "disabled");
    mon_out("ROM at $8000-$BFFF: %s\n", (currmode != CMODE_RAM) ? "enabled" : "disabled");
    mon_out("ROM bank: %d\n", currbank);
    return 0;
}

/* ---------------------------------------------------------------------*/
void blackbox9_config_init(void)
{
    DBG(("blackbox9_config_init"));
    currmode = CMODE_ULTIMAX;
    currbank = 1;
    cart_enabled = 1;
    cart_config_changed_slotmain(CMODE_RAM, currmode | 1 << CMODE_BANK_SHIFT, CMODE_READ);
}

void blackbox9_config_setup(uint8_t *rawcart)
{
    int i;
    DBG(("blackbox9_config_setup"));
    for (i = 0; i <= 2; i++) {
        memcpy(&roml_banks[0x2000 * i], &rawcart[0x0000 + (0x4000 * i)], 0x2000);
        memcpy(&romh_banks[0x2000 * i], &rawcart[0x2000 + (0x4000 * i)], 0x2000);
    }

    currmode = CMODE_ULTIMAX;
    currbank = 1;
    cart_enabled = 1;
    cart_config_changed_slotmain(CMODE_RAM, currmode | 1 << CMODE_BANK_SHIFT, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int blackbox9_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    io1_list_item = io_source_register(&io1_device);
    return 0;
}

int blackbox9_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return blackbox9_common_attach();
}

int blackbox9_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i, banks = 0;

    for (i = 0; i <= 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 2 || chip.size != 0x4000) {
            break;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            break;
        }
        ++banks;
    }

    if (banks != 2) {
        return -1;
    }

    return blackbox9_common_attach();
}

void blackbox9_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(io1_list_item);
    io1_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

static const char snap_module_name[] = "CARTBLACKBOX9";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int blackbox9_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)currmode) < 0)
        || (SMW_B(m, (uint8_t)currbank) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int blackbox9_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_B_INT(m, &currmode) < 0)
        || (SMR_B_INT(m, &currbank) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return blackbox9_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
