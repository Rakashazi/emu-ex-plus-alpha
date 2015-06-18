/*
 * isepic.c - ISEPIC emulation.
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
#define CARTRIDGE_INCLUDE_SLOT1_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT1_API
#include "c64export.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
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

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "isepic.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
 * ISEPIC is a RAM based freeze cart.
 *
 * It has 2KB of ram which is banked into a 256 byte page in the $DF00-$DFFF area.
 *
 * The page is selected by any kind of access to the I/O-1 area, as follows:
 *
 * PAGE   ACCESS ADDRESS
 * ----   --------------
 *  0         $DE00
 *  1         $DE04
 *  2         $DE02
 *  3         $DE06
 *  4         $DE01
 *  5         $DE05
 *  6         $DE03
 *  7         $DE07
 *
 * Because of the incomplete decoding this 8 byte area is mirrored throughout $DE08-$DEFF.
 *
 * The isepic cart has a switch which controls if the registers and ram is mapped in.
 *
 * When the switch is switched away from the computer the cart is put in 'hidden' mode,
 * where the registers, window and ram is not accessable.
 *
 * When the switch is switched towards the computer the cart is put in ultimax mode,
 * with the registers mapped, and the current page being mapped into any unmapped ultimax
 * memory space, it will also generate an NMI. Which activates the freezer.
 *
 */

/* #define DEBUGISEPIC */

#ifdef DEBUGISEPIC
#define DBG(x) printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

/* Flag: Do we enable the ISEPIC?  */
static int isepic_enabled;

/* Flag: what direction is the switch at, 0 = away, 1 = towards computer */
static int isepic_switch = 0;

static int isepic_write_image = 0;

/* 2 KB RAM */
static BYTE *isepic_ram;

/* current page */
static unsigned int isepic_page = 0;

static char *isepic_filename = NULL;
static int isepic_filetype = 0;

static const char STRING_ISEPIC[] = CARTRIDGE_NAME_ISEPIC;

#define ISEPIC_RAM_SIZE 2048

static int isepic_load_image(void);

/* ------------------------------------------------------------------------- */

/* some prototypes are needed */
static BYTE isepic_io1_read(WORD addr);
static BYTE isepic_io1_peek(WORD addr);
static void isepic_io1_store(WORD addr, BYTE byte);
static BYTE isepic_io2_read(WORD addr);
static BYTE isepic_io2_peek(WORD addr);
static void isepic_io2_store(WORD addr, BYTE byte);
static int isepic_dump(void);

static io_source_t isepic_io1_device = {
    CARTRIDGE_NAME_ISEPIC,
    IO_DETACH_RESOURCE,
    "IsepicCartridgeEnabled",
    0xde00, 0xdeff, 0x07,
    0, /* read is never valid */
    isepic_io1_store,
    isepic_io1_read,
    isepic_io1_peek,
    isepic_dump,
    CARTRIDGE_ISEPIC,
    0,
    0
};

static io_source_t isepic_io2_device = {
    CARTRIDGE_NAME_ISEPIC,
    IO_DETACH_RESOURCE,
    "IsepicCartridgeEnabled",
    0xdf00, 0xdfff, 0xff,
    0,
    isepic_io2_store,
    isepic_io2_read,
    isepic_io2_peek,
    isepic_dump,
    CARTRIDGE_ISEPIC,
    0,
    0
};

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_ISEPIC, 1, 1, &isepic_io1_device, &isepic_io2_device, CARTRIDGE_ISEPIC
};

static io_source_list_t *isepic_io1_list_item = NULL;
static io_source_list_t *isepic_io2_list_item = NULL;

/* ------------------------------------------------------------------------- */

int isepic_cart_enabled(void)
{
    if (isepic_enabled) {
        return 1;
    }
    return 0;
}

int isepic_cart_active(void)
{
    if (isepic_enabled && isepic_switch) {
        return 1;
    }
    return 0;
}

int isepic_freeze_allowed(void)
{
    return isepic_cart_enabled();
}

void isepic_freeze(void)
{
    /* TODO: do nothing ? */
}

static int isepic_activate(void)
{
    if (isepic_ram == NULL) {
        isepic_ram = lib_malloc(ISEPIC_RAM_SIZE);
    }

    if (!util_check_null_string(isepic_filename)) {
        log_message(LOG_DEFAULT, "Reading ISEPIC image %s.", isepic_filename);
        if (isepic_load_image() < 0) {
            log_error(LOG_DEFAULT, "Reading ISEPIC image %s failed.", isepic_filename);
            /* only create a new file if no file exists, so we dont accidently overwrite any files */
            isepic_filetype = CARTRIDGE_FILETYPE_BIN;
            if (!util_file_exists(isepic_filename)) {
                if (isepic_flush_image() < 0) {
                    log_error(LOG_DEFAULT, "Creating ISEPIC image %s failed.", isepic_filename);
                    return -1;
                }
            }
        }
    }

    return 0;
}

static int isepic_deactivate(void)
{
    if (isepic_ram == NULL) {
        return 0;
    }

    if (!util_check_null_string(isepic_filename)) {
        if (isepic_write_image) {
            log_message(LOG_DEFAULT, "Writing ISEPIC Cartridge image %s.", isepic_filename);
            if (isepic_flush_image() < 0) {
                log_error(LOG_DEFAULT, "Writing ISEPIC Cartridge image %s failed.", isepic_filename);
            }
        }
    }

    lib_free(isepic_ram);
    isepic_ram = NULL;
    return 0;
}

static int set_isepic_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("set enabled: %d\n", val));
    if (isepic_enabled && !val) {
        cart_power_off();
        lib_free(isepic_ram);
        isepic_ram = NULL;
        if (isepic_filename) {
            lib_free(isepic_filename);
            isepic_filename = NULL;
        }
        io_source_unregister(isepic_io1_list_item);
        io_source_unregister(isepic_io2_list_item);
        isepic_io1_list_item = NULL;
        isepic_io2_list_item = NULL;
        c64export_remove(&export_res);
        isepic_enabled = 0;
        if (isepic_switch) {
            cart_config_changed_slot1(2, 2, CMODE_READ | CMODE_RELEASE_FREEZE);
        }
    } else if (!isepic_enabled && val) {
        cart_power_off();
        isepic_ram = lib_malloc(ISEPIC_RAM_SIZE);
        isepic_io1_list_item = io_source_register(&isepic_io1_device);
        isepic_io2_list_item = io_source_register(&isepic_io2_device);
        if (c64export_add(&export_res) < 0) {
            lib_free(isepic_ram);
            isepic_ram = NULL;
            io_source_unregister(isepic_io1_list_item);
            io_source_unregister(isepic_io2_list_item);
            isepic_io1_list_item = NULL;
            isepic_io2_list_item = NULL;
            return -1;
        }
        isepic_enabled = 1;
        if (isepic_switch) {
            cart_config_changed_slot1(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
        }
    }
    return 0;
}

int isepic_enable(void)
{
    DBG(("ISEPIC: enable\n"));
    if (resources_set_int("IsepicCartridgeEnabled", 1) < 0) {
        return -1;
    }
    return 0;
}

static int set_isepic_switch(int value, void *param)
{
    int val = value ? 1 : 0;

    DBG(("set switch: %d\n", val));
    if (isepic_switch && !val) {
        isepic_switch = 0;
        if (isepic_enabled) {
            cart_config_changed_slot1(2, 2, CMODE_READ | CMODE_RELEASE_FREEZE);
        }
    } else if (!isepic_switch && val) {
        isepic_switch = 1;
        if (isepic_enabled) {
            cartridge_trigger_freeze();
            cart_config_changed_slot1(2, 3, CMODE_READ | CMODE_RELEASE_FREEZE);
        }
    }
    return 0;
}

static int set_isepic_rw(int val, void *param)
{
    isepic_write_image = val ? 1 : 0;

    return 0;
}

/* TODO: make sure setting filename works under all conditions */
static int set_isepic_filename(const char *name, void *param)
{
    if (isepic_filename != NULL && name != NULL && strcmp(name, isepic_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (isepic_enabled) {
        isepic_deactivate();
    }
    util_string_set(&isepic_filename, name);

    if (isepic_enabled) {
        isepic_activate();
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_string_t resources_string[] = {
    { "Isepicfilename", "", RES_EVENT_NO, NULL,
      &isepic_filename, set_isepic_filename, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "IsepicCartridgeEnabled", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &isepic_enabled, set_isepic_enabled, NULL },
    { "IsepicSwitch", 0, RES_EVENT_STRICT, (resource_value_t)1,
      &isepic_switch, set_isepic_switch, NULL },
    { "IsepicImageWrite", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &isepic_write_image, set_isepic_rw, NULL },
    { NULL }
};

int isepic_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void isepic_resources_shutdown(void)
{
    lib_free(isepic_filename);
    isepic_filename = NULL;
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-isepic", SET_RESOURCE, 0,
      NULL, NULL, "IsepicCartridgeEnabled", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_ISEPIC,
      NULL, NULL },
    { "+isepic", SET_RESOURCE, 0,
      NULL, NULL, "IsepicCartridgeEnabled", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_ISEPIC,
      NULL, NULL },
    { "-isepicimagename", SET_RESOURCE, 1,
      NULL, NULL, "Isepicfilename", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SET_ISEPIC_FILENAME,
      NULL, NULL },
    { "-isepicimagerw", SET_RESOURCE, 0,
      NULL, NULL, "IsepicImageWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ALLOW_WRITING_TO_ISEPIC_IMAGE,
      NULL, NULL },
    { "+isepicimagerw", SET_RESOURCE, 0,
      NULL, NULL, "IsepicImageWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DO_NOT_WRITE_TO_ISEPIC_IMAGE,
      NULL, NULL },
    { "-isepicswitch", SET_RESOURCE, 0,
      NULL, NULL, "IsepicSwitch", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_ISEPIC_SWITCH,
      NULL, NULL },
    { "+isepicswitch", SET_RESOURCE, 0,
      NULL, NULL, "IsepicSwitch", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_ISEPIC_SWITCH,
      NULL, NULL },
    { NULL }
};

int isepic_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

static BYTE isepic_io1_read(WORD addr)
{
    DBG(("io1 r %04x (sw:%d)\n", addr, isepic_switch));

    if (isepic_switch) {
        isepic_page = ((addr & 4) >> 2) | (addr & 2) | ((addr & 1) << 2);
    }
    return 0;
}

static BYTE isepic_io1_peek(WORD addr)
{
    return 0;
}

static void isepic_io1_store(WORD addr, BYTE byte)
{
    DBG(("io1 w %04x %02x (sw:%d)\n", addr, byte, isepic_switch));

    if (isepic_switch) {
        isepic_page = ((addr & 4) >> 2) | (addr & 2) | ((addr & 1) << 2);
    }
}

static BYTE isepic_io2_peek(WORD addr)
{
    BYTE retval = 0;

    if (isepic_switch) {
        retval = isepic_ram[(isepic_page * 256) + (addr & 0xff)];
    }

    return retval;
}

static BYTE isepic_io2_read(WORD addr)
{
    BYTE retval = 0;

    DBG(("io2 r %04x (sw:%d) (p:%d)\n", addr, isepic_switch, isepic_page));

    isepic_io2_device.io_source_valid = 0;

    if (isepic_switch) {
        isepic_io2_device.io_source_valid = 1;
        retval = isepic_ram[(isepic_page * 256) + (addr & 0xff)];
    }

    return retval;
}

static void isepic_io2_store(WORD addr, BYTE byte)
{
    DBG(("io2 w %04x %02x (sw:%d)\n", addr, byte, isepic_switch));

    if (isepic_switch) {
        isepic_ram[(isepic_page * 256) + (addr & 0xff)] = byte;
    }
}

static int isepic_dump(void)
{
    mon_out("Page: %d, Switch: %d\n", isepic_page, isepic_switch);
    return 0;
}

/* ------------------------------------------------------------------------- */

BYTE isepic_romh_read(WORD addr)
{
    switch (addr) {
        case 0xfffa:
        case 0xfffb:
            return isepic_ram[(isepic_page * 256) + (addr & 0xff)];
            break;
        default:
            return mem_read_without_ultimax(addr);
            break;
    }
}

void isepic_romh_store(WORD addr, BYTE byte)
{
    switch (addr) {
        case 0xfffa:
        case 0xfffb:
            isepic_ram[(isepic_page * 256) + (addr & 0xff)] = byte;
            break;
        default:
            mem_store_without_ultimax(addr, byte);
            break;
    }
}

BYTE isepic_page_read(WORD addr)
{
    if (isepic_switch) {
        return isepic_ram[(isepic_page * 256) + (addr & 0xff)];
    } else {
        return mem_read_without_ultimax(addr);
    }
}

void isepic_page_store(WORD addr, BYTE value)
{
    if (isepic_switch) {
        isepic_ram[(isepic_page * 256) + (addr & 0xff)] = value;
    } else {
        mem_store_without_ultimax(addr, value);
    }
}

int isepic_romh_phi1_read(WORD addr, BYTE *value)
{
    switch (addr) {
        case 0xfffa:
        case 0xfffb:
            *value = isepic_ram[(isepic_page * 256) + (addr & 0xff)];
            return CART_READ_VALID;
    }
    return CART_READ_C64MEM;
}

int isepic_romh_phi2_read(WORD addr, BYTE *value)
{
    return isepic_romh_phi1_read(addr, value);
}

int isepic_peek_mem(WORD addr, BYTE *value)
{
    if (isepic_switch) {
        if ((addr >= 0x1000) && (addr <= 0xcfff)) {
            *value = isepic_ram[(isepic_page * 256) + (addr & 0xff)];
            return CART_READ_VALID;
        } else if (addr >= 0xe000) {
            switch (addr) {
                case 0xfffa:
                case 0xfffb:
                    *value = isepic_ram[(isepic_page * 256) + (addr & 0xff)];
                    return CART_READ_VALID;
            }
        }
        return CART_READ_C64MEM;
    }
    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

const char *isepic_get_file_name(void)
{
    return isepic_filename;
}

void isepic_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    switch (addr & 0xf000) {
        case 0xc000:
        case 0xb000:
        case 0xa000:
        case 0x9000:
        case 0x8000:
        case 0x7000:
        case 0x6000:
        case 0x5000:
        case 0x4000:
        case 0x3000:
        case 0x2000:
        case 0x1000:
            *base = &isepic_ram[isepic_page * 256] - (addr & 0xff00);
            *start = (addr & 0xff00);
            *limit = (addr & 0xff00) | 0xfd;
            break;
        default:
            break;
    }
    *base = NULL;
    *start = 0;
    *limit = 0;
}

void isepic_config_init(void)
{
    /* TODO: do nothing ? */
}

void isepic_reset(void)
{
    /* TODO: do nothing ? */
}

void isepic_config_setup(BYTE *rawcart)
{
    memcpy(isepic_ram, rawcart, ISEPIC_RAM_SIZE);
}

static int isepic_common_attach(BYTE *rawcart)
{
    if (resources_set_int("IsepicCartridgeEnabled", 1) < 0) {
        return -1;
    }
    if (isepic_enabled) {
        memcpy(isepic_ram, rawcart, ISEPIC_RAM_SIZE);
        return 0;
    }
    return -1;
}

static int isepic_bin_load(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, ISEPIC_RAM_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    isepic_filetype = CARTRIDGE_FILETYPE_BIN;
    return 0;
}

int isepic_bin_attach(const char *filename, BYTE *rawcart)
{
    if (isepic_bin_load(filename, rawcart) < 0) {
        return -1;
    }

    if (set_isepic_filename(filename, NULL) < 0) {
        return -1;
    }
    return isepic_common_attach(rawcart);
}

int isepic_bin_save(const char *filename)
{
    FILE *fd;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (fwrite(isepic_ram, 1, ISEPIC_RAM_SIZE, fd) != ISEPIC_RAM_SIZE) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

static int isepic_crt_load(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != ISEPIC_RAM_SIZE) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    isepic_filetype = CARTRIDGE_FILETYPE_CRT;
    return 0;
}

int isepic_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    if (isepic_crt_load(fd, rawcart) < 0) {
        return -1;
    }
    if (set_isepic_filename(filename, NULL) < 0) {
        return -1;
    }
    resources_set_int("IsepicSwitch", 0);
    return isepic_common_attach(rawcart);
}

int isepic_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;

    fd = crt_create(filename, CARTRIDGE_ISEPIC, 1, 1, STRING_ISEPIC);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;               /* Chip type. (= FlashROM?) */
    chip.bank = 0;               /* Bank nr. (= 0) */
    chip.start = 0x8000;         /* Address. (= 0x8000) */
    chip.size = ISEPIC_RAM_SIZE; /* Length. (= 0x0800) */

    /* Write CHIP packet data. */
    if (crt_write_chip(isepic_ram, &chip, fd)) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    return 0;
}

static int isepic_load_image(void)
{
    int res = 0;
    FILE *fd;

    if (crt_getid(isepic_filename) == CARTRIDGE_ISEPIC) {
        fd = fopen(isepic_filename, MODE_READ);
        res = isepic_crt_load(fd, isepic_ram);
        fclose(fd);
    } else {
        res = isepic_bin_load(isepic_filename, isepic_ram);
    }
    return res;
}

int isepic_flush_image(void)
{
    if (isepic_filetype == CARTRIDGE_FILETYPE_BIN) {
        return isepic_bin_save(isepic_filename);
    } else if (isepic_filetype == CARTRIDGE_FILETYPE_CRT) {
        return isepic_crt_save(isepic_filename);
    }
    return -1;
}

void isepic_detach(void)
{
    resources_set_int("IsepicCartridgeEnabled", 0);
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTISEPIC"

int isepic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)isepic_enabled) < 0)
        || (SMW_B(m, (BYTE)isepic_switch) < 0)
        || (SMW_B(m, (BYTE)isepic_page) < 0)
        || (SMW_BA(m, isepic_ram, ISEPIC_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int isepic_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    isepic_ram = lib_malloc(ISEPIC_RAM_SIZE);

    if (0
        || (SMR_B_INT(m, &isepic_enabled) < 0)
        || (SMR_B_INT(m, &isepic_switch) < 0)
        || (SMR_B_INT(m, (int*)&isepic_page) < 0)
        || (SMR_BA(m, isepic_ram, ISEPIC_RAM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(isepic_ram);
        isepic_ram = NULL;
        return -1;
    }

    snapshot_module_close(m);

    isepic_filetype = 0;
    isepic_write_image = 0;
    isepic_enabled = 1;

    /* FIXME: ugly code duplication to avoid cart_config_changed calls */
    isepic_io1_list_item = io_source_register(&isepic_io1_device);
    isepic_io2_list_item = io_source_register(&isepic_io2_device);

    if (c64export_add(&export_res) < 0) {
        lib_free(isepic_ram);
        isepic_ram = NULL;
        io_source_unregister(isepic_io1_list_item);
        io_source_unregister(isepic_io2_list_item);
        isepic_io1_list_item = NULL;
        isepic_io2_list_item = NULL;
        isepic_enabled = 0;
        return -1;
    }

    return 0;
}
