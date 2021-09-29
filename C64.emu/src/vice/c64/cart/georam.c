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
#include "georam.h"
#include "snapshot.h"
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
static uint8_t georam[2];

/* GEORAM image.  */
static uint8_t *georam_ram = NULL;
static int old_georam_ram_size = 0;

static log_t georam_log = LOG_ERR;

static int georam_activate(void);
static int georam_deactivate(void);

/* Flag: Do we enable the external GEORAM?  */
static int georam_enabled = 0;

/* Size of the GEORAM.  */
static int georam_size = 0;

/* Size of the GEORAM in KiB.  */
static int georam_size_kb = 0;

/* Filename of the GEORAM image.  */
static char *georam_filename = NULL;

static int georam_write_image = 0;

/* Flag: swap io1/io2, currently only used for vic20 masC=uerade,
         but future usage of an io-swapper is possible */
static int georam_io_swap = 0;

/* ---------------------------------------------------------------------*/

static uint8_t georam_io1_read(uint16_t addr);
static void georam_io1_store(uint16_t addr, uint8_t byte);
static uint8_t georam_io2_peek(uint16_t addr);
static void georam_io2_store(uint16_t addr, uint8_t byte);
static int georam_dump(void);

static io_source_t georam_io1_device = {
    CARTRIDGE_NAME_GEORAM, /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "GEORAM",              /* resource to set to '0' */
    0xde00, 0xdeff, 0xff,  /* range for the device, range is different for vic20 */
    1,                     /* read is always valid */
    georam_io1_store,      /* store function */
    NULL,                  /* NO poke function */
    georam_io1_read,       /* read function */
    georam_io1_read,       /* peek function */
    georam_dump,           /* device state information dump function */
    CARTRIDGE_GEORAM,      /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_t georam_io2_device = {
    CARTRIDGE_NAME_GEORAM, /* name of the device */
    IO_DETACH_RESOURCE,    /* use resource to detach the device when involved in a read-collision */
    "GEORAM",              /* resource to set to '0' */
    0xdf80, 0xdfff, 0x01,  /* range for the device, regs:$dffe-$dfff, mirrors:$df80-$dffd, range is different for vic20 */
    0,                     /* read is never valid, regs are write only */
    georam_io2_store,      /* store function */
    NULL,                  /* NO poke function */
    NULL,                  /* NO read function */
    georam_io2_peek,       /* peek function */
    georam_dump,           /* device state information dump function */
    CARTRIDGE_GEORAM,      /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *georam_io1_list_item = NULL;
static io_source_list_t *georam_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_GEORAM, 0, 0, &georam_io1_device, &georam_io2_device, CARTRIDGE_GEORAM
};

/* ------------------------------------------------------------------------- */

int georam_cart_enabled(void)
{
    return georam_enabled;
}

static uint8_t georam_io1_read(uint16_t addr)
{
    uint8_t retval;

    retval = georam_ram[(georam[1] * 16384) + (georam[0] * 256) + addr];

    return retval;
}

static void georam_io1_store(uint16_t addr, uint8_t byte)
{
    georam_ram[(georam[1] * 16384) + (georam[0] * 256) + addr] = byte;
}

static uint8_t georam_io2_peek(uint16_t addr)
{
    if (addr < 2) {
        return georam[addr & 1];
    }
    return 0;
}

static void georam_io2_store(uint16_t addr, uint8_t byte)
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
    mon_out("Size: %d KiB, Bank: %d, Window: %d\n", georam_size_kb, georam[1], georam[0]);
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

    log_message(georam_log, "%dKiB unit installed.", georam_size >> 10);

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
        export_remove(&export_res);
        georam_enabled = 0;
    }
    if (!georam_enabled && val) {
        if (georam_activate() < 0) {
            return -1;
        }
        if (export_add(&export_res) < 0) {
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
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "GEORAM", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &georam_enabled, set_georam_enabled, NULL },
    { "GEORAMsize", 512, RES_EVENT_NO, NULL,
      &georam_size_kb, set_georam_size, NULL },
    { "GEORAMImageWrite", 0, RES_EVENT_NO, NULL,
      &georam_write_image, set_georam_image_write, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_mascuerade_int[] = {
    { "GEORAMIOSwap", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &georam_io_swap, set_georam_io_swap, NULL },
    RESOURCE_INT_LIST_END
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
    { "-georam", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAM", (resource_value_t)1,
      NULL, "Enable the GEO-RAM expansion unit" },
    { "+georam", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAM", (resource_value_t)0,
      NULL, "Disable the GEO-RAM expansion unit" },
    { "-georamsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "GEORAMsize", NULL,
      "<size in KiB>", "Size of the GEORAM expansion unit" },
    { "-georamimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "GEORAMfilename", NULL,
      "<Name>", "Specify name of GEORAM image" },
    { "-georamimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAMImageWrite", (resource_value_t)1,
      NULL, "Allow writing to GEORAM image" },
    { "+georamimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAMImageWrite", (resource_value_t)0,
      NULL, "Do not write to GEORAM image" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_mascuerade_options[] =
{
    { "-georamioswap", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAMIOSwap", (resource_value_t)1,
      NULL, "Swap io mapping (map cart I/O-1 to VIC20 I/O-3 and cart I/O-2 to VIC20 I/O-2)" },
    { "+georamioswap", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GEORAMIOSwap", (resource_value_t)0,
      NULL, "Don't swap io mapping (map cart I/O-1 to VIC20 I/O-2 and cart I/O-2 to VIC20 I/O-3)" },
    CMDLINE_LIST_END
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


int georam_disable(void)
{
    return resources_set_int("GEORAM", 0);
}


void georam_config_setup(uint8_t *rawcart)
{
    if (georam_size > 0) {
        memcpy(georam_ram, rawcart, georam_size);
    }
}

int georam_bin_attach(const char *filename, uint8_t *rawcart)
{
    FILE *fd;
    size_t size;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    size = util_file_length(fd);
    fclose(fd);

    if (set_georam_size((uint32_t)(size / 1024), NULL) < 0) {
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

/* GEORAM snapshot module format:

   type  | name    | version | description
   ---------------------------------------
   BYTE  | io swap |   0.1   | VIC20 I/O swap flag
   DWORD | size    |   0.0+  | size in KiB
   ARRAY | regs    |   0.0+  | 2 BYTES of register data
   ARRAY | RAM     |   0.0+  | 65536, 131072, 262144, 524288, 1048576, 2097152 or 4194304 BYTES of RAM data
 */

static const char snap_module_name[] = "GEORAM";
#define SNAP_MAJOR 0
#define SNAP_MINOR 1

int georam_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)georam_io_swap) < 0
        || SMW_DW(m, (georam_size >> 10)) < 0
        || SMW_BA(m, georam, sizeof(georam)) < 0
        || SMW_BA(m, georam_ram, georam_size) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int georam_read_snapshot_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    uint32_t size;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (!snapshot_version_is_smaller(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &georam_io_swap) < 0) {
            goto fail;
        }
    } else {
        georam_io_swap = 0;
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
