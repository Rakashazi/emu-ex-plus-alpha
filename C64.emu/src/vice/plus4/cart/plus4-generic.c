
/*
 * plus4cart.c -- Plus4 generic cartridge handling.
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

#define DEBUGGENERIC
/* #define DEBUGGENERICRW */

#include "vice.h"

#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "cartio.h"
#include "cmdline.h"
#include "crt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "resources.h"
#include "snapshot.h"
#include "sysfile.h"
#include "util.h"

#include "plus4-generic.h"

#ifdef DEBUGGENERIC
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#ifdef DEBUGGENERICRW
#define DBGRW(x)  log_debug x
#else
#define DBGRW(x)
#endif

/* FIXME: get rid of this ugly hack */
extern int plus4_rom_loaded;

static int generic_type = 0;
static int generic_filetype = 0;

/* Name of the external cartridge ROMs.  */
static char *c1lo_rom_name = NULL;
static char *c1hi_rom_name = NULL;

/* FIXME: allocate dynamically */
uint8_t extromlo2[PLUS4_C1LO_ROM_SIZE];
uint8_t extromhi2[PLUS4_C1HI_ROM_SIZE];

uint8_t generic_c1lo_read(uint16_t addr)
{
    DBGRW(("generic_c1lo_read %04x %02x", addr, extromlo2[addr & 0x3fff]));
    return extromlo2[addr & 0x3fff];
}

uint8_t generic_c1hi_read(uint16_t addr)
{
    DBGRW(("generic_c1hi_read %04x %02x", addr, extromhi2[addr & 0x3fff]));
    return extromhi2[addr & 0x3fff];
}

/*
    called by cartridge_attach_image after cart_crt/bin_attach
    XYZ_config_setup should copy the raw cart image into the
    individual implementations array.
*/

/* FIXME: this function must check the actual generic type and then
          update c1lo/c2hi accordingly */
void generic_config_setup(uint8_t *rawcart)
{
    DBG(("generic_config_setup"));
    if ((generic_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C1LO) {
        memcpy(extromlo2, rawcart, PLUS4_C1LO_ROM_SIZE);
        DBG(("generic_config_setup c1lo"));
    }
    if ((generic_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C1HI) {
        memcpy(extromhi2, rawcart + 0x4000, PLUS4_C1HI_ROM_SIZE);
        DBG(("generic_config_setup c1hi"));
    }
    if ((generic_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C2LO) {
        memcpy(extromlo3, rawcart + 0x8000, PLUS4_C2LO_ROM_SIZE);
        DBG(("generic_config_setup c2lo"));
    }
    if ((generic_type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) & CARTRIDGE_PLUS4_GENERIC_C2HI) {
        memcpy(extromhi3, rawcart + 0xc000, PLUS4_C2HI_ROM_SIZE);
        DBG(("generic_config_setup c2hi"));
    }
}

/* FIXME: alloc ROMs here */
static int generic_common_attach(void)
{
    DBG(("generic_common_attach (type :%04x)", (unsigned)generic_type));

    return generic_type;
}

/* since we also need to handle adding to existing carts, copy the old
   content to the new rawcart first */
static void prepare_rawcart(uint8_t *rawcart)
{
    memcpy(rawcart, extromlo2, PLUS4_C1LO_ROM_SIZE);
    memcpy(rawcart + 0x4000, extromhi2, PLUS4_C1HI_ROM_SIZE);
    memcpy(rawcart + 0x8000, extromlo3, PLUS4_C2LO_ROM_SIZE);
    memcpy(rawcart + 0xc000, extromhi3, PLUS4_C2HI_ROM_SIZE);
}

/* FIXME: handle mirroring of 4k/8k ROMs */
int generic_bin_attach(int type, const char *filename, uint8_t *rawcart)
{
    FILE *fd;
    int i;
    int offset = 0;

    DBG(("generic_bin_attach type: %04x", (unsigned)type));

    fd = fopen(filename, "rb");
    if (fd == NULL) {
        return -1;
    }

    prepare_rawcart(rawcart);

    /* get offset of first block */
    switch (type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK) {
        case CARTRIDGE_PLUS4_GENERIC_C1LO & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK:
            offset = 0;
            break;
        case CARTRIDGE_PLUS4_GENERIC_C1HI & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK:
            offset = 0x4000;
            break;
        case CARTRIDGE_PLUS4_GENERIC_C2LO & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK:
            offset = 0x8000;
            break;
        case CARTRIDGE_PLUS4_GENERIC_C2HI & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK:
            offset = 0xc000;
            break;
    }
    for (i = 0; i < 4; i++) {
        memset(&rawcart[offset], 0xff, PLUS4_CART16K_SIZE);
        if (fread(&rawcart[offset], 1, PLUS4_CART16K_SIZE, fd) < PLUS4_CART16K_SIZE) {
            break;
        }
        DBG(("loaded block %d offset %04x", i, (unsigned)offset));
        offset += PLUS4_CART16K_SIZE;
    }
    fclose (fd);
    /*return type;*/
    generic_type |= type;

    DBG(("generic_bin_attach generic_type: %04x", (unsigned)generic_type));

    return generic_common_attach();
}

/* FIXME: handle mirroring of 4k/8k ROMs */
int generic_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;
    int offset = 0, newtype = 0;

    DBG(("generic_crt_attach"));

    prepare_rawcart(rawcart);

    for (i = 0; i < 4; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        DBG(("bank: %d start: %04x size: %04x", chip.bank, chip.start, chip.size));

        if (chip.bank == 0) {
            /* c1 rom */
            if (chip.start == 0x8000) {
                /* c1lo */
                offset = 0;
                newtype = CARTRIDGE_PLUS4_GENERIC_C1LO;
            } else if (chip.start == 0xc000) {
                /* c1hi */
                offset = 0x4000;
                newtype = CARTRIDGE_PLUS4_GENERIC_C1HI;
            } else {
                return -1;
            }
        } else if (chip.bank == 1) {
            /* c2 rom */
            if (chip.start == 0x8000) {
                /* c2lo */
                offset = 0x8000;
                newtype = CARTRIDGE_PLUS4_GENERIC_C2LO;
            } else if (chip.start == 0xc000) {
                /* c2hi */
                offset = 0xc000;
                newtype = CARTRIDGE_PLUS4_GENERIC_C2HI;
            } else {
                return -1;
            }
        } else {
            return -1;
        }

        /* accept 4k,8k,16k banks */
        if (!((chip.size == 0x1000) || (chip.size == 0x2000) || (chip.size == 0x4000))) {
            return -1;
        }

        generic_type &= ~newtype;
        DBG(("offset: %04x size: %04x", (unsigned)offset, chip.size));
        memset(&rawcart[offset], 0xff, 0x4000);
        if (crt_read_chip(rawcart, offset, &chip, fd)) {
            return -1;
        }
        generic_type |= newtype;
        DBG(("generic_type: %04x", (unsigned)generic_type));
        DBG(("%02x %02x %02x %02x", rawcart[offset+0], rawcart[offset+1], rawcart[offset+2], rawcart[offset+3]));
    }

    return generic_common_attach();
}

/* c1lo is always external cartridge */
int plus4cart_load_c1lo(const char *rom_name)
{
#if 0

    if (!plus4_rom_loaded) {
        return 0;
    }
#endif
    DBG(("plus4cart_load_c1lo '%s'", rom_name));

    if ((rom_name == NULL) || (*rom_name == 0)) {
        return 0;
    }

    return cartridge_attach_image(CARTRIDGE_PLUS4_GENERIC_C1LO, rom_name);
}

/* c1hi is always external cartridge */
int plus4cart_load_c1hi(const char *rom_name)
{
#if 0
    if (!plus4_rom_loaded) {
        return 0;
    }
#endif
    DBG(("plus4cart_load_c1hi '%s'", rom_name));

    if ((rom_name == NULL) || (*rom_name == 0)) {
        return 0;
    }

    return cartridge_attach_image(CARTRIDGE_PLUS4_GENERIC_C1HI, rom_name);
}

void generic_detach(int type)
{
    DBG(("generic_detach type: '%04x'", (unsigned)type));
    if (type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK & CARTRIDGE_PLUS4_GENERIC_C1LO) {
        resources_set_string("c1loName", "");
        memset(extromlo2, 0xff, PLUS4_CART16K_SIZE);
    }
    if (type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK & CARTRIDGE_PLUS4_GENERIC_C1HI) {
        resources_set_string("c1hiName", "");
        memset(extromhi2, 0xff, PLUS4_CART16K_SIZE);
    }
    if (type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK & CARTRIDGE_PLUS4_GENERIC_C2LO) {
        resources_set_string("c2loName", "");
        memset(extromlo3, 0xff, PLUS4_CART16K_SIZE);
    }
    if (type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK & CARTRIDGE_PLUS4_GENERIC_C2HI) {
        resources_set_string("c2hiName", "");
        memset(extromhi3, 0xff, PLUS4_CART16K_SIZE);
    }

    generic_type &= ~(type & CARTRIDGE_PLUS4_GENERIC_TYPE_MASK);
}

#if 1
static int set_c1lo_rom_name(const char *val, void *param)
{
    DBG(("set_c1lo_rom_name '%s'", val));
    if (util_string_set(&c1lo_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_c1lo(c1lo_rom_name);
}

static int set_c1hi_rom_name(const char *val, void *param)
{
    DBG(("set_c1hi_rom_name '%s'", val));
    if (util_string_set(&c1hi_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_c1hi(c1hi_rom_name);
}

/* FIXME: this clashes with the general default cartridge name */
static const resource_string_t resources_string[] = {
    { "c1loName", "", RES_EVENT_NO, NULL,
      &c1lo_rom_name, set_c1lo_rom_name, NULL },
    { "c1hiName", "", RES_EVENT_NO, NULL,
      &c1hi_rom_name, set_c1hi_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};
#endif

int generic_resources_init(void)
{
#if 1
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
#endif
    /* return resources_register_int(resources_int); */
    return 0;
}

void generic_resources_shutdown(void)
{
    lib_free(c1lo_rom_name);
    lib_free(c1hi_rom_name);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-c1lo", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "c1loName", NULL,
      "<Name>", "Specify name of Cartridge 1 low ROM image" },
    { "-c1hi", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "c1hiName", NULL,
      "<Name>", "Specify name of Cartridge 1 high ROM image" },
    CMDLINE_LIST_END
};

int generic_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

/* CARTGENERIC snapshot module format:

   type  | name              | version | description
   -------------------------------------------------
   ARRAY | ROM C1LO          |   0.1+  | 16kiB of ROM data
   ARRAY | ROM C1HI          |   0.1+  | 16kiB of ROM data
 */

/* FIXME: since we cant actually make snapshots due to TED bugs, the following
          is completely untested */

static const char snap_module_name[] = "CARTGENERIC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int generic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    DBG(("generic_snapshot_write_module"));

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_BA(m, extromlo2, PLUS4_C1LO_ROM_SIZE)
        || SMW_BA(m, extromhi2, PLUS4_C1HI_ROM_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return 0;
}

int generic_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    DBG(("generic_snapshot_read_module"));

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
        || SMR_BA(m, extromlo2, PLUS4_C1LO_ROM_SIZE)
        || SMR_BA(m, extromhi2, PLUS4_C1HI_ROM_SIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    /* generic_common_attach(); */

    /* set filetype to none */
    generic_filetype = 0;

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
