/*
 * capture.c - Cartridge handling, Capture cart.
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
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64memrom.h"
#include "capture.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Jason Ranheim "Capture" Cartridge

    - 8K ROM (mapped to e000)
    - 8K RAM (mapped to 6000)

    7474  - 2 D-Flipflops (cart_enabled, register_enabled)
    74125 - 4 Tri-State Buffers
    74133 - NAND with 13 Inputs (adress decoder)

    the cartridge is disabled after a reset.

    when the freeze button is pressed the following happens:
    - an NMI is generated
    - as soon as the current adress is in bank 0xfe the cart switches
      to ultimax mode. the cart rom then contains one page full of
      "jmp $eaea", which ultimatively calls the freezer code.
    - the fff7/fff8 "register" logic is enabled

    after that any access (read or write) to $fff7 will turn the cart_enabled
    off (leave ultimax mode), and an access to $fff8 will turn the cart back
    on (enter ultimax mode). the "register logic" that causes this can only
    be disabled again by a hardware reset.
*/

/*
... program is loaded to $0800, then:

.C:fad3   A0 00      LDY #$00
.C:fad5   B9 00 08   LDA $0800,Y
.C:fad8   C0 03      CPY #$03
.C:fada   B0 05      BCS $FAE1

.C:fadc   D9 F8 FF   CMP $FFF8,Y  ; "4a 59 43" "JYC"
.C:fadf   D0 0C      BNE $FAED

.C:fae1   59 00 FE   EOR $FE00,Y
.C:fae4   99 00 08   STA $0800,Y
.C:fae7   C8         INY
.C:fae8   D0 EB      BNE $FAD5
.C:faea   6C 04 08   JMP ($0804)
.C:faed   60         RTS
*/

/* #define DBGCAPTURE */

#ifdef DBGCAPTURE
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static const export_resource_t export_res = {
    CARTRIDGE_NAME_CAPTURE, 1, 1, NULL, NULL, CARTRIDGE_CAPTURE
};

static int cart_enabled = 0;
static int freeze_pressed = 0;
static int register_enabled = 0;
static int romh_enabled = 0;

static void capture_reg(WORD addr)
{
    if (register_enabled) {
        if ((addr & 0xffff) == 0xfff7) {
            cart_enabled = 0;
            DBG(("CAPTURE: enable: %d\n", cart_enabled));
        } else if ((addr & 0xffff) == 0xfff8) {
            cart_enabled = 1;
            DBG(("CAPTURE: enable: %d\n", cart_enabled));
        } else if ((addr & 0xffff) == 0xfff9) {
            if ((!freeze_pressed) && (!romh_enabled)) {
                /* HACK: this one is needed to survive the ram clearing loop */
                cart_enabled = 0;
                DBG(("CAPTURE: enable: %d\n", cart_enabled));
            }
        }
    }
}

static void capture_romhflip(WORD addr)
{
    if (freeze_pressed) {
        if ((addr & 0xff00) == 0xfe00) {
            freeze_pressed = 0;
            romh_enabled = 1;
            DBG(("CAPTURE: romh enable: %d\n", romh_enabled));
        }
    }
}

BYTE capture_romh_read(WORD addr)
{
    capture_reg(addr);
    capture_romhflip(addr);

    if (cart_enabled) {
        if (romh_enabled) {
            return romh_banks[(addr & 0x1fff)];
        }
    }
    return mem_read_without_ultimax(addr);
}

void capture_romh_store(WORD addr, BYTE value)
{
    capture_reg(addr);
    /* capture_romhflip(addr); */
    if (cart_enabled == 0) {
        mem_store_without_ultimax(addr, value);
    }
}

/*
    there is Cartridge RAM at 0x6000..0x7fff
*/
BYTE capture_1000_7fff_read(WORD addr)
{
    if (cart_enabled) {
        if (addr >= 0x6000) {
            return export_ram0[addr - 0x6000];
        }
    }

    return mem_read_without_ultimax(addr);
}

void capture_1000_7fff_store(WORD addr, BYTE value)
{
    if (cart_enabled) {
        if (addr >= 0x6000) {
            export_ram0[addr - 0x6000] = value;
        }
    } else {
        mem_store_without_ultimax(addr, value);
    }
}

int capture_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int capture_romh_phi2_read(WORD addr, BYTE *value)
{
    return capture_romh_phi1_read(addr, value);
}

int capture_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (cart_enabled == 1) {
        if (addr >= 0x6000 && addr <= 0x7fff) {
            *value = export_ram0[addr - 0x6000];
            return CART_READ_VALID;
        }
        if (romh_enabled) {
            if (addr >= 0xe000) {
                *value = romh_banks[addr & 0x1fff];
                return CART_READ_VALID;
            }
        }
    }
    return CART_READ_THROUGH;
}
/******************************************************************************/

void capture_freeze(void)
{
    DBG(("CAPTURE: freeze\n"));
    if (freeze_pressed == 0) {
        cart_config_changed_slotmain(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
        cart_enabled = 1;
        freeze_pressed = 1;
        register_enabled = 1;
        romh_enabled = 0;
    }
}

void capture_config_init(void)
{
    DBG(("CAPTURE: config init\n"));
    cart_config_changed_slotmain(2, 2, CMODE_READ);
}

void capture_reset(void)
{
    DBG(("CAPTURE: reset\n"));
    cart_enabled = 0;
    register_enabled = 0;
    freeze_pressed = 0;
    cart_config_changed_slotmain(2, 2, CMODE_READ);
}

void capture_config_setup(BYTE *rawcart)
{
    DBG(("CAPTURE: config setup\n"));
    memcpy(romh_banks, rawcart, 0x2000);
    memset(export_ram0, 0, 0x2000);
    cart_config_changed_slotmain(2, 2, CMODE_READ);
}

static int capture_common_attach(void)
{
    DBG(("CAPTURE: attach\n"));
    if (export_add(&export_res) < 0) {
        return -1;
    }
    return 0;
}

int capture_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return capture_common_attach();
}

int capture_crt_attach(FILE *fd, BYTE *rawcart)
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

    return capture_common_attach();
}

void capture_detach(void)
{
    export_remove(&export_res);
}

/* ---------------------------------------------------------------------*/


/* CARTCAPTURE snapshot module format:

   type  | name            | description
   -------------------------------------
   BYTE  | enabled         | cartridge enabled flag
   BYTE  | freeze pressed  | freeze button pressed flag
   BYTE  | register enable | register enable flag
   BYTE  | ROMH enable     | ROMH enable flag
   ARRAY | ROMH            | 8192 BYTES of ROMH data
   ARRAY | RAM             | 8192 BYTES of RAM data
 */

static char snap_module_name[] = "CARTCAPTURE";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int capture_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)cart_enabled) < 0)
        || (SMW_B(m, (BYTE)freeze_pressed) < 0)
        || (SMW_B(m, (BYTE)register_enabled) < 0)
        || (SMW_B(m, (BYTE)romh_enabled) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int capture_snapshot_read_module(snapshot_t *s)
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

    if (0
        || (SMR_B_INT(m, &cart_enabled) < 0)
        || (SMR_B_INT(m, &freeze_pressed) < 0)
        || (SMR_B_INT(m, &register_enabled) < 0)
        || (SMR_B_INT(m, &romh_enabled) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return capture_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
