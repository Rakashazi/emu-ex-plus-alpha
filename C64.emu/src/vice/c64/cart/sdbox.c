/*
 * sdbox.c - Cartridge handling, SD-BOX cart.
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

/* #define DEBUG_SDBOX */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "sdbox.h"
#include "log.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

#ifdef DEBUG_SDBOX
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
    "SD Box"
    
    - 128k ROM, 8*16kb banks (16k GAME mode)
    
    one register at de00:
    
    bit 0-3     ROM bank
    bit 4       reset SD
    bit 5       CE ROM
    bit 6       EXROM
    bit 7       register enable
    
    additionally there are 3 "ram cells" at de01-de03
    
*/

static uint8_t currbank = 0;
static uint8_t reg_enable = 1;

static uint8_t regval = 0;
static uint8_t regs[4];

static uint8_t sdbox_io1_read(uint16_t addr)
{
    addr &= 0xff;
    if (reg_enable) {
        switch (addr) {
            case 0:
                DBG(("io1 read %02x %02x\n", addr, regs[addr]));
                return regs[addr]; /* FIXME: is the register readable? */
                break;
            case 1:
            case 2:
            case 3:
                DBG(("io1 read %02x %02x\n", addr, regs[addr]));
                return regs[addr];
                break;
            default:
                DBG(("io1 invalid read at %04x\n", addr));
                break;
        }
    } else {
        DBG(("io1 read %02x (disabled)\n", addr));
    }
    return 0;
}

static void sdbox_io1_store(uint16_t addr, uint8_t value)
{
    uint8_t exrom;
    addr &= 0xff;
    if (reg_enable) {
        switch (addr) {
            case 0:
                currbank = value & 0xf;
                exrom = ((value >> 6) ^ 1) & 1;
                if (exrom) {
                    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME | (currbank << CMODE_BANK_SHIFT), CMODE_WRITE);
                } else {
                    cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM | (currbank << CMODE_BANK_SHIFT), CMODE_WRITE);
                }
                reg_enable = (value >> 7) ^ 1;          
                regval = regs[addr] = value;
                DBG(("io1 write %02x value: $%02x bank: %d exrom: %d\n", 
                    addr, value, roml_bank, exrom));
                break;
            case 1:
            case 2:
            case 3:
                regs[addr] = value;
                DBG(("io1 write %02x value: $%02x\n", addr, value));
                break;
            default:
                DBG(("io1 invalid write at %04x\n", addr));
                break;
        }
    } else {
        DBG(("io1 write %02x value: $%02x (disabled)\n", addr, value));
    }
}

static int sdbox_dump(void)
{
    mon_out("Register: %02x (%s)\n", regval, reg_enable ? "enabled" : "disabled");
    mon_out("ROM Bank: %d\n", currbank);
    mon_out("on chip RAM: %02x %02x %02x\n", regs[1], regs[2], regs[3]);
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t sdbox_device = {
    CARTRIDGE_NAME_SDBOX,  /* name of the device */
    IO_DETACH_CART,        /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,  /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    1,                     /* read is always valid */
    sdbox_io1_store,       /* store function */
    NULL,                  /* NO poke function */
    sdbox_io1_read,        /* read function */
    NULL,                  /* NO peek function */
    sdbox_dump,            /* device state information dump function */
    CARTRIDGE_SDBOX,       /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *sdbox_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_SDBOX, 1, 1, &sdbox_device, NULL, CARTRIDGE_SDBOX
};

/* ---------------------------------------------------------------------*/

void sdbox_config_init(void)
{
    /* 16k configuration */
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    reg_enable = 1;
    currbank = 0;
}

void sdbox_ram_init(void)
{
    memset(regs, 0, 4);
}

void sdbox_config_setup(uint8_t *rawcart)
{
    int i;
    for (i = 0; i < 8; i++) {
        memcpy(&roml_banks[i * 0x2000], &rawcart[0x4000 * i], 0x2000);
        memcpy(&romh_banks[i * 0x2000], &rawcart[0x2000 + (0x4000 * i)], 0x2000);
    }
    /* 16k configuration */
    cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/
static int sdbox_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    sdbox_list_item = io_source_register(&sdbox_device);
    return 0;
}

int sdbox_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return sdbox_common_attach();
}

int sdbox_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if ((chip.bank > 7) || (chip.start != 0x8000) || (chip.size != 0x4000)) {
            return -1;
        }
        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }
    return sdbox_common_attach();
}

void sdbox_detach(void)
{
    io_source_unregister(sdbox_list_item);
    sdbox_list_item = NULL;
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTSDBOX snapshot module format:

   type  | name         | version | description
   --------------------------------------
   BYTE  | regval       |   1.0   | register
   BYTE  | currbank     |   1.0   | current bank
   BYTE  | reg_enable   |   1.0   | flag: is the register enabled
   BYTE  | regs         |   1.0   | 4 bytes of "RAM"
   ARRAY | ROML         |   1.0   | 8*8k BYTES of ROML data
   ARRAY | ROMH         |   1.0   | 8*8k BYTES of ROMH data

 */

static const char snap_module_name[] = "CARTSDBOX";
#define SNAP_MAJOR   1
#define SNAP_MINOR   0

int sdbox_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, regval) < 0
        || SMW_B(m, (uint8_t)currbank) < 0
        || SMW_B(m, reg_enable) < 0
        || SMW_BA(m, regs, 4) < 0
        || SMW_BA(m, roml_banks, 0x2000 * 8) < 0
        || SMW_BA(m, romh_banks, 0x2000 * 8) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int sdbox_snapshot_read_module(snapshot_t *s)
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

    /* Only accept compatible snapshots */
    if (snapshot_version_is_smaller(vmajor, vminor, 1, 0)) {
        snapshot_set_error(SNAPSHOT_MODULE_INCOMPATIBLE);
        goto fail;
    }

    if (0
        || SMR_B(m, &regval) < 0
        || SMR_B(m, &currbank) < 0
        || SMR_B(m, &reg_enable) < 0
        || SMR_BA(m, regs, 4) < 0
        || SMR_BA(m, roml_banks, 0x2000 * 8) < 0
        || SMR_BA(m, romh_banks, 0x2000 * 8) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return sdbox_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
