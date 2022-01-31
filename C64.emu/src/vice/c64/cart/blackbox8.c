/*
 * blackbox8.c - Cartridge handling, Blackbox v8 cart.
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
#include "blackbox8.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
   Black Box V8

   32k or 64k, 2 or 4 16K banks
   
   writing to IO2 sets the cartridge config:
   A0 - EXROM
   A1 - GAME
   A2 - bank lsb
   A3 - bank msb
*/

static int bb8_rom_banks = 4;
static uint8_t regval = 0;

/* some prototypes are needed */
static void blackbox8_io2_store(uint16_t addr, uint8_t value);
static int blackbox8_dump(void);

static io_source_t final3_io2_device = {
    CARTRIDGE_NAME_BLACKBOX8, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,     /* range for the device, regs:$df00-$dfff */
    1,                        /* read is always valid */
    blackbox8_io2_store,      /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    blackbox8_dump,           /* device state information dump function */
    CARTRIDGE_BLACKBOX8,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0                         /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *final3_io2_list_item = NULL;

static const export_resource_t export_res_v3 = {
    CARTRIDGE_NAME_BLACKBOX8, 1, 1, NULL, &final3_io2_device, CARTRIDGE_BLACKBOX8
};

/* ---------------------------------------------------------------------*/

void blackbox8_io2_store(uint16_t addr, uint8_t value)
{
    uint8_t mode, bank;

    regval = addr & 0x0f;

    /* printf("io2 write %04x %02x\n", addr, value); */
    mode = (addr & 3) ^ 1;
    bank = ((addr >> 2) & 3) ^ 3;
    bank &= (bb8_rom_banks - 1);
    roml_bank = romh_bank = bank;
    /* printf("switch to mode: %s bank: %d\n", cart_config_string(mode), bank); */
    cart_config_changed_slotmain(mode, mode | (bank << CMODE_BANK_SHIFT), CMODE_WRITE);
}

static int blackbox8_dump(void)
{
    mon_out("Bank: %d of %d, mode: %s\n",
            ((regval >> 2) ^ 3) & (bb8_rom_banks - 1), bb8_rom_banks,
            cart_config_string((regval & 3) ^ 1));
    return 0;
}

/* ---------------------------------------------------------------------*/

void blackbox8_config_init(void)
{
    roml_bank = romh_bank = (bb8_rom_banks - 1);
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME | (roml_bank << CMODE_BANK_SHIFT), CMODE_READ);
}

void blackbox8_config_setup(uint8_t *rawcart)
{
    int i;
    for (i = 0; i <= bb8_rom_banks; i++) {
        memcpy(&roml_banks[0x2000 * i], &rawcart[0x0000 + (0x4000 * i)], 0x2000);
        memcpy(&romh_banks[0x2000 * i], &rawcart[0x2000 + (0x4000 * i)], 0x2000);
    }
    roml_bank = romh_bank = (bb8_rom_banks - 1);

    /* FIXME: Triggers false positive with the static analyzer:
     *        "The result of the left shift is undefined because the left
     *         operand is negative"
     */
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME | (roml_bank << CMODE_BANK_SHIFT), CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int blackbox8_common_attach(void)
{
    if (export_add(&export_res_v3) < 0) {
        return -1;
    }

    final3_io2_list_item = io_source_register(&final3_io2_device);

    return 0;
}

int blackbox8_bin_attach(const char *filename, uint8_t *rawcart)
{
    bb8_rom_banks = 2;
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        bb8_rom_banks = 4;
    }

    return blackbox8_common_attach();
}

int blackbox8_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i, banks = 0;

    for (i = 0; i <= 4; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 4 || chip.size != 0x4000) {
            break;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            break;
        }
        ++banks;
    }

    if ((banks != 2) && (banks != 4)) {
        return -1;
    }
    bb8_rom_banks = banks;

    return blackbox8_common_attach();
}

void blackbox8_detach(void)
{
    export_remove(&export_res_v3);
    io_source_unregister(final3_io2_list_item);
    final3_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTBB8 snapshot module format:

   type  | name        | version | description
   -------------------------------------------
   BYTE  | ROML banks  |   1.1   | amount of ROML banks
   BYTE  | register    |   1.1   | register
   ARRAY | ROML        |   1.1   | 32768 or 65536 BYTES of ROML data
   ARRAY | ROMH        |   1.1   | 32768 or 65536 BYTES of ROML data

 */

static const char snap_module_name[] = "CARTBB8";
#define SNAP_MAJOR   1
#define SNAP_MINOR   1

int blackbox8_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)bb8_rom_banks) < 0
        || SMW_B(m, regval) < 0
        || SMW_BA(m, roml_banks, 0x2000 * bb8_rom_banks) < 0
        || SMW_BA(m, romh_banks, 0x2000 * bb8_rom_banks) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int blackbox8_snapshot_read_module(snapshot_t *s)
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
        || SMR_B_INT(m, &bb8_rom_banks) < 0
        || SMR_B(m, &regval) < 0 
        || SMR_BA(m, roml_banks, 0x2000 * bb8_rom_banks) < 0
        || SMR_BA(m, romh_banks, 0x2000 * bb8_rom_banks) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return blackbox8_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
