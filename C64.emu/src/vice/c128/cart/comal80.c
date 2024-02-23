
/*
 * comal80.c -- Comal 80 (C128) cartridge emulation
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
#include "comal80.h"

/* #define DBGCOMAL80 */

#ifdef DBGCOMAL80
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    The C128 Comal80 cartridge size is 96 KB (6 banks x 16KB) and it's using
    bankswitching, so that only 16 KB is visible at a time in the C128 memory
    map at address $C000 to $FFFF (Remark, it is using same address area as
    EDITOR and KERNAL).

    By write too and read from address $DE00 or call comal system routine, you
    can control which comal80 module routines you can call in our own machinecodeprogram.
*/

#define COMAL80_ROM_SIZE    (0x4000 * 6)
static int comal80_register = 0;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t c128comal80_io1_peek(uint16_t addr);
static uint8_t c128comal80_io1_read(uint16_t addr);
static void c128comal80_io1_store(uint16_t addr, uint8_t value);
static uint8_t c128comal80_io2_read(uint16_t addr);
static uint8_t c128comal80_io2_peek(uint16_t addr);
static void c128comal80_io2_store(uint16_t addr, uint8_t value);
static int c128comal80_dump(void);

static io_source_t c128comal80_io1_device = {
    CARTRIDGE_C128_NAME_COMAL80,    /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,           /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    1,                              /* read is never valid */
    c128comal80_io1_store,          /* store function */
    NULL,                           /* NO poke function */
    c128comal80_io1_read,           /* read function */
    c128comal80_io1_peek,           /* peek function */
    c128comal80_dump,               /* device state information dump function */
    CARTRIDGE_C128_COMAL80,         /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0,                              /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                  /* NO mirroring */
};

static io_source_t c128comal80_io2_device = {
    CARTRIDGE_C128_NAME_COMAL80,    /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,           /* range for the device, address is ignored by the read/write functions, reg:$df00, mirrors:$df01-$dfff */
    1,                              /* validity of the read is determined by the cartridge at read time */
    c128comal80_io2_store,          /* store function */
    NULL,                           /* NO poke function */
    c128comal80_io2_read,           /* read function */
    c128comal80_io2_peek,           /* peek function */
    c128comal80_dump,               /* device state information dump function */
    CARTRIDGE_C128_COMAL80,         /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0,                              /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                  /* NO mirroring */
};

static io_source_list_t *c128comal80_io1_list_item = NULL;
static io_source_list_t *c128comal80_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_COMAL80, 1, 1, &c128comal80_io1_device, &c128comal80_io2_device, CARTRIDGE_C128_COMAL80
};

/* ---------------------------------------------------------------------*/

static void c128comal80_io1_store(uint16_t addr, uint8_t value)
{
/*  B (D6)       A (D5)
    0            0          y0    U1 ROM
    0            1          y1    unused
    1            0          y2    U2 ROM
    1            1          y3    U3 ROM */
    int romoffset[4] = {
        0,  /* 0,      */
        0,  /* 0,      */
        2,  /* 0x8000, */
        4,  /* 0x10000 */
    };
    int bank;

    comal80_register = value & 0xf0;    /* upper 4 bit go into the register latch */

    bank = romoffset[(value >> 5) & 3];
    bank |= ((value >> 4) & 1);
    external_function_rom_set_bank(bank);

    DBG(("c128comal80_io1_store value:%02x bank: %d\n", value, bank));
}

static uint8_t c128comal80_io1_read(uint16_t addr)
{
    uint8_t value = comal80_register;

/*    value = ext_function_rom[0x1e00 + (addr & 0xff)]; */
    value |= ((value >> 4) & 1) << 4;

    DBG(("c128comal80_io1_read %04x %02x\n", addr, value));
    return value;
}

static uint8_t c128comal80_io1_peek(uint16_t addr)
{
    return ext_function_rom[0x1e00 + (addr & 0xff)];
}

static uint8_t c128comal80_io2_read(uint16_t addr)
{
    uint8_t value;

    value = ext_function_rom[0x1f00 + (addr & 0xff)];
    DBG(("c128comal80_io2_read %04x %02x\n", addr, value));
    return value;
}

static uint8_t c128comal80_io2_peek(uint16_t addr)
{
    return ext_function_rom[0x1f00 + (addr & 0xff)];
}

static void c128comal80_io2_store(uint16_t addr, uint8_t value)
{
    DBG(("c128comal80_io2_store %04x %02x\n", addr, value));
}

static int c128comal80_dump(void)
{
    return 0;
}

/* ---------------------------------------------------------------------*/

void c128comal80_config_setup(uint8_t *rawcart)
{
    DBG(("c128comal80_config_setup\n"));
    /* copy loaded cartridge data into actually used ROM array */
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 0)], rawcart + (0x4000 * 0), 0x4000);
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 1)], rawcart + (0x4000 * 1), 0x4000);
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 2)], rawcart + (0x4000 * 2), 0x4000);
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 3)], rawcart + (0x4000 * 3), 0x4000);
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 4)], rawcart + (0x4000 * 4), 0x4000);
    memcpy(&ext_function_rom[0x4000 + (0x8000 * 5)], rawcart + (0x4000 * 5), 0x4000);
}

static int c128comal80_common_attach(void)
{
    /* setup i/o device */
    if (export_add(&export_res) < 0) {
        return -1;
    }
    c128comal80_io1_list_item = io_source_register(&c128comal80_io1_device);
    c128comal80_io2_list_item = io_source_register(&c128comal80_io2_device);

    /* FIXME */
    c128comal80_io1_store(0, 0);
    return 0;
}

int c128comal80_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("c128comal80_bin_attach '%s'\n", filename));
    if (util_file_load(filename, rawcart, COMAL80_ROM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) == 0) {
        return c128comal80_common_attach();
    }
    return -1;
}

/*
    returns -1 on error, else a positive CRT ID
*/
int c128comal80_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int n;

    DBG(("c128comal80_crt_attach ptr:%p\n", (void*)rawcart));

    for (n = 0; n < 6; n++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }
        DBG(("chip %d at %02x len %02x offs %04x \n", n, chip.start, chip.size, (unsigned)(n * 0x4000)));
        if (chip.start == 0xc000 && chip.size == 0x4000) {
            if (crt_read_chip(rawcart + (n * 0x4000), 0, &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    return c128comal80_common_attach();
}

void c128comal80_detach(void)
{
    if (c128comal80_io1_list_item) {
        io_source_unregister(c128comal80_io1_list_item);
        c128comal80_io1_list_item = NULL;
    }
    if (c128comal80_io2_list_item) {
        io_source_unregister(c128comal80_io2_list_item);
        c128comal80_io2_list_item = NULL;
    }
    export_remove(&export_res);
}

void c128comal80_reset(void)
{
    DBG(("c128comal80_reset\n"));
    external_function_rom_set_bank(0);
}
