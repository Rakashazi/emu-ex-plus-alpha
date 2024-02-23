/*
 * partner128.c -- "Partner 128" cartridge emulation
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

#include "cartridge.h"
#include "cartio.h"
#include "util.h"

#include "maincpu.h"
#include "monitor.h"
#include "c128memrom.h"

#include "c64cart.h"
#include "export.h"
#include "c128cart.h"
#include "functionrom.h"

#include "crt.h"
#include "partner128.h"

/* #define DBGPARTNER */

#ifdef DBGPARTNER
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*

"Partner 128" (c)1984 Timeworks Inc.

$8000-$9fff contains the ROM
$de00-$de7f contains 128 bytes ram
$de80-$deff contains a mirror of $9e80-$9eff

Writing to $de80-$deff writes to a single registers:

 bit 0..5   selects which one of 64 blocks of 128 byte ram is visible in $de00-$de7f.
 bit 6      "unfreeze" when set
 bit 7      not used

- it has a button which generates an NMI
  - when freeze was pressed, it waits for the bus to show $fffa and then puts
    $de on the bus
- there is a cable, which has to be connected to joystick port 2. This connects
  to CIA1 PB2, which lets the cartridge "see" the roughly 60Hz pulses caused
  by the keyboard scanner. This pulse will then reset the freeze logic.

Note that the cartridge menus are displayed on the VDC!

FIXME: apparently the first 16 bytes of RAM are also mapped to d600..d60f, but
       "silently write only" - ie VDC writes would end up in the RAM.

NOTE: There is an apparently bad dump circulating (crc32: e165bdb6)

*/

#define PARTNER_ROM_SIZE  0x4000
#define PARTNER_RAM_SIZE  (64 * 128)    /* 64 * 128bytes */

static uint8_t regvalue = 0;
static uint8_t rambank = 0;
static uint8_t isdefreezing = 0;

static uint8_t rambanks[PARTNER_RAM_SIZE];
static uint8_t nmivector[2] = { 0, 0 };

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t partner128_io1_peek(uint16_t addr);
static uint8_t partner128_io1_read(uint16_t addr);
static void partner128_io1_store(uint16_t addr, uint8_t value);

static uint8_t partner128_iod6_peek(uint16_t addr);
static void partner128_iod6_store(uint16_t addr, uint8_t value);

static int partner128_dump(void);

static io_source_t partner128_io1_device = {
    CARTRIDGE_C128_NAME_PARTNER128, /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,           /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    1,                              /* read is never valid */
    partner128_io1_store,           /* store function */
    NULL,                           /* NO poke function */
    partner128_io1_read,            /* read function */
    partner128_io1_peek,            /* peek function */
    partner128_dump,                /* device state information dump function */
    CARTRIDGE_C128_PARTNER128,      /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0,                              /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                  /* NO mirroring */
};
static io_source_list_t *partner128_io1_list_item = NULL;

static io_source_t partner128_iod6_device = {
    CARTRIDGE_C128_NAME_PARTNER128, /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xd600, 0xd60f, 0x0f,           /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    0,                              /* read is never valid */
    partner128_iod6_store,          /* store function */
    NULL,                           /* NO poke function */
    NULL,                           /* NO read function */
    partner128_iod6_peek,           /* peek function */
    partner128_dump,                /* device state information dump function */
    CARTRIDGE_C128_PARTNER128,      /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0,                              /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                  /* NO mirroring */
};
static io_source_list_t *partner128_iod6_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_PARTNER128, 1, 1, &partner128_io1_device, NULL, CARTRIDGE_C128_PARTNER128
};

/* ---------------------------------------------------------------------*/

/* this is kind of ugly. the real cartridge "listens" to the right address on
   the bus and then forces the respective data lines to 0xde. we cant do this
   in a sane way in the current implementation, so we change the actual rom
   forth and back */

static void nmivector_save(void)
{
    if ((c128memrom_kernal_rom[0x1ffa] == 0xde) &&
        (c128memrom_kernal_rom[0x1ffb] == 0xde)) {
        return;
    }
    nmivector[0] = c128memrom_kernal_rom[0x1ffa];
    nmivector[1] = c128memrom_kernal_rom[0x1ffb];
}

static void nmivector_patch(void)
{
    c128memrom_kernal_rom[0x1ffa] = 0xde;
    c128memrom_kernal_rom[0x1ffb] = 0xde;
}

static void nmivector_restore(void)
{
    if ((nmivector[0] == 0) &&
        (nmivector[1] == 0)) {
        return;
    }
    c128memrom_kernal_rom[0x1ffa] = nmivector[0];
    c128memrom_kernal_rom[0x1ffb] = nmivector[1];
    nmivector[0] = nmivector[1] = 0;
}

/* ---------------------------------------------------------------------*/

static void partner128_io1_store(uint16_t addr, uint8_t value)
{
/* DBG(("partner128_io1_store %04x %02x\n", addr, value)); */
    if (addr < 0x80) {
        /* RAM */
        rambanks[(rambank * 128) + addr] = value;
    } else if (addr >= 0x80) {
        regvalue = value & 0x7f;
        rambank = value & 0x3f;
        isdefreezing = value & 0x40;
        if (isdefreezing) {
            DBG(("partner128_io1_store release freeze\n"));
            nmivector_restore();
            cartridge_release_freeze();
        }

        /*DBG(("partner128 bank:%02x\n", rambank));*/
    }
}

static uint8_t partner128_io1_read(uint16_t addr)
{
    uint8_t value;

    if (addr < 0x80) {
        /* RAM */
        value = rambanks[(rambank * 128) + addr];
    } else {
        value = ext_function_rom[0x1e80 + (addr & 0x7f)];
    }
    /*DBG(("partner128_io1_read %04x %02x\n", addr, value));*/
    return value;
}

static uint8_t partner128_io1_peek(uint16_t addr)
{
    uint8_t value;

    if (addr < 0x80) {
        /* RAM */
        value = rambanks[(rambank * 128) + addr];
    } else {
        value = ext_function_rom[0x1e80 + (addr & 0x7f)];
    }
    /*DBG(("partner128_io1_read %04x %02x\n", addr, value));*/
    return value;
}

static void partner128_iod6_store(uint16_t addr, uint8_t value)
{
    /* DBG(("partner128_iod6_store %04x %02x\n", addr, value)); */
    if (addr < 0x10) {
        /* RAM */
        rambanks[(rambank * 128) + addr] = value;
    }
}

static uint8_t partner128_iod6_peek(uint16_t addr)
{
    uint8_t value = 0;

    if (addr < 0x10) {
        /* RAM */
        value = rambanks[(rambank * 128) + addr];
    }
    /*DBG(("partner128_io1_read %04x %02x\n", addr, value));*/
    return value;
}

static int partner128_dump(void)
{
    mon_out("Register: $%02x\n", regvalue);
    mon_out("RAM bank: %d/64\n", rambank);
    mon_out("is de-freezing: %s\n", isdefreezing ? "yes" : "no");
    mon_out("vectors forced: %s\n",
        ((c128memrom_kernal_rom[0x1ffa] == 0xde) && (c128memrom_kernal_rom[0x1ffb] == 0xde)) ? "yes" : "no");
    return 0;
}

/* ---------------------------------------------------------------------*/

void partner128_config_setup(uint8_t *rawcart)
{
    /* copy loaded cartridge data into actually used ROM array */
    memcpy(&ext_function_rom[0], rawcart, PARTNER_ROM_SIZE);
}

static int partner128_common_attach(void)
{
    /* setup i/o device */
    if (export_add(&export_res) < 0) {
        return -1;
    }
    partner128_io1_list_item = io_source_register(&partner128_io1_device);
    partner128_iod6_list_item = io_source_register(&partner128_iod6_device);

    return 0;
}

int partner128_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, PARTNER_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    memcpy(rawcart + PARTNER_ROM_SIZE, rawcart, PARTNER_ROM_SIZE);
    return partner128_common_attach();
}

/*
    returns -1 on error, else a positive CRT ID
*/
int partner128_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    DBG(("chip1 at %02x len %02x\n", chip.start, chip.size));
    if (chip.start == 0x8000 && chip.size == 0x4000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        memcpy(rawcart + PARTNER_ROM_SIZE, rawcart, PARTNER_ROM_SIZE);
        return partner128_common_attach();
    }
    return -1;
}

void partner128_detach(void)
{
    if (partner128_io1_list_item) {
        io_source_unregister(partner128_io1_list_item);
        partner128_io1_list_item = NULL;
    }
    if (partner128_iod6_list_item) {
        io_source_unregister(partner128_iod6_list_item);
        partner128_iod6_list_item = NULL;
    }
    export_remove(&export_res);
    nmivector_restore();
}

void partner128_reset(void)
{
    DBG(("partner128_reset\n"));
    regvalue = 0;
    rambank = 0;
    isdefreezing = 0;
    nmivector_restore();
}

void partner128_freeze(void)
{
    DBG(("partner128_freeze\n"));
    nmivector_save();
    nmivector_patch();
}

void partner128_powerup(void)
{
    DBG(("partner128_powerup\n"));
    regvalue = 0;
    rambank = 0;
    isdefreezing = 0;
    nmivector_restore();
    memset(rambanks, 0xff, PARTNER_RAM_SIZE);
}
