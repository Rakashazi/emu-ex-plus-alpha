/*
 * rrnetmk3.c - Cartridge handling, RR-Net MK3 cart.
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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

#ifdef HAVE_PCAP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "c64cart.h" /* for export_t */
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "crt.h"
#include "cs8900io.h"
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

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "rrnetmk3.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    RR-Net MK3 (Individual Computers)

    - 8K ROM (EEPROM, writeable in flash mode)
    - CS8900a in de00-de0f range (similar to other - clockport - devices)

    - one "register":
      - a write to $de80 enables the ROM
      - a write to $de88 disables the ROM
*/

/* #define RRNETMK3DEBUG */
 
#ifdef RRNETMK3DEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

/* RRNETMK3 enable */
static int rrnetmk3_enabled;

/* RRNETMK3 bios writable */
static int rrnetmk3_bios_write;

/* Bios file name */
static char *rrnetmk3_bios_filename = NULL;

/* BIOS changed flag */
static int rrnetmk3_bios_changed = 0;

static int rrnetmk3_hw_flashjumper = 0; /* status of the flash jumper */

static BYTE rrnetmk3_biossel = 0; /* 0 = ROM is mapped */

static log_t rrnetmk3_log = LOG_ERR;

static BYTE rrnetmk3_bios[0x2002];
static int rrnetmk3_bios_offset = 0;
static int rrnetmk3_bios_type = 0;

static const char STRING_RRNETMK3[] = CARTRIDGE_NAME_RRNETMK3;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void rrnetmk3_io1_store(WORD addr, BYTE value);
static BYTE rrnetmk3_io1_peek(WORD addr);
static int rrnetmk3_dump(void);

static BYTE rrnetmk3_cs8900_read(WORD io_address);
static BYTE rrnetmk3_cs8900_peek(WORD io_address);
static void rrnetmk3_cs8900_store(WORD io_address, BYTE byte);

static io_source_t rrnetmk3_io1_device = {
    CARTRIDGE_NAME_RRNETMK3,
    IO_DETACH_RESOURCE,
    "RRNETMK3",
    0xde80, 0xde88, 0x88, /* FIXME */
    0,
    rrnetmk3_io1_store,
    NULL, /* read */
    rrnetmk3_io1_peek,
    rrnetmk3_dump,
    CARTRIDGE_RRNETMK3,
    0,
    0
};

static io_source_t rrnetmk3_cs8900_io1_device = {
    CARTRIDGE_NAME_RRNETMK3,
    IO_DETACH_RESOURCE,
    "RRNETMK3",
    0xde02, 0xde0f, 0x0f,
    0,
    rrnetmk3_cs8900_store,
    rrnetmk3_cs8900_read,
    rrnetmk3_cs8900_peek,
    rrnetmk3_dump,
    CARTRIDGE_RRNETMK3,
    0,
    0
};

static io_source_list_t *rrnetmk3_io1_list_item = NULL;
static io_source_list_t *rrnetmk3_cs8900_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_RRNETMK3, 0, 1, &rrnetmk3_io1_device, NULL, CARTRIDGE_RRNETMK3
};

/* ---------------------------------------------------------------------*/

/* Resets the card */
void rrnetmk3_reset(void)
{
    rrnetmk3_biossel = rrnetmk3_hw_flashjumper; /* disable bios at reset when flash jumper is set */
    if (rrnetmk3_enabled) {
        cs8900io_reset();
    }
    cart_config_changed_slotmain(CMODE_RAM, (BYTE)(rrnetmk3_biossel ? CMODE_RAM : CMODE_8KGAME), CMODE_READ);
}

static int set_rrnetmk3_flashjumper(int val, void *param)
{
    rrnetmk3_hw_flashjumper = val ? 1 : 0;
    LOG(("RRNETMK3 Flashjumper: %d", rrnetmk3_hw_flashjumper));
    return 0;
}

static int set_rrnetmk3_bios_write(int val, void *param)
{
    rrnetmk3_bios_write = val ? 1 : 0;
    return 0;
}

/* ---------------------------------------------------------------------*/

int rrnetmk3_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit)
{
    if (!rrnetmk3_biossel) {
        switch (addr & 0xf000) {
            case 0x9000:
            case 0x8000:
                *base = &rrnetmk3_bios[rrnetmk3_bios_offset] - 0x8000;
                *start = 0x8000;
                *limit = 0x9ffd;
                return CART_READ_VALID;
            default:
                break;
        }
    }
    return CART_READ_THROUGH;
}

void rrnetmk3_config_init(void)
{
    LOG(("RRNETMK3 rrnetmk3_config_init"));
    rrnetmk3_biossel = rrnetmk3_hw_flashjumper; /* disable bios at reset when flash jumper is set */
    cart_config_changed_slotmain(CMODE_RAM, (BYTE)(rrnetmk3_biossel ? CMODE_RAM : CMODE_8KGAME), CMODE_READ);
}

static void rrnetmk3_io1_store(WORD addr, BYTE value)
{
    LOG(("RRNETMK3: IO1 ST %04x %02x", addr, value));
    switch (addr) {
        case 0x80:      /* ROM_ENABLE */
            rrnetmk3_biossel = 0;
            cart_config_changed_slotmain(CMODE_RAM, CMODE_8KGAME, CMODE_READ);
            break;
        case 0x88:      /* ROM_DISABLE */
            rrnetmk3_biossel = 1;
            cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ);
            break;
        default:      /* Not for us */
            return;
    }
}

static BYTE rrnetmk3_io1_peek(WORD addr)
{
    switch (addr) {
        case 0x80:      /* ROM_ENABLE */
            return rrnetmk3_biossel;
        case 0x88:      /* ROM_DISABLE */
            return rrnetmk3_biossel;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static BYTE rrnetmk3_cs8900_read(WORD address)
{
    if (address < 0x02) {
        rrnetmk3_cs8900_io1_device.io_source_valid = 0;
        return 0;
    }
    rrnetmk3_cs8900_io1_device.io_source_valid = 1;
    if (address > 0x0b) {
        return rrnetmk3_bios[(0x1ff0 + address) + rrnetmk3_bios_offset];
    }
    address ^= 0x08;
    return cs8900io_read(address);
}

static BYTE rrnetmk3_cs8900_peek(WORD address)
{
    if (address < 0x02) {
        return 0;
    }
    if (address > 0x0b) {
        return rrnetmk3_bios[(0x1ff0 + address) + rrnetmk3_bios_offset];
    }
    address ^= 0x08;

    return cs8900io_read(address);
}

static void rrnetmk3_cs8900_store(WORD address, BYTE byte)
{
    if (address < 0x02) {
        return;
    }
    address ^= 0x08;

    cs8900io_store(address, byte);
}

/* ---------------------------------------------------------------------*/

static int rrnetmk3_dump(void)
{
    mon_out("Flashmode jumper is %s.\n", rrnetmk3_hw_flashjumper ? "set" : "not set");
    mon_out("ROM is %s.\n", rrnetmk3_biossel ? "not enabled" : "enabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

int rrnetmk3_roml_read(WORD addr)
{
    if (!rrnetmk3_biossel) {
        return rrnetmk3_bios[(addr & 0x1fff) + rrnetmk3_bios_offset];
    }
    return mem_ram[addr];
}

int rrnetmk3_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if ((addr >= 0x8000) && (addr <= 0x9fff)) {
        if (!rrnetmk3_biossel) {
            *value = rrnetmk3_bios[(addr & 0x1fff) + rrnetmk3_bios_offset];
            return CART_READ_VALID;
        }
    }
    return CART_READ_THROUGH;
}

int rrnetmk3_roml_store(WORD addr, BYTE byte)
{
    if (!rrnetmk3_biossel) {
        if (rrnetmk3_hw_flashjumper) {
            LOG(("RRNETMK3 Flash w %04x %02x", addr, byte));
            if (rrnetmk3_bios[(addr & 0x1fff) + rrnetmk3_bios_offset] != byte) {
                rrnetmk3_bios[(addr & 0x1fff) + rrnetmk3_bios_offset] = byte;
                rrnetmk3_bios_changed = 1;
            }
        }
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "RRNETMK3_flashjumper", 0, RES_EVENT_NO, NULL,
      &rrnetmk3_hw_flashjumper, set_rrnetmk3_flashjumper, NULL },
    { "RRNETMK3_bios_write", 0, RES_EVENT_NO, NULL,
      &rrnetmk3_bios_write, set_rrnetmk3_bios_write, NULL },
    RESOURCE_INT_LIST_END
};

int rrnetmk3_resources_init(void)
{
    return resources_register_int(resources_int);
}

void rrnetmk3_resources_shutdown(void)
{
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-rrnetmk3bioswrite", SET_RESOURCE, 0,
      NULL, NULL, "RRNETMK3_bios_write", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RRNETMK3_BIOS_WRITE,
      NULL, NULL },
    { "+rrnetmk3bioswrite", SET_RESOURCE, 0,
      NULL, NULL, "RRNETMK3_bios_write", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RRNETMK3_BIOS_READ_ONLY,
      NULL, NULL },
    { "-rrnetmk3flash", SET_RESOURCE, 0,
      NULL, NULL, "RRNETMK3_flashjumper", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RRNETMK3_SET_FLASH_JUMPER,
      NULL, NULL },
    { "+rrnetmk3flash", SET_RESOURCE, 0,
      NULL, NULL, "RRNETMK3_flashjumper", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_RRNETMK3_UNSET_FLASH_JUMPER,
      NULL, NULL },
    CMDLINE_LIST_END
};

int rrnetmk3_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void rrnetmk3_init(void)
{
    rrnetmk3_log = log_open("RRNETMK3");
    cs8900io_init();
}

void rrnetmk3_config_setup(BYTE *rawcart)
{
    memcpy(rrnetmk3_bios, rawcart, 0x2000 + rrnetmk3_bios_offset);
}

static int rrnetmk3_common_attach(void)
{
    LOG(("RRNETMK3: rrnetmk3_common_attach '%s'", rrnetmk3_bios_filename));
    cart_power_off();
    /* if the param is == NULL, then we should actually set the resource */
    if (export_add(&export_res) < 0) {
        LOG(("RRNETMK3: export did not register"));
        return -1;
    } else {
        LOG(("RRNETMK3: export registered"));
        if (cs8900io_enable(CARTRIDGE_NAME_RRNETMK3) < 0) {
            return -1;
        }
        rrnetmk3_bios_changed = 0;
        rrnetmk3_enabled = 1;
        cart_set_port_exrom_slotmain(1);
        cart_port_config_changed_slotmain();
        rrnetmk3_io1_list_item = io_source_register(&rrnetmk3_io1_device);
        rrnetmk3_cs8900_list_item = io_source_register(&rrnetmk3_cs8900_io1_device);
        rrnetmk3_reset();
    }
    return 0;
}

int rrnetmk3_bin_save(const char *filename)
{
    FILE *fd;
    int ret;

    if (filename == NULL) {
        return -1;
    }

    fd = fopen(filename, MODE_WRITE);
    if (fd == NULL) {
        return -1;
    }

    ret = fwrite(rrnetmk3_bios, 1, 0x2000 + rrnetmk3_bios_offset, fd);
    fclose(fd);
    if (ret != 0x2000 + rrnetmk3_bios_offset) {
        return -1;
    }
    rrnetmk3_bios_changed = 0;
    return 0;
}

int rrnetmk3_crt_save(const char *filename)
{
    FILE *fd;
    crt_chip_header_t chip;

    fd = crt_create(filename, CARTRIDGE_RRNETMK3, 1, 0, STRING_RRNETMK3);

    if (fd == NULL) {
        return -1;
    }

    chip.type = 2;
    chip.size = 0x2000;
    chip.start = 0x8000;
    chip.bank = 0;

    if (crt_write_chip(rrnetmk3_bios, &chip, fd)) {
        fclose(fd);
        return -1;
    }

    fclose(fd);
    return 0;
}

int rrnetmk3_bin_attach(const char *filename, BYTE *rawcart)
{
    int amount_read = 0;
    FILE *fd;

    fd = fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }

    amount_read = (int)fread(rawcart, 1, 0x2002, fd);
    fclose(fd);

    if (amount_read != 0x2000 && amount_read != 0x2002) {
        return -1;
    }

    rrnetmk3_bios_offset = amount_read & 3;
    rrnetmk3_bios_type = CARTRIDGE_FILETYPE_BIN;
    rrnetmk3_bios_filename = lib_stralloc(filename);
    return rrnetmk3_common_attach();
}

int rrnetmk3_crt_attach(FILE *fd, BYTE *rawcart, const char *filename)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.bank > 1 || chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    rrnetmk3_bios_offset = 0;
    rrnetmk3_bios_type = CARTRIDGE_FILETYPE_CRT;
    rrnetmk3_bios_filename = lib_stralloc(filename);
    return rrnetmk3_common_attach();
}

int rrnetmk3_flush_image(void)
{
    if (rrnetmk3_bios_type == CARTRIDGE_FILETYPE_BIN) {
        return rrnetmk3_bin_save(rrnetmk3_bios_filename);
    } else if (rrnetmk3_bios_type == CARTRIDGE_FILETYPE_CRT) {
        return rrnetmk3_crt_save(rrnetmk3_bios_filename);
    }
    return -1;
}

void rrnetmk3_detach(void)
{
    /* flush_image */
    if (rrnetmk3_bios_changed && rrnetmk3_bios_write) {
        rrnetmk3_flush_image();
    }
    cart_power_off();
    export_remove(&export_res);
#ifdef HAVE_FTE
    cs8900io_disable();
#endif
    rrnetmk3_enabled = 0;
    cart_set_port_exrom_slotmain(0);
    cart_port_config_changed_slotmain();
    io_source_unregister(rrnetmk3_io1_list_item);
    rrnetmk3_io1_list_item = NULL;
    io_source_unregister(rrnetmk3_cs8900_list_item);
    rrnetmk3_cs8900_list_item = NULL;
    lib_free(rrnetmk3_bios_filename);
    rrnetmk3_bios_filename = NULL;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTRRNETMK3"

/* FIXME: implement snapshot support */
int rrnetmk3_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int rrnetmk3_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
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

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (rrnetmk3_common_attach() < 0) {
        return -1;
    }
    return 0;
#endif
}

#endif

