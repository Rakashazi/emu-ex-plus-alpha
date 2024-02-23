/*
 * atomicpower.c - Cartridge handling, Atomic Power cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "atomicpower.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "log.h"
#include "monitor.h"
#include "ram.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"
#include "vicii-phi1.h"

/* #define DEBUGAP */

#ifdef DEBUGAP
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    Atomic Power/Nordic Power

    - the hardware is very similar to Action Replay 5, with one exception

    32K rom, 4*8k pages
    8K ram

    io1 (writes)

    7    extra ROM bank selector (A15) (unused)
    6    1 = resets FREEZE-mode (turns back to normal mode)
    5    1 = enable RAM at ROML ($8000-$9FFF) &
            I/O2 ($DF00-$DFFF = $9F00-$9FFF)
    4    ROM bank selector high (A14)
    3    ROM bank selector low  (A13)
    2    1 = disable cartridge (turn off $DE00)
    1    1 = /EXROM high
    0    1 = /GAME low

    different to original AR:

    if bit 5 (RAM enable) is 1,
       bit 0,1 (exrom/game) is == 2 (cart off),
       bit 2,6,7 (cart disable, freeze clear) are 0,

    then Cart ROM (Bank 0..3) is mapped at 8000-9fff,
     and Cart RAM (Bank 0) is mapped at a000-bfff
     and Cart RAM (Bank 0) is enabled in io2 area
     using 16K Game config

    io2 (r/w)
        cart RAM (if enabled) or cart ROM
*/

#define CART_RAM_SIZE (8 * 1024)

/* Atomic Power RAM hack. */
static int export_ram_at_a000 = 0;
static int ap_active;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t atomicpower_io1_read(uint16_t addr);
static void atomicpower_io1_store(uint16_t addr, uint8_t value);
static uint8_t atomicpower_io2_read(uint16_t addr);
static void atomicpower_io2_store(uint16_t addr, uint8_t value);
static int atomicpower_dump(void);

static io_source_t atomicpower_io1_device = {
    CARTRIDGE_NAME_ATOMIC_POWER, /* name of the device */
    IO_DETACH_CART,              /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,       /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,        /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                           /* read is never valid, there is no read or peek function */
    atomicpower_io1_store,       /* store function */
    NULL,                        /* NO poke function */
    atomicpower_io1_read,        /* read function */
    NULL,                        /* TODO: peek function */
    atomicpower_dump,            /* device state information dump function */
    CARTRIDGE_ATOMIC_POWER,      /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0,                           /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE               /* NO mirroring */
};

static io_source_t atomicpower_io2_device = {
    CARTRIDGE_NAME_ATOMIC_POWER, /* name of the device */
    IO_DETACH_CART,              /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,       /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,        /* range of the device */
    0,                           /* read validity is determined by the device upon a read */
    atomicpower_io2_store,       /* store function */
    NULL,                        /* NO poke function */
    atomicpower_io2_read,        /* read function */
    NULL,                        /* TODO: peek function */
    atomicpower_dump,            /* device state information dump function */
    CARTRIDGE_ATOMIC_POWER,      /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0,                           /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE               /* NO mirroring */
};

static io_source_list_t *atomicpower_io1_list_item = NULL;
static io_source_list_t *atomicpower_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_ATOMIC_POWER, 1, 1, &atomicpower_io1_device, &atomicpower_io2_device, CARTRIDGE_ATOMIC_POWER
};

/* ---------------------------------------------------------------------*/

static uint8_t atomicpower_control_reg = 0;

static void atomicpower_io1_store(uint16_t addr, uint8_t value)
{
    int flags = CMODE_WRITE, bank, mode;

    if (ap_active) {
        atomicpower_control_reg = value;
        bank = ((value >> 3) & 3);
        mode = (value & 3);
        DBG(("io1 w %02x mode %d bank %d (np special: %s)\n", value, mode, bank, ((value & 0xe7) == 0x22) ? "yes" : "no"));

        if ((value & 0xe7) == 0x22) {
            mode = 1; /* 16k Game */
            export_ram_at_a000 = 1; /* RAM at a000 enabled */
        } else {
            /* Action Replay 5 compatible values */
            export_ram_at_a000 = 0;
            if (value & 0x40) {
                flags |= CMODE_RELEASE_FREEZE;
            }
            if (value & 0x20) {
                flags |= CMODE_EXPORT_RAM;
            }
        }
        if (value & 4) {
            ap_active = 0;
        }

        cart_config_changed_slotmain((uint8_t) 2, (uint8_t) (mode | (bank << CMODE_BANK_SHIFT)), flags);
    }
}

static uint8_t atomicpower_io1_read(uint16_t addr)
{
    uint8_t value;
    /* the read is really never valid */
    atomicpower_io1_device.io_source_valid = 0;
    if (!ap_active) {
        return 0;
    }
    /* since the r/w line is not decoded, a read still changes the register,
       to whatever was on the bus before */
    value = vicii_read_phi1();
    atomicpower_io1_store(addr, value);
    log_warning(LOG_DEFAULT, "AP: reading IO1 area at 0xde%02x, this corrupts the register",
                addr & 0xffu);

    return value;
}

static uint8_t atomicpower_io2_read(uint16_t addr)
{
    atomicpower_io2_device.io_source_valid = 0;

    if (!ap_active) {
        return 0;
    }

    atomicpower_io2_device.io_source_valid = 1;

    if (export_ram || export_ram_at_a000) {
        return export_ram0[0x1f00 + (addr & 0xff)];
    }

    addr |= 0xdf00;

    switch (roml_bank) {
        case 0:
            return roml_banks[addr & 0x1fff];
        case 1:
            return roml_banks[(addr & 0x1fff) + 0x2000];
        case 2:
            return roml_banks[(addr & 0x1fff) + 0x4000];
        case 3:
            return roml_banks[(addr & 0x1fff) + 0x6000];
    }
    atomicpower_io2_device.io_source_valid = 0;
    return 0;
}

static void atomicpower_io2_store(uint16_t addr, uint8_t value)
{
    if (ap_active) {
        if (export_ram || export_ram_at_a000) {
            export_ram0[0x1f00 + (addr & 0xff)] = value;
        }
    }
}

static int atomicpower_dump(void)
{
    mon_out("EXROM line: %s, GAME line: %s, Mode: %s\n",
            (atomicpower_control_reg & 2) ? "high" : "low",
            (atomicpower_control_reg & 1) ? "low" : "high",
            cart_config_string((uint8_t)(atomicpower_control_reg & 3)));
    mon_out("ROM bank: %d, cart state: %s, reset freeze: %s\n",
            (atomicpower_control_reg & 0x18) >> 3,
            (atomicpower_control_reg & 4) ? "disabled" : "enabled",
            (atomicpower_control_reg & 0x40) ? "yes" : "no");
    /* FIXME: take system RAM and cart mode(s) into account here */
    mon_out("$8000-$9FFF: %s\n", (export_ram) ? "RAM" : "ROM");
    mon_out("$A000-$BFFF: %s\n", (export_ram_at_a000) ? "RAM" : "ROM");
    mon_out("$DF00-$DFFF: %s\n", (export_ram || export_ram_at_a000) ? "RAM" : "ROM");
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t atomicpower_roml_read(uint16_t addr)
{
    if (export_ram) {
        return export_ram0[addr & 0x1fff];
    }

    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

void atomicpower_roml_store(uint16_t addr, uint8_t value)
{
    if (export_ram) {
        export_ram0[addr & 0x1fff] = value;
    }
}

uint8_t atomicpower_romh_read(uint16_t addr)
{
    if (export_ram_at_a000) {
        return export_ram0[addr & 0x1fff];
    }
    return romh_banks[(addr & 0x1fff) + (romh_bank << 13)];
}

void atomicpower_romh_store(uint16_t addr, uint8_t value)
{
    if (export_ram_at_a000) {
        export_ram0[addr & 0x1fff] = value;
    } else {
        mem_store_without_romlh(addr, value);
    }
}

void atomicpower_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
/* FIXME: this is broken, code-in-ram execution from AR "acid test" fails */
#if 0
    switch (addr & 0xe000) {
        case 0x8000:
            if (export_ram) {
                *base = export_ram0;
            } else {
                *base = roml_banks + (roml_bank << 13) - 0x8000;
            }
            *start = 0x8000;
            *limit = 0x9ffd;
            return;
        case 0xa000:
            if (export_ram_at_a000) {
                *base = export_ram0;
            } else {
                *base = romh_banks + (romh_bank << 13) - 0xa000;
            }
            *start = 0xa000;
            *limit = 0xbffd;
            return;
        case 0xe000:
            if (export_ram_at_a000) {
                *base = export_ram0;
            } else {
                *base = romh_banks + (romh_bank << 13) - 0xe000;
            }
            *start = 0xe000;
            *limit = 0xfffd;
            return;
        default:
            break;
    }
#endif
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

/* FIXME: this still needs to be tweaked to match the hardware */
static RAMINITPARAM ramparam = {
    .start_value = 255,
    .value_invert = 2,
    .value_offset = 1,

    .pattern_invert = 0x100,
    .pattern_invert_value = 255,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

void atomicpower_powerup(void)
{
    ram_init_with_pattern(export_ram0, CART_RAM_SIZE, &ramparam);
}

void atomicpower_freeze(void)
{
    ap_active = 1;
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ | CMODE_EXPORT_RAM);
}

void atomicpower_config_init(void)
{
    ap_active = 1;
    atomicpower_control_reg = 0;
    export_ram_at_a000 = 0;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void atomicpower_reset(void)
{
    ap_active = 1;
    atomicpower_control_reg = 0;
}

void atomicpower_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x8000);
    memcpy(romh_banks, rawcart, 0x8000);
    cart_config_changed_slotmain(0, 0, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int atomicpower_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    atomicpower_io1_list_item = io_source_register(&atomicpower_io1_device);
    atomicpower_io2_list_item = io_source_register(&atomicpower_io2_device);

    return 0;
}

int atomicpower_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x8000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return atomicpower_common_attach();
}

int atomicpower_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 3; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.bank > 3 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return atomicpower_common_attach();
}

void atomicpower_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(atomicpower_io1_list_item);
    io_source_unregister(atomicpower_io2_list_item);
    atomicpower_io1_list_item = NULL;
    atomicpower_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTAP snapshot module format:

   type  | name        | description
   ---------------------------------
   BYTE  | active      | cartridge active flag
   BYTE  | ram at a000 | RAM at $A000 flag
   ARRAY | ROML        | 32768 BYTES of ROML data
   ARRAY | RAM         | 8192 BYTES of RAM data
 */

static const char snap_module_name[] = "CARTAP";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int atomicpower_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)ap_active) < 0)
        || (SMW_B(m, (uint8_t)export_ram_at_a000) < 0)
        || (SMW_BA(m, roml_banks, 0x8000) < 0)
        || (SMW_BA(m, export_ram0, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int atomicpower_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &ap_active) < 0)
        || (SMR_B_INT(m, &export_ram_at_a000) < 0)
        || (SMR_BA(m, roml_banks, 0x8000) < 0)
        || (SMR_BA(m, export_ram0, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    memcpy(romh_banks, roml_banks, 0x8000);

    return atomicpower_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
