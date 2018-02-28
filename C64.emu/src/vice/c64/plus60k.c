/*
 * plus60k.c - PLUS60K EXPANSION HACK emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

/* Introduction :
 *
 * +60K is a kind of memory expansion for C64 proposed in late '90s by Polish
 * sceners, for sceners. Basically the whole idea was to add another bank of
 * memory and provide a shared area to exchange data between the banks.
 *
 *
 * Hardware :
 *
 * +60K circuit is somewhat complicated because quite a few new ICs have to mounted
 * inside a C64 but it is not very hard to build. I will not get into details and
 * schematics because it was described quite well in disk magazines.
 *
 *
 * Software :
 *
 * - VIC address space is divided into 4 parts: $d000-$d0ff, $d100-$d1ff, $d200-$d2ff
 *   and $d300-$d3ff
 * - only $d000-$d0ff is still visible in I/O space as VIC
 * - $d100-$d1ff returns $ff on read
 * - $d200-$d3ff is unconnected and returns random values
 * - register latch for +60K is active in all $d100-$d1ff space, but programs should
 *   use $d100 only
 * - only data bit 7 is connected to the latch, but programs should use 0 as bits 0-6
 * - VIC fetches data only from bank 0 RAM (onboard)

 * +60K is controlled by a write-only register at $d100. There are only two possible
 * values that can be written there:

 * value     | $1000-$ffff RAM area
 * ---------------------------------------------
 * %0xxxxxxx | comes from onboard RAM (bank 0)
 * %1xxxxxxx | comes from additional RAM (bank 1)

 * x - reserved bit, it seems that all existing +60K-enabled programs use 0 here

 * RAM/ROM/IO is still controlled as usual by $0001. The only thing that changes is
 * where $1000-$ffff RAM comes from. The $0000-$0fff is the shared space and always
 * comes from onboard RAM.
 * It is important to say that VIC cannot see additional RAM. It still fetches data
 * from onboard RAM thus it is possible to keep gfx data in bank 0 and code with
 * sound data in bank 1.
 * The $d100 control register returns $ff on read. Although such usage is forbidden
 * I've seen at least one example of switching to bank 0 by "INC $d100" instruction
 * so it is emulated too.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c64cart.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "plus60k.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vicii.h"
#include "vicii-mem.h"

/* PLUS60K registers */
static BYTE plus60k_reg = 0;

static log_t plus60k_log = LOG_ERR;

static int plus60k_activate(void);
static int plus60k_deactivate(void);

int plus60k_enabled = 0;

int plus60k_base = 0xd100;

/* Filename of the +60K image.  */
static char *plus60k_filename = NULL;

static BYTE *plus60k_ram;

static int plus60k_dump(void)
{
    mon_out("$1000-$FFFF bank: %d\n", plus60k_reg);
    return 0;
}

static BYTE plus60k_ff_read(WORD addr)
{
    return 0xff;
}

static BYTE plus60k_peek(WORD addr)
{
    return plus60k_reg << 7;
}

static void plus60k_vicii_store(WORD addr, BYTE value)
{
    plus60k_reg = (value & 0x80) >> 7;
}

static io_source_t vicii_d000_device = {
    "VIC-II",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd000, 0xd03f, 0x3f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d000_full_device = {
    "VIC-II",
    IO_DETACH_CART, /* dummy */
    NULL,           /* dummy */
    0xd000, 0xd0ff, 0x3f,
    1, /* read is always valid */
    vicii_store,
    vicii_read,
    vicii_peek,
    vicii_dump,
    0, /* dummy (not a cartridge) */
    IO_PRIO_HIGH, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d040_device = {
    "+60K",
    IO_DETACH_RESOURCE,
    "PLUS60K",
    0xd040, 0xd0ff, 1,
    1, /* read is always valid */
    plus60k_vicii_store,
    plus60k_ff_read,
    plus60k_peek,
    plus60k_dump,
    CARTRIDGE_PLUS60K,
    IO_PRIO_NORMAL,
    0
};

static io_source_t vicii_d100_device = {
    "+60K",
    IO_DETACH_RESOURCE,
    "PLUS60K",
    0xd100, 0xd1ff, 1,
    1, /* read is always valid */
    plus60k_vicii_store,
    plus60k_ff_read,
    plus60k_peek,
    plus60k_dump,
    CARTRIDGE_PLUS60K,
    IO_PRIO_NORMAL,
    0
};

static io_source_list_t *vicii_d000_list_item = NULL;
static io_source_list_t *vicii_d000_full_list_item = NULL;
static io_source_list_t *vicii_d040_list_item = NULL;
static io_source_list_t *vicii_d100_list_item = NULL;

int set_plus60k_enabled(int value, int disable_reset)
{
    int val = value ? 1 : 0;

    if (val == plus60k_enabled) {
        return 0;
    }

    if (!val) {
        if (plus60k_deactivate() < 0) {
            return -1;
        }

        if (!disable_reset) {
            machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        }
        plus60k_enabled = 0;
        return 0;
    } else {
        if (plus60k_activate() < 0) {
            return -1;
        }
        plus60k_enabled = 1;
        if (!disable_reset) {
            machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        }
        return 0;
    }
}

static int set_plus60k_filename(const char *name, void *param)
{
    if (plus60k_filename != NULL && name != NULL && strcmp(name, plus60k_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (plus60k_enabled) {
        plus60k_deactivate();
        util_string_set(&plus60k_filename, name);
        plus60k_activate();
    } else {
        util_string_set(&plus60k_filename, name);
    }

    return 0;
}

static int set_plus60k_base(int val, void *param)
{
    if (val == plus60k_base) {
        return 0;
    }

    switch (val) {
        case 0xd040:
        case 0xd100:
            break;
        default:
            log_message(plus60k_log, "Unknown PLUS60K base address $%X.", val);
            return -1;
    }

    if (plus60k_enabled) {
        plus60k_deactivate();
        plus60k_base = val;
        plus60k_activate();
    } else {
        plus60k_base = val;
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "PLUS60Kfilename", "", RES_EVENT_NO, NULL,
      &plus60k_filename, set_plus60k_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "PLUS60Kbase", 0xd100, RES_EVENT_NO, NULL,
      &plus60k_base, set_plus60k_base, NULL },
    RESOURCE_INT_LIST_END
};

int plus60k_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void plus60k_resources_shutdown(void)
{
    lib_free(plus60k_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-plus60kimage", SET_RESOURCE, 1,
      NULL, NULL, "PLUS60Kfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PLUS60K_NAME,
      NULL, NULL },
    { "-plus60kbase", SET_RESOURCE, 1,
      NULL, NULL, "PLUS60Kbase", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_BASE_ADDRESS, IDCLS_PLUS60K_BASE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int plus60k_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void plus60k_init(void)
{
    plus60k_log = log_open("PLUS60");
}

void plus60k_reset(void)
{
    plus60k_reg = 0;
}

static int plus60k_activate(void)
{
    plus60k_ram = lib_realloc((void *)plus60k_ram, (size_t)0xf000);

    log_message(plus60k_log, "PLUS60K expansion installed.");

    if (!util_check_null_string(plus60k_filename)) {
        if (util_file_load(plus60k_filename, plus60k_ram, (size_t)0xf000, UTIL_FILE_LOAD_RAW) < 0) {
            log_message(plus60k_log, "Reading PLUS60K image %s failed.", plus60k_filename);
            if (util_file_save(plus60k_filename, plus60k_ram, 0xf000) < 0) {
                log_message(plus60k_log, "Creating PLUS60K image %s failed.", plus60k_filename);
                return -1;
            }
            log_message(plus60k_log, "Creating PLUS60K image %s.", plus60k_filename);
        } else {
            log_message(plus60k_log, "Reading PLUS60K image %s.", plus60k_filename);
        }
    }

    plus60k_reset();

    c64io_vicii_deinit();
    if (plus60k_base == 0xd100) {
        vicii_d000_full_list_item = io_source_register(&vicii_d000_full_device);
        vicii_d100_list_item = io_source_register(&vicii_d100_device);
    } else {
        vicii_d000_list_item = io_source_register(&vicii_d000_device);
        vicii_d040_list_item = io_source_register(&vicii_d040_device);
    }
    return 0;
}

static int plus60k_deactivate(void)
{
    if (!util_check_null_string(plus60k_filename)) {
        if (util_file_save(plus60k_filename, plus60k_ram, 0xf000) < 0) {
            log_message(plus60k_log, "Writing PLUS60K image %s failed.", plus60k_filename);
            return -1;
        }
        log_message(plus60k_log, "Writing PLUS60K image %s.", plus60k_filename);
    }
    lib_free(plus60k_ram);
    plus60k_ram = NULL;

    if (vicii_d000_list_item != NULL) {
        io_source_unregister(vicii_d000_list_item);
        vicii_d000_list_item = NULL;
    }

    if (vicii_d000_full_list_item != NULL) {
        io_source_unregister(vicii_d000_full_list_item);
        vicii_d000_full_list_item = NULL;
    }

    if (vicii_d040_list_item != NULL) {
        io_source_unregister(vicii_d040_list_item);
        vicii_d040_list_item = NULL;
    }

    if (vicii_d100_list_item != NULL) {
        io_source_unregister(vicii_d100_list_item);
        vicii_d100_list_item = NULL;
    }
    c64io_vicii_init();

    return 0;
}

void plus60k_shutdown(void)
{
    if (plus60k_enabled) {
        plus60k_deactivate();
    }
}

/* ------------------------------------------------------------------------- */

static void plus60k_memory_store(WORD addr, BYTE value)
{
    plus60k_ram[addr - 0x1000] = value;
}

static void vicii_mem_vbank_store_wrapper(WORD addr, BYTE value)
{
    vicii_mem_vbank_store(addr, value);
}

static void vicii_mem_vbank_39xx_store_wrapper(WORD addr, BYTE value)
{
    vicii_mem_vbank_39xx_store(addr, value);
}

static void vicii_mem_vbank_3fxx_store_wrapper(WORD addr, BYTE value)
{
    vicii_mem_vbank_3fxx_store(addr, value);
}

static void ram_hi_store_wrapper(WORD addr, BYTE value)
{
    ram_hi_store(addr, value);
}

static store_func_ptr_t plus60k_mem_write_tab[] = {
    vicii_mem_vbank_store_wrapper,
    plus60k_memory_store,
    vicii_mem_vbank_39xx_store_wrapper,
    plus60k_memory_store,
    vicii_mem_vbank_3fxx_store_wrapper,
    plus60k_memory_store,
    ram_hi_store_wrapper,
    plus60k_memory_store
};

void plus60k_vicii_mem_vbank_store(WORD addr, BYTE value)
{
    plus60k_mem_write_tab[plus60k_reg](addr, value);
}

void plus60k_vicii_mem_vbank_39xx_store(WORD addr, BYTE value)
{
    plus60k_mem_write_tab[plus60k_reg + 2](addr, value);
}

void plus60k_vicii_mem_vbank_3fxx_store(WORD addr, BYTE value)
{
    plus60k_mem_write_tab[plus60k_reg + 4](addr, value);
}

void plus60k_ram_hi_store(WORD addr, BYTE value)
{
    plus60k_mem_write_tab[plus60k_reg + 6](addr, value);
}

BYTE plus60k_ram_read(WORD addr)
{
    if (plus60k_enabled && addr >= 0x1000 && plus60k_reg == 1) {
        return plus60k_ram[addr - 0x1000];
    } else {
        return mem_ram[addr];
    }
}

void plus60k_ram_store(WORD addr, BYTE value)
{
    if (plus60k_enabled && addr >= 0x1000 && plus60k_reg == 1) {
        plus60k_ram[addr - 0x1000] = value;
    } else {
        mem_ram[addr] = value;
    }
}

/* ------------------------------------------------------------------------- */

/* PLUS60K snapshot module format:

   type  | name     | description
   --------------------------------------
   WORD  | base     | base address of register
   BYTE  | register | register
   ARRAY | RAM      | 61440 BYTES of RAM data

   Note: for some reason this snapshot module started at 0.1, so there never was a 0.0
 */

static char snap_module_name[] = "PLUS60K";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int plus60k_snapshot_write(struct snapshot_s *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_W (m, (WORD)plus60k_base) < 0
        || SMW_B (m, plus60k_reg) < 0
        || SMW_BA(m, plus60k_ram, 0xf000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int plus60k_snapshot_read(struct snapshot_s *s)
{
    snapshot_module_t *m;
    BYTE vmajor, vminor;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if ((vmajor != SNAP_MAJOR) || (vminor != SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_W_INT(m, &plus60k_base) < 0) {
        goto fail;
    }

    /* enable plus60k, without reset */
    set_plus60k_enabled(1, 1);

    if (0
        || SMR_B(m, &plus60k_reg) < 0
        || SMR_BA(m, plus60k_ram, 0xf000) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);

    /* disable plus60k, without reset */
    set_plus60k_enabled(0, 1);

    return -1;
}
