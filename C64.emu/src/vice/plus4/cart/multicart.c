
/*
 * multicart.h - plus4 multi cart handling
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

/*
    "Plus4 multi cart"

    - 64/128 banks mapped to c1lo and c1hi
    - bank register at $FDA0
 */

#define DEBUG_MULTICART

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "cartio.h"
#include "crt.h"
#include "lib.h"
#include "monitor.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "snapshot.h"
#include "util.h"

#include "multicart.h"

#ifdef DEBUG_MULTICART
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define MULTICART_MAX_SIZE  (4*1024*1024)   /* 4MB */

static int bankreg = 0;
static unsigned int multicart_filesize = 0;
static int multicart_filetype = 0;

static unsigned char *multicartromlo = NULL;
static unsigned char *multicartromhi = NULL;

/* a prototype is needed */
static void multicart_store(uint16_t addr, uint8_t value);
static int multicart_dump(void);

/* This is not a real cartridge, it is only used for debugging purposes */
static io_source_t multicart_device = {
    CARTRIDGE_PLUS4_NAME_MULTI, /* name of the device */
    IO_DETACH_CART,             /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,      /* does not use a resource for detach */
    0xfda0, 0xfda0, 0xff,       /* range for the device, reg:$fda0 */
    0,                          /* read is never valid, device is write only */
    multicart_store,            /* store function */
    NULL,                       /* NO poke function */
    NULL,                       /* NO read function */
    NULL,                       /* NO peek function */
    multicart_dump,             /* dump function for the monitor */
    CARTRIDGE_PLUS4_MULTI,      /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0                           /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *multicart_list_item = NULL;

static int multicart_dump(void)
{
    mon_out("ROM bank: %d\n", bankreg);
    return 0;
}

static void multicart_store(uint16_t addr, uint8_t value)
{
    bankreg = value;
    DBG(("multicart_store %04x %02x\n", addr, value));
}

uint8_t multicart_c1lo_read(uint16_t addr)
{
    unsigned int offset = ((addr & 0x3fff) + (bankreg * 0x4000)) & ((multicart_filesize - 1) >> 1);
    /* DBG(("multicart_c1lo_read %06x bank: %d\n", offset, bankreg)); */
    return multicartromlo[offset];
}

uint8_t multicart_c1hi_read(uint16_t addr)
{
    unsigned int offset = ((addr & 0x3fff) + (bankreg * 0x4000)) & ((multicart_filesize - 1) >> 1);
    /* DBG(("multicart_c1hi_read %06x bank: %d\n", offset, bankreg)); */
    return multicartromhi[offset];
}

void multicart_reset(void)
{
    DBG(("multicart_reset\n"));
    bankreg = 0;
}

void multicart_config_setup(uint8_t *rawcart)
{
    DBG(("multicart_config_setup\n"));
    memcpy(multicartromlo, rawcart, multicart_filesize / 2);
    memcpy(multicartromhi, &rawcart[MULTICART_MAX_SIZE / 2], multicart_filesize / 2);
}

static int multicart_common_attach(void)
{
    DBG(("multicart_common_attach size: %06x\n", multicart_filesize));

    if(!(multicartromlo = lib_malloc(multicart_filesize / 2))) {
        return -1;
    }
    if(!(multicartromhi = lib_malloc(multicart_filesize / 2))) {
        return -1;
    }

    multicart_list_item = io_source_register(&multicart_device);

    return 0;
}

int multicart_bin_attach(const char *filename, uint8_t *rawcart)
{
    FILE *fd;
    unsigned int len;

    multicart_filetype = 0;
    multicart_filesize = 0;

    DBG(("multicart_bin_attach '%s'\n", filename));

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    len = (unsigned int)util_file_length(fd);
    fclose(fd);

    DBG(("multicart_bin_attach len: %04x\n", len));

    /* we accept 2MiB/4MiB images */
    switch (len) {
        case 0x200000:
            if (util_file_load(filename, rawcart, 0x200000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
            memcpy(&rawcart[0x200000], &rawcart[0x100000], 0x100000);
            memset(&rawcart[0x100000], 0xff, 0x100000);
            break;
        case 0x400000:
            if (util_file_load(filename, rawcart, 0x400000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    multicart_filesize = len;
    multicart_filetype = CARTRIDGE_FILETYPE_BIN;
    return multicart_common_attach();
}

int multicart_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i, offset;

    DBG(("multicart_crt_attach\n"));
    memset(rawcart, 0xff, MULTICART_MAX_SIZE);

    for (i = 0; i < 256; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if ((chip.bank >= 256) || (chip.size != 0x4000)) {
            return -1;
        }

        offset = (chip.bank << 14) + ((chip.start == 0x8000) ? 0 : (MULTICART_MAX_SIZE / 2));
        /*DBG(("bank: %d offset: %06x start: %04x\n", chip.bank, offset, chip.start));*/

        if (crt_read_chip(rawcart, offset, &chip, fd)) {
            return -1;
        }
    }

    if ((i != 128) && (i != 256)) {
        return -1;
    }

    multicart_filesize = i * 0x4000;
    multicart_filetype = CARTRIDGE_FILETYPE_CRT;
    return multicart_common_attach();
}

void multicart_detach(void)
{
    DBG(("multicart_detach\n"));
    if (multicart_list_item) {
        io_source_unregister(multicart_list_item);
    }
    multicart_list_item = NULL;
    lib_free(multicartromlo);
    lib_free(multicartromhi);
    multicartromlo = NULL;
    multicartromhi = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTMULTI snapshot module format:

   type  | name              | version | description
   -------------------------------------------------
   BYTE  | bankreg           |   0.1+  | state of banking register
   DWORD | filesize          |   0.1+  | size of the ROM (combined)
   ARRAY | ROM C1LO          |   0.1+  | 1MiB/2MiB of ROM data
   ARRAY | ROM C1HI          |   0.1+  | 1MiB/2MiB of ROM data
 */

/* FIXME: since we cant actually make snapshots due to TED bugs, the following
          is completely untested */

static const char snap_module_name[] = "CARTMULTI";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int multicart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    DBG(("multicart_snapshot_write_module\n"));

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)bankreg) < 0
        || SMW_DW(m, (uint32_t)multicart_filesize) < 0
        || SMW_BA(m, multicartromlo, multicart_filesize / 2)
        || SMW_BA(m, multicartromhi, multicart_filesize / 2) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return 0;
}

int multicart_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    uint32_t temp_filesize;

    DBG(("multicart_snapshot_read_module\n"));

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
        || SMR_B_INT(m, &bankreg) < 0
        || SMR_DW(m, &temp_filesize) < 0
        ) {
        goto fail;
    }

    multicart_filesize = temp_filesize;

    if (0
        || SMR_BA(m, multicartromlo, multicart_filesize / 2)
        || SMR_BA(m, multicartromhi, multicart_filesize / 2) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    multicart_common_attach();

    /* set filetype to none */
    multicart_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}


