
/*
 * magicdesk128.c -- "Magic Desk 128" cartridge emulation
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
#include "log.h"
#include "util.h"
#include "monitor.h"
#include "vicii-phi1.h"

#include "c64cart.h"
#include "export.h"
#include "c128cart.h"
#include "functionrom.h"

#include "crt.h"
#include "magicdesk128.h"

/* #define DBGMD128 */

#ifdef DBGMD128
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*

 "Magic Desk 128 (1MB)" (https://github.com/RetroNynjah/Magic-Desk-128)

 used by the game "Volley for two" (https://kollektivet.nu/v42/)
 supported by https://csdb.dk/release/index.php?id=216617 (also contains an image
 for testing)

 - 16kB ROM per bank. Theoretically it could support up to 4MB (256x16kB).
 - current hardware has 1MB of ROM divided into 64 banks of 16kB.

 - After power-up or reset, ROM bank 0 (the first 16kB of the LOW chip) is active
   and accessible at ROML ($8000-$bfff)
 - de00 is the banking register

*/

#define MD128_ROM_BANKS   0x40
#define MD128_BANK_SIZE   0x4000
#define MD128_ROM_SIZE    (MD128_BANK_SIZE * MD128_ROM_BANKS)

static unsigned int md128reg = 0;
static unsigned int rombank = 0;
static unsigned int bankmask = (MD128_ROM_BANKS - 1);

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t magicdesk128_io1_peek(uint16_t addr);
static uint8_t magicdesk128_io1_read(uint16_t addr);
static void magicdesk128_io1_store(uint16_t addr, uint8_t value);
static int magicdesk128_dump(void);

static io_source_t magicdesk128_io1_device = {
    CARTRIDGE_C128_NAME_MAGICDESK128, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,         /* range for the device, address is ignored by the write functions, reg:$de00, mirrors:$de01-$deff */
    1,                            /* read is never valid */
    magicdesk128_io1_store,       /* store function */
    NULL,                         /* NO poke function */
    magicdesk128_io1_read,        /* read function */
    magicdesk128_io1_peek,        /* peek function */
    magicdesk128_dump,            /* device state information dump function */
    CARTRIDGE_C128_MAGICDESK128,  /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0,                            /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                /* NO mirroring */
};

static io_source_list_t *magicdesk128_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_MAGICDESK128, 1, 1, &magicdesk128_io1_device, NULL, CARTRIDGE_C128_MAGICDESK128
};

/* ---------------------------------------------------------------------*/

static void magicdesk128_io1_store(uint16_t addr, uint8_t value)
{
    md128reg = value & 0x3f;
    rombank = md128reg & bankmask;

    external_function_rom_set_bank(rombank);

    DBG(("magicdesk128_io1_store value:%02x bank: %d\n", value, (int)rombank));
}

static uint8_t magicdesk128_io1_read(uint16_t addr)
{
    uint8_t value = vicii_read_phi1();
    /* since r/w is not decoded for accesses to the register latch, reading IO1
       would actually write random values into the register */
    rombank = value & bankmask;
    external_function_rom_set_bank(rombank);
    log_warning(LOG_DEFAULT, "md128: read from $de%02x sets ROM bank to $%02x.\n", addr, rombank);
    /*DBG(("magicdesk128_io1_read %04x %02x\n", addr, value));*/
    return value;
}

static uint8_t magicdesk128_io1_peek(uint16_t addr)
{
    return rombank;
}

static int magicdesk128_dump(void)
{
    mon_out("Register: $%02x\n", md128reg);
    mon_out("ROM bank: $%02x (%d of %d)\n", rombank, (int)rombank, (int)(bankmask + 1));
    mon_out("Bank mask: $%02x\n", bankmask);
    return 0;
}

/* ---------------------------------------------------------------------*/

void magicdesk128_config_setup(uint8_t *rawcart)
{
    DBG(("magicdesk128_config_setup\n"));
    /* copy loaded cartridge data into actually used ROM array */
    for (int i = 0; i < MD128_ROM_BANKS; i++) {
        memcpy(ext_function_rom + (0x8000 * i), rawcart + (0x4000 * i), 0x4000);
    }
}

static int magicdesk128_common_attach(void)
{
    /* setup i/o device */
    if (export_add(&export_res) < 0) {
        return -1;
    }
    magicdesk128_io1_list_item = io_source_register(&magicdesk128_io1_device);

    /* FIXME */
    magicdesk128_io1_store(0, 0);
    return 0;
}

int magicdesk128_bin_attach(const char *filename, uint8_t *rawcart)
{
    unsigned int size = MD128_ROM_SIZE; /* start with 1MiB */
    DBG(("magicdesk128_bin_attach '%s'\n", filename));

    bankmask = (MD128_ROM_BANKS - 1);

    /* try all sizes that are a power of two, down to 16KiB */
    while (size >= 0x4000) {
        if (util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS) == 0) {
            DBG(("loaded 0x%x bytes\n", size));
            return magicdesk128_common_attach();
        }
        size >>= 1;
        bankmask = ((size / MD128_BANK_SIZE) - 1);
    }
    return -1;
}

/*
    returns -1 on error, else a positive CRT ID
*/
int magicdesk128_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int banks = 0;

    DBG(("magicdesk128_crt_attach ptr:%p\n", rawcart));

    bankmask = (MD128_ROM_BANKS - 1);

    for (banks = 0; banks < MD128_ROM_BANKS; banks++) {
        if (crt_read_chip_header(&chip, fd)) {
            goto chksize;
        }
        DBG(("chip %d at %02x len %02x offs %04x \n", banks, chip.start, chip.size, (unsigned)(banks * 0x4000)));
        if (chip.start == 0x8000 && chip.size == 0x4000) {
            if (crt_read_chip(rawcart + (banks * 0x4000), 0, &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }
    }
chksize:
    DBG(("loaded banks: %d\n", banks));

    /* decrement to handle the case when banks itself is a power of 2 */
    banks = banks - 1;
    /* do till only one bit is left */
    while (banks & (banks - 1)) {
        banks = banks & (banks - 1); /* unset rightmost bit */
    }
    /* banks is now a power of two*/
    banks = banks << 1; /* next power of 2 is what we want */

    /* mask is one less */
    bankmask = (banks - 1);

    DBG(("using banks: %d\n", banks));
    DBG(("using bankmask: $%02x\n", bankmask));

    return magicdesk128_common_attach();
}

void magicdesk128_detach(void)
{
    if (magicdesk128_io1_list_item) {
        io_source_unregister(magicdesk128_io1_list_item);
        magicdesk128_io1_list_item = NULL;
    }
    export_remove(&export_res);
}

void magicdesk128_reset(void)
{
    DBG(("magicdesk128_reset\n"));
    md128reg = 0;
    rombank = 0;
    external_function_rom_set_bank(0);
}
