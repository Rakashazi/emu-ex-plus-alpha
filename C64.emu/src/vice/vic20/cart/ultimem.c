/*
 * ultimem.c -- UltiMem emulation.
 *
 * Written by
 *  Marko Makela <marko.makela@iki.fi>
 * Based on vic-fp.c by
 *  Marko Makela <marko.makela@iki.fi>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on megacart.c and finalexpansion.c by
 *  Daniel Kahlin <daniel@kahlin.net>
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
 *  Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA.
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
#include "flash040.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "ultimem.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

/*
   UltiMem by Retro Innovations

   see http://www.go4retro.com/products/ultimem/
*/

/* ------------------------------------------------------------------------- */
/*
 * Cartridge RAM (512KiB or 1 MiB)
 */
static size_t cart_ram_size;
static uint8_t *cart_ram = NULL;
#define CART_RAM_SIZE_1M (1 << 20)
#define CART_RAM_SIZE_512K (512 << 10)
#define CART_RAM_SIZE_MAX CART_RAM_SIZE_1M

/*
 * Flash ROM
 */
static size_t cart_rom_size;
static uint8_t *cart_rom = NULL;
#define CART_ROM_SIZE_8M (8 << 20)
#define CART_ROM_SIZE_512K (512 << 10)
#define CART_ROM_SIZE_MAX CART_ROM_SIZE_8M

#define ultimem_reg0_regs_disable 0x80
#define ultimem_reg0_led 1
#define ultimem_reg3_8m 0x11 /* UltiMem 8MiB */
#define ultimem_reg3_512k 0x12 /* VicMidi+UltiMem 512KiB */

#define CART_CFG_DISABLE (ultimem[0] & ultimem_reg0_regs_disable)

/** Configuration registers, plus state for re-enabling the registers */
static uint8_t ultimem[17];

/** Used bits in ultimem[] */
static const uint8_t ultimem_mask[16] = {
    0xc7,
    0x3f, /* 00:IO3 config:IO2 config:RAM123 config */
    0xff, /* BLK5:BLK3:BLK2:BLK1 */
    0,
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1, /* RAM bank lo/hi (A13..A23) */
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1, /* I/O bank lo/hi (A13..A23) */
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1, /* BLK1 bank lo/hi (A13..A23) */
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1, /* BLK2 bank lo/hi (A13..A23) */
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1, /* BLK3 bank lo/hi (A13..A23) */
    0xff, (CART_ROM_SIZE_MAX >> 21) - 1  /* BLK5 bank lo/hi (A13..A23) */
};

/** Initial values for the registers at RESET */
static const uint8_t ultimem_reset[17] = {
    6 /* two switches, never asserted in VICE */,
    0, 64,
    ultimem_reg3_8m,
    1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 0, 0
};

/** Block states */
enum blk_state_t {
    BLK_STATE_DISABLED,
    BLK_STATE_ROM,
    BLK_STATE_RAM_RO,
    BLK_STATE_RAM_RW
};

/** Get the configuration bits for RAM123, IO2 and IO3 */
#define CART_CFG_IO(x) (enum blk_state_t) (ultimem[1] >> (2 * (x - 1)) & 3)
#define CART_CFG_RAM123 CART_CFG_IO(1)

/** Get the configuration bits for BLK1,BLK2,BLK3,BLK5
@param x the BLK number (4=BLK5) */
#define CART_CFG_BLK(x) (enum blk_state_t) (ultimem[2] >> (2 * (x - 1)) & 3)

/** Get the 8KiB bank address for a block */
#define CART_ADDR(r) ((unsigned) ultimem[r + 1] << 21 | ultimem[r] << 13)
#define CART_RAM123_ADDR CART_ADDR(4)
#define CART_IO_ADDR CART_ADDR(6)
#define CART_BLK_ADDR(blk) CART_ADDR(6 + 2 * blk)

/* Cartridge States */
/** Flash state */
static flash040_context_t flash_state;

/* ------------------------------------------------------------------------- */

static int vic_um_writeback;
static char *cartfile = NULL;   /* perhaps the one in vic20cart.c could
                                   be used instead? */

static log_t um_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static uint8_t vic_um_io2_read(uint16_t addr);
static uint8_t vic_um_io2_peek(uint16_t addr);
static void vic_um_io2_store(uint16_t addr, uint8_t value);
static uint8_t vic_um_io3_read(uint16_t addr);
static uint8_t vic_um_io3_peek(uint16_t addr);
static void vic_um_io3_store(uint16_t addr, uint8_t value);
static int vic_um_mon_dump(void);

static io_source_t ultimem_io2 = {
    CARTRIDGE_VIC20_NAME_UM, /* name of the device */
    IO_DETACH_CART,          /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,   /* does not use a resource for detach */
    0x9800, 0x9bff, 0x3ff,   /* range for the device, regs:$9800-$9bff */
    0,                       /* read validity is determined by the device upon a read */
    vic_um_io2_store,        /* store function */
    NULL,                    /* NO poke function */
    vic_um_io2_read,         /* read function */
    vic_um_io2_peek,         /* peek function */
    NULL,                    /* TODO: device state information dump function */
    CARTRIDGE_VIC20_UM,      /* cartridge ID */
    IO_PRIO_NORMAL,          /* normal priority, device read needs to be checked for collisions */
    0                        /* insertion order, gets filled in by the registration function */
};

static io_source_t ultimem_io3 = {
    CARTRIDGE_VIC20_NAME_UM, /* name of the device */
    IO_DETACH_CART,          /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,   /* does not use a resource for detach */
    0x9c00, 0x9fff, 0x3ff,   /* range for the device, regs:$9c00-$9fff */
    0,                       /* read validity is determined by the device upon a read */
    vic_um_io3_store,        /* store function */
    NULL,                    /* NO poke function */
    vic_um_io3_read,         /* read function */
    vic_um_io3_peek,         /* peek function */
    vic_um_mon_dump,         /* device state information dump function */
    CARTRIDGE_VIC20_UM,      /* cartridge ID */
    IO_PRIO_NORMAL,          /* normal priority, device read needs to be checked for collisions */
    0                        /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *io2_list_item = NULL;
static io_source_list_t *io3_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_VIC20_NAME_UM, 0, 0, &ultimem_io2, &ultimem_io2, CARTRIDGE_VIC20_UM
};

/* ------------------------------------------------------------------------- */

static int vic_um_mon_dump(void)
{
    mon_out("registers %sabled\n", CART_CFG_DISABLE ? "dis" : "en");
    return 0;
}

/* ------------------------------------------------------------------------- */

/* read 0x0400-0x0fff */
uint8_t vic_um_ram123_read(uint16_t addr)
{
    switch (CART_CFG_RAM123) {
        case BLK_STATE_DISABLED:
            return vic20_v_bus_last_data;
        case BLK_STATE_ROM:
            return flash040core_read(&flash_state,
                                    ((addr & 0x1fff) + CART_RAM123_ADDR) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr & 0x1fff) + CART_RAM123_ADDR) &
                            (cart_ram_size - 1)];
    }
    return 0;
}

/* store 0x0400-0x0fff */
void vic_um_ram123_store(uint16_t addr, uint8_t value)
{
    switch (CART_CFG_RAM123) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr & 0x1fff) + CART_RAM123_ADDR) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr & 0x1fff) + CART_RAM123_ADDR) & (cart_ram_size - 1)] =
                value;
    }
}

/* read 0x2000-0x3fff */
uint8_t vic_um_blk1_read(uint16_t addr)
{
    switch (CART_CFG_BLK(1)) {
        case BLK_STATE_DISABLED:
            return vic20_v_bus_last_data;
        case BLK_STATE_ROM:
            return flash040core_read(&flash_state,
                                    ((addr & 0x1fff) + CART_BLK_ADDR(1)) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(1)) &
                            (cart_ram_size - 1)];
    }
    return 0;
}

/* store 0x2000-0x3fff */
void vic_um_blk1_store(uint16_t addr, uint8_t value)
{
    switch (CART_CFG_BLK(1)) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr & 0x1fff) + CART_BLK_ADDR(1)) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(1)) & (cart_ram_size - 1)] =
                value;
    }
}

/* read 0x4000-0x7fff */
uint8_t vic_um_blk23_read(uint16_t addr)
{
    unsigned b = addr & 0x2000 ? 3 : 2;
    switch (CART_CFG_BLK(b)) {
        case BLK_STATE_DISABLED:
            return vic20_v_bus_last_data;
        case BLK_STATE_ROM:
            return flash040core_read(&flash_state,
                                    ((addr & 0x1fff) + CART_BLK_ADDR(b)) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(b)) &
                            (cart_ram_size - 1)];
    }
    return 0;
}

/* store 0x4000-0x7fff */
void vic_um_blk23_store(uint16_t addr, uint8_t value)
{
    unsigned b = addr & 0x2000 ? 3 : 2;
    switch (CART_CFG_BLK(b)) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr & 0x1fff) + CART_BLK_ADDR(b)) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(b)) & (cart_ram_size - 1)] =
                value;
    }
}

/* read 0xa000-0xbfff */
uint8_t vic_um_blk5_read(uint16_t addr)
{
    switch (CART_CFG_BLK(4)) {
        case BLK_STATE_DISABLED:
            return vic20_v_bus_last_data;
        case BLK_STATE_ROM:
            return flash040core_read(&flash_state,
                                    ((addr & 0x1fff) + CART_BLK_ADDR(4)) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(4)) &
                            (cart_ram_size - 1)];
    }
    return 0;
}

/* store 0xa000-0xbfff */
void vic_um_blk5_store(uint16_t addr, uint8_t value)
{
    switch (CART_CFG_BLK(4)) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr & 0x1fff) + CART_BLK_ADDR(4)) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr & 0x1fff) + CART_BLK_ADDR(4)) & (cart_ram_size - 1)] =
                value;
    }
}

/* read 0x9800-0x9bff */
static uint8_t vic_um_io2_read(uint16_t addr)
{
    ultimem_io2.io_source_valid = 0;

    switch (CART_CFG_IO(2)) {
        case BLK_STATE_DISABLED:
            break;
        case BLK_STATE_ROM:
            ultimem_io2.io_source_valid = 1;
            return flash040core_read(&flash_state,
                                    ((addr | 0x1800) + CART_IO_ADDR) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            ultimem_io2.io_source_valid = 1;
            return cart_ram[((addr | 0x1800) + CART_IO_ADDR) &
                            (cart_ram_size - 1)];
    }

    return vic20_v_bus_last_data;
}

/* peek 0x9800-0x9bff */
static uint8_t vic_um_io2_peek(uint16_t addr)
{
    switch (CART_CFG_IO(2)) {
        case BLK_STATE_DISABLED:
            break;
        case BLK_STATE_ROM:
            return cart_rom[((addr | 0x1800) + CART_IO_ADDR) &
                            (cart_rom_size - 1)];
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr | 0x1800) + CART_IO_ADDR) &
                            (cart_ram_size - 1)];
    }

    return vic20_v_bus_last_data;
}

/* store 0x9800-0x9bff */
static void vic_um_io2_store(uint16_t addr, uint8_t value)
{
    switch (CART_CFG_IO(2)) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr | 0x1800) + CART_IO_ADDR) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr | 0x1800) + CART_IO_ADDR) & (cart_ram_size - 1)] =
                value;
    }
}

/* read 0x9c00-0x9fff */
static uint8_t vic_um_io3_read(uint16_t addr)
{
    ultimem_io3.io_source_valid = 0;

    if (CART_CFG_DISABLE) {
        /* Implement the state machine for re-enabling the register. */
        switch (addr) {
            case 0x355: /* Access 0x9f55 */
                /* Advance the state if this is the 1st access, else reset. */
                ultimem[16] = ((ultimem[16] == 0) ? 1 : 0);
                break;
            case 0x3aa: /* Access 0x9faa */
                /* Advance the state if this is the 2nd access, else reset. */
                ultimem[16] = ((ultimem[16] == 1) ? 2 : 0);
                break;
            case 0x301: /* Access 0x9f01 */
                /* Advance the state if this is the 3rd access, else reset. */
                if (ultimem[16] == 2) {
                    ultimem[0] &= ~ultimem_reg0_regs_disable;
                }
                ultimem[16] = 0;
                break;
            default:
                if (addr < 0x300) {
                    break;
                }
                /* Accessing 0x9f00..0x9fff resets the state machine. */
                ultimem[16] = 0;
        }

        if (addr >= 0x3f0) {
            return vic20_v_bus_last_data;
        }
    } else if (addr >= 0x3f0) {
        ultimem_io3.io_source_valid = 1;
        return ultimem[addr & 0xf];
    }

    switch (CART_CFG_IO(3)) {
        case BLK_STATE_DISABLED:
            break;
        case BLK_STATE_ROM:
            ultimem_io3.io_source_valid = 1;
            return flash040core_read(&flash_state,
                                    ((addr | 0x1c00) + CART_IO_ADDR) &
                                    (cart_rom_size - 1));
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            ultimem_io3.io_source_valid = 1;
            return cart_ram[((addr | 0x1c00) + CART_IO_ADDR) &
                            (cart_ram_size - 1)];
    }

    return vic20_v_bus_last_data;
}

/* peek 0x9c00-0x9fff */
static uint8_t vic_um_io3_peek(uint16_t addr)
{
    if (addr >= 0x3f0) {
        return CART_CFG_DISABLE ? vic20_v_bus_last_data : ultimem[addr & 0xf];
    }

    switch (CART_CFG_IO(3)) {
        case BLK_STATE_DISABLED:
            break;
        case BLK_STATE_ROM:
            return cart_rom[((addr | 0x1c00) + CART_IO_ADDR) &
                            (cart_rom_size - 1)];
        case BLK_STATE_RAM_RO:
        case BLK_STATE_RAM_RW:
            return cart_ram[((addr | 0x1c00) + CART_IO_ADDR) &
                            (cart_ram_size - 1)];
    }

    return vic20_v_bus_last_data;
}

/* store 0x9c00-0x9fff */
static void vic_um_io3_store(uint16_t addr, uint8_t value)
{
    if (CART_CFG_DISABLE) {
        /* Implement the state machine for re-enabling the register. */
        switch (addr) {
            case 0x355: /* Access 0x9f55 */
                /* Advance the state if this is the 1st access, else reset. */
                ultimem[16] = ((ultimem[16] == 0) ? 1 : 0);
                break;
            case 0x3aa: /* Access 0x9faa */
                /* Advance the state if this is the 2nd access, else reset. */
                ultimem[16] = ((ultimem[16] == 1) ? 2 : 0);
                break;
            case 0x301: /* Access 0x9f01 */
                /* Advance the state if this is the 3rd access, else reset. */
                if (ultimem[16] == 2) {
                    ultimem[0] &= ~ultimem_reg0_regs_disable;
                }
                ultimem[16] = 0;
                break;
            default:
                /* Accessing 0x9f00..0x9fff resets the state machine. */
                if (addr >= 0x300) {
                    ultimem[16] = 0;
                }
        }

        if (addr >= 0x3f0) {
            return;
        }
    } else if (addr >= 0x3f0) {
        addr &= 0xf;
        value &= ultimem_mask[addr];
        switch (addr) {
            case 0:
                value |= ultimem_reset[0];
                if (value & 0x40) {
                    machine_trigger_reset(MACHINE_RESET_MODE_HARD);
                }
                break;
            case 3:
                return; /* not writable */
            case 7: case 9: case 11: case 13: case 15:
                if (ultimem[3] == ultimem_reg3_512k) {
                    value = 0;
                }
                break;
            case 6: case 8: case 10: case 12: case 14:
                if (ultimem[3] == ultimem_reg3_512k) {
                    value &= (CART_ROM_SIZE_512K >> 13) - 1;
                }
        }
        ultimem[addr] = value;
        return;
    }

    switch (CART_CFG_IO(3)) {
        case BLK_STATE_DISABLED:
        case BLK_STATE_RAM_RO:
            break;
        case BLK_STATE_ROM:
            flash040core_store(&flash_state,
                            ((addr | 0x1c00) + CART_IO_ADDR) &
                            (cart_rom_size - 1),
                            value);
            break;
        case BLK_STATE_RAM_RW:
            cart_ram[((addr | 0x1c00) + CART_IO_ADDR) & (cart_ram_size - 1)] =
                value;
    }
}

/* ------------------------------------------------------------------------- */

void vic_um_init(void)
{
    if (um_log == LOG_ERR) {
        um_log = log_open(CARTRIDGE_VIC20_NAME_UM);
    }
}

void vic_um_reset(void)
{
    flash040core_reset(&flash_state);
    if (ultimem[0] & 0x40) {
        /* soft reset triggered by write to $9ff0 */
        ultimem[0] &= ~0x40;
    } else {
        memcpy(ultimem, ultimem_reset, sizeof ultimem);
        switch (cart_rom_size) {
            case CART_ROM_SIZE_8M:
                ultimem[3] = ultimem_reg3_8m;
                break;
            case CART_ROM_SIZE_512K:
                ultimem[3] = ultimem_reg3_512k;
                break;
        }
    }
}

void vic_um_config_setup(uint8_t *rawcart)
{
    memcpy(ultimem, ultimem_reset, sizeof ultimem);
}

int vic_um_bin_attach(const char *filename)
{
    FILE *fd = zfile_fopen(filename, MODE_READ);

    util_string_set(&cartfile, filename);

    if (!fd) {
        vic_um_detach();
        return -1;
    }
    cart_rom_size = util_file_length(fd);

    switch (cart_rom_size) {
        case CART_ROM_SIZE_8M:
            cart_ram_size = CART_RAM_SIZE_1M;
            break;
        case CART_ROM_SIZE_512K:
            cart_ram_size = CART_RAM_SIZE_512K;
            break;
        default:
            zfile_fclose(fd);
            vic_um_detach();
            return -1;
    }

    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE_MAX);
    }

    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE_MAX);
    }

    if (fread(cart_rom, cart_rom_size, 1, fd) < 1) {
        zfile_fclose(fd);
        vic_um_detach();
        return -1;
    }

    if (export_add(&export_res) < 0) {
        return -1;
    }

    zfile_fclose(fd);

    flash040core_init(&flash_state, maincpu_alarm_context,
                      (cart_rom_size == CART_ROM_SIZE_512K) ? FLASH040_TYPE_B : FLASH040_TYPE_064,
                      cart_rom);

    mem_cart_blocks = VIC_CART_RAM123 |
                      VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 |
                      VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    io2_list_item = io_source_register(&ultimem_io2);
    io3_list_item = io_source_register(&ultimem_io3);

    return 0;
}

void vic_um_detach(void)
{
    long n = 0;
    FILE *fd;

    /* try to write back cartridge contents if write back is enabled
       and cartridge wasn't from a snapshot */
    if (vic_um_writeback && !cartridge_is_from_snapshot) {
        if (flash_state.flash_dirty) {
            log_message(um_log, "Flash dirty, trying to write back...");
            fd = fopen(cartfile, "wb");
            if (fd) {
                n = fwrite(flash_state.flash_data, cart_rom_size, 1, fd);
                fclose(fd);
            }
            if (n < 1) {
                log_message(um_log, "Failed to write back image `%s'!",
                            cartfile);
            } else {
                log_message(um_log, "Wrote back image `%s'.",
                            cartfile);
            }
        } else {
            log_message(um_log, "Flash clean, skipping write back.");
        }
    }

    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(cart_ram);
    lib_free(cart_rom);
    lib_free(cartfile);
    cart_ram = NULL;
    cart_rom = NULL;
    cartfile = NULL;

    export_remove(&export_res);

    if (io2_list_item != NULL) {
        io_source_unregister(io2_list_item);
        io2_list_item = NULL;
    }
    if (io3_list_item != NULL) {
        io_source_unregister(io3_list_item);
        io3_list_item = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static int set_vic_um_writeback(int val, void *param)
{
    vic_um_writeback = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "UltiMemWriteBack", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &vic_um_writeback, set_vic_um_writeback, NULL },
    RESOURCE_INT_LIST_END
};

int vic_um_resources_init(void)
{
    return resources_register_int(resources_int);
}

void vic_um_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
    { "-umwriteback", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UltiMemWriteBack", (resource_value_t)1,
      NULL, "Enable UltiMem write back to ROM file" },
    { "+umwriteback", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "UltiMemWriteBack", (resource_value_t)0,
      NULL, "Disable UltiMem write back to ROM file" },
    CMDLINE_LIST_END
};

int vic_um_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   2
#define VIC20CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "ULTIMEM"
#define FLASH_SNAP_MODULE_NAME  "FLASH040"

int vic_um_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               VIC20CART_DUMP_VER_MAJOR,
                               VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_BA(m, ultimem, sizeof ultimem) < 0)
        || (SMW_BA(m, cart_ram, (unsigned int)cart_ram_size) < 0)
        || (SMW_BA(m, cart_rom, (unsigned int)cart_rom_size) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if ((flash040core_snapshot_write_module(s, &flash_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        return -1;
    }

    return 0;
}

int vic_um_snapshot_read_module(snapshot_t *s)
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
        cart_ram = lib_malloc(CART_RAM_SIZE_MAX);
    }
    if (!cart_rom) {
        cart_rom = lib_malloc(CART_ROM_SIZE_MAX);
    }

    if (SMR_BA(m, ultimem, sizeof ultimem) < 0) {
    snapshot_error:
        snapshot_module_close(m);
        lib_free(cart_ram);
        lib_free(cart_rom);
        cart_ram = NULL;
        cart_rom = NULL;
        return -1;
    }

    switch (ultimem[3]) {
        case ultimem_reg3_8m:
            cart_rom_size = CART_ROM_SIZE_8M;
            cart_ram_size = CART_RAM_SIZE_1M;
            break;
        case ultimem_reg3_512k:
            cart_rom_size = CART_ROM_SIZE_512K;
            cart_ram_size = CART_RAM_SIZE_512K;
            break;
        default:
            goto snapshot_error;
    }

    if (0
        || (SMR_BA(m, cart_ram, (unsigned int)cart_ram_size) < 0)
        || (SMR_BA(m, cart_rom, (unsigned int)cart_rom_size) < 0)) {
        goto snapshot_error;
    }

    flash040core_init(&flash_state, maincpu_alarm_context,
                      (cart_rom_size == CART_ROM_SIZE_512K) ? FLASH040_TYPE_B : FLASH040_TYPE_064,
                      cart_rom);

    snapshot_module_close(m);

    if ((flash040core_snapshot_read_module(s, &flash_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        flash040core_shutdown(&flash_state);
        lib_free(cart_ram);
        lib_free(cart_rom);
        cart_ram = NULL;
        cart_rom = NULL;
        return -1;
    }

    mem_cart_blocks = VIC_CART_RAM123 |
                      VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 |
                      VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    return 0;
}

