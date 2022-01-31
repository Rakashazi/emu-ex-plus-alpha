/*
 * rexramfloppy.c - Cartridge handling, REX Ramfloppy cart.
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
#include <string.h>

#include "rexramfloppy.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "lib.h"
#include "monitor.h"
#include "ram.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/* 256KB of RAM */
#define RRF_RAM_SIZE   256*1024

#define RRF_NO_FORCE_SAVE 0
#define RRF_FORCE_SAVE    1

/* #define DEBUGRF */

#ifdef DEBUGRF
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/*
    REX RAM-Floppy

    8k ROM
    32k up to 256k RAM (Battery buffered)

    dfa0    (write) selects RAM bank

    df50    (read) toggles RAM writeable
    dfc0    (read) toggles cartridge enable
    dfe0    (read) toggles RAM enable

    TODO:
    - implement the disable switch
    - implement RAM size option
*/

static int ram_bank = 0;
static int ram_enabled = 0;
static int cart_enabled = 1;
static int ram_writeable = 0;

/* RAM image.  */
static uint8_t *rexramfloppy_ram = NULL;

/* Filename of the RAM image.  */
static char *rexramfloppy_filename = NULL;

/* Write image on detach */
static int rexramfloppy_write_image = 0;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t rexramfloppy_io2_peek(uint16_t addr);
static uint8_t rexramfloppy_io2_read(uint16_t addr);
static void rexramfloppy_io2_store(uint16_t addr, uint8_t value);
static int rexramfloppy_dump(void);

static io_source_t rexramfloppy_io2_device = {
    CARTRIDGE_NAME_REX_RAMFLOPPY, /* name of the device */
    IO_DETACH_CART,               /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,        /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,         /* range of the device */
    0,                            /* read validity is determined by the device upon a read */
    rexramfloppy_io2_store,       /* store function */
    NULL,                         /* NO poke function */
    rexramfloppy_io2_read,        /* read function */
    rexramfloppy_io2_peek,        /* peek function */
    rexramfloppy_dump,            /* device state information dump function */
    CARTRIDGE_REX_RAMFLOPPY,      /* cartridge ID */
    IO_PRIO_NORMAL,               /* normal priority, device read needs to be checked for collisions */
    0                             /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *rexramfloppy_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_REX_RAMFLOPPY, 0, 1, NULL, &rexramfloppy_io2_device, CARTRIDGE_REX_RAMFLOPPY
};

/* ---------------------------------------------------------------------*/

static uint8_t rexramfloppy_io2_peek(uint16_t addr)
{
    return 0; /* FIXME */
}

static uint8_t rexramfloppy_io2_read(uint16_t addr)
{

    addr &= 0xff;

    switch (addr) {
        case 0x50:
            ram_writeable ^= 1;
            break;
        case 0xc0:
            cart_enabled ^= 1;
            if (cart_enabled) {
                cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
            } else {
                cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
            }
            break;
        case 0xe0:
            ram_enabled ^= 1;
            break;
        default:
            /* printf("io2 read %04x\n", addr);  */
            break;
    }

    return 0;
}

static void rexramfloppy_io2_store(uint16_t addr, uint8_t value)
{

    addr &= 0xff;

    switch (addr) {
        case 0xa0:
            ram_bank = (value & 7) | ((value & 0x30) >> 1);
            break;
        default:
            /* printf("io2 write %04x %02x\n", addr, value); */
            break;
    }
}

static int rexramfloppy_dump(void)
{
    mon_out("mode: %s\n", (cart_enabled) ? "8K Game" : "RAM");
    mon_out("$8000-$9FFF: %s\n", (ram_enabled) ? "RAM" : "ROM");
    mon_out("RAM bank: %d\n", ram_bank);
    mon_out("RAM writeable: %s\n", ram_writeable ? "yes" : "no");
    return 0;
}

/* ---------------------------------------------------------------------*/

uint8_t rexramfloppy_roml_read(uint16_t addr)
{
    if (ram_enabled) {
        return rexramfloppy_ram[(addr & 0x1fff) + (ram_bank * 0x2000)];
    }

    return roml_banks[addr & 0x1fff];
}

void rexramfloppy_roml_store(uint16_t addr, uint8_t value)
{
    if (ram_enabled && ram_writeable) {
        rexramfloppy_ram[(addr & 0x1fff) + (ram_bank * 0x2000)] = value;
    } else {
        mem_store_without_romlh(addr, value);
    }
}

/* ---------------------------------------------------------------------*/

void rexramfloppy_config_init(void)
{
    cart_enabled = 1;
    ram_writeable = 0;
    ram_enabled = 0;
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void rexramfloppy_reset(void)
{
    cart_enabled = 1;
    ram_writeable = 0;
    ram_enabled = 0;
}

void rexramfloppy_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    memset(export_ram0, 0xff, 0x2000 * 32);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int rexramfloppy_load_ram_image(void)
{
    if (!util_check_null_string(rexramfloppy_filename)) {
        if (util_file_load(rexramfloppy_filename, rexramfloppy_ram, RRF_RAM_SIZE, UTIL_FILE_LOAD_RAW) < 0) {
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            if (!util_file_exists(rexramfloppy_filename)) {
                if (util_file_save(rexramfloppy_filename, rexramfloppy_ram, RRF_RAM_SIZE) < 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

static int rexramfloppy_save_ram_image(int force)
{
    if (!util_check_null_string(rexramfloppy_filename)) {
        if (rexramfloppy_write_image || force) {
            if (util_file_save(rexramfloppy_filename, rexramfloppy_ram, RRF_RAM_SIZE) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

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

static int rexramfloppy_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    rexramfloppy_ram = lib_malloc(RRF_RAM_SIZE);
    ram_init_with_pattern(rexramfloppy_ram, RRF_RAM_SIZE, &ramparam);

    if (rexramfloppy_load_ram_image() < 0) {
        lib_free(rexramfloppy_ram);
        return -1;
    }

    rexramfloppy_io2_list_item = io_source_register(&rexramfloppy_io2_device);

    return 0;
}

int rexramfloppy_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return rexramfloppy_common_attach();
}

int rexramfloppy_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 0 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return rexramfloppy_common_attach();
}

void rexramfloppy_detach(void)
{
    export_remove(&export_res);
    io_source_unregister(rexramfloppy_io2_list_item);
    rexramfloppy_io2_list_item = NULL;

    rexramfloppy_save_ram_image(RRF_NO_FORCE_SAVE);

    lib_free(rexramfloppy_ram);
    rexramfloppy_ram = NULL;
}

int rexramfloppy_flush_image(void)
{
    if (rexramfloppy_ram) {
        return rexramfloppy_save_ram_image(RRF_FORCE_SAVE);
    }
    return 0;
}

int rexramfloppy_bin_save(const char *filename)
{
    if (!rexramfloppy_ram) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (util_file_save(filename, rexramfloppy_ram, RRF_RAM_SIZE) < 0) {
        return -1;
    }

    return 0;
}

/* ---------------------------------------------------------------------*/

static int set_rexramfloppy_filename(const char *name, void *param)
{
    if (rexramfloppy_filename != NULL && name != NULL && strcmp(name, rexramfloppy_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (rexramfloppy_ram) {
        rexramfloppy_save_ram_image(RRF_NO_FORCE_SAVE);
        util_string_set(&rexramfloppy_filename, name);
        rexramfloppy_load_ram_image();
    } else {
        util_string_set(&rexramfloppy_filename, name);
    }

    return 0;
}

static int set_rexramfloppy_image_write(int val, void *param)
{
    rexramfloppy_write_image = val ? 1 : 0;

    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "RRFfilename", "", RES_EVENT_NO, NULL,
      &rexramfloppy_filename, set_rexramfloppy_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "RRFImageWrite", 0, RES_EVENT_NO, NULL,
      &rexramfloppy_write_image, set_rexramfloppy_image_write, NULL },
    RESOURCE_INT_LIST_END
};

int rexramfloppy_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void rexramfloppy_resources_shutdown(void)
{
    lib_free(rexramfloppy_filename);
    rexramfloppy_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-rexramfloppyimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RRFfilename", NULL,
      "<Name>", "Specify REX RAM-Floppy filename" },
    { "-rexramfloppyimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RRFImageWrite", (resource_value_t)1,
      NULL, "Allow writing to REX RAM-Floppy image" },
    { "+rexramfloppyimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RRFImageWrite", (resource_value_t)0,
      NULL, "Do not write to REX RAM-Floppy image" },
    CMDLINE_LIST_END
};

int rexramfloppy_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* CARTRRF snapshot module format:

   type  | name             | description
   ---------------------------------
   BYTE  | cart_enabled     | cartridge active flag
   BYTE  | ram_enabled      | RAM enabled at $8000
   BYTE  | ram_writeable    | RAM is writeable
   BYTE  | ram_bank         | currently selected RAM bank
   ARRAY | ROML             | 8192 BYTES of ROML data
   ARRAY | RAM              | 256k BYTES of RAM data
 */

static const char snap_module_name[] = "CARTRRF";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int rexramfloppy_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)cart_enabled) < 0)
        || (SMW_B(m, (uint8_t)ram_enabled) < 0)
        || (SMW_B(m, (uint8_t)ram_writeable) < 0)
        || (SMW_B(m, (uint8_t)ram_bank) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, rexramfloppy_ram, 0x2000 * 32) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int rexramfloppy_snapshot_read_module(snapshot_t *s)
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
        || (SMR_B_INT(m, &cart_enabled) < 0)
        || (SMR_B_INT(m, &ram_enabled) < 0)
        || (SMR_B_INT(m, &ram_writeable) < 0)
        || (SMR_B_INT(m, &ram_bank) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, rexramfloppy_ram, 0x2000 * 32) < 0)) {
        goto fail;
    }

    if (rexramfloppy_common_attach() < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_BA(m, rexramfloppy_ram, 0x2000 * 32) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
