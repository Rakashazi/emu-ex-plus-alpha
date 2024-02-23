/*
 * finalplus.c - Cartridge handling, Final cart.
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

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "finalplus.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* #define DEBUGFC */

#ifdef DEBUGFC
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Final Cartridge Plus

    - reset button, cart enable switch
    - 32K ROM, of which 24K are used

        EPROM $2000-$4000 is visible at $e000-$ffff, if enabled
        EPROM $4000-$5fff is visible at $8000-$9fff, if enabled
        EPROM $6000-$7fff is visible at $a000-$bfff, if enabled

An NMI can is triggered by the cart, if address $0001 is written to and the cartridge is enabled.

The cart can also be disabled of by software, writing clearing bit 4 when writing to $df00-$dfff.
cart ROM at $e000-$ffff can be disabled by setting bit 5 to "zero" when writing to $df00-$dfff.
cart ROM at $8000-$bfff can be disabled by setting bit 6 to "one" when writing to $df00-$dfff.
Bit 7 of a byte written to $df00-$dfff can be read back from the cartridge if enabled (kind of a memory cell).

*/

static int fcplus_enabled;
static int fcplus_bit;
static int fcplus_roml;
static int fcplus_romh;

/* some prototypes are needed */
static uint8_t final_plus_io2_read(uint16_t addr);
static void final_plus_io2_store(uint16_t addr, uint8_t value);
static int final_plus_dump(void);

static io_source_t final_plus_io2_device = {
    CARTRIDGE_NAME_FINAL_PLUS, /* name of the device */
    IO_DETACH_CART,            /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,     /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,      /* range for the device, address is ignored, reg:$df00, mirrors:$df01-$dfff */
    1,                         /* read is always valid */
    final_plus_io2_store,      /* store function */
    NULL,                      /* NO poke function */
    final_plus_io2_read,       /* read function */
    final_plus_io2_read,       /* peek function */
    final_plus_dump,           /* device state information dump function */
    CARTRIDGE_FINAL_PLUS,      /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0,                         /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE             /* NO mirroring */
};

static io_source_list_t *final_plus_io2_list_item = NULL;

static const export_resource_t export_res_plus = {
    CARTRIDGE_NAME_FINAL_PLUS, 1, 1, NULL, &final_plus_io2_device, CARTRIDGE_FINAL_PLUS
};

/* ---------------------------------------------------------------------*/

uint8_t final_plus_io2_read(uint16_t addr)
{
    DBG(("io2 r %04x\n", addr));
    return ((fcplus_bit << 7) | (fcplus_roml << 6) | (fcplus_romh << 5) | (fcplus_enabled << 4));
}

void final_plus_io2_store(uint16_t addr, uint8_t value)
{
    if (fcplus_enabled == 1) {
        fcplus_bit = (value >> 7) & 1;
        fcplus_roml = ((value >> 6) & 1) ^ 1;
        fcplus_romh = ((value >> 5) & 1);
        /* fcplus_enabled = ((value >> 4) & 1) ^ 1; */
        DBG(("io2 w %04x %02x (bit:%d, rom8000:%d, romE000:%d, enabled: %d)\n", addr, value, fcplus_bit, fcplus_roml, fcplus_romh, fcplus_enabled));

        if ((fcplus_roml == 0) && (fcplus_romh == 0)) {
            cart_config_changed_slotmain(2, 2, CMODE_WRITE);
        } else {
            cart_config_changed_slotmain(0, 3, CMODE_WRITE | CMODE_PHI2_RAM);
        }
    }
}

static int final_plus_dump(void)
{
    int rom_8000 = 0;
    int rom_a000 = 0;
    int rom_e000 = 0;

    if (fcplus_enabled) {
        if (!fcplus_roml) {
            rom_8000 = 1;
        }
        rom_a000 = 1;
        if (fcplus_romh) {
            rom_e000 = 1;
        }
    }

    mon_out("$8000-$9FFF ROM: %s\n", (rom_8000) ? "enabled" : "disabled");
    mon_out("$A000-$BFFF ROM: %s\n", (rom_a000) ? "enabled" : "disabled");
    mon_out("$E000-$FFFF ROM: %s\n", (rom_e000) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t final_plus_roml_read(uint16_t addr)
{
    if (fcplus_roml == 1) {
        return roml_banks[(addr & 0x1fff)];
    } else {
        return mem_read_without_ultimax(addr);
    }
}

uint8_t final_plus_romh_read(uint16_t addr)
{
    if ((fcplus_enabled == 1) && (fcplus_romh == 1)) {
        return romh_banks[(addr & 0x1fff)];
    } else {
        return mem_read_without_ultimax(addr);
    }
}

uint8_t final_plus_a000_bfff_read(uint16_t addr)
{
    if ((fcplus_enabled == 1) && (fcplus_roml == 1)) {
        return roml_banks[0x2000 + (addr & 0x1fff)];
    } else {
        return mem_read_without_ultimax(addr);
    }
}

int final_plus_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    return CART_READ_C64MEM;
}

int final_plus_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    return final_plus_romh_phi1_read(addr, value);
}

int final_plus_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if (fcplus_roml == 1) {
        if (addr >= 0x8000 && addr <= 0x9fff) {
            *value = roml_banks[addr & 0x1fff];
            return CART_READ_VALID;
        }
        if (addr >= 0xa000 && addr <= 0xbfff) {
            *value = roml_banks[(addr & 0x1fff) + 0x2000];
            return CART_READ_VALID;
        }
    }
    if (fcplus_romh == 1) {
        if (addr >= 0xe000) {
            *value = romh_banks[addr & 0x1fff];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

void final_plus_freeze(void)
{
    DBG(("fc+ freeze\n"));
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_ULTIMAX, CMODE_READ | CMODE_RELEASE_FREEZE | CMODE_PHI2_RAM);
    fcplus_enabled = 1;
    fcplus_roml = 1;
    fcplus_romh = 1;
}

void final_plus_config_init(void)
{
    DBG(("fc+ config init\n"));
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_ULTIMAX, CMODE_READ | CMODE_PHI2_RAM);
    fcplus_enabled = 1;
    fcplus_roml = 1;
    fcplus_romh = 1;
}
/*
EPROM $2000-$4000 is visible at $e000-$ffff, if enabled
EPROM $4000-$5fff is visible at $8000-$9fff, if enabled
EPROM $6000-$7fff is visible at $a000-$bfff, if enabled
*/
void final_plus_config_setup(uint8_t *rawcart)
{
    DBG(("fc+ config setup\n"));
    memcpy(roml_banks, &rawcart[0x4000], 0x4000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_ULTIMAX, CMODE_READ | CMODE_PHI2_RAM);
}

/* ---------------------------------------------------------------------*/

static int final_plus_common_attach(void)
{
    if (export_add(&export_res_plus) < 0) {
        return -1;
    }

    final_plus_io2_list_item = io_source_register(&final_plus_io2_device);

    return 0;
}

int final_plus_bin_attach(const char *filename, uint8_t *rawcart)
{
    DBG(("fc+ bin attach\n"));

    /* accept 32k and 24k binaries */
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x6000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        memmove(&rawcart[0x2000], rawcart, 0x6000);
    }

    return final_plus_common_attach();
}

int final_plus_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    DBG(("fc+ crt attach\n"));
    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x8000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return final_plus_common_attach();
}

void final_plus_detach(void)
{
    export_remove(&export_res_plus);
    io_source_unregister(final_plus_io2_list_item);
    final_plus_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTFCP snapshot module format:

   type  | name      | description
   -------------------------------
   BYTE  | enable    | cartridge enabled flag
   BYTE  | bit7      | bit 7 of the register
   BYTE  | ROML bank | ROML bank
   BYTE  | ROMH bank | ROMH bank
   ARRAY | ROML      | 16384 BYTES of ROML data
   ARRAY | ROMH      | 8192 BYTES of ROMH data
 */

static const char snap_module_name[] = "CARTFCP";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int final_plus_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)fcplus_enabled) < 0)
        || (SMW_B(m, (uint8_t)fcplus_bit) < 0)
        || (SMW_B(m, (uint8_t)fcplus_roml) < 0)
        || (SMW_B(m, (uint8_t)fcplus_romh) < 0)
        || (SMW_BA(m, roml_banks, 0x4000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int final_plus_snapshot_read_module(snapshot_t *s)
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

    if (0
        || (SMR_B_INT(m, &fcplus_enabled) < 0)
        || (SMR_B_INT(m, &fcplus_bit) < 0)
        || (SMR_B_INT(m, &fcplus_roml) < 0)
        || (SMR_B_INT(m, &fcplus_romh) < 0)
        || (SMR_BA(m, roml_banks, 0x4000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return final_plus_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
