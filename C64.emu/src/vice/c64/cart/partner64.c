/*
 * partner64.c - Cartridge handling, Partner64 cart.
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
#include <string.h>

#include "partner64.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "log.h"
#include "monitor.h"
#include "ram.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"
#include "vicii-phi1.h"

#define DBGPARTNER

#ifdef DBGPARTNER
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
    Partner 64 (Timeworks)

    WARNING: all of the following is guesswork based on a disassembly and some
             educated guessing

    - 16K ROM, ROML at $8000 and ROMH at $e000
    - 8K ROM at $a000

    IO1 (writes):

    $de00    - disable cart
    $def0    - disable cart
    $def1    - enable cart
    $deff    - acknowledge freeze

*/

#define CART_RAM_SIZE   0x2000

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t partner64_io1_read(uint16_t addr);
static void partner64_io1_store(uint16_t addr, uint8_t value);
static int partner64_dump(void);

static io_source_t partner64_io1_device = {
    CARTRIDGE_NAME_PARTNER64, /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    1,                        /* read is always valid */
    partner64_io1_store,      /* store function */
    NULL,                     /* NO poke function */
    partner64_io1_read,       /* read function */
    NULL,                     /* NO peek function */
    partner64_dump,           /* device state information dump function */
    CARTRIDGE_PARTNER64,      /* cartridge ID */
    IO_PRIO_NORMAL,           /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_list_t *partner64_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_PARTNER64, 1, 1, &partner64_io1_device, NULL, CARTRIDGE_PARTNER64
};

/* ---------------------------------------------------------------------*/

static void partner64_io1_store(uint16_t addr, uint8_t value)
{
    if (addr == 0x00) {
        /* disable cartridge (before starting BASIC) (@ $8471) */
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
    } else if (addr == 0xf0) {
        /* disable cartridge */
        cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
    } else if (addr == 0xf1) {
        /* enable cartridge (@ $dec0)*/
        cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
    } else if (addr == 0xff) {
        /* enable cartridge (at freeze start) (@ $8012) */
        cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ | CMODE_RELEASE_FREEZE);
    } else {
        DBG(("partner64_io1_store %04x %02x\n", addr, value));
    }
}

static uint8_t partner64_io1_read(uint16_t addr)
{
    return roml_banks[(addr + 0x1e00) & 0x1fff];
}

static int partner64_dump(void)
{
    /* FIXME */
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t partner64_roml_read(uint16_t addr)
{
    return roml_banks[addr & 0x1fff];
}

uint8_t partner64_romh_read(uint16_t addr)
{
    return romh_banks[addr & 0x1fff];
}

uint8_t partner64_a000_bfff_read(uint16_t addr)
{
    return export_ram0[addr & 0x1fff];
}

void partner64_a000_bfff_store(uint16_t addr, uint8_t value)
{
    export_ram0[addr & 0x1fff] = value;
}

/* ---------------------------------------------------------------------*/

/* FIXME: this still needs to be tweaked to match the hardware */
static RAMINITPARAM ramparam = {
    .start_value = 255,
    .value_invert = 2,
    .value_offset = 1,

    .pattern_invert = 0x100,
    .pattern_invert_value = 255,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

void partner64_powerup(void)
{
    ram_init_with_pattern(export_ram0, CART_RAM_SIZE, &ramparam);
}

void partner64_freeze(void)
{
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

void partner64_config_init(void)
{
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

void partner64_reset(void)
{
}

void partner64_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, rawcart + 0x2000, 0x2000);
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int partner64_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    partner64_io1_list_item = io_source_register(&partner64_io1_device);

    return 0;
}

int partner64_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return partner64_common_attach();
}

int partner64_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 0 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart + (i * 0x2000), 0, &chip, fd)) {
            return -1;
        }
    }

    return partner64_common_attach();
}

void partner64_detach(void)
{
    io_source_unregister(partner64_io1_list_item);
    partner64_io1_list_item = NULL;
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTAR snapshot module format:

   type  | name   | description
   ----------------------------
   BYTE  | active | cartridge is active
   ARRAY | ROML   | 8192 BYTES of ROML data
   ARRAY | ROMH   | 8192 BYTES of ROMH data
   ARRAY | RAM    | 8192 BYES of RAM data
 */

static const char snap_module_name[] = "CARTPARTNER64";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int partner64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int partner64_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return partner64_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
