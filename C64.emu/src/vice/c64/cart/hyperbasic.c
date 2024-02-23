/*
 * hyperbasic.c - Cartridge handling, Hyper BASIC cart.
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

#define HYPERBASIC_DEBUG

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "log.h"
#include "resources.h"
#include "hyperbasic.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef HYPERBASIC_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "HYPERBASIC" 64k Cartridge by "SHS-BUDE"

    - 64Kb (8 banks)
    - ROM is always mapped in at $8000-$9FFF (8k game).

    - 1 register at io1 / de00:

    bit 0 - _CE for ROM2, 0 = enable (don't select both)
    bit 1 - _CE for ROM1, 0 = enable (don't select both)
    (bit 2 - probably unused)
    (bit 3 - probably unused)
    bit 4 - A13
    bit 5 - A14
    bit 6 and/or 7 - _EXROM,  0 = make cart visible
*/

#define NUMBANKS    8

static uint8_t regval = 0;

static uint8_t disabled = 0;
static uint8_t curr_bank = 0;
static uint8_t enable1 = 0;
static uint8_t enable2 = 0;

static void hyperbasic_io1_store(uint16_t addr, uint8_t value)
{
    regval = value;
    disabled = ((value >> 7) & 1) & ((value >> 6) & 1);
    enable1 = ((value >> 1) & 1) ^ 1;
    enable2 = (value & 1) ^ 1;
    curr_bank = (value >> 4) & 3;

    if (enable1 && enable2) {
        /* both ROMs selected */
        log_message(LOG_DEFAULT, "Hyper-BASIC: WARNING! both ROMs are selected!");
        cart_set_port_exrom_slotmain(0); /* FIXME: this is not really what happens */
    } else if (!enable1 && !enable2) {
        /* no ROM is selected */
        cart_set_port_exrom_slotmain(0); /* FIXME: this is not really what happens */
    } else {
        /* FIXME: this is not really what happens */
        curr_bank += (enable2 << 2);

        DBG(("hyperbasic_io1_store %04x %02x (disabled:%d rom1CE:%d rom2CE:%d bank:%d)\n",
            addr, value, disabled, enable1, enable2, curr_bank));

        cart_set_port_exrom_slotmain(disabled ^ 1);
        cart_romlbank_set_slotmain(curr_bank);
    }

    cart_port_config_changed_slotmain();
}

static uint8_t hyperbasic_io1_peek(uint16_t addr)
{
    return regval;
}

static int hyperbasic_dump(void)
{
    mon_out("Register: %02x\n", regval);
    mon_out("Cartridge is %s\n", disabled ? "disabled" : "enabled");
    mon_out("ROM1: %s ROM2: %s (Bank: %d of 8)\n",
            enable1 ? "selected" : "deselected", enable2 ? "selected" : "deselected", curr_bank);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t hyperbasic_device = {
    CARTRIDGE_NAME_HYPERBASIC,   /* name of the device */
    IO_DETACH_CART,        /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,  /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                     /* read is never valid, reg is write only */
    hyperbasic_io1_store,        /* store function */
    NULL,                  /* NO poke function */
    NULL,                  /* NO read function */
    hyperbasic_io1_peek,         /* peek function */
    hyperbasic_dump,             /* device state information dump function */
    CARTRIDGE_HYPERBASIC,        /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0,                     /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE         /* NO mirroring */
};

static io_source_list_t *hyperbasic_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_HYPERBASIC, 0, 1, &hyperbasic_device, NULL, CARTRIDGE_HYPERBASIC
};

/* ---------------------------------------------------------------------*/

void hyperbasic_reset(void)
{
    disabled = 0;
    cart_set_port_exrom_slotmain(1);
}

void hyperbasic_config_init(void)
{
    disabled = 0;
    cart_config_changed_slotmain(CMODE_8KGAME + (3 << CMODE_BANK_SHIFT), CMODE_8KGAME + (3 << CMODE_BANK_SHIFT), CMODE_READ);
}

void hyperbasic_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * NUMBANKS);
    cart_config_changed_slotmain(CMODE_8KGAME + (3 << CMODE_BANK_SHIFT), CMODE_8KGAME + (3 << CMODE_BANK_SHIFT), CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int hyperbasic_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    hyperbasic_list_item = io_source_register(&hyperbasic_device);
    return 0;
}

int hyperbasic_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return hyperbasic_common_attach();
}

int hyperbasic_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int highbank = 0;
    int numbanks = 0;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank >= NUMBANKS) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x2000)) {
            return -1;
        }
        if (chip.bank > highbank) {
            highbank = chip.bank;
        }
        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        numbanks++;
    }

    if ((highbank != (NUMBANKS - 1)) || (numbanks != NUMBANKS)) {
        return -1;
    }

    return hyperbasic_common_attach();
}

void hyperbasic_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(hyperbasic_list_item);
    hyperbasic_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTHYPERBASIC snapshot module format:

   type  | name      | version | description
   -----------------------------------------
   BYTE  | regval    |   0.0   | register
   ARRAY | ROML      |   0.0   | 65536 BYTES of ROML data

 */

static const char snap_module_name[] = "CARTHYPERBASIC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int hyperbasic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)regval) < 0
        || SMW_BA(m, roml_banks, 0x2000 * NUMBANKS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int hyperbasic_snapshot_read_module(snapshot_t *s)
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

    if (SMR_B(m, &regval) < 0) {
        goto fail;
    }

    if (SMR_BA(m, roml_banks, 0x2000 * NUMBANKS) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    if (hyperbasic_common_attach() < 0) {
        return -1;
    }
    hyperbasic_io1_store(0xde00, regval);
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
