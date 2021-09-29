/*
 * vic20-generic.c -- VIC20 generic cartridge emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Original individual file code by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

/* #define DEBUGCART */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "lib.h"
#include "log.h"
#include "mem.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "vic20-generic.h"
#include "zfile.h"

#ifdef DEBUGCART
#define DBG(x) printf x
#else
#define DBG(x)
#endif

#define TRY_RESOURCE_CARTFILE2 (1 << 8)
#define TRY_RESOURCE_CARTFILE4 (1 << 9)
#define TRY_RESOURCE_CARTFILE6 (1 << 10)
#define TRY_RESOURCE_CARTFILEA (1 << 11)
#define TRY_RESOURCE_CARTFILEB (1 << 12)

/* ------------------------------------------------------------------------- */

/* actual resources */
static char *cartridge_file_2 = NULL;
static char *cartridge_file_4 = NULL;
static char *cartridge_file_6 = NULL;
static char *cartridge_file_A = NULL;
static char *cartridge_file_B = NULL;

/* local shadow of some resources (e.g not yet set as default) */
/* filenames of separate binaries. */
static char *cartfile2 = NULL;
static char *cartfile4 = NULL;
static char *cartfile6 = NULL;
static char *cartfileA = NULL;
static char *cartfileB = NULL;

/* ------------------------------------------------------------------------- */

/*
 * Cartridge RAM
 *
 * Mapping
 *      RAM                 VIC20
 *   0x0000 - 0x1fff  ->  0xa000 - 0xbfff
 *   0x2000 - 0x7fff  ->  0x2000 - 0x7fff
 *   0x8400 - 0x8fff  ->  0x0400 - 0x0fff
 *
 */
#define CART_RAM_SIZE 0x9000
static uint8_t *cart_ram = NULL;

/*
 * Cartridge ROM
 *
 * Mapping
 *      ROM                 VIC20
 *   0x0000 - 0x1fff  ->  0xa000 - 0xbfff
 *   0x2000 - 0x7fff  ->  0x2000 - 0x7fff
 *   0x8400 - 0x8fff  ->  0x0400 - 0x0fff
 *
 */
#define CART_ROM_SIZE 0x9000
static uint8_t *cart_rom = NULL;

/* Cartridge States */
int generic_ram_blocks = 0;
int generic_rom_blocks = 0;


/* ------------------------------------------------------------------------- */

uint8_t generic_ram123_read(uint16_t addr)
{
    if (generic_ram_blocks & VIC_CART_RAM123) {
        return cart_ram[(addr & 0x0fff) | 0x8000];
    }
    return cart_rom[(addr & 0x0fff) | 0x8000];
}

void generic_ram123_store(uint16_t addr, uint8_t value)
{
    if (generic_ram_blocks & VIC_CART_RAM123) {
        cart_ram[(addr & 0x0fff) | 0x8000] = value;
    }
}

uint8_t generic_blk1_read(uint16_t addr)
{
    if (generic_ram_blocks & VIC_CART_BLK1) {
        return cart_ram[addr];
    }
    return cart_rom[addr];
}

void generic_blk1_store(uint16_t addr, uint8_t value)
{
    if (generic_ram_blocks & VIC_CART_BLK1) {
        cart_ram[addr] = value;
    }
}

uint8_t generic_blk2_read(uint16_t addr)
{
    if (generic_ram_blocks & VIC_CART_BLK2) {
        return cart_ram[addr];
    }
    return cart_rom[addr];
}

void generic_blk2_store(uint16_t addr, uint8_t value)
{
    if (generic_ram_blocks & VIC_CART_BLK2) {
        cart_ram[addr] = value;
    }
}

uint8_t generic_blk3_read(uint16_t addr)
{
    if (generic_ram_blocks & VIC_CART_BLK3) {
        return cart_ram[addr];
    }
    return cart_rom[addr];
}

void generic_blk3_store(uint16_t addr, uint8_t value)
{
    if (generic_ram_blocks & VIC_CART_BLK3) {
        cart_ram[addr] = value;
    }
}

uint8_t generic_blk5_read(uint16_t addr)
{
    if (generic_ram_blocks & VIC_CART_BLK5) {
        return cart_ram[addr & 0x1fff];
    }
    return cart_rom[addr & 0x1fff];
}

void generic_blk5_store(uint16_t addr, uint8_t value)
{
    if (generic_ram_blocks & VIC_CART_BLK5) {
        cart_ram[addr & 0x1fff] = value;
    }
}

/* ------------------------------------------------------------------------- */

void generic_init(void)
{
}

void generic_reset(void)
{
}

void generic_config_setup(uint8_t *rawcart)
{
}

/* ------------------------------------------------------------------------- */

/*
 * Those cartridge files are different from "normal" ROM images.
 * The VIC20 cartridges are saved with their start address before
 * the actual data.
 * This allows us to autodetect them etc.
 */
static int attach_image(int type, const char *filename)
{
    uint8_t rawcart[0x8000];
    FILE *fd;
    unsigned int addr;
    long len;
    size_t n;

    DBG(("attach_image type %d (%04x), file=`%s'.\n", type, type, filename));

    fd = zfile_fopen(filename, MODE_READ);
    if (!fd) {
        return -1;
    }

    /* check length and find out if there is a load address */
    fseek(fd, 0, SEEK_END);
    len = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    switch (len & 0xfff) {
        case 0: /* plain binary */
            addr = 0;
            break;
        case 2: /* load address */
            addr = fgetc(fd);
            addr = (addr & 0xff) | ((fgetc(fd) << 8) & 0xff00);
            len -= 2; /* remove load address from length */
            break;
        default: /* not a valid file */
            zfile_fclose(fd);
            DBG(("attach_image error (length check), len=%d.\n", len));
            return -1;
            break;
    }

    if (type == CARTRIDGE_VIC20_DETECT) {
        if (len == 0x8000) {
            /* 32K image fills all blocks */
            type = CARTRIDGE_VIC20_32KB_2000;
        } else if (len == 0x4000) {
            /* 16K image */
            if (addr == 0x2000) {
                type = CARTRIDGE_VIC20_16KB_2000;
            } else if (addr == 0x4000) {
                type = CARTRIDGE_VIC20_16KB_4000;
            } else if (addr == 0x6000) {
                type = CARTRIDGE_VIC20_16KB_6000;
            } else {
                log_error(LOG_DEFAULT, "could not determine type of cartridge.");
            }
        } else if (len == 0x2000) {
            /* 8K image */
            if (addr == 0x2000) {
                type = CARTRIDGE_VIC20_8KB_2000;
            } else if (addr == 0x4000) {
                type = CARTRIDGE_VIC20_8KB_4000;
            } else if (addr == 0x6000) {
                type = CARTRIDGE_VIC20_8KB_6000;
            } else if (addr == 0xA000) {
                type = CARTRIDGE_VIC20_8KB_A000;
            } else {
                /* raw 8KB binary images default to $a000-$bfff */
                type = CARTRIDGE_VIC20_8KB_A000;
                log_message(LOG_DEFAULT, "could not determine type of cartridge, defaulting to 8KiB $a000-$bfff");
            }
        } else if (len == 0x1000) {
            /* 4K image */
            if (addr == 0x2000) {
                type = CARTRIDGE_VIC20_4KB_2000;
            } else if (addr == 0x3000) {
                type = CARTRIDGE_VIC20_4KB_3000;
            } else if (addr == 0x4000) {
                type = CARTRIDGE_VIC20_4KB_4000;
            } else if (addr == 0x5000) {
                type = CARTRIDGE_VIC20_4KB_5000;
            } else if (addr == 0x6000) {
                type = CARTRIDGE_VIC20_4KB_6000;
            } else if (addr == 0x7000) {
                type = CARTRIDGE_VIC20_4KB_7000;
            } else if (addr == 0xA000) {
                type = CARTRIDGE_VIC20_4KB_A000;
            } else if (addr == 0xB000) {
                type = CARTRIDGE_VIC20_4KB_B000;
            } else {
                /* raw 4KB binary images default to $a000-$bfff */
                type = CARTRIDGE_VIC20_4KB_A000;
                log_message(LOG_DEFAULT, "could not determine type of cartridge, defaulting to 4KiB $a000-$afff");
            }
        } else {
            DBG(("attach_image error (autodetect), len=%d.\n", len));
            return -1;
        }
            
        DBG(("detected cartridge type: %04x addr: %04x len: %04x\n", type, addr, len));
    }

    memset(rawcart, 0xff, 0x8000); /* init all blocks with 0xff */

    /* FIXME: the following can probably be shortened and simplified by using the
              file length that was determined above */
    
    if ((n = fread(rawcart, 0x1000, 8, fd)) < 1) {
        zfile_fclose(fd);
        return -1;
    }
    zfile_fclose(fd);

    DBG(("loaded %d 4KiB chunks, preparing type %d (%04x)\n", n, type, type));
    
    switch (type) {
        case CARTRIDGE_VIC20_4KB_2000:  /* fall through */
        case CARTRIDGE_VIC20_8KB_2000:  /* fall through */
        case CARTRIDGE_VIC20_16KB_2000: /* fall through */
        case CARTRIDGE_VIC20_32KB_2000: /* fall through */
        case CARTRIDGE_VIC20_4KB_3000:
            util_string_set(&cartfile2, filename);
            break;
        case CARTRIDGE_VIC20_4KB_4000:  /* fall through */
        case CARTRIDGE_VIC20_8KB_4000:  /* fall through */
        case CARTRIDGE_VIC20_16KB_4000: /* fall through */
        case CARTRIDGE_VIC20_4KB_5000:
            util_string_set(&cartfile4, filename);
            break;
        case CARTRIDGE_VIC20_4KB_6000:  /* fall through */
        case CARTRIDGE_VIC20_8KB_6000:  /* fall through */
        case CARTRIDGE_VIC20_16KB_6000: /* fall through */
        case CARTRIDGE_VIC20_4KB_7000:
            util_string_set(&cartfile6, filename);
            break;
        case CARTRIDGE_VIC20_4KB_A000:  /* fall through */
        case CARTRIDGE_VIC20_8KB_A000:
            util_string_set(&cartfileA, filename);
            break;
        case CARTRIDGE_VIC20_4KB_B000:
            util_string_set(&cartfileB, filename);
            break;
        default:
            DBG(("attach_image error (string set), len=%d.\n", len));
            return -1;
            break;
    }
    
    switch (type) {
        case CARTRIDGE_VIC20_4KB_2000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_3000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_4000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_5000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_6000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_7000:  /* fall through */
        case CARTRIDGE_VIC20_4KB_A000:
            break;

        case CARTRIDGE_VIC20_4KB_B000:
            /* if no cart is present at A000, mirror this one at A000 */
            if (!(cartfileA && *cartfileA)) {
                type = CARTRIDGE_VIC20_8KB_A000;
                memcpy(rawcart + 0x1000, rawcart, 0x1000);
            }
            break;
        
        case CARTRIDGE_VIC20_8KB_2000:
            /* if we loaded just 4k, create a mirror at 3000 */
            if (n < 2) {
                type = CARTRIDGE_VIC20_4KB_2000;
                memcpy(rawcart + 0x1000, rawcart, 0x1000);
            }
            break;
        case CARTRIDGE_VIC20_8KB_4000:
            /* if we loaded just 4k, create a mirror at 5000 */
            if (n < 2) {
                type = CARTRIDGE_VIC20_4KB_4000;
                memcpy(rawcart + 0x1000, rawcart, 0x1000);
            }
            break;
        case CARTRIDGE_VIC20_8KB_6000:
            /* if we loaded just 4k, create a mirror at 7000 */
            if (n < 2) {
                type = CARTRIDGE_VIC20_4KB_6000;
                memcpy(rawcart + 0x1000, rawcart, 0x1000);
            }
            break;
        case CARTRIDGE_VIC20_8KB_A000:
            /* if we loaded just 4k, check if a cart exists at B000. if so,
               change the type to 4k. if not, create a mirror at B000. */
            if (n < 2) {
                if (cartfileB && *cartfileB) {
                    type = CARTRIDGE_VIC20_4KB_A000;
                } else {
                    memcpy(rawcart + 0x1000, rawcart, 0x1000);
                }
            }
            break;
            
        case CARTRIDGE_VIC20_16KB_2000:         /* fall through */
        case CARTRIDGE_VIC20_16KB_2000_A000:
            if (n < 4) {
                type = CARTRIDGE_VIC20_8KB_2000;
                if (n < 2) {
                    type = CARTRIDGE_VIC20_4KB_2000;
                    memcpy(rawcart + 0x1000, rawcart, 0x1000);
                }
            }
            break;
        case CARTRIDGE_VIC20_16KB_4000:         /* fall through */
        case CARTRIDGE_VIC20_16KB_4000_A000:
            if (n < 4) {
                type = CARTRIDGE_VIC20_8KB_4000;
                if (n < 2) {
                    type = CARTRIDGE_VIC20_4KB_4000;
                    memcpy(rawcart + 0x1000, rawcart, 0x1000);
                }
            }
            break;
        case CARTRIDGE_VIC20_16KB_6000:
            if (n < 4) {
                type = CARTRIDGE_VIC20_8KB_6000;
                if (n < 2) {
                    type = CARTRIDGE_VIC20_4KB_6000;
                    memcpy(rawcart + 0x1000, rawcart, 0x1000);
                }
            }
            break;

        case CARTRIDGE_VIC20_32KB_2000:
            break;

        default:
            DBG(("attach_image error (creating mirrors), len=%d.\n", len));
            return -1;
            break;
    }

    DBG(("attaching type %d (%04x)\n", n, type, type));
    
    /* attach cartridge data */
    switch (type) {
        case CARTRIDGE_VIC20_4KB_2000:
            memcpy(cart_rom + 0x2000, rawcart, 0x1000); /* block 1 (1st 4k) */
            generic_rom_blocks |= VIC_CART_BLK1;
            break;
        case CARTRIDGE_VIC20_4KB_3000:
            memcpy(cart_rom + 0x3000, rawcart, 0x1000); /* block 1 (2nd 4k) */
            generic_rom_blocks |= VIC_CART_BLK1;
            break;
        case CARTRIDGE_VIC20_4KB_4000:
            memcpy(cart_rom + 0x4000, rawcart, 0x1000); /* block 2 (1st 4k) */
            generic_rom_blocks |= VIC_CART_BLK2;
            break;
        case CARTRIDGE_VIC20_4KB_5000:
            memcpy(cart_rom + 0x5000, rawcart, 0x1000); /* block 2 (2nd 4k) */
            generic_rom_blocks |= VIC_CART_BLK2;
            break;
        case CARTRIDGE_VIC20_4KB_6000:
            memcpy(cart_rom + 0x6000, rawcart, 0x1000); /* block 3 (1st 4k) */
            generic_rom_blocks |= VIC_CART_BLK3;
            break;
        case CARTRIDGE_VIC20_4KB_7000:
            memcpy(cart_rom + 0x7000, rawcart, 0x1000); /* block 3 (2nd 4k) */
            generic_rom_blocks |= VIC_CART_BLK3;
            break;
        case CARTRIDGE_VIC20_4KB_A000:
            memcpy(cart_rom + 0x0000, rawcart, 0x1000); /* block 5 (1st 4k) */
            generic_rom_blocks |= VIC_CART_BLK5;
            break;
        case CARTRIDGE_VIC20_4KB_B000:
            memcpy(cart_rom + 0x1000, rawcart, 0x1000); /* block 5 (2nd 4k) */
            generic_rom_blocks |= VIC_CART_BLK5;
            break;

        case CARTRIDGE_VIC20_8KB_2000:
            memcpy(cart_rom + 0x2000, rawcart, 0x2000); /* block 1 */
            generic_rom_blocks |= VIC_CART_BLK1;
            break;
        case CARTRIDGE_VIC20_8KB_4000:
            memcpy(cart_rom + 0x4000, rawcart, 0x2000); /* block 2 */
            generic_rom_blocks |= VIC_CART_BLK2;
            break;
        case CARTRIDGE_VIC20_8KB_6000:
            memcpy(cart_rom + 0x6000, rawcart, 0x2000); /* block 3 */
            generic_rom_blocks |= VIC_CART_BLK3;
            break;
        case CARTRIDGE_VIC20_8KB_A000:
            memcpy(cart_rom + 0x0000, rawcart, 0x2000); /* block 5 */
            generic_rom_blocks |= VIC_CART_BLK5;
            break;
            
        case CARTRIDGE_VIC20_16KB_2000:
            memcpy(cart_rom + 0x2000, rawcart, 0x4000); /* block 1, block 2 */
            generic_rom_blocks |= (VIC_CART_BLK1 | VIC_CART_BLK2);
            break;
        case CARTRIDGE_VIC20_16KB_2000_A000:
            memcpy(cart_rom + 0x2000, rawcart, 0x2000); /* block 1 */
            memcpy(cart_rom + 0x0000, rawcart + 0x2000, 0x2000); /* block 5 */
            generic_rom_blocks |= (VIC_CART_BLK1 | VIC_CART_BLK5);
            break;
        case CARTRIDGE_VIC20_16KB_4000:
            memcpy(cart_rom + 0x4000, rawcart, 0x4000); /* block 2, block 3 */
            generic_rom_blocks |= (VIC_CART_BLK2 | VIC_CART_BLK3);
            break;
        case CARTRIDGE_VIC20_16KB_4000_A000:
            memcpy(cart_rom + 0x4000, rawcart, 0x2000); /* block 2 */
            memcpy(cart_rom + 0x0000, rawcart + 0x2000, 0x2000); /* block 5 */
            generic_rom_blocks |= (VIC_CART_BLK2 | VIC_CART_BLK5);
            break;
        case CARTRIDGE_VIC20_16KB_6000:
            memcpy(cart_rom + 0x6000, rawcart, 0x2000); /* block 3 */
            memcpy(cart_rom + 0x0000, rawcart + 0x2000, 0x2000); /* block 5 */
            generic_rom_blocks |= (VIC_CART_BLK3 | VIC_CART_BLK5);
            break;

        case CARTRIDGE_VIC20_32KB_2000:
            memcpy(cart_rom + 0x2000, rawcart, 0x6000); /* block 1,2,3 */
            memcpy(cart_rom + 0x0000, rawcart + 0x6000, 0x2000); /* block 5 */
            generic_rom_blocks |= (VIC_CART_BLK1 | VIC_CART_BLK2 | 
                                   VIC_CART_BLK3 | VIC_CART_BLK5);
            break;
        default:
            DBG(("attach_image error (image attach), len=%d.\n", len));
            return -1;
            break;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

int generic_attach_from_resource(int type, const char *filename)
{
    if (filename == NULL || *filename == '\0') {
        return cartridge_attach_image(CARTRIDGE_VIC20_16KB_2000, cartfile2)
               || cartridge_attach_image(CARTRIDGE_VIC20_16KB_4000, cartfile4)
               || cartridge_attach_image(CARTRIDGE_VIC20_16KB_6000, cartfile6)
               || cartridge_attach_image(CARTRIDGE_VIC20_8KB_A000, cartfileA)
               || cartridge_attach_image(CARTRIDGE_VIC20_4KB_B000, cartfileB);
    }
    return cartridge_attach_image(type, filename);
}

int generic_bin_attach(int type, const char *filename)
{
    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
    }
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    DBG(("generic_bin_attach type %d, file=`%s'.\n", type, filename));

    if (type == CARTRIDGE_VIC20_GENERIC) {
        /*
         * The only difference between these two is that
         * CARTRIDGE_VIC20_GENERIC detaches the previous cart.
         * (in vic20cart.c)
         */
        type = CARTRIDGE_VIC20_DETECT;
    }

    if (attach_image(type, filename) < 0) {
        generic_detach();
        return -1;
    }

    mem_cart_blocks = generic_ram_blocks | generic_rom_blocks;
    mem_initialize_memory();
    return 0;
}

void generic_detach(void)
{
    generic_ram_blocks = 0;
    generic_rom_blocks = 0;
    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(cart_ram);
    lib_free(cart_rom);
    cart_ram = NULL;
    cart_rom = NULL;

    util_string_set(&cartfile2, NULL);
    util_string_set(&cartfile4, NULL);
    util_string_set(&cartfile6, NULL);
    util_string_set(&cartfileA, NULL);
    util_string_set(&cartfileB, NULL);
}

/* ------------------------------------------------------------------------- */

static int set_cartridge_file_2(const char *name, void *param)
{
    util_string_set(&cartridge_file_2, name);
    util_string_set(&cartfile2, name);
    return try_cartridge_attach(TRY_RESOURCE_CARTFILE2);
}

static int set_cartridge_file_4(const char *name, void *param)
{
    util_string_set(&cartridge_file_4, name);
    util_string_set(&cartfile4, name);
    return try_cartridge_attach(TRY_RESOURCE_CARTFILE4);
}

static int set_cartridge_file_6(const char *name, void *param)
{
    util_string_set(&cartridge_file_6, name);
    util_string_set(&cartfile6, name);
    return try_cartridge_attach(TRY_RESOURCE_CARTFILE6);
}

static int set_cartridge_file_A(const char *name, void *param)
{
    util_string_set(&cartridge_file_A, name);
    util_string_set(&cartfileA, name);
    return try_cartridge_attach(TRY_RESOURCE_CARTFILEA);
}

static int set_cartridge_file_B(const char *name, void *param)
{
    util_string_set(&cartridge_file_B, name);
    util_string_set(&cartfileB, name);
    return try_cartridge_attach(TRY_RESOURCE_CARTFILEB);
}

static const resource_string_t resources_string[] =
{
    { "GenericCartridgeFile2000", "", RES_EVENT_STRICT, (resource_value_t)"",
      &cartridge_file_2, set_cartridge_file_2, NULL },
    { "GenericCartridgeFile4000", "", RES_EVENT_STRICT, (resource_value_t)"",
      &cartridge_file_4, set_cartridge_file_4, NULL },
    { "GenericCartridgeFile6000", "", RES_EVENT_STRICT, (resource_value_t)"",
      &cartridge_file_6, set_cartridge_file_6, NULL },
    { "GenericCartridgeFileA000", "", RES_EVENT_STRICT, (resource_value_t)"",
      &cartridge_file_A, set_cartridge_file_A, NULL },
    { "GenericCartridgeFileB000", "", RES_EVENT_STRICT, (resource_value_t)"",
      &cartridge_file_B, set_cartridge_file_B, NULL },
    RESOURCE_STRING_LIST_END
};

int generic_resources_init(void)
{
    return resources_register_string(resources_string);
}

void generic_resources_shutdown(void)
{
    lib_free(cartridge_file_2);
    lib_free(cartridge_file_4);
    lib_free(cartridge_file_6);
    lib_free(cartridge_file_A);
    lib_free(cartridge_file_B);
    lib_free(cartfile2);
    lib_free(cartfile4);
    lib_free(cartfile6);
    lib_free(cartfileA);
    lib_free(cartfileB);
}

void generic_set_default(void)
{
    set_cartridge_file_2(cartfile2, NULL);
    set_cartridge_file_4(cartfile4, NULL);
    set_cartridge_file_6(cartfile6, NULL);
    set_cartridge_file_A(cartfileA, NULL);
    set_cartridge_file_B(cartfileB, NULL);
}

void generic_unset_default(void)
{
    util_string_set(&cartridge_file_2, "");
    util_string_set(&cartridge_file_4, "");
    util_string_set(&cartridge_file_6, "");
    util_string_set(&cartridge_file_A, "");
    util_string_set(&cartridge_file_B, "");
}

/* FIXME: rewrite to use cartids defined in cartridge.h instead of an address */
const char *generic_get_file_name(uint16_t addr)
{
    switch (addr) {
        case 0x2000:
            return cartfile2;
        case 0x4000:
            return cartfile4;
        case 0x6000:
            return cartfile6;
        case 0xa000:
            return cartfileA;
        case 0xb000:
            return cartfileB;
        default:
            return NULL;
    }
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   2
#define VIC20CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "GENERICCART"

int generic_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_DW(m, (uint32_t)generic_ram_blocks) < 0)
        || (SMW_DW(m, (uint32_t)generic_rom_blocks) < 0)
        || (SMW_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMW_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
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

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != VIC20CART_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
    }
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE);
    }

    if (0
        || (SMR_DW_INT(m, &generic_ram_blocks) < 0)
        || (SMR_DW_INT(m, &generic_rom_blocks) < 0)
        || (SMR_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMR_BA(m, cart_rom, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        lib_free(cart_ram);
        cart_ram = NULL;
        lib_free(cart_rom);
        cart_rom = NULL;
        return -1;
    }

    snapshot_module_close(m);

    mem_cart_blocks = generic_ram_blocks | generic_rom_blocks;
    mem_initialize_memory();

    return 0;
}
