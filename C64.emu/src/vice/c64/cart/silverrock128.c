/*
 *
 * silverrock128.c - Cartridge handling, Silverrock Game Cartridge 128K ROM
 *
 * Original Silverrock cartridge hardware design by:
 *  Uffe Jakobsen <microtop@starion.dk>
 *
 * Silverrock cartridge emulation written by:
 *  Uffe Jakobsen <microtop@starion.dk>
 *
 * Documentation found further below
 *
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

/* ---------------------------------------------------------------------*/

/*
 *
 * Documentation:
 *
 * Silverrock Productions 128K ROM Cartridge emulation
 *
 * Original Silverrock cartridge hardware design by:
 *  Uffe Jakobsen <microtop@starion.dk>
 *
 * Silverrock cartridge emulation for VICE written by:
 *  Uffe Jakobsen <microtop@starion.dk>
 *
 * Cartridge emulation supports the following games from Silverrock Productions:
 *
 *  - "Skaermtrolden Hugo" (1990/1991)
 *  - "Guldkorn Expressen" (1991)
 *  - "Kalas Puffs Expressen" (1991)
 *  - "Harald Haardtand" ("og kampen om de rene taender") (1992)
 *
 *
 * Background:
 *
 * Silverrock Productions produced and sold around 50000 cartridges of this design:
 *
 *  - "Skaermtrolden Hugo" accounting for more than 30000 of cartridge units.
 *  - "Guldkorn Expressen" accounting for aprox 5000 cartridge units.
 *  - "Kalas Puffs Expressen" accounting for aprox 5000 cartridge units.
 *  - "Harald Haardtand" accounting for aprox 5000-15000 cartridge units.
 *
 *
 * Cartridge hardware design:
 *
 * Cartridge hardware is designed around a 128Kb ROM (PROM or Mask ROM) which is
 * diveded into 16 banks each of 8Kb. When adressing the onboard bank-switching logic
 * the requested ROM bank is mapped at address range $8000-$9fff (8Kb)
 *
 * The cartridge PCB layout was cost-optimized for mass-production purposes.
 *
 * The address-/data-lines and bank-switching logic uses the closest address-/data-lines
 * with shortest and most direct path/distance in order to avoid too much PCB re-routing
 * and keep production costs as low as possible.
 *
 * This means that the respective address-/data-lines from the cartridge port
 * may not necessarily connect with the corresponding address-/data-line of
 * the ROM according to the official specs of the ROM.
 *
 * This has over the years given some headaches and invalid dumps when cartridge
 * dumpers that wanted to dump the cartridge contents by de-soldering the ROM and
 * dumping it using an EPROM programmer/reader.
 * The image extracted from such an operation would need transformation according
 * to the actual cartridge PCB re-routing of address-/data-lines. Alternatively
 * one can make an adapter that implements this applied re-routing of the
 * address-/data-lines.
 *
 * This mass-production cost optimization also results in an obscured bank-switching
 * address pattern/values more info on that later.
 *
 *
 * Cartridge hardware revisions:
 *
 * [HWrev1]: The original "Hugo" PCB [HWrev1] is labeled "HUGO Copyright 1990"
 * on both sides of the PCB.
 * PCB production date is stamped on the back og PCB in the format: "YYMM"
 * 1Mbit 27C010 EPROM or PROM (typical Atmel)
 * "Hugo" PCB [HWrev1] contains the following discrete logic IC components:
 * 74LS00N (DIP) and 74LS237N (DIP)
 * DIP pitch (pin spacing) 2.54mm (0.1 inch)
 * Assembly/production facilities: Philips, Strandlodsvej (Amager), Copenhagen, Denmark
 *
 * [HWrev1] Bank-switching pattern:
 * In order for the rom contents to appear as a continuous memory layout the following
 * ROM bank-switching pattern must be applied by writing to adress 0xDE00:
 * Cartridge bank-switching values: 00 80 10 90 20 a0 30 b0 40 c0 50 d0 60 e0 70 f0
 * Writing the bank-switch value to any address in address-range 0xDE00-0xDEFF
 * will work.
 *
 * [HWrev2]: Revised PCB [HWrev2] for SMD mount is labeled "SO-A4-1"
 * on both sides of PCB.
 * 1Mbit Mask ROM (SOIC-32)
 * PCB contains the following discrete logic SMD IC components:
 * 74HCT02T (SOIC-14) and 74HCT174T (SOIC-16)
 * SOIC (pitch) pin spacing 1.27mm (SMD mounted)
 * Assembly/production facilities: Sono Press, Germany.
 *
 * [HWrev2] Uses alternative bank-switching pattern:
 * Writing *ANY* value to address 0xDE0y (offset y) will select ROM bank y
 * Cartridge still has 16 banks and the lower four bits in the address selects
 * the right bank. Hence the valid address range is 0xDE00-0xDE0F
 *
 *
 * Cartridge releases - hardware/software combinations:
 *
 * "Skaermtrolden Hugo" and "Harald Haardtand":
 * All units were produced using the original "Hugo" PCB [HWrev1].
 * PROMs and EPROMs (and not Mask ROMs) were used for these releases.
 * In the case of "Skaermtrolden Hugo" it should hit the stores before Christmas 1990.
 * We barely made it - just a few days before Christmas.
 * The local cartridge assembly/production facility (Philips) was running
 * on 24x7 standby just waiting for the master ROM image.
 *
 * "Skaermtrolden Hugo" [SWrev1]:
 * The PROMs for the first batches of "Skaermtrolden Hugo" are labeled "HUGO 2012"
 * Short for "HUGO December 20. 1990"
 *
 * "Skaermtrolden Hugo" [SWrev2]:
 * In January 1991 an updated master ROM image was made containing fixes for C64C.
 * The PROMs for these batches of "Skaermtrolden Hugo" are labeled "HUGO 1001"
 * Short for "HUGO January 10. 1991"
 *
 * This means that "Skaermtrolden Hugo" actually exists in two different versions
 * in the wild: [SWrev1] and [SWrev2].
 * There is no to tell the difference from the cartridge casing.
 * "Skaermtrolden Hugo" [SWrev2] contains a greetings screen that [SWrev1] does not have.
 * Press CTRL Top-Left-Arrow 1 2 during startup will take you to the greetings
 * on "Skaermtrolden Hugo" [SWrev2].
 *
 * "Harald Haardtand" [SWrev1] [HWrev1]:
 * "Harald Haardtand": only exist in one software/hardware revision.
 * Hardware is based on original "HUGO" PCB [HWrev1]
 *
 * "Guldkorn Expressen" [SWrev1] [HWrev1] :
 * "Guldkorn Expressen" is the danish (primary) version of "Kalas Puffs Expressen"
 * "Guldkorn Expressen" [SWrev1] first batches used the original "Hugo" PCB [HWrev1].
 * "Guldkorn Expressen" [HWrev1] PROMs/EPROMs are labeled "OTA 27.09.91"

 * "Guldkorn Expressen" [SWrev2] [HWrev2] :
 * "Guldkorn Expressen" later batches used the revised SMD PCB [HWrev2].
 * "Guldkorn Expressen" [HWrev2] Mask ROMs are labeled "SO OTAGULD DK xxxx" (TDB)
 * The software on "Guldkorn Expressen" [HWrev1] and [HWrev2] is not identical.
 * The difference between "Guldkorn Expressen" [SWrev1] and [SWrev2] is five (5) bytes
 * in what appears to be a charmap AFAIK.
 * AFAIK: I have no clear memory of this - need to dig out more notes.
 *
 * "Kalas Puffs Expressen" [SWrev1] [HWrev2]
 * "Kalas Puffs Expressen" is the swedish (secondary) version of "Guldkorn Expressen"
 * "Kalas Puffs Expressen" used the revised SMD PCB [HWrev2].
 * Mask ROMs are labeled "SO OTAGULD SW 00900047"  ("10/91") (October 1991)
 * "Kalas Puffs Expressen" using [HWrev1] is never seen "in the wild" AFAIK.
 * AFAIK: I have no clear memory of this - need to dig out more notes.
 *
 *
 * Signed: Uffe Jakobsen, September 5th 2011
 *
 * Contact info: <microtop@starion.dk>
 *
 */

/* ---------------------------------------------------------------------*/

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "silverrock128.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"


static const BYTE bank_seq[] = {0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
/* static const BYTE bank_val[] = {0x00, 0x80, 0x10, 0x90, 0x20, 0xa0, 0x30, 0xb0, 0x40, 0xc0, 0x50, 0xd0, 0x60, 0xe0, 0x70, 0xf0}; */

static BYTE regval = 0;
static int currbank = 0;

static void silverrock128_io1_store(WORD addr, BYTE value)
{
    BYTE bank_number, bank_index;

    if (addr == 0x0) {
        /* Cartridge HW rev01 (or HW rev02 adressing first bank (special case)) */
        bank_index = ((value & 0xf0) >> 4);
        bank_number = bank_seq[bank_index];
    } else {
        if (addr <= 0x0f) {
            /* Cartridge HW rev02 */
            bank_number = (BYTE)addr;
            /* safe check that we've done things right... */
            bank_index = ((value & 0xf0) >> 4);
            if (bank_number != bank_seq[bank_index]) {
                bank_number = 0; /* ERROR (suggest bank 0) */
            }
        } else {
            bank_number = 0; /* ERROR: out of range (suggest bank 0) */
        }
    }

    cart_romlbank_set_slotmain(bank_number);
    regval = value;
    currbank = bank_number;
}

static BYTE silverrock128_io1_peek(WORD addr)
{
    return regval;
}

static int silverrock128_dump(void)
{
    mon_out("Currently selected EPROM bank: %d, cart status: %s\n",
            currbank,
            (regval & 0x80) ? "Disabled" : "Enabled");
    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t silverrock128_device = {
    CARTRIDGE_NAME_SILVERROCK_128,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    silverrock128_io1_store,
    NULL,
    silverrock128_io1_peek,
    silverrock128_dump,
    CARTRIDGE_SILVERROCK_128,
    0,
    0
};

static io_source_list_t *silverrock128_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_SILVERROCK_128, 1, 0, &silverrock128_device, NULL, CARTRIDGE_SILVERROCK_128
};

/* ---------------------------------------------------------------------*/

void silverrock128_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

void silverrock128_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000 * 33);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    cart_romlbank_set_slotmain(0);
}

/* ---------------------------------------------------------------------*/
static int silverrock128_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    silverrock128_list_item = io_source_register(&silverrock128_device);
    return 0;
}

int silverrock128_bin_attach(const char *filename, BYTE *rawcart)
{
    int size = 0x42000;

    memset(rawcart, 0xff, size);
    while (size != 0) {
        if (util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            size -= 0x2000;
        } else {
            return silverrock128_common_attach();
        }
    }
    return -1;
}

int silverrock128_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    memset(rawcart, 0xff, 0x42000);

    while (1) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }
        if (chip.bank > 32 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }
    return silverrock128_common_attach();
}

void silverrock128_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(silverrock128_list_item);
    silverrock128_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSILVERROCK128 snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | regval |   0.1   | register
   BYTE  | bank   |   0.0+  | current bank
   ARRAY | ROML   |   0.0+  | 262144 BYTES of ROML data
 */

static char snap_module_name[] = "CARTSILVERROCK128";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int silverrock128_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, regval) < 0
        || SMW_B(m, (BYTE)currbank) < 0
        || SMW_BA(m, roml_banks, 0x2000 * 32) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int silverrock128_snapshot_read_module(snapshot_t *s)
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

     /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B(m, &regval) < 0) {
            goto fail;
        }
    } else {
        regval = 0;
    }

   if (0
        || SMR_B_INT(m, &currbank) < 0
        || SMR_BA(m, roml_banks, 0x2000 * 32) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return silverrock128_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
