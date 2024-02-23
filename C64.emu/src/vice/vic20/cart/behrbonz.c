/*
 * behrbonz.c -- VIC20 BehrBonz Cartridge emulation.
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

#include "archdep.h"
#include "behrbonz.h"
#include "cartio.h"
#include "cartridge.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

/* #define DEBUGBEHRBONZ */

#ifdef DEBUGBEHRBONZ
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

#define BUTTON_RESET 0
#define SOFTWARE_RESET 1
static uint8_t reset_mode = BUTTON_RESET;

#define CART_ROM_SIZE 0x200000
static uint8_t *cart_rom = NULL;

static uint8_t bank_reg = 0;
static uint8_t write_once = 1;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t behrbonz_io3_peek(uint16_t addr);
static void behrbonz_io3_store(uint16_t addr, uint8_t value);
static int behrbonz_mon_dump(void);

static io_source_t behrbonz_io3_device = {
    CARTRIDGE_VIC20_NAME_BEHRBONZ, /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0x9c00, 0x9fff, 0x3ff,         /* range for the device, address is ignored, reg:$9c00, mirrors:$9c01-$9fff */
    0,                             /* read is never valid, reg is write only */
    behrbonz_io3_store,            /* store function */
    NULL,                          /* NO poke function */
    NULL,                          /* NO read function */
    behrbonz_io3_peek,             /* peek function */
    behrbonz_mon_dump,             /* device state information dump function */
    CARTRIDGE_VIC20_BEHRBONZ,      /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0,                             /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                 /* NO mirroring */
};

static io_source_list_t *behrbonz_io3_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_VIC20_NAME_BEHRBONZ, 0, 0, NULL, &behrbonz_io3_device, CARTRIDGE_VIC20_BEHRBONZ
};

/* ------------------------------------------------------------------------- */

uint8_t behrbonz_blk13_read(uint16_t addr)
{
    return cart_rom[(addr & 0x1fff) | ((bank_reg * 0x4000) + 0x2000)];
}

uint8_t behrbonz_blk25_read(uint16_t addr)
{
    return cart_rom[(addr & 0x1fff) | (bank_reg * 0x4000)];
}

static uint8_t behrbonz_io3_peek(uint16_t addr)
{
    return bank_reg;
}

/* store 0x9c00-0x9fff */
static void behrbonz_io3_store(uint16_t addr, uint8_t value)
{
    /* with the original cartridge a write to the register changes the bank and
       triggers a reset. also the bank can not be changed anymore until the next
       power cycle */
    DBG(("behrbonz_io3_store %04x,%02x\n", addr, value));
    if (write_once) {
        bank_reg = value & 0x7f;
        reset_mode = SOFTWARE_RESET;
        machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);
    }
    write_once = 0;
}

/* ------------------------------------------------------------------------- */

void behrbonz_init(void)
{
    bank_reg = 0;
    reset_mode = BUTTON_RESET;
    DBG(("behrbonz_init bank %02x\n", bank_reg));
}

void behrbonz_reset(void)
{
    /* FIXME: the actual cartridge will only reset the bank to 0 on a power cycle */
    if (reset_mode == BUTTON_RESET) {
        bank_reg = 0;
        write_once = 1;
    }
    reset_mode = BUTTON_RESET;
    DBG(("behrbonz_reset bank %02x\n", bank_reg));
}

/* ------------------------------------------------------------------------- */

static int zfile_load(const char *filename, uint8_t *dest, size_t size)
{
    FILE *fd;
    off_t len;

    fd = zfile_fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }
    len = archdep_file_size(fd);
    if (len < 0 || (size_t)len != size) {
        zfile_fclose(fd);
        return -1;
    }
    if (fread(dest, size, 1, fd) < 1) {
        zfile_fclose(fd);
        return -1;
    }
    zfile_fclose(fd);
    return 0;
}

int behrbonz_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int idx = 0;

    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    for (idx = 0; idx < 256; idx++) {
        if (crt_read_chip_header(&chip, fd)) {
            goto exiterror;
        }

        DBG(("chip %d at %02x len %02x\n", idx, chip.start, chip.size));
        if (chip.size != 0x2000) {
            goto exiterror;
        }

        if (crt_read_chip(&cart_rom[0x2000 * idx], 0, &chip, fd)) {
            goto exiterror;
        }
    }

    if (export_add(&export_res) < 0) {
        goto exiterror;
    }
    mem_cart_blocks = VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 | VIC_CART_IO3;
    mem_initialize_memory();

    behrbonz_io3_list_item = io_source_register(&behrbonz_io3_device);

    return CARTRIDGE_VIC20_BEHRBONZ;

exiterror:
    behrbonz_detach();
    return -1;
}

int behrbonz_bin_attach(const char *filename)
{
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (zfile_load(filename, cart_rom, (size_t)CART_ROM_SIZE) < 0) {
        behrbonz_detach();
        return -1;
    }

    if (export_add(&export_res) < 0) {
        return -1;
    }
    mem_cart_blocks = VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 | VIC_CART_IO3;
    mem_initialize_memory();

    behrbonz_io3_list_item = io_source_register(&behrbonz_io3_device);

    return 0;
}

void behrbonz_detach(void)
{
    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(cart_rom);
    cart_rom = NULL;

    export_remove(&export_res);
    if (behrbonz_io3_list_item != NULL) {
        io_source_unregister(behrbonz_io3_list_item);
        behrbonz_io3_list_item = NULL;
    }
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   0
#define VIC20CART_DUMP_VER_MINOR   2
#define SNAP_MODULE_NAME  "BEHRBONZ"

int behrbonz_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, bank_reg) < 0)
        || (SMW_B(m, reset_mode) < 0)
        || (SMW_B(m, write_once) < 0)
        || (SMW_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int behrbonz_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != VIC20CART_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (0
        || (SMR_B(m, &bank_reg) < 0)
        || (SMR_B(m, &reset_mode) < 0)
        || (SMR_B(m, &write_once) < 0)
        || (SMR_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(cart_rom);
        cart_rom = NULL;
        return -1;
    }

    snapshot_module_close(m);

    mem_cart_blocks = VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 | VIC_CART_IO3;
    mem_initialize_memory();

    return 0;
}

/* ------------------------------------------------------------------------- */

static int behrbonz_mon_dump(void)
{
    mon_out("Bank $%02x\n", bank_reg);

    return 0;
}
