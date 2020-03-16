/*
 * maxbasic.c - Cartridge handling, max-basic cart.
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
#include "maxbasic.h"
#include "export.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    MAX Basic

    16K ROM
    - ROM is mapped to $8000 and $e000 using ultimax mode

    2K RAM
    - RAM is mapped to $0800
*/

static const export_resource_t export_res = {
    CARTRIDGE_NAME_MAX_BASIC, 0, 1, NULL, NULL, CARTRIDGE_MAX_BASIC
};

/* ---------------------------------------------------------------------*/

uint8_t maxbasic_0800_0fff_read(uint16_t addr)
{
    return export_ram0[addr & 0x07ff];
}

void maxbasic_0800_0fff_store(uint16_t addr, uint8_t value)
{
    export_ram0[addr & 0x07ff] = value;
}

uint8_t maxbasic_roml_read(uint16_t addr)
{
    return roml_banks[(addr & 0x1fff)];
}

uint8_t maxbasic_romh_read(uint16_t addr)
{
    return romh_banks[(addr & 0x1fff)];
}

int maxbasic_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if (addr >= 0xe000) {
        *value = romh_banks[addr & 0x1fff];
        return CART_READ_VALID;
    } else if ((addr >= 0xa000) && (addr <= 0xbfff)) {
        *value = roml_banks[addr & 0x1fff];
        return CART_READ_VALID;
    } else if ((addr >= 0x0800) && (addr <= 0x0fff)) {
        *value = export_ram0[addr & 0x07ff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void maxbasic_config_init(void)
{
    cart_config_changed_slotmain(3, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

void maxbasic_config_setup(uint8_t *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    cart_config_changed_slotmain(3, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int maxbasic_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    return 0;
}

int maxbasic_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return maxbasic_common_attach();
}

int maxbasic_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart + (i * 0x2000), 0, &chip, fd)) {
            return -1;
        }
    }

    return maxbasic_common_attach();
}

void maxbasic_detach(void)
{
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/

/* CARTMAXBASIC snapshot module format:

   type  | name | description
   --------------------------
   ARRAY | ROML | 8192 BYTES of ROML data
   ARRAY | ROMH | 8192 BYTES of ROMH data
   ARRAY | RAM  | 2048 BYTES of RAM data
 */

static char snap_module_name[] = "CARTMAXBASIC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int maxbasic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, roml_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMW_BA(m, romh_banks, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMW_BA(m, export_ram0, 0x0800) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int maxbasic_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_BA(m, roml_banks, 0x2000) < 0) {
        goto fail;
    }

    if (SMR_BA(m, romh_banks, 0x2000) < 0) {
        goto fail;
    }

    if (SMR_BA(m, export_ram0, 0x0800) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return maxbasic_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
