/*
 * superexplode5.c - Cartridge handling, Super Explode V5 cart.
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
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "snapshot.h"
#include "superexplode5.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    FIXME: this one has been implemented purely based on guesswork and by
           examining the cartridge rom dump.

    The Soft Group "Super Explode V5"

    - 2 ROM banks, 8k each == 16kb
    - one button (reset)

    ROM banks are always mapped to $8000
    the last page of the ROM bank is also visible at DFxx

    controlregister is $df00:
        bit 7 selects bank

    this is a very strange cartridge, almost no information about it seems
    to exist, from http://www.mayhem64.co.uk/cartpower.htm:

    Super Explode! version 5 is primarily a graphics cartridge. It is designed
    to capture, manipulate, and edit screens and then print them. Its color
    print capability includes recolorization, and it dumps to all but one
    available color printer. Its extensive ability to manipulate graphics
    images makes it the cartridge of choice for graphics buffs. (Note that
    Super Explode! interfaces with The Soft Group's Video Byte system, a low-
    cost video digitizer designed to capture full-color images from a VCR or
    live camera.)

    Super Explode! 5's modest utility repertoire includes a complete disk-turbo
    feature, directory list to screen, single-stroke disk commands, and easy
    access to the error channel. These commands are not implemented on function
    keys, nor are the function keys programmed. There is no BASIC toolkit,
    monitor, or disk-backup or archiving capability. There is a fast multiple-
    copy file routine, as well as an unnew command. The freeze button doubles
    as a reset.

    The manual is on disk (you must print it out) and is rather haphazard.
    Nonetheless, it contains a wealth of technical information. Topics include
    split screens, elementary and advanced file conversion (for Doodle, Koala,
    text screens, and custom character sets), sprite manipulation, and sprite
    overlay. If you require few utility functions but extensive graphics
    capability, Super Explode! 5 is for you.

*/

/* #define SE5_DEBUG */

#ifdef SE5_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define SE5_CART_SIZE (2 * 0x2000)

/* ---------------------------------------------------------------------*/

static void se5_io2_store(WORD addr, BYTE value)
{
    DBG(("io2 wr %04x %02x\n", addr, value));
    cart_romlbank_set_slotmain((value & 0x80) >> 7);
}

static BYTE se5_io2_read(WORD addr)
{
    addr |= 0xdf00;
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

/* ---------------------------------------------------------------------*/

static io_source_t se5_io2_device = {
    CARTRIDGE_NAME_SUPER_EXPLODE_V5,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is alway valid */
    se5_io2_store,
    se5_io2_read,
    NULL,
    NULL,
    CARTRIDGE_SUPER_EXPLODE_V5,
    0,
    0
};

static io_source_list_t *se5_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_SUPER_EXPLODE_V5, 1, 0, NULL, &se5_io2_device, CARTRIDGE_SUPER_EXPLODE_V5
};

/* ---------------------------------------------------------------------*/

BYTE se5_roml_read(WORD addr)
{
    if (addr < 0x9f00) {
        return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
    } else {
        return ram_read(addr);
        /* return mem_read_without_ultimax(addr); */
    }
}

/* ---------------------------------------------------------------------*/

void se5_config_init(void)
{
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

void se5_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, SE5_CART_SIZE);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

/* ---------------------------------------------------------------------*/

static int se5_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }

    se5_io2_list_item = io_source_register(&se5_io2_device);

    return 0;
}

int se5_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, SE5_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return se5_common_attach();
}

int se5_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i, cnt = 0;

    for (i = 0; i <= 0x01; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 0x1f || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
        cnt++;
    }

    return se5_common_attach();
}

void se5_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(se5_io2_list_item);
    se5_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTSE5"

int se5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, roml_banks, SE5_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int se5_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_BA(m, roml_banks, SE5_CART_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return se5_common_attach();
}
