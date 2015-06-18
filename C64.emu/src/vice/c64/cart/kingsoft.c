/*
 * kingsoft.h - Cartridge handling, Kingsoft "Business Basic" cart.
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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
#include "comal80.h"
#include "lib.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Kingsoft Cartridge ("business basic" aka "s'more")

    - 24K ROM (3 8K Eproms)

    reading io1:
    - switches to 16k game mode
    - first eprom mapped to 8000 (ROML)
    - second eprom mapped to A000 (ROMH)

    writing io1:
    - switches to ultimax mode _only_:
      - if 0xc000 > address >= 0x8000
      - if address >= 0xe000
      (meaning 0x0000-0x7fff and 0xc000-0xdfff gives normal c64 ram/io)
    - first eprom mapped to 8000 (ROML)
    - third eprom mapped to e000 (ROMH)
*/

/* #define DEBUGKS */

#ifdef DEBUGKS
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

static int mode = 0;

static void setmode(int m)
{
    mode = m;
    DBG(("KINGSOFT: mode: %s\n", mode ? "ultimax" : "16k"));
    if (mode) {
        cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ);
    } else {
        cart_config_changed_slotmain(CMODE_16KGAME, CMODE_16KGAME, CMODE_READ);
    }
}

static BYTE kingsoft_io1_read(WORD addr)
{
    setmode(0);
    return 0;
}

static void kingsoft_io1_store(WORD addr, BYTE value)
{
    setmode(1);
}

static BYTE kingsoft_io1_peek(WORD addr)
{
    return mode;
}

static int kingsoft_dump(void)
{
    mon_out("mode: %s\n", mode ? "ultimax" : "16k");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t kingsoft_device = {
    CARTRIDGE_NAME_KINGSOFT,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    kingsoft_io1_store,
    kingsoft_io1_read,
    kingsoft_io1_peek,
    kingsoft_dump,
    CARTRIDGE_KINGSOFT,
    0,
    0
};

static io_source_list_t *kingsoft_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_KINGSOFT, 1, 1, &kingsoft_device, NULL, CARTRIDGE_KINGSOFT
};

/* ---------------------------------------------------------------------*/

/* ROML read - mapped to 8000 in 8k,16k,ultimax */
BYTE kingsoft_roml_read(WORD addr)
{
    return roml_banks[(addr & 0x1fff)];
}

/* ROMH read - mapped to A000 in 16k, to E000 in ultimax */
BYTE kingsoft_romh_read(WORD addr)
{
    return romh_banks[(addr & 0x1fff) + (mode << 13)];
}

/* ---------------------------------------------------------------------*/

void kingsoft_config_init(void)
{
    setmode(0);
}

void kingsoft_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x4000], 0x2000);
    setmode(0);
}

/* ---------------------------------------------------------------------*/
static int kingsoft_common_attach(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    kingsoft_list_item = io_source_register(&kingsoft_device);
    return 0;
}

int kingsoft_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x6000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return kingsoft_common_attach();
}

int kingsoft_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.start != 0x8000 || chip.size != 0x2000 || chip.bank > 3) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }
    return kingsoft_common_attach();
}

void kingsoft_detach(void)
{
    c64export_remove(&export_res);
    io_source_unregister(kingsoft_list_item);
    kingsoft_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTKINGSOFT"

int kingsoft_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)mode) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x4000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int kingsoft_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &mode) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x4000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    setmode(mode);
    return kingsoft_common_attach();
}
