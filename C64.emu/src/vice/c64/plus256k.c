/*
 * plus256k.c - +256K EXPANSION emulation.
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
#include "plus256k.h"
#include "resources.h"
#include "reu.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vicii.h"
#include "vicii-mem.h"

/* PLUS256K registers */
static BYTE plus256k_reg = 0;

static log_t plus256k_log = LOG_ERR;

static int plus256k_activate(void);
static int plus256k_deactivate(void);

int plus256k_enabled = 0;

static int plus256k_video_bank = 0;
static int plus256k_low_bank = 0;
static int plus256k_high_bank = 0;
static int plus256k_protected = 0;

/* Filename of the +256K image.  */
static char *plus256k_filename = NULL;

BYTE *plus256k_ram = NULL;

/* ------------------------------------------------------------------------- */

static BYTE plus256k_peek(WORD addr)
{
    return plus256k_reg;
}

static BYTE plus256k_ff_read(WORD addr)
{
    return 0xff;
}

static int plus256k_dump(void)
{
    mon_out("$0000-$0FFF bank: %d\n", plus256k_low_bank);
    mon_out("$1000-$FFFF bank: %d\n", plus256k_high_bank);
    mon_out("VICII-bank : %d\n", plus256k_video_bank);
    mon_out("Register protection: %s\n", (plus256k_protected) ? "on" : "off");
    return 0;
}

static void plus256k_vicii_store(WORD addr, BYTE value)
{
    int new_bank;

    if (plus256k_protected == 0) {
        plus256k_reg = value;
        plus256k_high_bank = (value & 0xc0) >> 6;
        plus256k_low_bank = value & 3;
        plus256k_protected = (value & 0x10) >> 4;
        new_bank = (value & 0xc) >> 2;
        if (new_bank != plus256k_video_bank) {
            vicii_set_ram_base(plus256k_ram + (new_bank * 0x10000));
            plus256k_video_bank = new_bank;
        }
    }
}

static io_source_t vicii_d000_device = {
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
    1, /* priority, device and mirrors never involved in collisions */
    0
};

static io_source_t vicii_d100_device = {
    "+256K",
    IO_DETACH_RESOURCE,
    "PLUS256K",
    0xd100, 0xd1ff, 1,
    1, /* read is always valid */
    plus256k_vicii_store,
    plus256k_ff_read,
    plus256k_peek,
    plus256k_dump,
    CARTRIDGE_PLUS256K,
    0,
    0
};

static io_source_list_t *vicii_d000_list_item = NULL;
static io_source_list_t *vicii_d100_list_item = NULL;

int set_plus256k_enabled(int value, int disable_reset)
{
    int val = value ? 1 : 0;

    if (val == plus256k_enabled) {
        return 0;
    }

    if (!val) {
        if (plus256k_deactivate() < 0) {
            return -1;
        }
        if (!disable_reset) {
            machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        }
        plus256k_enabled = 0;
        return 0;
    } else {
        if (plus256k_activate() < 0) {
            return -1;
        }
        if (!disable_reset) {
            machine_trigger_reset(MACHINE_RESET_MODE_HARD);
        }
        plus256k_enabled = 1;
        return 0;
    }
}

static int set_plus256k_filename(const char *name, void *param)
{
    if (plus256k_filename != NULL && name != NULL && strcmp(name, plus256k_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (plus256k_enabled) {
        plus256k_deactivate();
        util_string_set(&plus256k_filename, name);
        plus256k_activate();
    } else {
        util_string_set(&plus256k_filename, name);
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "PLUS256Kfilename", "", RES_EVENT_NO, NULL,
      &plus256k_filename, set_plus256k_filename, NULL },
    RESOURCE_STRING_LIST_END
};

int plus256k_resources_init(void)
{
    return resources_register_string(resources_string);
}

void plus256k_resources_shutdown(void)
{
    lib_free(plus256k_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-plus256kimage", SET_RESOURCE, 1,
      NULL, NULL, "PLUS256Kfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_PLUS256K_NAME,
      NULL, NULL },
    CMDLINE_LIST_END
};

int plus256k_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void plus256k_init(void)
{
    plus256k_log = log_open("PLUS256K");
}

void plus256k_reset(void)
{
    plus256k_reg = 0;
    plus256k_video_bank = 0;
    plus256k_low_bank = 0;
    plus256k_high_bank = 0;
    plus256k_protected = 0;
    if (plus256k_enabled) {
        vicii_set_ram_base(plus256k_ram);
    }
}

static int plus256k_activate(void)
{
    plus256k_ram = lib_realloc((void *)plus256k_ram, (size_t)0x40000);

    log_message(plus256k_log, "PLUS256K hack installed.");

    if (!util_check_null_string(plus256k_filename)) {
        if (util_file_load(plus256k_filename, plus256k_ram, (size_t)0x40000, UTIL_FILE_LOAD_RAW) < 0) {
            log_message(plus256k_log, "Reading PLUS256K image %s failed.", plus256k_filename);
            if (util_file_save(plus256k_filename, plus256k_ram, 0x40000) < 0) {
                log_message(plus256k_log, "Creating PLUS256K image %s failed.", plus256k_filename);
                return -1;
            } else {
                log_message(plus256k_log, "Creating PLUS256K image %s.", plus256k_filename);
            }
        }
        log_message(plus256k_log, "Reading PLUS256K image %s.", plus256k_filename);
    }
    plus256k_reset();
    c64io_vicii_deinit();
    vicii_d000_list_item = io_source_register(&vicii_d000_device);
    vicii_d100_list_item = io_source_register(&vicii_d100_device);
    return 0;
}

static int plus256k_deactivate(void)
{
    if (!util_check_null_string(plus256k_filename)) {
        if (util_file_save(plus256k_filename, plus256k_ram, 0x40000) < 0) {
            log_message(plus256k_log, "Writing PLUS256K image %s failed.", plus256k_filename);
            return -1;
        }
        log_message(plus256k_log, "Writing PLUS256K image %s.", plus256k_filename);
    }
    vicii_set_ram_base(mem_ram);
    lib_free(plus256k_ram);
    plus256k_ram = NULL;

    if (vicii_d000_list_item != NULL) {
        io_source_unregister(vicii_d000_list_item);
        vicii_d000_list_item = NULL;
    }

    if (vicii_d100_list_item != NULL) {
        io_source_unregister(vicii_d100_list_item);
        vicii_d100_list_item = NULL;
    }
    c64io_vicii_init();
    return 0;
}

void plus256k_shutdown(void)
{
    if (plus256k_enabled) {
        plus256k_deactivate();
    }
}

/* ------------------------------------------------------------------------- */

void plus256k_ram_low_store(WORD addr, BYTE value)
{
    plus256k_ram[(plus256k_low_bank << 16) + addr] = value;
}

void plus256k_ram_high_store(WORD addr, BYTE value)
{
    plus256k_ram[(plus256k_high_bank << 16) + addr] = value;
    if (addr == 0xff00) {
        reu_dma(-1);
    }
}

BYTE plus256k_ram_low_read(WORD addr)
{
    return plus256k_ram[(plus256k_low_bank << 16) + addr];
}

BYTE plus256k_ram_high_read(WORD addr)
{
    return plus256k_ram[(plus256k_high_bank * 0x10000) + addr];
}

/* ------------------------------------------------------------------------- */

/* PLUS256K snapshot module format:

   type  | name          | description
   -----------------------------------
   BYTE  | register      | register
   BYTE  | video bank    | current video bank
   BYTE  | low bank      | current low bank
   BYTE  | high bank     | current high bank
   BYTE  | write protect | write protect flag
   ARRAY | RAM           | 262144 BYTES of RAM data

   Note: for some reason this snapshot module revision started at 0.1, so there never was a 0.0
 */

static char snap_module_name[] = "PLUS256K";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int plus256k_snapshot_write(struct snapshot_s *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B (m, plus256k_reg) < 0
        || SMW_B (m, (BYTE)plus256k_video_bank) < 0
        || SMW_B (m, (BYTE)plus256k_low_bank) < 0
        || SMW_B (m, (BYTE)plus256k_high_bank) < 0
        || SMW_B (m, (BYTE)plus256k_protected) < 0
        || SMW_BA(m, plus256k_ram, 0x40000) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int plus256k_snapshot_read(struct snapshot_s *s)
{
    snapshot_module_t *m;
    BYTE vmajor, vminor;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* enable plus256k, without reset */
    set_plus256k_enabled(1, 1);

    /* overwrite registers */
    if (0
        || SMR_B(m, &plus256k_reg) < 0
        || SMR_B_INT(m, &plus256k_video_bank) < 0
        || SMR_B_INT(m, &plus256k_low_bank) < 0
        || SMR_B_INT(m, &plus256k_high_bank) < 0
        || SMR_B_INT(m, &plus256k_protected) < 0
        || SMR_BA(m, plus256k_ram, 0x40000) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);
    
fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }

    /* disable plus256k, without reset */
    set_plus256k_enabled(0, 1);

    return -1;
}
