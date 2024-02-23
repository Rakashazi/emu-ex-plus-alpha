/*
 * freezeframe2.c - Cartridge handling, Freeze Frame MK2/MK3 cart.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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
#include "freezeframe2.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define FF2DEBUG */

#ifdef FF2DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    FIXME: this implementation is based on vague guesses on how the hardware
           really works.

    Evesham Micros "Freeze Frame MK2" and "Freeze Frame MK3 (v1)"

    - 2 Buttons (Freeze, Reset)
    - 16k ROM

    - reading io1 (the software uses de00 only it seems)
      - disables cartridge ROM
      - in freeze mode switch to 8k mode

    - reading io2 (the software uses df00 only it seems)
      - switches to 16k game mode
      - ROM bank 0 is mapped to 8000
      - ROM bank 1 is mapped to A000

    - reset
      - enables 8K game mode
      - ROM bank 0 is mapped to 8000

    - freeze
      - enables ultimax mode
      - ROM bank 0 is mapped to 8000
      - ROM bank 0 is mapped to E000
*/

#define FREEZE_FRAME_MK2_CART_SIZE (16 * 0x400)

static int roml_toggle = 0;  /* when set, bank 1 will be used for roml */
static int freezemode = 0;
/* ---------------------------------------------------------------------*/

static uint8_t freezeframe2_io1_read(uint16_t addr)
{
    DBG(("io1 r %04x\n", addr));
    /* if (addr == 0) */ {
        if (freezemode == 1) {
            cart_config_changed_slotmain(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
            DBG(("Freeze Frame MK2: switch to 8k in freeze mode\n"));
            freezemode = 0;
        } else {
            cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
            DBG(("Freeze Frame MK2: disabled\n"));
        }
    }

    return 0; /* invalid */
}

static uint8_t freezeframe2_io1_peek(uint16_t addr)
{
    return 0; /* invalid */
}

static void freezeframe2_io1_store(uint16_t addr, uint8_t value)
{
    DBG(("io1 w %04x %02x\n", addr, value));
}

static uint8_t freezeframe2_io2_read(uint16_t addr)
{
    DBG(("io2 r %04x\n", addr));
    /* if (addr == 0) */ {
        roml_toggle = 1;
        cart_config_changed_slotmain(CMODE_RAM, CMODE_16KGAME, CMODE_READ);
        DBG(("Freeze Frame MK2: switching to 16k game mapping\n"));
    }
    return 0; /* invalid */
}

static uint8_t freezeframe2_io2_peek(uint16_t addr)
{
    return 0; /* invalid */
}

static void freezeframe2_io2_store(uint16_t addr, uint8_t value)
{
    DBG(("io2 w %04x %02x\n", addr, value));
}

static io_source_t freezeframe2_io1_device = {
    CARTRIDGE_NAME_FREEZE_FRAME_MK2, /* name of the device */
    IO_DETACH_CART,                  /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,           /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,            /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                               /* read is never valid */
    freezeframe2_io1_store,          /* store function */
    NULL,                            /* NO poke function */
    freezeframe2_io1_read,           /* read function */
    freezeframe2_io1_peek,           /* peek function */
    NULL,                            /* TODO: device state information dump function */
    CARTRIDGE_FREEZE_FRAME_MK2,      /* cartridge ID */
    IO_PRIO_NORMAL,                  /* normal priority, device read needs to be checked for collisions */
    0,                               /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                   /* NO mirroring */
};

static io_source_t freezeframe2_io2_device = {
    CARTRIDGE_NAME_FREEZE_FRAME_MK2, /* name of the device */
    IO_DETACH_CART,                  /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,           /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,            /* range for the device, address is ignored, reg:$df00, mirrors:$df01-$dfff */
    0,                               /* read is never valid */
    freezeframe2_io2_store,          /* store function */
    NULL,                            /* NO poke function */
    freezeframe2_io2_read,           /* read function */
    freezeframe2_io2_peek,           /* peek function */
    NULL,                            /* TODO: device state information dump function */
    CARTRIDGE_FREEZE_FRAME_MK2,      /* cartridge ID */
    IO_PRIO_NORMAL,                  /* normal priority, device read needs to be checked for collisions */
    0,                               /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                   /* NO mirroring */
};

static io_source_list_t *freezeframe2_io1_list_item = NULL;
static io_source_list_t *freezeframe2_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_FREEZE_FRAME_MK2, 1, 1, &freezeframe2_io1_device, &freezeframe2_io2_device, CARTRIDGE_FREEZE_FRAME_MK2
};

/* ---------------------------------------------------------------------*/

uint8_t freezeframe2_roml_read(uint16_t addr)
{
    if (roml_toggle) {
        return romh_banks[addr & 0x1fff];
    }
    return roml_banks[addr & 0x1fff];
}

/* ---------------------------------------------------------------------*/

void freezeframe2_reset(void)
{
    roml_toggle = 0;
    freezemode = 0;
    cart_config_changed_slotmain(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
    DBG(("Freeze Frame MK2: reset\n"));
}

void freezeframe2_freeze(void)
{
    DBG(("Freeze Frame MK2: freeze\n"));
    roml_toggle = 1;
    freezemode = 1;
    cart_config_changed_slotmain(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ | CMODE_RELEASE_FREEZE);
}

void freezeframe2_config_init(void)
{
    cart_config_changed_slotmain(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
}

void freezeframe2_config_setup(uint8_t *rawcart)
{
    roml_toggle = 0;
    memcpy(roml_banks, rawcart, 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    cart_config_changed_slotmain(CMODE_RAM, CMODE_8KGAME | (0 << CMODE_BANK_SHIFT), CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int freezeframe2_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    freezeframe2_io1_list_item = io_source_register(&freezeframe2_io1_device);
    freezeframe2_io2_list_item = io_source_register(&freezeframe2_io2_device);

    return 0;
}

int freezeframe2_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("Freeze Frame MK2: bin attach '%s'\n", filename));
    if (util_file_load(filename, rawcart, FREEZE_FRAME_MK2_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return freezeframe2_common_attach();
}

/*
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $4000 $4010
 *
 */
int freezeframe2_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }
    DBG(("ffmk2 attach: chip.size %04x\n", chip.size));
    /* first the new format */
    if (chip.size == 0x4000) {
        if ((chip.bank > 1) || (chip.start != 0x8000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, (chip.bank << 14), &chip, fd)) {
            return -1;
        }
    } else {
        return -1;
    }
    return freezeframe2_common_attach();
}

void freezeframe2_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(freezeframe2_io1_list_item);
    io_source_unregister(freezeframe2_io2_list_item);
    freezeframe2_io1_list_item = NULL;
    freezeframe2_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTFREEZEM snapshot module format:

   type  | name         | version | description
   --------------------------------------------
   BYTE  | ROML toggle  |   0.0+  | ROML toggle flag
   BYTE  | freeze mode  |   0.0+  | flag: are we in freeze mode
   ARRAY | ROML         |   0.0+  | 8192 BYTES of ROML data
   ARRAY | ROMH         |   0.0+  | 8192 BYTES of ROMH data
 */

static const char snap_module_name[] = "CARTFFMK2";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int freezeframe2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)roml_toggle) < 0
        || SMW_B(m, (uint8_t)freezemode) < 0
        || SMW_BA(m, roml_banks, 0x2000) < 0
        || SMW_BA(m, romh_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int freezeframe2_snapshot_read_module(snapshot_t *s)
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
        || SMR_B_INT(m, &roml_toggle) < 0
        || SMR_B_INT(m, &freezemode) < 0) {
        goto fail;
    }

    if (0
        || SMR_BA(m, roml_banks, 0x2000) < 0
        || SMR_BA(m, romh_banks, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return freezeframe2_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
