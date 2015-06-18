/*
 * georam.c - GEORAM emulation.
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

#include "archdep.h"
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "georam.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"

/*
 * The GeoRAM is a banked memory system. It uses the registers at
 * $dffe and $dfff to determine what part of the GeoRAM memory should
 * be mapped to $de00-$deff.
 *
 * The BBG (Battery Backed GeoRAM) is a version that retains the
 * RAM contents after power-off.
 *
 * The register at $dfff selects which 16k block to map, and $dffe
 * selects a 256-byte page in that block. Since there are only 64
 * 256-byte pages inside of 16k, the value in $dffe ranges from 0 to
 * 63.
 *
 * Register | bits
 * -------------------
 * $dffe    | xx543210
 *
 * x = unused, not connected.
 *
 *
 * The number of 16k blocks that is available depends on the
 * size of the GeoRAM/BBG:
 *
 * RAM size | $dfff
 * ------------------
 *    64k   | $00-$03
 *   128k   | $00-$07
 *   256k   | $00-$0f
 *   512k   | $00-$1f
 *  1024k   | $00-$3f
 *  2048k   | $00-$7f
 *  4096k   | $00-$ff
 *
 * The unused bits in both registers are ignored and using them in
 * software will cause a wrap-around.
 *
 * The two registers are write-only. Attempting to read them will
 * only return random values.
 *
 * Currently both the BBG and GeoRAM are emulated, BBG mode is
 * used when selecting a save-file.
 *
 * The current emulation has the two registers mirrorred through the
 * range of $df80-$dffd
 *
 * There is also a user-made clone of the GeoRAM called the NeoRAM,
 * it works in the same way as the GeoRAM but seems to have extra
 * RAM sizes currently not supported by this emulation (like 1536k).
 *
 */

/*
 Offsets of the different GEORAM registers
*/
#define GEORAM_REG_PAGE_LOW  0xfe
#define GEORAM_REG_PAGE_HIGH 0xff

/* GEORAM registers */
static BYTE georam[2];

/* GEORAM image.  */
static BYTE *georam_ram = NULL;
static int old_georam_ram_size = 0;

static log_t georam_log = LOG_ERR;

static int georam_activate(void);
static int georam_deactivate(void);

/* Flag: Do we enable the external GEORAM?  */
static int georam_enabled = 0;

/* Size of the GEORAM.  */
static int georam_size = 0;

/* Size of the GEORAM in KB.  */
static int georam_size_kb = 0;

/* Filename of the GEORAM image.  */
static char *georam_filename = NULL;

static int georam_write_image = 0;

/* Flag: swap io1/io2, currently only used for vic20 masC=uerade,
         but future usage of an io-swapper is possible */
static int georam_io_swap = 0;

/* ---------------------------------------------------------------------*/

static BYTE georam_io1_read(WORD addr);
static void georam_io1_store(WORD addr, BYTE byte);
static BYTE georam_io2_peek(WORD addr);
static void georam_io2_store(WORD addr, BYTE byte);
static int georam_dump(void);

static io_source_t georam_io1_device = {
    CARTRIDGE_NAME_GEORAM,
    IO_DETACH_RESOURCE,
    "GEORAM",
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    georam_io1_store,
    georam_io1_read,
    georam_io1_read,
    georam_dump,
    CARTRIDGE_GEORAM,
    0,
    0
};

static io_source_t georam_io2_device = {
    CARTRIDGE_NAME_GEORAM,
    IO_DETACH_RESOURCE,
    "GEORAM",
    0xdf80, 0xdfff, 0x7f,
    0,
    georam_io2_store,
    NULL,
    georam_io2_peek,
    georam_dump,
    CARTRIDGE_GEORAM,
    0,
    0
};

static io_source_list_t *georam_io1_list_item = NULL;
static io_source_list_t *georam_io2_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_GEORAM, 0, 0, &georam_io1_device, &georam_io2_device, CARTRIDGE_GEORAM
};

/* ------------------------------------------------------------------------- */

int georam_cart_enabled(void)
{
    return georam_enabled;
}

static BYTE georam_io1_read(WORD addr)
{
    BYTE retval;

    retval = georam_ram[(georam[1] * 16384) + (georam[0] * 256) + addr];

    return retval;
}

static void georam_io1_store(WORD addr, BYTE byte)
{
    georam_ram[(georam[1] * 16384) + (georam[0] * 256) + addr] = byte;
}

static BYTE georam_io2_peek(WORD addr)
{
    if (addr < 2) {
        return georam[addr & 1];
    }
    return 0;
}

static void georam_io2_store(WORD addr, BYTE byte)
{
    if ((addr & 1) == 1) {
        while (byte > ((georam_size_kb / 16) - 1)) {
            byte = byte - (unsigned char)(georam_size_kb / 16);
        }
        georam[1] = byte;
    }
    if ((addr & 1) == 0) {
        while (byte > 63) {
            byte = byte - 64;
        }
        georam[0] = byte;
    }
}

static int georam_dump(void)
{
    mon_out("Size: %d Kb, Bank: %d, Window: %d\n", georam_size_kb, georam[1], georam[0]);
    return 0;
}

/* ------------------------------------------------------------------------- */

static int georam_activate(void)
{
    if (!georam_size) {
        return 0;
    }

    georam_ram = lib_realloc((void *)georam_ram, (size_t)georam_size);

    /* Clear newly allocated RAM.  */
    if (georam_size > old_georam_ram_size) {
        memset(georam_ram, 0, (size_t)(georam_size - old_georam_ram_size));
    }

    old_georam_ram_size = georam_size;

    log_message(georam_log, "%dKB unit installed.", georam_size >> 10);

    if (!util_check_null_string(georam_filename)) {
        if (util_file_load(georam_filename, georam_ram, (size_t)georam_size, UTIL_FILE_LOAD_RAW) < 0) {
            log_message(georam_log, "Reading GEORAM image %s failed.", georam_filename);
            if (util_file_save(georam_filename, georam_ram, georam_size) < 0) {
                log_message(georam_log, "Creating GEORAM image %s failed.", georam_filename);
                return -1;
            }
            log_message(georam_log, "Creating GEORAM image %s.", georam_filename);
            return 0;
        }
        log_message(georam_log, "Reading GEORAM image %s.", georam_filename);
    }

    georam_reset();
    return 0;
}

static int georam_deactivate(void)
{
    if (georam_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(georam_filename)) {
        if (georam_write_image) {
            log_message(LOG_DEFAULT, "Writing GEORAM image %s.", georam_filename);
            if (georam_flush_image() < 0) {
                log_message(LOG_DEFAULT, "Writing GEORAM image %s failed.", georam_filename);
            }
        }
    }

    lib_free(georam_ram);
    georam_ram = NULL;
    old_georam_ram_size = 0;

    return 0;
}

static int set_georam_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (georam_enabled && !val) {
        if (georam_deactivate() < 0) {
            return -1;
        }
        io_source_unregister(georam_io1_list_item);
        io_source_unregister(georam_io2_list_item);
        georam_io1_list_item = NULL;
        georam_io2_list_item = NULL;
        c64export_remove(&export_res);
        georam_enabled = 0;
    }
    if (!georam_enabled && val) {
        if (georam_activate() < 0) {
            return -1;
        }
        if (c64export_add(&export_res) < 0) {
            return -1;
        }
        if (machine_class == VICE_MACHINE_VIC20) {
            /* set correct addresses for masC=uerade */
            if (georam_io_swap) {
                georam_io1_device.start_address = 0x9c00;
                georam_io1_device.end_address = 0x9fff;
                georam_io2_device.start_address = 0x9800;
                georam_io2_device.end_address = 0x9bff;
            } else {
                georam_io1_device.start_address = 0x9800;
                georam_io1_device.end_address = 0x9bff;
                georam_io2_device.start_address = 0x9c00;
                georam_io2_device.end_address = 0x9fff;
            }
        }
        georam_io1_list_item = io_source_register(&georam_io1_device);
        georam_io2_list_item = io_source_register(&georam_io2_device);
        georam_enabled = 1;
    }
    return 0;
}

static int set_georam_size(int val, void *param)
{
    if (val == georam_size_kb) {
        return 0;
    }

    switch (val) {
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
        case 2048:
        case 4096:
            break;
        default:
            log_message(georam_log, "Unknown GEORAM size %d.", val);
            return -1;
    }

    if (georam_enabled) {
        georam_deactivate();
        georam_size_kb = val;
        georam_size = georam_size_kb << 10;
        georam_activate();
    } else {
        georam_size_kb = val;
        georam_size = georam_size_kb << 10;
    }

    return 0;
}

static int set_georam_io_swap(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val == georam_io_swap) {
        return 0;
    }

    if (georam_enabled) {
        georam_deactivate();
        georam_io_swap = val;
        georam_activate();
    } else {
        georam_io_swap = val;
    }
    return 0;
}

static int set_georam_filename(const char *name, void *param)
{
    if (georam_filename != NULL && name != NULL && strcmp(name, georam_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (georam_enabled) {
        georam_deactivate();
        util_string_set(&georam_filename, name);
        georam_activate();
    } else {
        util_string_set(&georam_filename, name);
    }

    return 0;
}

static int set_georam_image_write(int val, void *param)
{
    georam_write_image = val ? 1 : 0;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "GEORAMfilename", "", RES_EVENT_NO, NULL,
      &georam_filename, set_georam_filename, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "GEORAM", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &georam_enabled, set_georam_enabled, NULL },
    { "GEORAMsize", 512, RES_EVENT_NO, NULL,
      &georam_size_kb, set_georam_size, NULL },
    { "GEORAMImageWrite", 0, RES_EVENT_NO, NULL,
      &georam_write_image, set_georam_image_write, NULL },
    { NULL }
};

static const resource_int_t resources_mascuerade_int[] = {
    { "GEORAMIOSwap", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &georam_io_swap, set_georam_io_swap, NULL },
    { NULL }
};

int georam_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        if (resources_register_int(resources_mascuerade_int) < 0) {
            return -1;
        }
    }

    return resources_register_int(resources_int);
}

void georam_resources_shutdown(void)
{
    lib_free(georam_filename);
    georam_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-georam", SET_RESOURCE, 0,
      NULL, NULL, "GEORAM", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_GEORAM,
      NULL, NULL },
    { "+georam", SET_RESOURCE, 0,
      NULL, NULL, "GEORAM", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_GEORAM,
      NULL, NULL },
    { "-georamsize", SET_RESOURCE, 1,
      NULL, NULL, "GEORAMsize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_SIZE_IN_KB, IDCLS_GEORAM_SIZE,
      NULL, NULL },
    { "-georamimage", SET_RESOURCE, 1,
      NULL, NULL, "GEORAMfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_GEORAM_NAME,
      NULL, NULL },
    { "-georamimagerw", SET_RESOURCE, 0,
      NULL, NULL, "GEORAMImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_GEORAM_IMAGE,
      NULL, NULL },
    { "+georamimagerw", SET_RESOURCE, 0,
      NULL, NULL, "GEORAMImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_GEORAM_IMAGE,
      NULL, NULL },
    { NULL }
};

static const cmdline_option_t cmdline_mascuerade_options[] =
{
    { "-georamioswap", SET_RESOURCE, 0,
      NULL, NULL, "GEORAMIOSwap", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SWAP_CART_IO,
      NULL, NULL },
    { "+georamioswap", SET_RESOURCE, 0,
      NULL, NULL, "GEORAMIOSwap", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DONT_SWAP_CART_IO,
      NULL, NULL },
    { NULL }
};

int georam_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_VIC20) {
        if (cmdline_register_options(cmdline_mascuerade_options) < 0) {
            return -1;
        }
    }

    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

const char *georam_get_file_name(void)
{
    return georam_filename;
}

void georam_init(void)
{
    georam_log = log_open("GEORAM");
}

void georam_reset(void)
{
    georam[0] = 0;
    georam[1] = 0;
}

void georam_detach(void)
{
    resources_set_int("GEORAM", 0);
}

int georam_enable(void)
{
    if (resources_set_int("GEORAM", 1) < 0) {
        return -1;
    }
    return 0;
}

void georam_config_setup(BYTE *rawcart)
{
    if (georam_size > 0) {
        memcpy(georam_ram, rawcart, georam_size);
    }
}

int georam_bin_attach(const char *filename, BYTE *rawcart)
{
    FILE *fd;
    int size;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    size = util_file_length(fd);
    fclose(fd);

    if (set_georam_size(size / 1024, NULL) < 0) {
        return -1;
    }

    if (set_georam_filename(filename, NULL) < 0) {
        return -1;
    }

    if (util_file_load(filename, rawcart, size, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return georam_enable();
}

int georam_bin_save(const char *filename)
{
    if (georam_ram == NULL) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (util_file_save(filename, georam_ram, georam_size) < 0) {
        return -1;
    }

    return 0;
}

int georam_flush_image(void)
{
    return georam_bin_save(georam_filename);
}

/* ------------------------------------------------------------------------- */

static char snap_module_name[] = "GEORAM";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0

int georam_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_DW(m, (georam_size >> 10)) < 0 || SMW_BA(m, georam, sizeof(georam)) < 0 || SMW_BA(m, georam_ram, georam_size) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int georam_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    DWORD size;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version != SNAP_MAJOR) {
        log_error(georam_log, "Major version %d not valid; should be %d.", major_version, SNAP_MAJOR);
        goto fail;
    }

    /* Read RAM size.  */
    if (SMR_DW(m, &size) < 0) {
        goto fail;
    }

    if (size > 4096) {
        log_error(georam_log, "Size %d in snapshot not supported.", (int)size);
        goto fail;
    }

    set_georam_size((int)size, NULL);

    if (!georam_enabled) {
        set_georam_enabled(1, NULL);
    }

    if (SMR_BA(m, georam, sizeof(georam)) < 0 || SMR_BA(m, georam_ram, georam_size) < 0) {
        goto fail;
    }

    snapshot_module_close(m);
    georam_enabled = 1;
    return 0;

fail:
    snapshot_module_close(m);
    georam_enabled = 0;
    return -1;
}
