/*
 * rgcd.c - Cartridge handling, RGCD cart.
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

/* #define RGCD_DEBUG */

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
#include "resources.h"
#include "rgcd.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef RGCD_DEBUG
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    "RGCD" 64k Cartridge

    - 64Kb (8 banks)
    - ROM is always mapped in at $8000-$9FFF (8k game).

    - 1 register at io1 / de00:

    bit 0-2   bank number
    bit 3     exrom (1 = cart rom and I/O disabled until reset/powercycle)

    the eprom cartridge by "hucky" works exactly the same, except the banks
    are in reverse order. it is implemented as hardware revision 1.

    the Hucky cartridge may also be equipped with 8k, 16k or 32k EPROMs
*/

#define MAXBANKS 8

static uint8_t regval = 0;
static uint8_t disabled = 0;
static int rgcd_revision = 0;
static int bankmask = 7;

static void rgcd_io1_store(uint16_t addr, uint8_t value)
{
    regval = value & 0x0f;
    if (rgcd_revision == RGCD_REV_HUCKY) {
        value ^= 7;
    }
    cart_set_port_game_slotmain(0);
    disabled |= (value & 0x08) ? 1 : 0;
    if (disabled) {
        /* turn off cart ROM */
        cart_set_port_exrom_slotmain(0);
    } else {
        cart_romlbank_set_slotmain(value & bankmask);
        cart_set_port_exrom_slotmain(1);
    }
    cart_port_config_changed_slotmain();
    DBG(("RGCD: Reg: %02x (Bank: %d of %d, %s)\n",
        regval, (regval & bankmask), bankmask + 1, disabled ? "disabled" : "enabled"));
}

static uint8_t rgcd_io1_peek(uint16_t addr)
{
    return regval;
}

static int rgcd_dump(void)
{
    mon_out("Reg: %02x (Bank: %d of %d, %s)\n",
            regval, (regval & bankmask), bankmask + 1, disabled ? "disabled" : "enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t rgcd_device = {
    CARTRIDGE_NAME_RGCD,   /* name of the device */
    IO_DETACH_CART,        /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,  /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                     /* read is never valid, reg is write only */
    rgcd_io1_store,        /* store function */
    NULL,                  /* NO poke function */
    NULL,                  /* NO read function */
    rgcd_io1_peek,         /* peek function */
    rgcd_dump,             /* device state information dump function */
    CARTRIDGE_RGCD,        /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *rgcd_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_RGCD, 0, 1, &rgcd_device, NULL, CARTRIDGE_RGCD
};

/* ---------------------------------------------------------------------*/

void rgcd_reset(void)
{
    disabled = 0;
    cart_set_port_exrom_slotmain(1);
}

void rgcd_config_init(void)
{
    disabled = 0;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    rgcd_io1_store((uint16_t)0xde00, 0);
}

void rgcd_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * MAXBANKS);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int set_rgcd_revision(int val, void *param)
{
    rgcd_revision = val ? 1 : 0;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "RGCDrevision", RGCD_REV_RGCD_64K, RES_EVENT_NO, NULL,
      &rgcd_revision, set_rgcd_revision, NULL },
    RESOURCE_INT_LIST_END
};

int rgcd_resources_init(void)
{
    return resources_register_int(resources_int);
}

void rgcd_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
    { "-rgcdrev", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RGCDrevision", NULL,
      "<Revision>", "Set RGCD Revision (0: RGCD 64K, 1: Hucky)" },
    CMDLINE_LIST_END
};

int rgcd_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int rgcd_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    rgcd_list_item = io_source_register(&rgcd_device);
    return 0;
}

int rgcd_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return rgcd_common_attach();
}

int rgcd_crt_attach(FILE *fd, uint8_t *rawcart, uint8_t revision)
{
    crt_chip_header_t chip;
    int highbank = 0;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        printf("bank %d %04x %04x\n", chip.bank, chip.start, chip.size);
        if ((chip.bank >= MAXBANKS) || ((chip.start != 0x8000) && (chip.start != 0xa000)) || (chip.size != 0x2000)) {
            return -1;
        }
        if (chip.bank > highbank) {
            highbank = chip.bank;
        }
        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    if (revision > 0) {
        rgcd_revision = RGCD_REV_HUCKY;
        if ((highbank != 7) && (highbank != 3) && (highbank != 1) && (highbank != 0)) {
            return -1;
        }
        bankmask = highbank;
    } else {
        if (highbank != 7) {
            return -1;
        }
        bankmask = 7;
    }

    return rgcd_common_attach();
}

void rgcd_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(rgcd_list_item);
    rgcd_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTRGCD snapshot module format:

   type  | name     | version | description
   ----------------------------------------
   BYTE  | regval   |   0.1+  | register
   BYTE  | disabled |   0.2   | cartridge disabled flag
   BYTE  | revision |   0.3   | hw revision
   BYTE  | mask     |   0.4   | bank mask
   ARRAY | ROML     |   0.1+  | 65536 BYTES of ROML data

   Note: for some reason this module was created at rev 0.1, so there never was a 0.0
 */

static const char snap_module_name[] = "CARTRGCD";
#define SNAP_MAJOR   0
#define SNAP_MINOR   4

int rgcd_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)regval) < 0
        || SMW_B(m, (uint8_t)disabled) < 0
        || SMW_B(m, (uint8_t)rgcd_revision) < 0
        || SMW_B(m, (uint8_t)bankmask) < 0
        || SMW_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int rgcd_snapshot_read_module(snapshot_t *s)
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

    /* new in 0.2 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 2)) {
        if (SMR_B(m, &disabled) < 0) {
            goto fail;
        }
    } else {
        disabled = 0;
    }

    /* new in 0.3 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 3)) {
        if (SMR_B_INT(m, &rgcd_revision) < 0) {
            goto fail;
        }
    } else {
        rgcd_revision = 0;
    }

    /* new in 0.4 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 4)) {
        if (SMR_B_INT(m, &bankmask) < 0) {
            goto fail;
        }
    } else {
        bankmask = 7;
    }

    if (SMR_BA(m, roml_banks, 0x2000 * MAXBANKS) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    if (rgcd_common_attach() < 0) {
        return -1;
    }
    rgcd_io1_store(0xde00, regval);
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
