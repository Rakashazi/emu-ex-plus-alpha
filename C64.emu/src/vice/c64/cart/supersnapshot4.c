/*
 * supersnapshot4.c - Cartridge handling, Super Snapshot cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Nathan Huizinga <nathan.huizinga@chess.nl>
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
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "log.h"
#include "snapshot.h"
#include "supersnapshot4.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Super Snapshot v4

    - 32K ROM,2*16K Banks
    - 8k RAM

    io1: (read/write)
        second last page of cart ram

    io2 (read)
     df01 - ram config (register)
     else - cart rom (last page of first 8k of current bank)
    io2 (write)
     df00 -  (register)

        7xxx3210

        bit 0 - ?
        bit 1 - ? (write 1 to release freeze mode)
        bit 2 - ROM bank select
        bit 3 - write 1 to disable cartridge
        bit 7 - ?

        if bit0, bit1, bit7 are all 0, then
            - ultimax mapping is selected
            - RAM is enabled at ROML
        else if bit 0 is 0, then
            - 16k mapping is enabled
            if bit 0 is 1, then
            - 8k mapping is enabled

     df01 - ram config (register)

        if written value == last value - 1, then
            - ultimax mapping is selected
            - RAM is enabled at ROML

        if written value == last value + 1, then
            - ROM is enabled at ROML
            - exrom is deasserted (switch to either 8k or 16k mapping)
*/

/* #define DBGSS4 */

#ifdef DBGSS4
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/* Super Snapshot configuration flags.  */
static BYTE ramconfig = 0xff, romconfig = 9;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE supersnapshot_v4_io1_read(WORD addr);
static void supersnapshot_v4_io1_store(WORD addr, BYTE value);
static BYTE supersnapshot_v4_io2_read(WORD addr);
static void supersnapshot_v4_io2_store(WORD addr, BYTE value);

static io_source_t ss4_io1_device = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    supersnapshot_v4_io1_store,
    supersnapshot_v4_io1_read,
    NULL,
    NULL, /* TODO: dump */
    CARTRIDGE_SUPER_SNAPSHOT,
    0,
    0
};

static io_source_t ss4_io2_device = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    0,
    supersnapshot_v4_io2_store,
    supersnapshot_v4_io2_read,
    NULL,
    NULL, /* TODO: dump */
    CARTRIDGE_SUPER_SNAPSHOT,
    0,
    0
};

static io_source_list_t *ss4_io1_list_item = NULL;
static io_source_list_t *ss4_io2_list_item = NULL;


static const export_resource_t export_res_v4 = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT, 1, 1, &ss4_io1_device, &ss4_io2_device, CARTRIDGE_SUPER_SNAPSHOT
};

/* ---------------------------------------------------------------------*/

BYTE supersnapshot_v4_io1_read(WORD addr)
{
    return export_ram0[0x1e00 + (addr & 0xff)];
}

void supersnapshot_v4_io1_store(WORD addr, BYTE value)
{
    export_ram0[0x1e00 + (addr & 0xff)] = value;
}

BYTE supersnapshot_v4_io2_read(WORD addr)
{
    ss4_io2_device.io_source_valid = 1;

    if ((addr & 0xff) == 1) {
        return ramconfig;
    }

    addr |= 0xdf00;
    return roml_banks[(addr & 0x1fff) + (0x2000 * roml_bank)];
}

void supersnapshot_v4_io2_store(WORD addr, BYTE value)
{
    DBG(("SS4: io2 w %04x %02x\n", addr, value));

    if ((addr & 0xff) == 0) {
        int mode = CMODE_WRITE;

#ifdef DBGSS4
        if (value & ~(0x80 | 0x08 | 0x04 | 0x02 | 0x01)) {
            DBG(("poof!\n"));
            exit(-1);
        }
#endif
        mode |= ((ramconfig == 0) ? CMODE_EXPORT_RAM : 0);

        if (value & 0x83) {
            if (value & 0x01) {
                romconfig = CMODE_8KGAME;
            } else {
                romconfig = CMODE_16KGAME;
            }
        } else {
            romconfig = CMODE_ULTIMAX;
            mode |= CMODE_EXPORT_RAM;
        }

        if (value & 0x02) {
            mode |= CMODE_RELEASE_FREEZE;
        }
        if (value & 0x04) {
            romconfig |= (1 << CMODE_BANK_SHIFT);
        }
        if (value & 0x08) {
            romconfig = CMODE_RAM; /* disable cart */
        }

/* old code, remove this if the above seems to work ok */
#if 0
        romconfig = (BYTE)((value == 2) ? 1 : (1 | (1 << CMODE_BANK_SHIFT)));
        mode = mode | ((ramconfig == 0) ? CMODE_EXPORT_RAM : 0);
        if ((value & 0x7f) == 0) {
            romconfig = 3;
            mode |= CMODE_EXPORT_RAM;
        }
        if ((value & 0x7f) == 1 || (value & 0x7f) == 3) {
            romconfig = 0;
        }
        if ((value & 0x7f) == 6) {
            romconfig = 1 | (1 << CMODE_BANK_SHIFT);
            mode |= CMODE_RELEASE_FREEZE;
        }
        if ((value & 0x7f) == 9) {
            romconfig = 2; /* exrom */
            /* mode |= CMODE_PHI2_RAM; */
        }
#endif
        cart_config_changed_slotmain((BYTE)(romconfig & 3), romconfig, mode);
    }
    if ((addr & 0xff) == 1) {
        int mode = CMODE_WRITE;
        /* FIXME: this is odd, it probably doesnt quite do what really happens */
        if (((ramconfig - 1) & 0xff) == value) {
            DBG(("SS4: - %02x %02x\n", ramconfig, value));
            ramconfig = value;
            romconfig |= 3; /* game,exrom */
            mode |= CMODE_EXPORT_RAM;
        }
        if (((ramconfig + 1) & 0xff) == value) {
            DBG(("SS4: + %02x %02x\n", ramconfig, value));
            ramconfig = value;
            romconfig &= ~(1 << 1); /* exrom */
            mode &= ~(CMODE_EXPORT_RAM);
        }
        cart_config_changed_slotmain((BYTE)(romconfig & 3), romconfig, mode);
    }
}

/* ---------------------------------------------------------------------*/

BYTE supersnapshot_v4_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[addr & 0x1fff];
    }

    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void supersnapshot_v4_roml_store(WORD addr, BYTE value)
{
    if (export_ram) {
        export_ram0[addr & 0x1fff] = value;
    }
}

/* ---------------------------------------------------------------------*/

void supersnapshot_v4_freeze(void)
{
    cart_config_changed_slotmain(3, 3, CMODE_READ | CMODE_EXPORT_RAM);
}

void supersnapshot_v4_config_init(void)
{
    cart_config_changed_slotmain(1 | (1 << CMODE_BANK_SHIFT), 1 | (1 << CMODE_BANK_SHIFT), CMODE_READ);
}

void supersnapshot_v4_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    cart_config_changed_slotmain(1 | (1 << CMODE_BANK_SHIFT), 1 | (1 << CMODE_BANK_SHIFT), CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int supersnapshot_v4_common_attach(void)
{
    if (export_add(&export_res_v4) < 0) {
        return -1;
    }
    ss4_io1_list_item = io_source_register(&ss4_io1_device);
    ss4_io2_list_item = io_source_register(&ss4_io2_device);
    return 0;
}

int supersnapshot_v4_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return supersnapshot_v4_common_attach();
}

/*
 * (old) wrong formats:
 *
 * cartconv produced this until 2011:
 *
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $2000 $2010
 * $002050 CHIP ROM   #001 $8000 $2000 $2010
 * $004060 CHIP ROM   #002 $8000 $2000 $2010
 * $006070 CHIP ROM   #003 $8000 $2000 $2010
 *
 * cartconv produced this from 2011 to 12/2015:
 * 
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $2000 $2010
 * $002050 CHIP ROM   #000 $a000 $2000 $2010
 * $004060 CHIP ROM   #001 $8000 $2000 $2010
 * $006070 CHIP ROM   #001 $a000 $2000 $2010
 *
 * (new) correct format (since 12/2015):
 *
 * offset  sig  type  bank start size  chunklen
 * $000040 CHIP ROM   #000 $8000 $4000 $4010
 * $004050 CHIP ROM   #001 $8000 $4000 $4010
 *
 */
int supersnapshot_v4_crt_attach(FILE *fd, BYTE *rawcart)
{
    int i, pos, banks, chips;
    crt_chip_header_t chip;

    /* find out how many banks and chips are in the file */
    /* FIXME: this is kindof ugly, perhaps make it a generic function */
    banks = 0;
    pos = ftell(fd);
    for (chips = 0; chips < 4; chips++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        if (chip.bank > banks) {
            banks = chip.bank;
        }
    }
    banks++;
    if ((chips != 2) && (chips != 4)) {
        return -1;
    }
    fseek(fd, pos, SEEK_SET);

    for (i = 0; i < chips; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }
        if ((chips == 2) && (banks == 2) && (chip.size == 0x4000)) {
            if ((chip.bank > 1) || (chip.start != 0x8000)) {
                return -1;
            }
            if (crt_read_chip(rawcart, (chip.bank << 14), &chip, fd)) {
                return -1;
            }
        } else if ((chips == 4) && (banks == 2) && (chip.size == 0x2000)) {
            if ((chip.bank > 1) || ((chip.start != 0x8000) && (chip.start != 0xa000))) {
                return -1;
            }
            if (crt_read_chip(rawcart, (chip.start & 0x2000) + (chip.bank << 14), &chip, fd)) {
                return -1;
            }
        } else if ((chips == 4) && (banks == 4) && (chip.size == 0x2000)) {
            if ((chip.bank > 3) || (chip.start != 0x8000)) {
                return -1;
            }
            if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    return supersnapshot_v4_common_attach();
}

void supersnapshot_v4_detach(void)
{
    export_remove(&export_res_v4);
    io_source_unregister(ss4_io1_list_item);
    io_source_unregister(ss4_io2_list_item);
    ss4_io1_list_item = NULL;
    ss4_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSS4 snapshot module format:

   type  | name       | description
   --------------------------------
   BYTE  | RAM config | RAM configuration
   BYTE  | ROM config | ROM configuration
   ARRAY | ROML       | 16384 BYTES of ROML data
   ARRAY | ROMH       | 16384 BYTES of ROMH data
   ARRAY | RAM        | 8192 BYTES of RAM data
 */

static char snap_module_name[] = "CARTSS4";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int supersnapshot_v4_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, ramconfig) < 0
        || SMW_B(m, romconfig) < 0
        || SMW_BA(m, roml_banks, 0x4000) < 0
        || SMW_BA(m, romh_banks, 0x4000) < 0
        || SMW_BA(m, export_ram0, 0x2000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int supersnapshot_v4_snapshot_read_module(snapshot_t *s)
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
        || SMR_B(m, &ramconfig) < 0
        || SMR_B(m, &romconfig) < 0
        || SMR_BA(m, roml_banks, 0x4000) < 0
        || SMR_BA(m, romh_banks, 0x4000) < 0
        || SMR_BA(m, export_ram0, 0x2000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return supersnapshot_v4_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
