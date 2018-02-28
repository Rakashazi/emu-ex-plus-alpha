/*
 * exos.c - Cartridge handling, Exos cart.
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

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "exos.h"
#include "export.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Exos v3
    - this refers to a family of cartridge boards produced by REX Datentechnik.
      generally it can be used with any kernal replacement

    8K ROM
    - ROM is mapped to $e000 using ultimax mode, but only when hirom is selected
      (the cartridge uses a clip to the inside of the computer for this)
*/

static const export_resource_t export_res = {
    CARTRIDGE_NAME_EXOS, 1, 0, NULL, NULL, CARTRIDGE_EXOS
};

/* ---------------------------------------------------------------------*/

BYTE exos_romh_read_hirom(WORD addr)
{
    return romh_banks[(addr & 0x1fff)];
}

int exos_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int exos_romh_phi2_read(WORD addr, BYTE *value)
{
    return exos_romh_phi1_read(addr, value);
}

int exos_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[addr & 0x1fff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void exos_config_init(void)
{
    cart_config_changed_slotmain(2, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

void exos_config_setup(BYTE *rawcart)
{
    memcpy(romh_banks, &rawcart[0], 0x2000);
    cart_config_changed_slotmain(2, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int exos_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    return 0;
}

int exos_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return exos_common_attach();
}

int exos_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return exos_common_attach();
}

void exos_detach(void)
{
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTEXOS snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | ROMH | 8192 BYTES of ROMH data
 */

static char snap_module_name[] = "CARTEXOS";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int exos_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, romh_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int exos_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_BA(m, romh_banks, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return exos_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
