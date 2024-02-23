/*
 * supersnapshot.c - Cartridge handling, Super Snapshot cart.
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
#include "cmdline.h"
#include "export.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "supersnapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    Super Snapshot v5

    - 64K ROM,8*8K Banks (4*16k)
    - 32K RAM,4*8K Banks (8k stock, 32k optional)

    note: apparently the hardware supports 128k ROMs too, but no such dump exists.

    io1: (read)
        cart ROM mirror from current 9e00-9eff page. RAM can NOT be mirrored here!

    io1 (write)

    there is one register mirrored from de00-deff (the software uses de00/de01)

    bit 6-7  not connected
    bit 5    rom/ram bank bit2 (address line 16) (unused, for 128k ROM)
    bit 4    rom/ram bank bit1 (address line 15)
    bit 3    !rom enable (0: enabled, 1: disabled)
             note: disabling ROM also disables this register
    bit 2    rom/ram bank bit0 (address line 14)
    bit 1    !ram enable (0: enabled, 1: disabled), !EXROM (0: high, 1: low)
    bit 0    GAME (0: low, 1: high)
*/

#define CART_RAM_SIZE (32 * 1024)

/* Super Snapshot configuration flags.  */
static uint8_t romconfig = 9;
static int ram_bank = 0; /* Version 5 supports 4 - 8Kb RAM banks. */
static int currbank = 0;
static int currreg  = 0;
static int ss_32k_enabled = 0;
static int ss_rom_disabled = 0;
static int ss_rom_banks = 4;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t supersnapshot_v5_io1_read(uint16_t addr);
static uint8_t supersnapshot_v5_io1_peek(uint16_t addr);
static void supersnapshot_v5_io1_store(uint16_t addr, uint8_t value);
static int supersnapshot_v5_dump(void);

static io_source_t ss5_device = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5, /* name of the device */
    IO_DETACH_CART,                   /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,            /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,             /* range for the device, regs:$de00-$deff */
    0,                                /* read validity is determined by the device upon a read */
    supersnapshot_v5_io1_store,       /* store function */
    NULL,                             /* NO poke function */
    supersnapshot_v5_io1_read,        /* read function */
    supersnapshot_v5_io1_peek,        /* peek function */
    supersnapshot_v5_dump,            /* device state information dump function */
    CARTRIDGE_SUPER_SNAPSHOT_V5,      /* cartridge ID */
    IO_PRIO_NORMAL,                   /* normal priority, device read needs to be checked for collisions */
    0,                                /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                    /* NO mirroring */
};

static io_source_list_t *ss5_list_item = NULL;

static const export_resource_t export_res_v5 = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5, 1, 1, &ss5_device, NULL, CARTRIDGE_SUPER_SNAPSHOT_V5
};

/* ---------------------------------------------------------------------*/

static uint8_t supersnapshot_v5_io1_read(uint16_t addr)
{
    ss5_device.io_source_valid = 1;

    if (!ss_rom_disabled) {
        return roml_banks[0x1e00 + (addr & 0xff) + (roml_bank << 13)];
    }

    ss5_device.io_source_valid = 0;

    return 0;
}

static uint8_t supersnapshot_v5_io1_peek(uint16_t addr)
{
    return roml_banks[0x1e00 + (addr & 0xff) + (roml_bank << 13)];
}

static void supersnapshot_v5_io1_store(uint16_t addr, uint8_t value)
{
    if (!ss_rom_disabled) {
        int mode = CMODE_WRITE;

        currreg = value & 0x3f;

        /* FIXME: is this correct? */
        if ((value & 1) == 1) {
            mode |= CMODE_RELEASE_FREEZE;
        }

        romconfig = ((value & 1) ^ 1) | ((value & 2) ^ 2);
        currbank = ((value >> 2) & 0x1) | (((value >> 4) & 0x1) << 1);
        ram_bank = ss_32k_enabled ? currbank : 0; /* Select RAM banknr. */
        if (ss_rom_banks == 8) {
            currbank |= (((value >> 5) & 0x1) << 2);
        }
        ss_rom_disabled = ((value >> 3) & 0x1);
        romconfig |= (currbank << CMODE_BANK_SHIFT);
        if (((value >> 1) & 1) == 0) {
            mode |= CMODE_EXPORT_RAM;                 /* export_ram */
        }
        cart_config_changed_slotmain(romconfig, romconfig, mode);
    }
}

static int supersnapshot_v5_dump(void)
{
    mon_out("Register: $%02x (%s)\n", (unsigned int)currreg, (ss_rom_disabled) ? "disabled" : "enabled");
    mon_out(" EXROM: %d GAME: %d (%s)\n", ((romconfig >> 1) & 1), (romconfig & 1) ^ 1, cart_config_string((uint8_t)(romconfig & 3)));
    mon_out(" ROM %s, Bank: %d of %d\n", (ss_rom_disabled) ? "disabled" : "enabled", currbank, ss_rom_banks);
    mon_out(" RAM %s, Bank: %d of %d\n", (export_ram) ? "enabled" : "disabled", ram_bank, ss_32k_enabled ? 4 : 1);
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t supersnapshot_v5_roml_read(uint16_t addr)
{
    if (export_ram) {
        return export_ram0[(addr & 0x1fff) + (ram_bank << 13)];
    }

    if (!ss_rom_disabled) {
        return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
    }
    return 0; /* FIXME: open bus? */
}

void supersnapshot_v5_roml_store(uint16_t addr, uint8_t value)
{
    if (export_ram) {
        export_ram0[(addr & 0x1fff) + (ram_bank << 13)] = value;
    }
}

void supersnapshot_v5_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    switch (addr & 0xe000) {
        case 0x8000:
            if (export_ram) {
                *base = export_ram0 + (ram_bank << 13) - 0x8000;
            } else {
                *base = roml_banks + (roml_bank << 13) - 0x8000;
            }
            *start = 0x8000;
            *limit = 0x9ffd;
            return;
        default:
            break;
    }
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

void supersnapshot_v5_powerup(void)
{
    ram_init_with_pattern(export_ram0, CART_RAM_SIZE, &ramparam);
}

void supersnapshot_v5_freeze(void)
{
    ss_rom_disabled = 0; /* enable the register */
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ | CMODE_EXPORT_RAM);
}

void supersnapshot_v5_config_init(void)
{
    ss_rom_disabled = 0; /* enable the register */
    supersnapshot_v5_io1_store((uint16_t)0xde00, 0);    /* start up in ultimax mode! */
}

void supersnapshot_v5_config_setup(uint8_t *rawcart)
{
    int i;
    for (i = 0; i < 8; i++) {
        memcpy(&roml_banks[0x2000 * i], &rawcart[0x0000 + (0x4000 * i)], 0x2000);
        memcpy(&romh_banks[0x2000 * i], &rawcart[0x2000 + (0x4000 * i)], 0x2000);
    }
    supersnapshot_v5_io1_store((uint16_t)0xde00, 2);
}

/* ---------------------------------------------------------------------*/

static int supersnapshot_v5_common_attach(void)
{
    if (export_add(&export_res_v5) < 0) {
        return -1;
    }

    ss5_list_item = io_source_register(&ss5_device);

    return 0;
}

int supersnapshot_v5_bin_attach(const char *filename, uint8_t *rawcart)
{
    ss_rom_banks = 4;
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, 0x20000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
        ss_rom_banks = 8;
    }

    return supersnapshot_v5_common_attach();
}

int supersnapshot_v5_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    ss_rom_banks = 4;
    for (i = 0; i < 8; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 7) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }

    if (!((i == 4) || (i == 8))) {
        return -1;
    }
    ss_rom_banks = i;

    return supersnapshot_v5_common_attach();
}

void supersnapshot_v5_detach(void)
{
    export_remove(&export_res_v5);
    io_source_unregister(ss5_list_item);
    ss5_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

static int set_32k_enabled(int val, void *param)
{
    ss_32k_enabled = val ? 1 : 0;
    /* the ROM code stores in RAM if the RAM expansion is available, so clear
       it here to make sure it checks again */
    memset(export_ram0, 0, 0x8000);
    return 0;
}

static const resource_int_t resources_int[] = {
    { "SSRamExpansion", 0, RES_EVENT_NO, NULL,
      &ss_32k_enabled, set_32k_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int supersnapshot_v5_resources_init(void)
{
    return resources_register_int(resources_int);
}

void supersnapshot_v5_resources_shutdown(void)
{
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-ssramexpansion", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SSRamExpansion", (resource_value_t)1,
      NULL, "Enable SS 32KiB RAM expansion" },
    { "+ssramexpansion", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SSRamExpansion", (resource_value_t)0,
      NULL, "Disable SS 32KiB RAM expansion" },
    CMDLINE_LIST_END
};

int supersnapshot_v5_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
/* ---------------------------------------------------------------------*/

/* CARTSS5 snapshot module format:

   type  | name        | version | description
   -------------------------------------------
   BYTE  | ROM bank    |   0.2   | current ROM bank
   BYTE  | register    |   0.2   | register
   BYTE  | ROM config  |   0.0+  | ROM configuration
   BYTE  | RAM bank    |   0.0+  | current RAM bank
   BYTE  | 32K enabled |   0.1+  | 32KB enabled flag
   BYTE  | ROM disable |   0.1+  | ROM disable flag
   BYTE  | ROM banks   |   0.3   | number of ROM banks (4 or 8)
   ARRAY | ROML        |   0.0+  | 0x8000 or 0x10000 BYTES of ROML data
   ARRAY | ROMH        |   0.0+  | 0x8000 or 0x10000 BYTES of ROMH data
   ARRAY | RAM         |   0.0+  | 32768 BYTES of RAM data
 */

static const char snap_module_name[] = "CARTSS5";
#define SNAP_MAJOR   0
#define SNAP_MINOR   3

int supersnapshot_v5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;
    unsigned int rom_bank_size = ss_rom_banks * 0x2000;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)currbank) < 0
        || SMW_B(m, (uint8_t)currreg) < 0
        || SMW_B(m, romconfig) < 0
        || SMW_B(m, (uint8_t)ram_bank) < 0
        || SMW_B(m, (uint8_t)ss_32k_enabled) < 0
        || SMW_B(m, (uint8_t)ss_rom_disabled) < 0
        || SMW_B(m, (uint8_t)ss_rom_banks) < 0
        || SMW_BA(m, roml_banks, rom_bank_size) < 0
        || SMW_BA(m, romh_banks, rom_bank_size) < 0
        || SMW_BA(m, export_ram0, 0x8000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int supersnapshot_v5_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    unsigned int rom_bank_size;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.2 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 2)) {
        if (0
            || SMR_B_INT(m, &currbank) < 0
            || SMR_B_INT(m, &currreg) < 0) {
            goto fail;
        }
    } else {
        currbank = 0;
        currreg = 0;
    }

    if (0
        || SMR_B(m, &romconfig) < 0
        || SMR_B_INT(m, &ram_bank) < 0) {
        goto fail;
    }

    /* new in 0.1 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 1)) {
        if (0
            || SMR_B_INT(m, &ss_32k_enabled) < 0
            || SMR_B_INT(m, &ss_rom_disabled) < 0) {
            goto fail;
        }
    } else {
        ss_32k_enabled = 0;
        ss_rom_disabled = 0;
    }

    /* new in 0.3 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 3)) {
        if (0
            || SMR_B_INT(m, &ss_rom_banks) < 0) {
            goto fail;
        }
    } else {
        ss_rom_banks = 4;
    }

    rom_bank_size = ss_rom_banks * 0x2000;

    if (0
        || SMR_BA(m, roml_banks, rom_bank_size) < 0
        || SMR_BA(m, romh_banks, rom_bank_size) < 0
        || SMR_BA(m, export_ram0, 0x8000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return supersnapshot_v5_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
