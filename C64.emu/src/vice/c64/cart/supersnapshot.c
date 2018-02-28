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
#include "resources.h"
#include "snapshot.h"
#include "supersnapshot.h"
#include "translate.h"
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

/* Super Snapshot configuration flags.  */
static BYTE romconfig = 9;
static int ram_bank = 0; /* Version 5 supports 4 - 8Kb RAM banks. */
static int currbank = 0;
static int currreg  = 0;
static int ss_32k_enabled = 0;
static int ss_rom_disabled = 0;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE supersnapshot_v5_io1_read(WORD addr);
static BYTE supersnapshot_v5_io1_peek(WORD addr);
static void supersnapshot_v5_io1_store(WORD addr, BYTE value);
static int supersnapshot_v5_dump(void);

static io_source_t ss5_device = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    supersnapshot_v5_io1_store,
    supersnapshot_v5_io1_read,
    supersnapshot_v5_io1_peek,
    supersnapshot_v5_dump,
    CARTRIDGE_SUPER_SNAPSHOT_V5,
    0,
    0
};

static io_source_list_t *ss5_list_item = NULL;

static const export_resource_t export_res_v5 = {
    CARTRIDGE_NAME_SUPER_SNAPSHOT_V5, 1, 1, &ss5_device, NULL, CARTRIDGE_SUPER_SNAPSHOT_V5
};

/* ---------------------------------------------------------------------*/

static BYTE supersnapshot_v5_io1_read(WORD addr)
{
    ss5_device.io_source_valid = 1;

    if (!ss_rom_disabled) {
        switch (roml_bank) {
            case 0:
                return roml_banks[0x1e00 + (addr & 0xff)];
            case 1:
                return roml_banks[0x1e00 + (addr & 0xff) + 0x2000];
            case 2:
                return roml_banks[0x1e00 + (addr & 0xff) + 0x4000];
            case 3:
                return roml_banks[0x1e00 + (addr & 0xff) + 0x6000];
        }
    }

    ss5_device.io_source_valid = 0;

    return 0;
}

static BYTE supersnapshot_v5_io1_peek(WORD addr)
{
    switch (roml_bank) {
        case 0:
            return roml_banks[0x1e00 + (addr & 0xff)];
        case 1:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x2000];
        case 2:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x4000];
        case 3:
            return roml_banks[0x1e00 + (addr & 0xff) + 0x6000];
    }
    return 0;
}

static void supersnapshot_v5_io1_store(WORD addr, BYTE value)
{
    if (!ss_rom_disabled) {
        int mode = CMODE_WRITE;

        currreg = value & 0x3f;

        /* FIXME: is this correct? */
        if ((value & 1) == 1) {
            mode |= CMODE_RELEASE_FREEZE;
        }

        romconfig = ((value & 1) ^ 1) | ((value & 2) ^ 2);
        currbank = ((value >> 2) & 0x1) | (((value >> 4) & 0x1) << 1) /* | (((value >> 5) & 0x1) << 2)*/;
        ram_bank = ss_32k_enabled ? currbank : 0; /* Select RAM banknr. */
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
    mon_out("Register: $%02x (%s)\n", currreg, (ss_rom_disabled) ? "disabled" : "enabled");
    mon_out(" EXROM: %d GAME: %d (%s)\n", ((romconfig >> 1) & 1), (romconfig & 1) ^ 1, cart_config_string((BYTE)(romconfig & 3)));
    mon_out(" ROM %s, Bank: %d\n", (ss_rom_disabled) ? "disabled" : "enabled", currbank);
    mon_out(" RAM %s, Bank: %d\n", (export_ram) ? "enabled" : "disabled", ram_bank);
    return 0;
}

/* ---------------------------------------------------------------------*/

BYTE supersnapshot_v5_roml_read(WORD addr)
{
    if (export_ram) {
        return export_ram0[(addr & 0x1fff) + (ram_bank << 13)];
    }

    if (!ss_rom_disabled) {
        return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
    }
    return 0; /* FIXME: open bus? */
}

void supersnapshot_v5_roml_store(WORD addr, BYTE value)
{
    if (export_ram) {
        export_ram0[(addr & 0x1fff) + (ram_bank << 13)] = value;
    }
}

void supersnapshot_v5_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
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

void supersnapshot_v5_freeze(void)
{
    ss_rom_disabled = 0; /* enable the register */
    cart_config_changed_slotmain(CMODE_ULTIMAX, CMODE_ULTIMAX, CMODE_READ | CMODE_EXPORT_RAM);
}

void supersnapshot_v5_config_init(void)
{
    ss_rom_disabled = 0; /* enable the register */
    supersnapshot_v5_io1_store((WORD)0xde00, 2);
}

void supersnapshot_v5_config_setup(BYTE *rawcart)
{
    memcpy(&roml_banks[0x0000], &rawcart[0x0000], 0x2000);
    memcpy(&romh_banks[0x0000], &rawcart[0x2000], 0x2000);
    memcpy(&roml_banks[0x2000], &rawcart[0x4000], 0x2000);
    memcpy(&romh_banks[0x2000], &rawcart[0x6000], 0x2000);
    memcpy(&roml_banks[0x4000], &rawcart[0x8000], 0x2000);
    memcpy(&romh_banks[0x4000], &rawcart[0xa000], 0x2000);
    memcpy(&roml_banks[0x6000], &rawcart[0xc000], 0x2000);
    memcpy(&romh_banks[0x6000], &rawcart[0xe000], 0x2000);
    supersnapshot_v5_io1_store((WORD)0xde00, 2);
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

int supersnapshot_v5_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x10000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return supersnapshot_v5_common_attach();
}

int supersnapshot_v5_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 4; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000 || chip.bank > 3) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }

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
    { "-ssramexpansion", SET_RESOURCE, 0,
      NULL, NULL, "SSRamExpansion", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SS_RAM_EXPANSION,
      NULL, NULL },
    { "+ssramexpansion", SET_RESOURCE, 0,
      NULL, NULL, "SSRamExpansion", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SS_RAM_EXPANSION,
      NULL, NULL },
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
   ARRAY | ROML        |   0.0+  | 32768 BYTES of ROML data
   ARRAY | ROMH        |   0.0+  | 32768 BYTES of ROMH data
   ARRAY | RAM         |   0.0+  | 32768 BYTES of RAM data
 */

static char snap_module_name[] = "CARTSS5";
#define SNAP_MAJOR   0
#define SNAP_MINOR   2

int supersnapshot_v5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)currbank) < 0
        || SMW_B(m, (BYTE)currreg) < 0
        || SMW_B(m, romconfig) < 0
        || SMW_B(m, (BYTE)ram_bank) < 0
        || SMW_B(m, (BYTE)ss_32k_enabled) < 0
        || SMW_B(m, (BYTE)ss_rom_disabled) < 0
        || SMW_BA(m, roml_banks, 0x8000) < 0
        || SMW_BA(m, romh_banks, 0x8000) < 0
        || SMW_BA(m, export_ram0, 0x8000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int supersnapshot_v5_snapshot_read_module(snapshot_t *s)
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

    /* new in 0.2 */
    if (SNAPVAL(vmajor, vminor, 0, 2)) {
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
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (0
            || SMR_B_INT(m, &ss_32k_enabled) < 0
            || SMR_B_INT(m, &ss_rom_disabled) < 0) {
            goto fail;
        }
    } else {
        ss_32k_enabled = 0;
        ss_rom_disabled = 0;
    }

    if (0
        || SMR_BA(m, roml_banks, 0x8000) < 0
        || SMR_BA(m, romh_banks, 0x8000) < 0
        || SMR_BA(m, export_ram0, 0x8000) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return supersnapshot_v5_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
