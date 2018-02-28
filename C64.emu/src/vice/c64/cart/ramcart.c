/*
 * ramcart.c - RAMCART emulation.
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

#define CARTRIDGE_INCLUDE_SLOT1_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT1_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "ramcart.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    "RamCart"

    - 64kb or 128kb RAM

    RamCart is a memory expansion module with battery backup for C64/128
    that was designed and produced in Poland. At start there was only Atari
    version, in 1993 a C64/128 cartridge appeared. It was produced in two
    flavours: 64KB and 128KB.

    The memory is seen in a 256-byte window, placed at $df00-$dfff. The upper
    bits of the address are set by writing page number to $de00 (and the
    lowest bit of $de01 in 128KB version).

    Additionaly, there is a switch that protects the memory contents from
    overwriting. If the switch is set to Read-Only and bit 7 of $de01 is
    cleared (default), then contents of memory window are also visible in
    the $8000-$80ff area. This allows to emulate usual cartridge takeover
    after hardware reset by placing boot code with magic CBM80 string in
    the very first page of RamCart memory.

    There was some firmware on a floppy that allowed the RamCart to be
    used as a memory disk, as device number 7. You could load and save
    files, load directory and delete files. Note that only LOAD and SAVE
    worked. It wasn't possible to use BASIC command OPEN to create a file.
    The firmware took over control right after hardware reset and presented
    the user with a list of stored files. By pressing a letter key it was
    possible to quickload a file and execute it. Hence RamCart was ideal for
    storing frequently used tools.

    The register at $de01 only exists in the 128KB version.

    Register | bits
    -------------------
    $de01    | 7xxxxxx0

    x = unused, not connected.

    bit 7 is used in combination with the read-only switch to mirror
        $df00-$dfff into $8000-$80ff, when set to 1 and switch is
        on, area is mirrored.

    bit 0 is used as 64k bank selector.

    The current emulation has support for both 64k and 128k flavors,
    the unused bits of the $de01 register is assumed to be not
    connected.
*/

/* RAMCART registers */
static BYTE ramcart[2];

/* RAMCART image.  */
static BYTE *ramcart_ram = NULL;
static int old_ramcart_ram_size = 0;

static log_t ramcart_log = LOG_ERR;

static int ramcart_activate(void);
static int ramcart_deactivate(void);

/* Flag: Do we enable the external RAMCART?  */
static int ramcart_enabled;

/* Flag: Is the RAMCART readonly ?  */
static int ramcart_readonly = 0;

/* Size of the RAMCART.  */
static int ramcart_size = 0;

/* Size of the RAMCART in KB.  */
static int ramcart_size_kb = 0;

/* Filename of the RAMCART image.  */
static char *ramcart_filename = NULL;

static int ramcart_write_image = 0;

/* x128 exrom active */
static int ramcart_exrom_active = 0;

/* ------------------------------------------------------------------------- */

static BYTE ramcart_io1_peek(WORD addr);
static BYTE ramcart_io1_read(WORD addr);
static void ramcart_io1_store(WORD addr, BYTE byte);
static BYTE ramcart_io2_read(WORD addr);
static void ramcart_io2_store(WORD addr, BYTE byte);
static int ramcart_dump(void);

static io_source_t ramcart_io1_device = {
    CARTRIDGE_NAME_RAMCART,
    IO_DETACH_RESOURCE,
    "RAMCART",
    0xde00, 0xdeff, 0x01,
    1, /* read is always valid */
    ramcart_io1_store,
    ramcart_io1_read,
    ramcart_io1_peek,
    ramcart_dump,
    CARTRIDGE_RAMCART,
    0,
    0
};

static io_source_t ramcart_io2_device = {
    CARTRIDGE_NAME_RAMCART,
    IO_DETACH_RESOURCE,
    "RAMCART",
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    ramcart_io2_store,
    ramcart_io2_read,
    ramcart_io2_read,
    ramcart_dump,
    CARTRIDGE_RAMCART,
    0,
    0
};

static io_source_list_t *ramcart_io1_list_item = NULL;
static io_source_list_t *ramcart_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_RAMCART, 1, 0, &ramcart_io1_device, &ramcart_io2_device, CARTRIDGE_RAMCART
};

/* ------------------------------------------------------------------------- */

/* x128 exrom check */
static void ramcart_exrom_check(void)
{
    if (ramcart_exrom_active) {
        if (ramcart_size_kb != 128 || !ramcart_readonly || !ramcart_enabled || (ramcart[1] & 0x80)) {
            cart_set_port_exrom_slot1(0);
            cart_port_config_changed_slot1();
            ramcart_exrom_active = 0;
        }
    } else {
        if (ramcart_size_kb == 128 && ramcart_readonly && ramcart_enabled && !(ramcart[1] & 0x80)) {
            cart_set_port_exrom_slot1(1);
            cart_port_config_changed_slot1();
            ramcart_exrom_active = 1;
        }
    }
}

int ramcart_cart_enabled(void)
{
    return ramcart_enabled;
}

static BYTE ramcart_io1_peek(WORD addr)
{
    return ramcart[addr];
}

static BYTE ramcart_io1_read(WORD addr)
{
    BYTE retval;

    if (addr == 1 && ramcart_size_kb == 128) {
        retval = vicii_read_phi1() & 0x7e;
        retval += ramcart[addr];
    } else {
        retval = ramcart[addr];
    }

    return retval;
}

static void ramcart_io1_store(WORD addr, BYTE byte)
{
    if (addr == 1 && ramcart_size_kb == 128) {
        ramcart[1] = byte & 0x81;
        if (machine_class == VICE_MACHINE_C128) {
            ramcart_exrom_check();
        }
    }
    if (addr == 0) {
        ramcart[0] = byte;
    }
}

static BYTE ramcart_io2_read(WORD addr)
{
    BYTE retval;

    retval = ramcart_ram[((ramcart[1] & 1) << 16) + (ramcart[0] * 256) + (addr & 0xff)];

    return retval;
}

static void ramcart_io2_store(WORD addr, BYTE byte)
{
    ramcart_ram[((ramcart[1] & 1) * 65536) + (ramcart[0] * 256) + (addr & 0xff)] = byte;
}

static int ramcart_dump(void)
{
    int bank = 0;
    int mirrored = 0;

    if (ramcart_size_kb == 128) {
        bank = (ramcart[1] & 1) << 8;
        if ((ramcart[1] & 0x80) && ramcart_readonly) {
            mirrored = 1;
        }
    }
    bank += ramcart[0];

    mon_out("RAM size: %s, bank: %d, status: %s\n",
            (ramcart_size_kb == 128) ? "128Kb" : "64Kb",
            bank,
            (ramcart_readonly) ? ((mirrored) ? "read-only and mirrored at $8000-$80FF" : "read-only") : "read/write");
    return 0;
}

/* ------------------------------------------------------------------------- */

static int ramcart_activate(void)
{
    if (!ramcart_size) {
        return 0;
    }

    ramcart_ram = lib_realloc((void *)ramcart_ram, (size_t)ramcart_size);

    /* Clear newly allocated RAM.  */
    if (ramcart_size > old_ramcart_ram_size) {
        memset(ramcart_ram, 0, (size_t)(ramcart_size - old_ramcart_ram_size));
    }

    old_ramcart_ram_size = ramcart_size;

    log_message(ramcart_log, "%dKB unit installed.", ramcart_size >> 10);

    if (!util_check_null_string(ramcart_filename)) {
        if (util_file_load(ramcart_filename, ramcart_ram, (size_t)ramcart_size, UTIL_FILE_LOAD_RAW) < 0) {
            log_error(ramcart_log, "Reading RAMCART image %s failed.", ramcart_filename);
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            if (!util_file_exists(ramcart_filename)) {
                if (util_file_save(ramcart_filename, ramcart_ram, ramcart_size) < 0) {
                    log_error(ramcart_log, "Creating RAMCART image %s failed.", ramcart_filename);
                    return -1;
                }
                log_message(ramcart_log, "Creating RAMCART image %s.", ramcart_filename);
                return 0;
            }
        }
        log_message(ramcart_log, "Reading RAMCART image %s.", ramcart_filename);
    }

    ramcart_reset();
    return 0;
}

static int ramcart_deactivate(void)
{
    if (ramcart_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(ramcart_filename)) {
        if (ramcart_write_image) {
            log_message(LOG_DEFAULT, "Writing RAMCART image %s.", ramcart_filename);
            if (ramcart_flush_image() < 0) {
                log_error(LOG_DEFAULT, "Writing RAMCART image %s failed.", ramcart_filename);
            }
        }
    }

    lib_free(ramcart_ram);
    ramcart_ram = NULL;
    old_ramcart_ram_size = 0;

    return 0;
}

static int set_ramcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!ramcart_enabled && val) {
        cart_power_off();
        if (ramcart_activate() < 0) {
            return -1;
        }
        if (export_add(&export_res) < 0) {
            return -1;
        }
        ramcart_io1_list_item = io_source_register(&ramcart_io1_device);
        ramcart_io2_list_item = io_source_register(&ramcart_io2_device);
        ramcart_enabled = 1;
        if (machine_class == VICE_MACHINE_C128) {
            ramcart_exrom_check();
        } else {
            cart_set_port_exrom_slot1(1);
            cart_port_config_changed_slot1();
        }
    } else if (ramcart_enabled && !val) {
        cart_power_off();
        if (ramcart_deactivate() < 0) {
            return -1;
        }
        io_source_unregister(ramcart_io1_list_item);
        io_source_unregister(ramcart_io2_list_item);
        ramcart_io1_list_item = NULL;
        ramcart_io2_list_item = NULL;
        export_remove(&export_res);
        ramcart_enabled = 0;
        if (machine_class == VICE_MACHINE_C128) {
            ramcart_exrom_check();
        } else {
            cart_set_port_exrom_slot1(0);
            cart_port_config_changed_slot1();
        }
    }
    return 0;
}

static int set_ramcart_readonly(int val, void *param)
{
    ramcart_readonly = val ? 1 : 0;

    if (machine_class == VICE_MACHINE_C128) {
        ramcart_exrom_check();
    }

    return 0;
}

static int set_ramcart_size(int val, void *param)
{
    if (val == ramcart_size_kb) {
        return 0;
    }

    switch (val) {
        case 64:
        case 128:
            break;
        default:
            log_message(ramcart_log, "Unknown RAMCART size %d.", val);
            return -1;
    }

    if (ramcart_enabled) {
        ramcart_deactivate();
        ramcart_size_kb = val;
        ramcart_size = ramcart_size_kb << 10;
        ramcart_activate();
        if (machine_class == VICE_MACHINE_C128) {
            ramcart_exrom_check();
        }
    } else {
        ramcart_size_kb = val;
        ramcart_size = ramcart_size_kb << 10;
    }

    return 0;
}

static int set_ramcart_filename(const char *name, void *param)
{
    if (ramcart_filename != NULL && name != NULL && strcmp(name, ramcart_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (ramcart_enabled) {
        ramcart_deactivate();
        util_string_set(&ramcart_filename, name);
        ramcart_activate();
    } else {
        util_string_set(&ramcart_filename, name);
    }

    return 0;
}

static int set_ramcart_image_write(int val, void *param)
{
    ramcart_write_image = val ? 1 : 0;

    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_string_t resources_string[] = {
    { "RAMCARTfilename", "", RES_EVENT_NO, NULL,
      &ramcart_filename, set_ramcart_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "RAMCART", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ramcart_enabled, set_ramcart_enabled, NULL },
    { "RAMCART_RO", 0, RES_EVENT_NO, NULL,
      &ramcart_readonly, set_ramcart_readonly, NULL },
    { "RAMCARTsize", 128, RES_EVENT_NO, NULL,
      &ramcart_size_kb, set_ramcart_size, NULL },
    { "RAMCARTImageWrite", 0, RES_EVENT_NO, NULL,
      &ramcart_write_image, set_ramcart_image_write, NULL },
    RESOURCE_INT_LIST_END
};

int ramcart_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void ramcart_resources_shutdown(void)
{
    lib_free(ramcart_filename);
    ramcart_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-ramcart", SET_RESOURCE, 0,
      NULL, NULL, "RAMCART", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAMCART,
      NULL, NULL },
    { "+ramcart", SET_RESOURCE, 0,
      NULL, NULL, "RAMCART", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAMCART,
      NULL, NULL },
    { "-ramcartsize", SET_RESOURCE, 1,
      NULL, NULL, "RAMCARTsize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_SIZE_IN_KB, IDCLS_RAMCART_SIZE,
      NULL, NULL },
    { "-ramcartimage", SET_RESOURCE, 1,
      NULL, NULL, "RAMCARTfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_RAMCART_NAME,
      NULL, NULL },
    { "-ramcartimagerw", SET_RESOURCE, 0,
      NULL, NULL, "RAMCARTImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_RAMCART_IMAGE,
      NULL, NULL },
    { "+ramcartimagerw", SET_RESOURCE, 0,
      NULL, NULL, "RAMCARTImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_RAMCART_IMAGE,
      NULL, NULL },
    { "-ramcartro", SET_RESOURCE, 0,
      NULL, NULL, "RAMCART_RO", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RAMCART_READ_ONLY,
      NULL, NULL },
    { "-ramcartrw", SET_RESOURCE, 0,
      NULL, NULL, "RAMCART_RO", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RAMCART_READ_WRITE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int ramcart_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

const char *ramcart_get_file_name(void)
{
    return ramcart_filename;
}

void ramcart_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    if (ramcart_readonly == 1 && ramcart_size_kb == 128 && addr >= 0x8000 && addr <= 0x80ff) {
        *base = &ramcart_ram[((ramcart[1] & 1) * 65536) + (ramcart[0] * 256)] - 0x8000;
        *start = 0x8000;
        *limit = 0x80fd;
        return;
    }
    *base = NULL;
    *start = 0;
    *limit = -1;
}

void ramcart_init_config(void)
{
    if (ramcart_enabled) {
        if (machine_class != VICE_MACHINE_C128) {
            cart_set_port_exrom_slot1(1);
            cart_port_config_changed_slot1();
        }
    }
}

void ramcart_init(void)
{
    ramcart_log = log_open("RAMCART");
}

void ramcart_reset(void)
{
    ramcart[0] = 0;
    ramcart[1] = 0;
}

void ramcart_config_setup(BYTE *rawcart)
{
    memcpy(ramcart_ram, rawcart, ramcart_size);
}

void ramcart_detach(void)
{
    resources_set_int("RAMCART", 0);
}

int ramcart_enable(void)
{
    if (resources_set_int("RAMCART", 1) < 0) {
        return -1;
    }
    return 0;
}

int ramcart_bin_attach(const char *filename, BYTE *rawcart)
{
    int size = 128;

    if (util_file_load(filename, rawcart, 128 * 1024, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        size = 64;
        if (util_file_load(filename, rawcart, 64 * 1024, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
    }
    set_ramcart_size(size, NULL);
    set_ramcart_filename(filename, NULL);
    return ramcart_enable();
}

int ramcart_bin_save(const char *filename)
{
    if (ramcart_ram == NULL) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (util_file_save(filename, ramcart_ram, ramcart_size) < 0) {
        log_message(ramcart_log, "Writing RAMCART image %s failed.", filename);
        return -1;
    }
    log_message(ramcart_log, "Writing RAMCART image %s.", filename);

    return 0;
}

int ramcart_flush_image(void)
{
    return ramcart_bin_save(ramcart_filename);
}

/* ------------------------------------------------------------------------- */

BYTE ramcart_roml_read(WORD addr)
{
    if (ramcart_readonly == 1 && ramcart_size_kb == 128 && addr >= 0x8000 && addr <= 0x80ff) {
        return ramcart_ram[((ramcart[1] & 1) * 65536) + (ramcart[0] * 256) + (addr & 0xff)];
    }
    return mem_ram[addr];
}

void ramcart_roml_store(WORD addr, BYTE byte)
{
    /* FIXME: this can't be right */
    mem_ram[addr] = byte;
}

int ramcart_peek_mem(WORD addr, BYTE *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        if (ramcart_readonly == 1 && ramcart_size_kb == 128 && addr >= 0x8000 && addr <= 0x80ff) {
            *value = ramcart_ram[((ramcart[1] & 1) * 65536) + (ramcart[0] * 256) + (addr & 0xff)];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

/* CARTRAMCART snapshot module format:

   type  | name      | description
   -------------------------------
   BYTE  | enabled   | cartridge enabled flag
   BYTE  | readonly  | read-only flag
   DWORD | BSIZE     | RAM size in BYTES
   BYTE  | KBSIZE    | RAM size in KB
   ARRAY | registers | 2 BYTES of register data
   ARRAY | RAM       | 65536 or 131072 BYTES of RAM data
 */

static char snap_module_name[] = "CARTRAMCART";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ramcart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)ramcart_enabled) < 0)
        || (SMW_B(m, (BYTE)ramcart_readonly) < 0)
        || (SMW_DW(m, (DWORD)ramcart_size) < 0)
        || (SMW_B(m, (BYTE)ramcart_size_kb) < 0)
        || (SMW_BA(m, ramcart, 2) < 0)
        || (SMW_BA(m, ramcart_ram, ramcart_size) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int ramcart_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &ramcart_enabled) < 0)
        || (SMR_B_INT(m, &ramcart_readonly) < 0)
        || (SMR_DW_INT(m, &ramcart_size) < 0)
        || (SMR_B_INT(m, &ramcart_size_kb) < 0)
        || (SMR_BA(m, ramcart, 2) < 0)) {
        goto fail;
    }

    ramcart_ram = lib_malloc(ramcart_size);

    if (SMR_BA(m, ramcart_ram, ramcart_size) < 0) {
        snapshot_module_close(m);
        lib_free(ramcart_ram);
        ramcart_ram = NULL;
        return -1;
    }

    snapshot_module_close(m);

    /* ramcart_filetype = 0; */
    ramcart_write_image = 0;
    ramcart_enabled = 1;

    /* FIXME: ugly code duplication to avoid cart_config_changed calls */
    ramcart_io1_list_item = io_source_register(&ramcart_io1_device);
    ramcart_io2_list_item = io_source_register(&ramcart_io2_device);

    if (export_add(&export_res) < 0) {
        lib_free(ramcart_ram);
        ramcart_ram = NULL;
        io_source_unregister(ramcart_io1_list_item);
        io_source_unregister(ramcart_io2_list_item);
        ramcart_io1_list_item = NULL;
        ramcart_io2_list_item = NULL;
        ramcart_enabled = 0;
        return -1;
    }

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
