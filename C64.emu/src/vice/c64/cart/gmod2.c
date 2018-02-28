/*
 * gmod2.c - Cartridge handling, GMod2 cart.
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

#include "archdep.h"
#include "c64cart.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "flash040.h"
#include "lib.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "m93c86.h"
#include "translate.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "gmod2.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    GMod2 (Individual Computers)

    512K Flash ROM (29F040), 64*8k pages
    2K serial EEPROM (m93C86)

    io1
        - register at de00 (mirrored over IO1 bank)

        bit7   (rw)  write enable (write 1), EEPROM data output (read)
        bit6   (ro)  EXROM (0=active) and EEPROM chip select (1=selected)
        bit5-0 (ro)  rom bank  bit5 EEPROM clock bit4 EEPROM data input

    see http://wiki.icomp.de/wiki/GMod2
*/

/* #define DEBUGGMOD2 */

#ifdef DEBUGGMOD2
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define GMOD2_FLASH_SIZE (512*1024)

static int gmod2_enabled = 0;

/* current GAME/EXROM mode */
static int gmod2_cmode = CMODE_8KGAME;

/* current bank */
static int gmod2_bank;
static int gmod2_flash_write = 0;

/* the 29F010 statemachine */
static flash040_context_t *flashrom_state = NULL;

static char *gmod2_filename = NULL;
static int gmod2_filetype = 0;

static char *gmod2_eeprom_filename = NULL;
static int gmod2_eeprom_rw = 0;

static int eeprom_cs = 0, eeprom_data = 0, eeprom_clock = 0;

static const char STRING_GMOD2[] = CARTRIDGE_NAME_GMOD2;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static BYTE gmod2_io1_read(WORD addr);
static BYTE gmod2_io1_peek(WORD addr);
static void gmod2_io1_store(WORD addr, BYTE value);
static int gmod2_dump(void);

static io_source_t gmod2_io1_device = {
    CARTRIDGE_NAME_GMOD2,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0,
    gmod2_io1_store,
    gmod2_io1_read,
    gmod2_io1_peek,
    gmod2_dump,
    CARTRIDGE_GMOD2,
    1,
    0
};
static io_source_list_t *gmod2_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_GMOD2, 1, 1, &gmod2_io1_device, NULL, CARTRIDGE_GMOD2
};

/* ---------------------------------------------------------------------*/

BYTE gmod2_io1_read(WORD addr)
{
    gmod2_io1_device.io_source_valid = 0;

    /* DBG(("io1 r %04x (cs:%d)\n", addr, eeprom_cs)); */

    gmod2_io1_device.io_source_valid = 1;
    if (eeprom_cs) {
        return (m93c86_read_data() << 7) | (vicii_read_phi1() & 0x7f);
    }
    return (vicii_read_phi1() & 0xff);
}

BYTE gmod2_io1_peek(WORD addr)
{
    return (m93c86_read_data() << 7);
}

void gmod2_io1_store(WORD addr, BYTE value)
{
    int mode = CMODE_WRITE;

    DBG(("io1 w %04x %02x (cs:%d data:%d clock:%d)\n", addr, value, (value >> 6) & 1, (value >> 4) & 1, (value >> 5) & 1));

    gmod2_bank = value & 0x3f;
    if ((value & 0xc0) == 0xc0) {
        /* FIXME: flash mode enable, ultimax for e000-ffff */
        gmod2_cmode = CMODE_ULTIMAX;
    } else if ((value & 0x40) == 0x00) {
        gmod2_cmode = CMODE_8KGAME;
    } else if ((value & 0x40) == 0x40) {
        gmod2_cmode = CMODE_RAM;
    }
    eeprom_cs = (value >> 6) & 1;
    eeprom_data = (value >> 4) & 1;
    eeprom_clock = (value >> 5) & 1;
    m93c86_write_select((BYTE)eeprom_cs);
    if (eeprom_cs) {
        m93c86_write_data((BYTE)(eeprom_data));
        m93c86_write_clock((BYTE)(eeprom_clock));
    }
    cart_config_changed_slotmain(CMODE_8KGAME, (BYTE)(gmod2_cmode | (gmod2_bank << CMODE_BANK_SHIFT)), mode);
}

/* ---------------------------------------------------------------------*/

BYTE gmod2_roml_read(WORD addr)
{
    return flash040core_read(flashrom_state, (addr & 0x1fff) + (roml_bank << 13));
}

void gmod2_romh_store(WORD addr, BYTE value)
{
    flash040core_store(flashrom_state, (addr & 0x1fff) + (roml_bank << 13), value);
    if (flashrom_state->flash_state != FLASH040_STATE_READ) {
        maincpu_resync_limits();
    }
}

int gmod2_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (addr >= 0x8000 && addr <= 0x9fff) {
        *value = gmod2_roml_read(addr);
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void gmod2_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
#if 0
    if (flashrom_state && flashrom_state->flash_data) {
        switch (addr & 0xe000) {
            case 0xe000:
                if (flashrom_state->flash_state == FLASH040_STATE_READ) {
                    *base = flashrom_state->flash_data + (roml_bank << 13) - 0xe000;
                    *start = 0xe000;
                    *limit = 0xfffd;
                    return;
                }
                break;
            case 0x8000:
                if (flashrom_state->flash_state == FLASH040_STATE_READ) {
                    *base = flashrom_state->flash_data + (roml_bank << 13) - 0x8000;
                    *start = 0x8000;
                    *limit = 0x9ffd;
                    return;
                }
                break;
            default:
                break;
        }
    }
#endif
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

static int gmod2_dump(void)
{
    /* FIXME: incomplete */
    mon_out("GAME/EXROM status: %s%s\n", 
            cart_config_string(gmod2_cmode),
            (gmod2_cmode == CMODE_ULTIMAX) ? " (Flash mode)" : "");
    mon_out("ROM bank: %d\n", gmod2_bank);
    mon_out("EEPROM CS: %d data: %d clock: %d\n", eeprom_cs, eeprom_data, eeprom_clock);

    return 0;
}

/* ---------------------------------------------------------------------*/

void gmod2_config_init(void)
{
    gmod2_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((BYTE)gmod2_cmode, (BYTE)gmod2_cmode, CMODE_READ);
    eeprom_cs = 0;
    m93c86_write_select((BYTE)eeprom_cs);
    flash040core_reset(flashrom_state);
}

void gmod2_reset(void)
{
    gmod2_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((BYTE)gmod2_cmode, (BYTE)gmod2_cmode, CMODE_READ);
    eeprom_cs = 0;
    m93c86_write_select((BYTE)eeprom_cs);

    /* on the real hardware pressing reset would NOT reset the flash statemachine,
       only a powercycle would help. we do it here anyway :)
    */
    flash040core_reset(flashrom_state);
}

void gmod2_config_setup(BYTE *rawcart)
{
    gmod2_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((BYTE)gmod2_cmode, (BYTE)gmod2_cmode, CMODE_READ);

    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_NORMAL, roml_banks);
    memcpy(flashrom_state->flash_data, rawcart, GMOD2_FLASH_SIZE);
}

/* ---------------------------------------------------------------------*/

static int set_gmod2_eeprom_filename(const char *name, void *param)
{
    if ((gmod2_eeprom_filename != NULL) && (name != NULL) && (strcmp(name, gmod2_eeprom_filename) == 0)) {
        return 0;
    }

    if ((name != NULL) && (*name != '\0')) {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    util_string_set(&gmod2_eeprom_filename, name);

    if (gmod2_enabled) {
        return m93c86_open_image(gmod2_eeprom_filename, gmod2_eeprom_rw);
    }

    return 0;
}

static int set_gmod2_eeprom_rw(int val, void* param)
{
    gmod2_eeprom_rw = val ? 1 : 0;
    return 0;
}

static int set_gmod2_flash_write(int val, void *param)
{
    gmod2_flash_write = val ? 1 : 0;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "GMod2EEPROMImage", "", RES_EVENT_NO, NULL,
      &gmod2_eeprom_filename, set_gmod2_eeprom_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "GMod2FlashWrite", 0, RES_EVENT_NO, NULL,
      &gmod2_flash_write, set_gmod2_flash_write, NULL },
    { "GMod2EEPROMRW", 1, RES_EVENT_NO, NULL,
      &gmod2_eeprom_rw, set_gmod2_eeprom_rw, NULL },
    RESOURCE_INT_LIST_END
};

int gmod2_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void gmod2_resources_shutdown(void)
{
    lib_free(gmod2_eeprom_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-gmod2eepromimage", SET_RESOURCE, 1,
      NULL, NULL, "GMod2EEPROMImage", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_P_FILE, IDCLS_SELECT_GMOD2_EEPROM_IMAGE,
      NULL, NULL },
    { "-gmod2eepromrw", SET_RESOURCE, 0,
      NULL, NULL, "GMod2EEPROMRW", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_GMOD2_EEPROM_WRITE_ENABLE,
      NULL, NULL },
    { "+gmod2eepromrw", SET_RESOURCE, 0,
      NULL, NULL, "GMod2EEPROMRW", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_GMOD2_EEPROM_WRITE_DISABLE,
      NULL, NULL },
    { "-gmod2flashwrite", SET_RESOURCE, 0,
      NULL, NULL, "GMod2FlashWrite", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SAVE_GMOD2_ROM_AT_EXIT,
      NULL, NULL },
    { "+gmod2flashwrite", SET_RESOURCE, 0,
      NULL, NULL, "GMod2FlashWrite", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SAVE_GMOD2_ROM_AT_EXIT,
      NULL, NULL },
    CMDLINE_LIST_END
};

int gmod2_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int gmod2_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    gmod2_io1_list_item = io_source_register(&gmod2_io1_device);
    m93c86_open_image(gmod2_eeprom_filename, gmod2_eeprom_rw);

    gmod2_enabled = 1;

    return 0;
}

int gmod2_bin_attach(const char *filename, BYTE *rawcart)
{
    gmod2_filetype = 0;
    gmod2_filename = NULL;

    if (util_file_load(filename, rawcart, GMOD2_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    gmod2_filetype = CARTRIDGE_FILETYPE_BIN;
    gmod2_filename = lib_stralloc(filename);
    return gmod2_common_attach();
}

int gmod2_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    memset(rawcart, 0xff, GMOD2_FLASH_SIZE);

    gmod2_filetype = 0;
    gmod2_filename = NULL;

    for (i = 0; i <= 63; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 63 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    gmod2_filetype = CARTRIDGE_FILETYPE_CRT;
    gmod2_filename = lib_stralloc(filename);

    return gmod2_common_attach();
}

int gmod2_bin_save(const char *filename)
{
    FILE *fd;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (fwrite(roml_banks, 1, GMOD2_FLASH_SIZE, fd) != GMOD2_FLASH_SIZE) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    return 0;
}

int gmod2_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    BYTE *data;
    int i;

    fd = crt_create(filename, CARTRIDGE_GMOD2, 1, 0, STRING_GMOD2);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;

    data = &roml_banks[0x10000];

    for (i = 0; i < 64; i++) {
        chip.bank = i; /* bank */

        if (crt_write_chip(data, &chip, fd)) {
            fclose(fd);
            return -1;
        }
        data += 0x2000;
    }

    fclose(fd);
    return 0;
}

int gmod2_flush_image(void)
{
    if (gmod2_filetype == CARTRIDGE_FILETYPE_BIN) {
        return gmod2_bin_save(gmod2_filename);
    } else if (gmod2_filetype == CARTRIDGE_FILETYPE_CRT) {
        return gmod2_crt_save(gmod2_filename);
    }
    return -1;
}

void gmod2_detach(void)
{
    if (gmod2_flash_write && flashrom_state->flash_dirty) {
        gmod2_flush_image();
    }

    flash040core_shutdown(flashrom_state);
    lib_free(flashrom_state);
    flashrom_state = NULL;
    lib_free(gmod2_filename);
    gmod2_filename = NULL;
    m93c86_close_image(gmod2_eeprom_rw);
    export_remove(&export_res);
    io_source_unregister(gmod2_io1_list_item);
    gmod2_io1_list_item = NULL;

    gmod2_enabled = 0;
}

/* ---------------------------------------------------------------------*/

static char snap_module_name[] = "CARTGMOD2";
static char flash_snap_module_name[] = "FLASH040GMOD2";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int gmod2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)gmod2_cmode) < 0
        || SMW_B(m, (BYTE)gmod2_bank) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return flash040core_snapshot_write_module(s, flashrom_state, flash_snap_module_name);
}

int gmod2_snapshot_read_module(snapshot_t *s)
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
        || SMR_B_INT(m, &gmod2_cmode) < 0
        || SMR_B_INT(m, &gmod2_bank) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_NORMAL, roml_banks);

    if (flash040core_snapshot_read_module(s, flashrom_state, flash_snap_module_name) < 0) {
        flash040core_shutdown(flashrom_state);
        lib_free(flashrom_state);
        flashrom_state = NULL;
        return -1;
    }

    gmod2_common_attach();

    /* set filetype to none */
    gmod2_filename = NULL;
    gmod2_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
