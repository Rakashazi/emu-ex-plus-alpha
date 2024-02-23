/*
 * warpspeed128.c -- "Warpspeed 128" cartridge emulation
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

#include "c64cart.h"
#include "export.h"
#include "c128cart.h"
#include "functionrom.h"

#include "crt.h"
#include "warpspeed128.h"

/* #define DBGWS */

#ifdef DBGWS
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
     Note: the actual cartridge contains both a C64 and a C128 ROM, which can
    be selected with a switch. We implement them as if these two ROMs were
    different cartridges - this way they can be selected by using the other
    .crt file, instead of having to flip the switch in the user interface.
*/

#define WARPSPEED_ROM_SIZE  0x4000

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t warpspeed128_io1_peek(uint16_t addr);
static uint8_t warpspeed128_io1_read(uint16_t addr);
static void warpspeed128_io1_store(uint16_t addr, uint8_t value);
static uint8_t warpspeed128_io2_read(uint16_t addr);
static uint8_t warpspeed128_io2_peek(uint16_t addr);
static void warpspeed128_io2_store(uint16_t addr, uint8_t value);
static int warpspeed128_dump(void);

static io_source_t warpspeed128_io1_device = {
    CARTRIDGE_C128_NAME_WARPSPEED128, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,         /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    1,                            /* read is never valid */
    warpspeed128_io1_store,       /* store function */
    NULL,                         /* NO poke function */
    warpspeed128_io1_read,        /* read function */
    warpspeed128_io1_peek,        /* peek function */
    warpspeed128_dump,            /* device state information dump function */
    CARTRIDGE_C128_WARPSPEED128,  /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0,                            /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                /* NO mirroring */
};

static io_source_t warpspeed128_io2_device = {
    CARTRIDGE_C128_NAME_WARPSPEED128, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,         /* range for the device, address is ignored by the read/write functions, reg:$df00, mirrors:$df01-$dfff */
    1,                            /* validity of the read is determined by the cartridge at read time */
    warpspeed128_io2_store,       /* store function */
    NULL,                         /* NO poke function */
    warpspeed128_io2_read,        /* read function */
    warpspeed128_io2_peek,        /* peek function */
    warpspeed128_dump,            /* device state information dump function */
    CARTRIDGE_C128_WARPSPEED128,  /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0,                            /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                /* NO mirroring */
};

static io_source_list_t *warpspeed128_io1_list_item = NULL;
static io_source_list_t *warpspeed128_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_WARPSPEED128, 1, 1, &warpspeed128_io1_device, &warpspeed128_io2_device, CARTRIDGE_C128_WARPSPEED128
};

/* ---------------------------------------------------------------------*/

static void warpspeed128_io1_store(uint16_t addr, uint8_t value)
{
    DBG(("warpspeed128_io1_store %04x %02x\n", addr, value));
}

static uint8_t warpspeed128_io1_read(uint16_t addr)
{
    uint8_t value;

    value = ext_function_rom[0x1e00 + (addr & 0xff)];
    DBG(("warpspeed128_io1_read %04x %02x\n", addr, value));
    return value;
}

static uint8_t warpspeed128_io1_peek(uint16_t addr)
{
    return ext_function_rom[0x1e00 + (addr & 0xff)];
}

static uint8_t warpspeed128_io2_read(uint16_t addr)
{
    uint8_t value;

    value = ext_function_rom[0x1f00 + (addr & 0xff)];
    DBG(("warpspeed128_io2_read %04x %02x\n", addr, value));
    return value;
}

static uint8_t warpspeed128_io2_peek(uint16_t addr)
{
    return ext_function_rom[0x1f00 + (addr & 0xff)];
}

static void warpspeed128_io2_store(uint16_t addr, uint8_t value)
{
    DBG(("warpspeed128_io2_store %04x %02x\n", addr, value));
}

static int warpspeed128_dump(void)
{
    return 0;
}

/* ---------------------------------------------------------------------*/

void warpspeed128_config_setup(uint8_t *rawcart)
{
    /* copy loaded cartridge data into actually used ROM array */
    memcpy(&ext_function_rom[0], rawcart, EXTERNAL_FUNCTION_ROM_SIZE);
}

static int warpspeed128_common_attach(void)
{
    /* setup i/o device */
    if (export_add(&export_res) < 0) {
        return -1;
    }
    warpspeed128_io1_list_item = io_source_register(&warpspeed128_io1_device);
    warpspeed128_io2_list_item = io_source_register(&warpspeed128_io2_device);

    return 0;
}

int warpspeed128_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, WARPSPEED_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    memcpy(rawcart + WARPSPEED_ROM_SIZE, rawcart, WARPSPEED_ROM_SIZE);
    return warpspeed128_common_attach();
}

/*
    returns -1 on error, else a positive CRT ID
*/
int warpspeed128_crt_attach(FILE *fd, uint8_t *rawcart)
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
        memcpy(rawcart + WARPSPEED_ROM_SIZE, rawcart, WARPSPEED_ROM_SIZE);
        return warpspeed128_common_attach();
    }
    return -1;
}

void warpspeed128_detach(void)
{
    if (warpspeed128_io1_list_item) {
        io_source_unregister(warpspeed128_io1_list_item);
        warpspeed128_io1_list_item = NULL;
    }
    if (warpspeed128_io2_list_item) {
        io_source_unregister(warpspeed128_io2_list_item);
        warpspeed128_io2_list_item = NULL;
    }
    export_remove(&export_res);
}

void warpspeed128_reset(void)
{
    DBG(("warpspeed128_reset\n"));
}
