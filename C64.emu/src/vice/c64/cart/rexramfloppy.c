/*
 * rexramfloppy.c - Cartridge handling, REX Ramfloppy cart.
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

#include "rexramfloppy.h"
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

/* #define DEBUGRF */

#ifdef DEBUGRF
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    REX RAM-Floppy
    
    8k ROM
    up to 256k RAM
    
    dfa0    (write) selects RAM bank
    
    df50    (read) toggles RAM writeable
    dfc0    (read) toggles cartridge enable
    dfe0    (read) toggles RAM enable
    
    TODO:
    - implement loading/saving of the RAM content
    - implement the disable switch
*/

static int ram_bank = 0;
static int ram_enabled = 0;
static int cart_enabled = 1;
static int ram_writeable = 0;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t rexramfloppy_io2_peek(uint16_t addr);
static uint8_t rexramfloppy_io2_read(uint16_t addr);
static void rexramfloppy_io2_store(uint16_t addr, uint8_t value);
static int rexramfloppy_dump(void);

static io_source_t rexramfloppy_io2_device = {
    CARTRIDGE_NAME_REX_RAMFLOPPY, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,         /* range of the device */
    0,                            /* read validity is determined by the device upon a read */
    rexramfloppy_io2_store,       /* store function */
    NULL,                         /* NO poke function */
    rexramfloppy_io2_read,        /* read function */
    rexramfloppy_io2_peek,        /* peek function */
    rexramfloppy_dump,            /* device state information dump function */
    CARTRIDGE_REX_RAMFLOPPY,      /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *rexramfloppy_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_REX_RAMFLOPPY, 0, 1, NULL, &rexramfloppy_io2_device, CARTRIDGE_REX_RAMFLOPPY
};

/* ---------------------------------------------------------------------*/

static uint8_t rexramfloppy_io2_peek(uint16_t addr)
{
    addr &= 0xff;
    return 0; /* FIXME */
}

static uint8_t rexramfloppy_io2_read(uint16_t addr)
{
       
    addr &= 0xff;
    
    switch (addr) {
        case 0x50:
            ram_writeable ^= 1;
            break;
        case 0xc0:
            cart_enabled ^= 1;
            if (cart_enabled) {
                cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
            } else {
                cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
            }
            break;
        case 0xe0:
            ram_enabled ^= 1;
            break;
        default:
            /* printf("io2 read %04x\n", addr);  */
            break;
    }
    
    return 0;
}

static void rexramfloppy_io2_store(uint16_t addr, uint8_t value)
{
    
    addr &= 0xff;
    
    switch (addr) {
        case 0xa0:
            ram_bank = (value & 7) | ((value & 0x30) >> 1);
            break;
        default:
            /* printf("io2 write %04x %02x\n", addr, value); */
            break;
    }
}

static int rexramfloppy_dump(void)
{
    mon_out("mode: %s\n", (cart_enabled) ? "8K Game" : "RAM");
    mon_out("$8000-$9FFF: %s\n", (ram_enabled) ? "RAM" : "ROM");
    mon_out("RAM bank: %d\n", ram_bank);
    mon_out("RAM writeable: %s\n", ram_writeable ? "yes" : "no");
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t rexramfloppy_roml_read(uint16_t addr)
{
    if (ram_enabled) {
        return export_ram0[(addr & 0x1fff) + (ram_bank * 0x2000)];
    }

    return roml_banks[addr & 0x1fff];
}

void rexramfloppy_roml_store(uint16_t addr, uint8_t value)
{
    if (ram_enabled && ram_writeable) {
        export_ram0[(addr & 0x1fff) + (ram_bank * 0x2000)] = value;
    } else {
        mem_store_without_romlh(addr, value);
    }
}

/* ---------------------------------------------------------------------*/

void rexramfloppy_config_init(void)
{
    cart_enabled = 1;
    ram_writeable = 0;
    ram_enabled = 0;
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

void rexramfloppy_reset(void)
{
    cart_enabled = 1;
    ram_writeable = 0;
    ram_enabled = 0;
}

void rexramfloppy_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memset(export_ram0, 0xff, 0x2000 * 32);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int rexramfloppy_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    rexramfloppy_io2_list_item = io_source_register(&rexramfloppy_io2_device);

    return 0;
}

int rexramfloppy_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return rexramfloppy_common_attach();
}

int rexramfloppy_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return rexramfloppy_common_attach();
}

void rexramfloppy_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(rexramfloppy_io2_list_item);
    rexramfloppy_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTRRF snapshot module format:

   type  | name             | description
   ---------------------------------
   BYTE  | cart_enabled     | cartridge active flag
   BYTE  | ram_enabled      | RAM enabled at $8000
   BYTE  | ram_writeable    | RAM is writeable
   BYTE  | ram_bank         | currently selected RAM bank
   ARRAY | ROML             | 8192 BYTES of ROML data
   ARRAY | RAM              | 256k BYTES of RAM data
 */

static const char snap_module_name[] = "CARTRRF";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int rexramfloppy_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)cart_enabled) < 0)
        || (SMW_B(m, (uint8_t)ram_enabled) < 0)
        || (SMW_B(m, (uint8_t)ram_writeable) < 0)
        || (SMW_B(m, (uint8_t)ram_bank) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000 * 256) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int rexramfloppy_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &cart_enabled) < 0)
        || (SMR_B_INT(m, &ram_enabled) < 0)
        || (SMR_B_INT(m, &ram_writeable) < 0)
        || (SMR_B_INT(m, &ram_bank) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000 * 256) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return rexramfloppy_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
