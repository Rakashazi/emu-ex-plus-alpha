/*
 * gmod2c128.c - Cartridge handling, GMod2-C128 cart.
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
#include "c128cart.h"
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
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#include "gmod2c128.h"

/*
    GMod2-C128 (Daniel Mantione)

    512K Flash ROM (29F040), 32*16k pages
    2K serial EEPROM (m93C86)

    io1
        - register at de00 (mirrored over IO1 bank)

        bit7   (rw)  write enable (write 1), EEPROM data output (read)
        bit6   (ro)  EEPROM chip select (1=selected)
        bit5         EEPROM clock
        bit4-0 (ro)  rom bank  bit4 EEPROM data input

    see https://www.freepascal.org/~daniel/gmod2/
*/

/* #define DEBUGGMOD2 */

#ifdef DEBUGGMOD2
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define GMOD2_MAX_BANKS  32
#define GMOD2_BANK_SIZE  0x4000
#define GMOD2_FLASH_SIZE (512*1024)

static int c128gmod2_enabled = 0;

/* current bank */
static int c128gmod2_bank;
static int c128gmod2_flash_write = 0;

/* the 29F010 statemachine */
static flash040_context_t *flashrom_state = NULL;

static char *c128gmod2_filename = NULL;
static int c128gmod2_filetype = 0;

static char *c128gmod2_eeprom_filename = NULL;
static int c128gmod2_eeprom_rw = 0;

static int eeprom_cs = 0, eeprom_data = 0, eeprom_clock = 0;

static const char STRING_GMOD2[] = CARTRIDGE_C128_NAME_GMOD2C128;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t c128gmod2_io1_read(uint16_t addr);
static uint8_t c128gmod2_io1_peek(uint16_t addr);
static void c128gmod2_io1_store(uint16_t addr, uint8_t value);
static int c128gmod2_dump(void);

static io_source_t c128gmod2_io1_device = {
    CARTRIDGE_C128_NAME_GMOD2C128,  /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,           /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                              /* read validity is determined by the device upon a read */
    c128gmod2_io1_store,            /* store function */
    NULL,                           /* NO poke function */
    c128gmod2_io1_read,             /* read function */
    c128gmod2_io1_peek,             /* peek function */
    c128gmod2_dump,                 /* device state information dump function */
    CARTRIDGE_C128_GMOD2C128,       /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0,                              /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                  /* NO mirroring */
};

static io_source_list_t *c128gmod2_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_C128_NAME_GMOD2C128, 0, 0, &c128gmod2_io1_device, NULL, CARTRIDGE_C128_GMOD2C128
};

/* ---------------------------------------------------------------------*/

uint8_t c128gmod2_io1_read(uint16_t addr)
{
    c128gmod2_io1_device.io_source_valid = 0;
    /* DBG(("io1 r %04x (cs:%d)\n", addr, eeprom_cs)); */

    if (eeprom_cs) {
        c128gmod2_io1_device.io_source_valid = 1;
        return (m93c86_read_data() << 7) | (vicii_read_phi1() & 0x7f);
    }
    return 0;
}

uint8_t c128gmod2_io1_peek(uint16_t addr)
{
    return (m93c86_read_data() << 7);
}

void c128gmod2_io1_store(uint16_t addr, uint8_t value)
{
    DBG(("io1 w %04x %02x (cs:%d data:%d clock:%d)\n", addr, value, (value >> 6) & 1, (value >> 4) & 1, (value >> 5) & 1));

    c128gmod2_bank = value & 0x1f;

    eeprom_cs = (value >> 6) & 1;
    eeprom_data = (value >> 4) & 1;
    eeprom_clock = (value >> 5) & 1;
    m93c86_write_select((uint8_t)eeprom_cs);
    if (eeprom_cs) {
        m93c86_write_data((uint8_t)(eeprom_data));
        m93c86_write_clock((uint8_t)(eeprom_clock));
    }
}

/* ---------------------------------------------------------------------*/

uint8_t c128gmod2_roml_read(uint16_t addr)
{
    return flash040core_read(flashrom_state, (addr & 0x3fff) + (c128gmod2_bank << 14));
}

void c128gmod2_roml_store(uint16_t addr, uint8_t value)
{
    flash040core_store(flashrom_state, (addr & 0x3fff) + (c128gmod2_bank << 14), value);
    if (flashrom_state->flash_state != FLASH040_STATE_READ) {
        maincpu_resync_limits();
    }
}

/* ---------------------------------------------------------------------*/

static int c128gmod2_dump(void)
{
    /* FIXME: incomplete */
    mon_out("ROM bank: %d\n", c128gmod2_bank);
    mon_out("EEPROM CS: %d data: %d clock: %d\n", eeprom_cs, eeprom_data, eeprom_clock);

    return 0;
}

/* ---------------------------------------------------------------------*/

void c128gmod2_config_init(void)
{
    eeprom_cs = 0;
    m93c86_write_select((uint8_t)eeprom_cs);
    flash040core_reset(flashrom_state);
}

void c128gmod2_reset(void)
{
    eeprom_cs = 0;
    m93c86_write_select((uint8_t)eeprom_cs);
    if (flashrom_state) {
        /* on the real hardware pressing reset would NOT reset the flash statemachine,
        only a powercycle would help. we do it here anyway :)
        */
        flash040core_reset(flashrom_state);
    }
}

void c128gmod2_config_setup(uint8_t *rawcart)
{
    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_NORMAL, ext_function_rom);
    memcpy(flashrom_state->flash_data, rawcart, GMOD2_FLASH_SIZE);
}

/* ---------------------------------------------------------------------*/

static int set_c128gmod2_eeprom_filename(const char *name, void *param)
{
    if ((c128gmod2_eeprom_filename != NULL) && (name != NULL) && (strcmp(name, c128gmod2_eeprom_filename) == 0)) {
        return 0;
    }

    if ((name != NULL) && (*name != '\0')) {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    util_string_set(&c128gmod2_eeprom_filename, name);

    if (c128gmod2_enabled) {
        return m93c86_open_image(c128gmod2_eeprom_filename, c128gmod2_eeprom_rw);
    }

    return 0;
}

static int set_c128gmod2_eeprom_rw(int val, void* param)
{
    c128gmod2_eeprom_rw = val ? 1 : 0;
    m93c86_set_image_rw(c128gmod2_eeprom_rw);
    return 0;
}

static int set_c128gmod2_flash_write(int val, void *param)
{
    c128gmod2_flash_write = val ? 1 : 0;

    return 0;
}

static const resource_string_t resources_string[] = {
    { "GMod128EEPROMImage", "", RES_EVENT_NO, NULL,
      &c128gmod2_eeprom_filename, set_c128gmod2_eeprom_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "GMod128FlashWrite", 0, RES_EVENT_NO, NULL,
      &c128gmod2_flash_write, set_c128gmod2_flash_write, NULL },
    { "GMod128EEPROMRW", 1, RES_EVENT_NO, NULL,
      &c128gmod2_eeprom_rw, set_c128gmod2_eeprom_rw, NULL },
    RESOURCE_INT_LIST_END
};

int c128gmod2_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void c128gmod2_resources_shutdown(void)
{
    lib_free(c128gmod2_eeprom_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-gmod128eepromimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "GMod128EEPROMImage", NULL,
      "<filename>", "Specify " CARTRIDGE_C128_NAME_GMOD2C128 " EEPROM image filename" },
    { "-gmod128eepromrw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod128EEPROMRW", (resource_value_t)1,
      NULL, "Enable writes to " CARTRIDGE_C128_NAME_GMOD2C128 " EEPROM image" },
    { "+gmod128eepromrw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod128EEPROMRW", (resource_value_t)0,
      NULL, "Disable writes to " CARTRIDGE_C128_NAME_GMOD2C128 " EEPROM image" },
    { "-gmod128flashwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod128FlashWrite", (resource_value_t)1,
      NULL, "Enable saving of the " CARTRIDGE_C128_NAME_GMOD2C128 " ROM at exit" },
    { "+gmod128flashwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod128FlashWrite", (resource_value_t)0,
      NULL, "Disable saving of the " CARTRIDGE_C128_NAME_GMOD2C128 " ROM at exit" },
    CMDLINE_LIST_END
};

int c128gmod2_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int c128gmod2_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    c128gmod2_io1_list_item = io_source_register(&c128gmod2_io1_device);
    m93c86_open_image(c128gmod2_eeprom_filename, c128gmod2_eeprom_rw);

    c128gmod2_enabled = 1;

    return 0;
}

int c128gmod2_bin_attach(const char *filename, uint8_t *rawcart)
{
    c128gmod2_filetype = 0;
    c128gmod2_filename = NULL;

    if (util_file_load(filename, rawcart, GMOD2_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    c128gmod2_filetype = CARTRIDGE_FILETYPE_BIN;
    c128gmod2_filename = lib_strdup(filename);
    return c128gmod2_common_attach();
}

int c128gmod2_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    memset(rawcart, 0xff, GMOD2_FLASH_SIZE);

    c128gmod2_filetype = 0;
    c128gmod2_filename = NULL;

    for (i = 0; i <= (GMOD2_MAX_BANKS - 1); i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > (GMOD2_MAX_BANKS - 1) || chip.size != GMOD2_BANK_SIZE) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    c128gmod2_filetype = CARTRIDGE_FILETYPE_CRT;
    c128gmod2_filename = lib_strdup(filename);

    return c128gmod2_common_attach();
}

int c128gmod2_bin_save(const char *filename)
{
    FILE *fd;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (fwrite(ext_function_rom, 1, GMOD2_FLASH_SIZE, fd) != GMOD2_FLASH_SIZE) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    return 0;
}

int c128gmod2_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    uint8_t *data;
    int i;

    fd = crt_create(filename, CARTRIDGE_C128_GMOD2C128, 1, 0, STRING_GMOD2);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = GMOD2_BANK_SIZE;
    chip.start = 0x8000;

    data = ext_function_rom;

    for (i = 0; i < GMOD2_MAX_BANKS; i++) {
        chip.bank = i; /* bank */

        if (crt_write_chip(data, &chip, fd)) {
            fclose(fd);
            return -1;
        }
        data += GMOD2_BANK_SIZE;
    }

    fclose(fd);
    return 0;
}

int c128gmod2_flush_image(void)
{
    if (c128gmod2_filetype == CARTRIDGE_FILETYPE_BIN) {
        return c128gmod2_bin_save(c128gmod2_filename);
    } else if (c128gmod2_filetype == CARTRIDGE_FILETYPE_CRT) {
        return c128gmod2_crt_save(c128gmod2_filename);
    }
    return -1;
}

int c128gmod2_can_save_eeprom(void)
{
    return 1;
}

int c128gmod2_can_flush_eeprom(void)
{
    if ((c128gmod2_eeprom_filename != NULL) && (*c128gmod2_eeprom_filename != 0)) {
        return 1;
    }
    return 0;
}

/** \brief  Save a copy of the GMod2 EEPROM image to a file
 *
 * \param[in]   filename    filename for EEPROM image copy
 *
 * \return  0 on success, -1 on failre
 */
int c128gmod2_eeprom_save(const char *filename)
{
    return m93c86_save_image(filename);
}

/** \brief  FLush current contents of the GMod2 EEPROM to file
 *
 * \return  0 on success, -1 on failure
 */
int c128gmod2_flush_eeprom(void)
{
    return m93c86_flush_image();
}

void c128gmod2_detach(void)
{
    if (c128gmod2_flash_write && flashrom_state->flash_dirty) {
        c128gmod2_flush_image();
    }

    if (flashrom_state) {
        flash040core_shutdown(flashrom_state);
        lib_free(flashrom_state);
        flashrom_state = NULL;
    }
    if (c128gmod2_filename) {
        lib_free(c128gmod2_filename);
        c128gmod2_filename = NULL;
    }
    m93c86_close_image(c128gmod2_eeprom_rw);

    if (c128gmod2_io1_list_item) {
        io_source_unregister(c128gmod2_io1_list_item);
        c128gmod2_io1_list_item = NULL;
    }
    export_remove(&export_res);

    c128gmod2_enabled = 0;
}

/* ---------------------------------------------------------------------*/

static const char snap_module_name[] = "CARTGMOD2C128";
static const char flash_snap_module_name[] = "FLASH040GMOD2";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int c128gmod2_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)c128gmod2_bank) < 0
        || SMW_BA(m, flashrom_state->flash_data, GMOD2_FLASH_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (m93c86_snapshot_write_module(s) < 0) {
        return -1;
    }

    return flash040core_snapshot_write_module(s, flashrom_state, flash_snap_module_name);
}

int c128gmod2_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* reject snapshot modules newer than what we can handle (this VICE is too old) */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* reject snapshot modules older than what we can handle (the snapshot is too old) */
    if (snapshot_version_is_smaller(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_INCOMPATIBLE);
        goto fail;
    }

    if (0
        || SMR_B_INT(m, &c128gmod2_bank) < 0
        || SMR_BA(m, ext_function_rom, GMOD2_FLASH_SIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    if (m93c86_snapshot_read_module(s) < 0) {
        return -1;
    }

    flashrom_state = lib_malloc(sizeof(flash040_context_t));
    flash040core_init(flashrom_state, maincpu_alarm_context, FLASH040_TYPE_NORMAL, ext_function_rom);

    if (flash040core_snapshot_read_module(s, flashrom_state, flash_snap_module_name) < 0) {
        flash040core_shutdown(flashrom_state);
        lib_free(flashrom_state);
        flashrom_state = NULL;
        return -1;
    }

    c128gmod2_common_attach();

    /* set filetype to none */
    c128gmod2_filename = NULL;
    c128gmod2_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
