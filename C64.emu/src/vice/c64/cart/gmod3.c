/*
 * gmod3.c - Cartridge handling, GMod3 cart.
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
#include "c64pla.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "export.h"
#include "lib.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "spi-flash.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "gmod3.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    GMod3 (Individual Computers)

    2/4/8/16Mb serial Flash

    see http://wiki.icomp.de/wiki/GMod3

    IO1

    on write:

    $DE00   Bank    0- 255       all versions
    $DE01   Bank  256- 511   4MB and higher versions only
    $DE02   Bank  512- 767   8MB and higher versions only
    $DE03   Bank  768-1023   8MB and higher versions only
    $DE04   Bank 1024-1279  16MB version only
    $DE05   Bank 1280-1535  16MB version only
    $DE06   Bank 1536-1791  16MB version only
    $DE07   Bank 1792-2047  16MB version only

    the upper 3 bits of the bank are determined from the address, ie A0-A2

    $DE08   bit7    1: bitbang mode enabled
            bit6    exrom
            bit5    hw vector replacing enabled

    on read:

    $DE00...$DE07   bit 0-7   - lower 8 bits of the current bank
    $DE08...$DE0F   bit 0,1,2 - upper 3 bits of the current bank

    if bitbang mode is enabled:

    on write:

    $DE00   bit6    Flash CS
            bit5    Flash clock
            bit4    Flash Din

    on read:

    $DExx   bit7    Flash Dout
*/

/* #define DEBUGGMOD3 */

#ifdef DEBUGGMOD3
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define GMOD3_2MB_FLASH_SIZE (2*1024*1024)
#define GMOD3_4MB_FLASH_SIZE (4*1024*1024)
#define GMOD3_8MB_FLASH_SIZE (8*1024*1024)
#define GMOD3_16MB_FLASH_SIZE (16*1024*1024)

static uint8_t gmod3_rom[GMOD3_16MB_FLASH_SIZE];    /* FIXME, should not be static */
static uint32_t gmod3_flashsize = 0;

static int gmod3_enabled = 0;

static int gmod3_bitbang_enabled = 0;
static int gmod3_vectors_enabled = 0;

/* current GAME/EXROM mode */
static int gmod3_cmode = CMODE_8KGAME;

/* current bank */
static int gmod3_bank = 0;

static char *gmod3_filename = NULL;
static int gmod3_filetype = 0;

static char *gmod3_flash_filename = NULL;
static int gmod3_flash_write = 1;

static int eeprom_cs = 1; /* active low */
static int eeprom_data_in = 0;
static int eeprom_data_out = 0;
static int eeprom_clock = 0;

static const char STRING_GMOD3[] = CARTRIDGE_NAME_GMOD3;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static uint8_t gmod3_io1_read(uint16_t addr);
static uint8_t gmod3_io1_peek(uint16_t addr);
static void gmod3_io1_store(uint16_t addr, uint8_t value);
static int gmod3_dump(void);

static io_source_t gmod3_io1_device = {
    CARTRIDGE_NAME_GMOD3,  /* name of the device */
    IO_DETACH_CART,        /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE, /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,  /* range for the device, address is ignored, reg:$df00, mirrors:$de01-$deff */
    0,                     /* read validity is determined by the device upon a read */
    gmod3_io1_store,       /* store function */
    NULL,                  /* NO poke function */
    gmod3_io1_read,        /* read function */
    gmod3_io1_peek,        /* peek function */
    gmod3_dump,            /* device state information dump function */
    CARTRIDGE_GMOD3,       /* cartridge ID */
    IO_PRIO_NORMAL,        /* normal priority, device read needs to be checked for collisions */
    0                      /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *gmod3_io1_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_GMOD3, 1, 1, &gmod3_io1_device, NULL, CARTRIDGE_GMOD3
};

/* ---------------------------------------------------------------------*/

uint8_t gmod3_io1_read(uint16_t addr)
{
    gmod3_io1_device.io_source_valid = 0;
    /* DBG(("io1 r %04x (cs:%d)\n", addr, eeprom_cs)); */

    if (gmod3_bitbang_enabled) {
        if (eeprom_cs == 0) { /* active low */
            gmod3_io1_device.io_source_valid = 1;
            eeprom_data_out = spi_flash_read_data() << 7;
            return eeprom_data_out;
        }
    } else {
        if (/*(addr >= 0x00) &&*/ (addr <= 0x07)) {
            gmod3_io1_device.io_source_valid = 1;
            return gmod3_bank & 0xff;
        } else if ((addr >= 0x08) && (addr <= 0x0f)) {
            gmod3_io1_device.io_source_valid = 1;
            return (gmod3_bank & 0x0700) >> 8;
        }
    }
    return 0; /* FIXME */
}

uint8_t gmod3_io1_peek(uint16_t addr)
{
    if (gmod3_bitbang_enabled) {
        if (eeprom_cs == 0) { /* active low */
            return eeprom_data_out;
        }
    } else {
        if (/*(addr >= 0x00) &&*/ (addr <= 0x07)) {
            return gmod3_bank & 0xff;
        } else if ((addr >= 0x08) && (addr <= 0x0f)) {
            return (gmod3_bank & 0x0700) >> 8;
        }
    }
    return 0;
}

void gmod3_io1_store(uint16_t addr, uint8_t value)
{
    int mode = CMODE_WRITE;

    DBG(("io1 w %04x %02x\n", addr, value));

    addr &= 0xff;

    /* banking */
    if (/*(addr >= 0x00) &&*/ (addr <= 0x07)) {
        if (gmod3_bitbang_enabled) {
            eeprom_cs = ((value >> 6) & 1);   /* active low */
            eeprom_clock = (value >> 5) & 1;
            eeprom_data_in = (value >> 4) & 1;

            DBG(("io1 w %04x %02x (cs:%d data:%d clock:%d)\n",
                addr, value, eeprom_cs, eeprom_data_in, eeprom_clock));
        } else {
            gmod3_bank = value + ((addr & 0x07) << 8);
            DBG(("io1 w %04x %02x (bank: %d)\n",
                addr, value, gmod3_bank));
        }
    } else if (addr == 0x08) {
        gmod3_bitbang_enabled = (value >> 7) & 1;
        gmod3_vectors_enabled = (value >> 5) & 1;
        if ((value & 0x40) == 0x00) {
            if (gmod3_vectors_enabled) {
                gmod3_cmode = CMODE_ULTIMAX;
            } else {
                gmod3_cmode = CMODE_8KGAME;
            }
        } else if ((value & 0x40) == 0x40) {
            gmod3_cmode = CMODE_RAM;
        }
        DBG(("io1 w %04x %02x (bitbang: %d vectors: %d mode: %d)\n",
            addr, value, gmod3_bitbang_enabled, gmod3_vectors_enabled, gmod3_cmode));
    }

    spi_flash_write_select((uint8_t)eeprom_cs);
    if (eeprom_cs == 0) { /* active low */
        spi_flash_write_data((uint8_t)(eeprom_data_in));
        spi_flash_write_clock((uint8_t)(eeprom_clock));
    }
    cart_config_changed_slotmain(gmod3_cmode,
        (uint8_t)(gmod3_cmode | (gmod3_bank << CMODE_BANK_SHIFT)), mode);
}

/* ---------------------------------------------------------------------*/

uint8_t gmod3_roml_read(uint16_t addr)
{
    int mem_config = ((~pport.dir | pport.data) & 0x7);
    if (!gmod3_vectors_enabled) {
        return gmod3_rom[(addr & 0x1fff) + (gmod3_bank << 13)];
    }
    /* handle fake ultimax */
    if ((mem_config == 7) || (mem_config == 3)) {
        return gmod3_rom[(addr & 0x1fff) + (gmod3_bank << 13)];
    }
    return ram_read(addr);
}

static uint8_t vectors[8] = { 0x08, 0x00, 0x08, 0x00, 0x0c, 0x80, 0x0c, 0x00 };

uint8_t gmod3_romh_read(uint16_t addr)
{
    DBG(("gmod3_romh_read %04x\n", addr));
    if (addr >= 0xfff8 /*&& addr <= 0xffff*/) {
        return vectors[addr & 7];
    }
    return mem_read_without_ultimax(addr);
}

/* VIC reads */
int gmod3_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    return CART_READ_C64MEM;
}
/* CPU reads */
int gmod3_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    return CART_READ_C64MEM;
}

int gmod3_peek_mem(export_t *ex, uint16_t addr, uint8_t *value)
{
    if (addr >= 0x8000 && addr <= 0x9fff) {
        *value = gmod3_roml_read(addr);
        return CART_READ_VALID;
    }
    if (gmod3_vectors_enabled) {
        if (addr >= 0xfff8 /*&& addr <= 0xffff*/) {
            *value = gmod3_romh_read(addr);
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

void gmod3_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    /* TODO */
    *base = NULL;
    *start = 0;
    *limit = 0;
}

/* ---------------------------------------------------------------------*/

static int gmod3_dump(void)
{
    /* FIXME: incomplete */
    mon_out("status: %s\n", gmod3_cmode == CMODE_RAM ? "disabled" : "8k Game");
    mon_out("ROM bank: %d\n", gmod3_bank);
    mon_out("bitbang mode is %s\n", gmod3_bitbang_enabled ? "enabled" : "disabled");
    mon_out("hw vectors are %s\n", gmod3_vectors_enabled ? "enabled" : "disabled");
    mon_out("EEPROM CS: %d clock: %d data from flash: %d data to flash: %d \n",
            eeprom_cs, eeprom_clock, eeprom_data_out, eeprom_data_in);

    return 0;
}

/* ---------------------------------------------------------------------*/

void gmod3_config_init(void)
{
    gmod3_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((uint8_t)gmod3_cmode, (uint8_t)gmod3_cmode, CMODE_READ);
    eeprom_cs = 1; /* active low */
    spi_flash_write_select((uint8_t)eeprom_cs);
}

void gmod3_reset(void)
{
    gmod3_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((uint8_t)gmod3_cmode, (uint8_t)gmod3_cmode, CMODE_READ);
    eeprom_cs = 1; /* active low */
    spi_flash_write_select((uint8_t)eeprom_cs);
    gmod3_bitbang_enabled = 0;
    gmod3_vectors_enabled = 0;  /* FIXME: this can be enabled at reset as a factory option */
    gmod3_bank = 0;
}

void gmod3_config_setup(uint8_t *rawcart)
{
    gmod3_cmode = CMODE_8KGAME;
    cart_config_changed_slotmain((uint8_t)gmod3_cmode, (uint8_t)gmod3_cmode, CMODE_READ);

    spi_flash_set_image(gmod3_rom, gmod3_flashsize);
    memcpy(gmod3_rom, rawcart, GMOD3_16MB_FLASH_SIZE);
}

/* ---------------------------------------------------------------------*/

static int set_gmod3_flash_write(int val, void *param)
{
    gmod3_flash_write = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "GMod3FlashWrite", 1, RES_EVENT_NO, NULL,
      &gmod3_flash_write, set_gmod3_flash_write, NULL },
    RESOURCE_INT_LIST_END
};

int gmod3_resources_init(void)
{
    return resources_register_int(resources_int);
}

void gmod3_resources_shutdown(void)
{
    lib_free(gmod3_flash_filename);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-gmod3flashwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod3FlashWrite", (resource_value_t)1,
      NULL, "Enable saving of the GMod3 ROM at exit" },
    { "+gmod3flashwrite", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "GMod3FlashWrite", (resource_value_t)0,
      NULL, "Disable saving of the GMod3 ROM at exit" },
    CMDLINE_LIST_END
};

int gmod3_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int gmod3_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    gmod3_io1_list_item = io_source_register(&gmod3_io1_device);

    gmod3_enabled = 1;

    return 0;
}

int gmod3_bin_attach(const char *filename, uint8_t *rawcart)
{
    gmod3_filetype = 0;
    gmod3_filename = NULL;
    gmod3_flashsize = 0;
    memset(rawcart, 0xff, GMOD3_16MB_FLASH_SIZE);

    if (util_file_load(filename, rawcart, GMOD3_16MB_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        if (util_file_load(filename, rawcart, GMOD3_8MB_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            if (util_file_load(filename, rawcart, GMOD3_4MB_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                if (util_file_load(filename, rawcart, GMOD3_2MB_FLASH_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
                    return -1;
                } else {
                    gmod3_flashsize = GMOD3_2MB_FLASH_SIZE;
                }
            } else {
                gmod3_flashsize = GMOD3_4MB_FLASH_SIZE;
            }
        } else {
            gmod3_flashsize = GMOD3_8MB_FLASH_SIZE;
        }
    } else {
        gmod3_flashsize = GMOD3_16MB_FLASH_SIZE;
    }

    gmod3_filetype = CARTRIDGE_FILETYPE_BIN;
    gmod3_filename = lib_strdup(filename);
    return gmod3_common_attach();
}

int gmod3_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    gmod3_filetype = 0;
    gmod3_filename = NULL;
    gmod3_flashsize = 0;
    memset(rawcart, 0xff, GMOD3_16MB_FLASH_SIZE);

    for (i = 0; i < (GMOD3_16MB_FLASH_SIZE / 0x2000); i++) { /* FIXME */
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank >= (GMOD3_16MB_FLASH_SIZE / 0x2000) || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    i *= 0x2000;
    if ((i != GMOD3_16MB_FLASH_SIZE) &&
        (i != GMOD3_8MB_FLASH_SIZE) &&
        (i != GMOD3_4MB_FLASH_SIZE) &&
        (i != GMOD3_2MB_FLASH_SIZE)) {
        return -1;
    }

    gmod3_flashsize = i;
    gmod3_filetype = CARTRIDGE_FILETYPE_CRT;
    gmod3_filename = lib_strdup(filename);

    return gmod3_common_attach();
}

int gmod3_bin_save(const char *filename)
{
    FILE *fd;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    if (fwrite(gmod3_rom, 1, gmod3_flashsize, fd) != gmod3_flashsize) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    return 0;
}

int gmod3_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;
    uint8_t *data;
    int i;

    fd = crt_create(filename, CARTRIDGE_GMOD3, 1, 0, STRING_GMOD3);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;

    data = gmod3_rom;

    for (i = 0; i < (gmod3_flashsize / 0x2000); i++) {
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

int gmod3_flush_image(void)
{
    if (gmod3_filetype == CARTRIDGE_FILETYPE_BIN) {
        return gmod3_bin_save(gmod3_filename);
    } else if (gmod3_filetype == CARTRIDGE_FILETYPE_CRT) {
        return gmod3_crt_save(gmod3_filename);
    }
    return -1;
}

void gmod3_detach(void)
{
    if (gmod3_flash_write /* && flashrom_state->flash_dirty */) {
        gmod3_flush_image();
    }

    lib_free(gmod3_filename);
    gmod3_filename = NULL;

    export_remove(&export_res);
    io_source_unregister(gmod3_io1_list_item);
    gmod3_io1_list_item = NULL;

    gmod3_enabled = 0;
}

/* ---------------------------------------------------------------------*/

static const char snap_module_name[] = "CARTGMOD3";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int gmod3_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)gmod3_cmode) < 0
        || SMW_B(m, (uint8_t)gmod3_bank) < 0
        || SMW_BA(m, gmod3_rom, GMOD3_16MB_FLASH_SIZE) < 0
    ) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return spi_flash_snapshot_write_module(s);
}

int gmod3_snapshot_read_module(snapshot_t *s)
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
        || SMR_B_INT(m, &gmod3_cmode) < 0
        || SMR_B_INT(m, &gmod3_bank) < 0
        || SMR_BA(m, gmod3_rom, GMOD3_16MB_FLASH_SIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    if (spi_flash_snapshot_read_module(s) < 0) {
        return -1;
    }

    gmod3_common_attach();

    /* set filetype to none */
    gmod3_filename = NULL;
    gmod3_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
