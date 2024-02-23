/*
 * generic.c -- generic C128 cartridges ("external Function ROM")
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
#include "util.h"
#include "log.h"

#include "c64cart.h"
#include "export.h"
#include "c128cart.h"
#include "functionrom.h"

#include "crt.h"
#include "generic.h"

/* #define DBGGENERIC */

#ifdef DBGGENERIC
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_GENERIC, 1, 1, NULL, NULL, CARTRIDGE_C128_GENERIC
};

void c128generic_config_setup(uint8_t *rawcart)
{
    DBG(("c128generic_config_setup(ptr:%p)\n", (void*)rawcart));
    /* copy loaded cartridge data into actually used ROM array */
    memcpy(&ext_function_rom[0], rawcart, EXTERNAL_FUNCTION_ROM_SIZE);
}

static int c128generic_common_attach(void)
{
    /* setup i/o device */
    if (export_add(&export_res) < 0) {
        return -1;
    }
    return 0;
}

int c128generic_bin_attach(const char *filename, uint8_t *rawcart)
{
    unsigned int len = EXTERNAL_FUNCTION_ROM_SIZE;
    DBG(("c128generic_bin_attach '%s'\n", filename));
    while(len >= 0x400) {
        if (util_file_load(filename, rawcart, len, UTIL_FILE_LOAD_SKIP_ADDRESS) == 0) {
            DBG(("c128generic_bin_attach loaded 0x%04x bytes\n", len));
            /* create mirrors */
            while (len < EXTERNAL_FUNCTION_ROM_SIZE) {
                memcpy(rawcart + len, rawcart, len);
                len *= 2;
            }
            return c128generic_common_attach();
        }
        len /= 2;
    }
    return -1;
}

/*
    returns -1 on error, else a positive CRT ID
*/
int c128generic_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    unsigned int len;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    DBG(("chip1 at %02x len %02x\n", chip.start, chip.size));
    if (chip.start == 0x8000 && chip.size > 0 && chip.size <= 0x4000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        /* create mirrors */
        len = chip.size;
        while (len < 0x4000) {
            memcpy(rawcart + len, rawcart, len);
            len *= 2;
        }
    } else if (chip.start == 0xc000 && chip.size > 0 && chip.size <= 0x4000) {
        if (crt_read_chip(rawcart + 0x4000, 0, &chip, fd)) {
            return -1;
        }
        /* create mirrors */
        len = chip.size;
        while (len < 0x4000) {
            memcpy(rawcart + 0x4000 + len, rawcart + 0x4000, len);
            len *= 2;
        }
    } else {
        log_error(LOG_DEFAULT, "invalid CRT CHIP address: $%04x", chip.start);
        return -1;
    }

    if (crt_read_chip_header(&chip, fd)) {
        DBG(("no second chip\n"));
        c128generic_common_attach();
        return CARTRIDGE_C128_GENERIC;
    }

    DBG(("chip2 at %02x len %02x\n", chip.start, chip.size));

    if (chip.start == 0x8000 && chip.size > 0 && chip.size <= 0x4000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        /* create mirrors */
        len = chip.size;
        while (len < 0x4000) {
            memcpy(rawcart + len, rawcart, len);
            len *= 2;
        }
    } else if (chip.start == 0xc000 && chip.size > 0 && chip.size <= 0x4000) {
        if (crt_read_chip(rawcart + 0x4000, 0, &chip, fd)) {
            return -1;
        }
        /* create mirrors */
        len = chip.size;
        while (len < 0x4000) {
            memcpy(rawcart + 0x4000 + len, rawcart + 0x4000, len);
            len *= 2;
        }
    } else {
        log_error(LOG_DEFAULT, "invalid CRT CHIP address: $%04x", chip.start);
        return -1;
    }
    c128generic_common_attach();
    return CARTRIDGE_C128_GENERIC;
}

void c128generic_detach(void)
{
    /* io_source_unregister(c128generic_io1_list_item);
    io_source_unregister(c128generic_io2_list_item);
    c128generic_io1_list_item = NULL;
    c128generic_io2_list_item = NULL; */
    export_remove(&export_res);
}

void c128generic_reset(void)
{
    DBG(("c128generic_reset\n"));
}
