/*
 * finalexpansion.c -- VIC20 Final Expansion v3.2 emulation.
 *
 * Written by
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
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "finalexpansion.h"
#include "flash040.h"
#include "log.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "util.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20mem.h"
#include "zfile.h"

/* ------------------------------------------------------------------------- */

/*#define FE_DEBUG_ENABLED*/

#ifdef FE_DEBUG_ENABLED
#define FE_DEBUG(x) log_debug x
#else
#define FE_DEBUG(x)
#endif

/* emulate Final Expansion v3.2 Super ROM write bug */
/*#define FE3_2_SUPER_ROM_BUG*/

/* base addresses for the blocks */
#define BLK0_BASE 0x0000
#define BLK1_BASE 0x0000
#define BLK2_BASE 0x2000
#define BLK3_BASE 0x4000
#define BLK5_BASE 0x6000

/*
 * Cartridge RAM
 *
 * Mapping in RAM mode:
 *      RAM                    VIC20
 *   0x00400 - 0x00fff  ->  0x0400 - 0x0fff
 *   0x10000 - 0x11fff  ->  0x2000 - 0x3fff
 *   0x12000 - 0x13fff  ->  0x4000 - 0x5fff
 *   0x14000 - 0x15fff  ->  0x6000 - 0x7fff
 *   0x16000 - 0x17fff  ->  0xa000 - 0xbfff
 * (INV_A13/INV_A14 changes this)
 *
 * Mapping in Super RAM mode:
 *      RAM                    VIC20
 *   0x00400 - 0x00fff  ->  0x0400 - 0x0fff
 *   0xN0000 - 0xN1fff  ->  0x2000 - 0x3fff
 *   0xN2000 - 0xN3fff  ->  0x4000 - 0x5fff
 *   0xN4000 - 0xN5fff  ->  0x6000 - 0x7fff
 *   0xN6000 - 0xN7fff  ->  0xa000 - 0xbfff
 * (INV_A13/INV_A14 changes this)
 *
 */
#define CART_RAM_SIZE 0x80000
static BYTE *cart_ram = NULL;

/*
 * Cartridge ROM
 *
 * Mapping
 *      ROM                    VIC20
 *   0xN0000 - 0xN1fff  ->  0x2000 - 0x3fff
 *   0xN2000 - 0xN3fff  ->  0x4000 - 0x5fff
 *   0xN4000 - 0xN5fff  ->  0x6000 - 0x7fff
 *   0xN6000 - 0xN7fff  ->  0xa000 - 0xbfff
 * (INV_A13/INV_A14 changes this)
 *
 */
#define CART_ROM_SIZE 0x80000

/* Cartridge States */
static flash040_context_t flash_state;
static BYTE register_a;
static BYTE register_b;
static BYTE lock_bit;

static int finalexpansion_writeback;
static char *cartfile = NULL;   /* perhaps the one in vic20cart.c could
                                   be used instead? */

static log_t fe_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

/* Register A ($9c02) */
/* #define REGA_BANK_MASK   0x1f */
#define REGA_BANK_MASK   0x0f  /* only 512 KB connected */

#define REGA_MODE_MASK   0xe0
#define MODE_START       0x00  /* Start Modus         (000zzzzz) [Mod_START] */
#define MODE_FLASH       0x20  /* Flash-Schreib-Modus (001zzzzz) [Mod_FLASH] */
#define MODE_SUPER_ROM   0x40  /* Super ROM Modus     (010zzzzz) [Mod_SROM]  */
#define MODE_ROM_RAM     0x60  /* RAM/ROM Modus       (011zzzzz) [Mod_ROMRAM]*/
#define MODE_RAM1        0x80  /* RAM 1 Modus         (100zzzzz) [Mod_RAM1]  */
#define MODE_SUPER_RAM   0xa0  /* Super RAM Modus     (101zzzzz) [Mod_SRAM]  */
#define MODE_RAM2        0xc0  /* RAM 2 Modus         (110zzzzz) [Mod_RAM2]  */

/* in MODE_RAM1, MODE_RAM2, MODE_ROM_RAM */
#define REGA_BLK0_RO     0x01
#define REGA_BLK1_SEL    0x02
#define REGA_BLK2_SEL    0x04
#define REGA_BLK3_SEL    0x08
#define REGA_BLK5_SEL    0x10

/* Register B ($9c03) */
#define REGB_BLK0_OFF    0x01
#define REGB_BLK1_OFF    0x02
#define REGB_BLK2_OFF    0x04
#define REGB_BLK3_OFF    0x08
#define REGB_BLK5_OFF    0x10
#define REGB_INV_A13     0x20
#define REGB_INV_A14     0x40
#define REGB_REG_OFF     0x80

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static BYTE finalexpansion_io3_read(WORD addr);
static BYTE finalexpansion_io3_peek(WORD addr);
static void finalexpansion_io3_store(WORD addr, BYTE value);
static int finalexpansion_mon_dump(void);

static io_source_t finalexpansion_device = {
    CARTRIDGE_VIC20_NAME_FINAL_EXPANSION,
    IO_DETACH_CART,
    NULL,
    0x9c00, 0x9fff, 0x3ff,
    0,
    finalexpansion_io3_store,
    finalexpansion_io3_read,
    finalexpansion_io3_peek,
    finalexpansion_mon_dump,
    CARTRIDGE_VIC20_FINAL_EXPANSION,
    0,
    0
};

static io_source_list_t *finalexpansion_list_item = NULL;

/* ------------------------------------------------------------------------- */

static int is_locked(void)
{
    if (register_b & REGB_REG_OFF) {
        return 1;
    }
    if ((register_a & REGA_MODE_MASK) == MODE_START) {
        return lock_bit;
    }
    return 0;
}

static unsigned int calc_addr(WORD addr, int bank, WORD base)
{
    unsigned int faddr;

    faddr = (addr & 0x1fff) | (bank * 0x8000) | base;

    faddr ^= (register_b & REGB_INV_A13) ? 0x2000 : 0;
    faddr ^= (register_b & REGB_INV_A14) ? 0x4000 : 0;

    return faddr;
}

static BYTE internal_blk0_read(WORD addr, WORD base)
{
    BYTE mode;
    int bank;
    unsigned int faddr;
    BYTE value;

    mode = register_a & REGA_MODE_MASK;

    /* Blk0 is hardwired to bank 0 */
    bank = 0;

    /* Calculate Address */
    faddr = calc_addr(addr, bank, base);

    /* Perform access */
    switch (mode) {
        case MODE_ROM_RAM:
        case MODE_RAM1:
        case MODE_RAM2:
        case MODE_SUPER_ROM:
        case MODE_SUPER_RAM:
            value = cart_ram[faddr];
            break;
        default:
            value = vic20_cpu_last_data;
            break;
    }
    return value;
}

static void internal_blk0_store(WORD addr, BYTE value, WORD base, int ro)
{
    BYTE mode;
    int bank;
    unsigned int faddr;

    mode = register_a & REGA_MODE_MASK;

    /* Blk0 is hardwired to bank 0 */
    bank = 0;

    /* Calculate Address */
    faddr = calc_addr(addr, bank, base);

    /* Perform access */
    switch (mode) {
        case MODE_ROM_RAM:
        case MODE_RAM1:
        case MODE_RAM2:
            if (!ro) {
                cart_ram[faddr] = value;
            }
            break;
        case MODE_SUPER_ROM:
        case MODE_SUPER_RAM:
            cart_ram[faddr] = value;
            break;
        default:
            break;
    }
}

static BYTE internal_read(WORD addr, int blk, WORD base, int sel)
{
    BYTE mode;
    int bank;
    unsigned int faddr;
    BYTE value;

    mode = register_a & REGA_MODE_MASK;

    /* Determine which bank to access */
    switch (mode) {
        case MODE_FLASH:
        case MODE_SUPER_ROM:
        case MODE_SUPER_RAM:
            bank = register_a & REGA_BANK_MASK;
            break;
        case MODE_ROM_RAM:
        case MODE_RAM1:
            bank = 1;
            break;
        case MODE_RAM2:
            if (sel) {
                bank = 2;
            } else {
                bank = 1;
            }
            break;
        default:
            bank = 0;
            break;
    }

    /* Calculate Address */
    faddr = calc_addr(addr, bank, base);

    /* Perform access */
    switch (mode) {
        case MODE_START:
            if (blk == 5) {
                value = flash040core_read(&flash_state, faddr);
            } else {
                value = vic20_cpu_last_data;
            }
            break;
        case MODE_FLASH:
        case MODE_SUPER_ROM:
            value = flash040core_read(&flash_state, faddr);
            break;
        case MODE_ROM_RAM:
            if (sel) {
                value = flash040core_read(&flash_state, faddr);
            } else {
                value = cart_ram[faddr];
            }
            break;
        case MODE_RAM1:
        case MODE_RAM2:
        case MODE_SUPER_RAM:
            value = cart_ram[faddr];
            break;
        default:
            value = vic20_cpu_last_data;
            break;
    }
    return value;
}

static void internal_store(WORD addr, BYTE value, int blk, WORD base, int sel)
{
    BYTE mode;
    int bank;
    unsigned int faddr;

    mode = register_a & REGA_MODE_MASK;

    /* Determine which bank to access */
    switch (mode) {
        case MODE_FLASH:
        case MODE_SUPER_RAM:
            bank = register_a & REGA_BANK_MASK;
            break;
        case MODE_SUPER_ROM:
#ifdef FE3_2_SUPER_ROM_BUG
            bank = 1 | (register_a & REGA_BANK_MASK);
            break;
#endif
        case MODE_START:
            bank = 1;
            break;
        case MODE_ROM_RAM:
        case MODE_RAM1:
            if (sel) {
                bank = 2;
            } else {
                bank = 1;
            }
            break;
        case MODE_RAM2:
            bank = 1;
            break;
        default:
            bank = 0;
            break;
    }

    /* Calculate Address */
    faddr = calc_addr(addr, bank, base);

    /* Perform access */
    switch (mode) {
        case MODE_FLASH:
            flash040core_store(&flash_state, faddr, value);
            break;
        case MODE_ROM_RAM:
            if (sel) {
                cart_ram[faddr] = value;
            }
            break;
        case MODE_START:
        case MODE_RAM1:
        case MODE_RAM2:
        case MODE_SUPER_ROM:
        case MODE_SUPER_RAM:
            cart_ram[faddr] = value;
            break;
        default:
            break;
    }
}

/* ------------------------------------------------------------------------- */

/* read 0x0400 - 0x0fff */
BYTE finalexpansion_ram123_read(WORD addr)
{
    BYTE value;
    if (!(register_b & REGB_BLK0_OFF)) {
        value = internal_blk0_read(addr, BLK0_BASE);
    } else {
        value = vic20_v_bus_last_data;
    }
    return value;
}

/* store 0x0400 - 0x0fff */
void finalexpansion_ram123_store(WORD addr, BYTE value)
{
    if (!(register_b & REGB_BLK0_OFF)) {
        internal_blk0_store(addr, value, BLK0_BASE, (register_a & REGA_BLK0_RO));
    }
}

/* read 0x2000-0x3fff */
BYTE finalexpansion_blk1_read(WORD addr)
{
    BYTE value;
    if (!(register_b & REGB_BLK1_OFF)) {
        value = internal_read(addr, 1, BLK1_BASE, register_a & REGA_BLK1_SEL);
    } else {
        value = vic20_cpu_last_data;
    }
    return value;
}

/* store 0x2000-0x3fff */
void finalexpansion_blk1_store(WORD addr, BYTE value)
{
    if (!(register_b & REGB_BLK1_OFF)) {
        internal_store(addr, value, 1, BLK1_BASE, register_a & REGA_BLK1_SEL);
    }
}

/* read 0x4000-0x5fff */
BYTE finalexpansion_blk2_read(WORD addr)
{
    BYTE value;
    if (!(register_b & REGB_BLK2_OFF)) {
        value = internal_read(addr, 2, BLK2_BASE, register_a & REGA_BLK2_SEL);
    } else {
        value = vic20_cpu_last_data;
    }
    return value;
}

/* store 0x4000-0x5fff */
void finalexpansion_blk2_store(WORD addr, BYTE value)
{
    if (!(register_b & REGB_BLK2_OFF)) {
        internal_store(addr, value, 2, BLK2_BASE, register_a & REGA_BLK2_SEL);
    }
}

/* read 0x6000-0x7fff */
BYTE finalexpansion_blk3_read(WORD addr)
{
    BYTE value;
    if (!(register_b & REGB_BLK3_OFF)) {
        value = internal_read(addr, 3, BLK3_BASE, register_a & REGA_BLK3_SEL);
    } else {
        value = vic20_cpu_last_data;
    }
    return value;
}

/* store 0x6000-0x7fff */
void finalexpansion_blk3_store(WORD addr, BYTE value)
{
    if (!(register_b & REGB_BLK3_OFF)) {
        internal_store(addr, value, 3, BLK3_BASE, register_a & REGA_BLK3_SEL);
    }
}

/* read 0xa000-0xbfff */
BYTE finalexpansion_blk5_read(WORD addr)
{
    BYTE value;

    lock_bit = 1;

    if (!(register_b & REGB_BLK5_OFF)) {
        value = internal_read(addr, 5, BLK5_BASE, register_a & REGA_BLK5_SEL);
    } else {
        value = vic20_cpu_last_data;
    }
    return value;
}

/* store 0xa000-0xbfff */
void finalexpansion_blk5_store(WORD addr, BYTE value)
{
    lock_bit = 0;

    if (!(register_b & REGB_BLK5_OFF)) {
        internal_store(addr, value, 5, BLK5_BASE, register_a & REGA_BLK5_SEL);
    }
}

/* read 0x9c00-0x9fff */
static BYTE finalexpansion_io3_read(WORD addr)
{
    BYTE value;

    finalexpansion_device.io_source_valid = 0;

    addr &= 0x03;
    FE_DEBUG(("Read reg%02x. (locked=%d)", addr, is_locked()));
    if (!is_locked()) {
        switch (addr) {
            case 0x02:
                value = register_a;
                finalexpansion_device.io_source_valid = 1;
                break;
            case 0x03:
                value = register_b;
                finalexpansion_device.io_source_valid = 1;
                break;
            default:
                value = vic20_cpu_last_data;
                break;
        }
    } else {
        value = vic20_cpu_last_data;
    }
    return value;
}

static BYTE finalexpansion_io3_peek(WORD addr)
{
    BYTE value;

    addr &= 0x03;
    FE_DEBUG(("Peek reg%02x", addr));

    switch (addr) {
        case 0x02:
            value = register_a;
            break;
        case 0x03:
            value = register_b;
            break;
        default:
            value = 0;
            break;
    }
    return value;
}

/* store 0x9c00-0x9fff */
static void finalexpansion_io3_store(WORD addr, BYTE value)
{
    addr &= 0x03;
    FE_DEBUG(("Wrote reg%02x = %02x. (locked=%d)", addr, value, is_locked()));
    if (!is_locked()) {
        switch (addr) {
            case 0x02:
                register_a = value;
                break;
            case 0x03:
                register_b = value;
                break;
        }
    }
}

/* ------------------------------------------------------------------------- */

void finalexpansion_init(void)
{
    if (fe_log == LOG_ERR) {
        fe_log = log_open(CARTRIDGE_VIC20_NAME_FINAL_EXPANSION);
    }

    register_a = 0x00;
    register_b = 0x00;
    lock_bit = 1;
}

void finalexpansion_reset(void)
{
    flash040core_reset(&flash_state);
    register_a = 0x00;
    register_b = 0x00;
    lock_bit = 1;
}

void finalexpansion_config_setup(BYTE *rawcart)
{
}

static int zfile_load(const char *filename, BYTE *dest)
{
    FILE *fd;
    size_t fsize;

    fd = zfile_fopen(filename, MODE_READ);
    if (!fd) {
        log_message(fe_log, "Failed to open image `%s'!",
                    filename);
        return -1;
    }
    fsize = util_file_length(fd);

    if (fsize < 0x8000) {
        size_t tsize;
        size_t offs;
        tsize = (fsize + 0x0fff) & 0xfffff000;
        offs = 0x8000 - tsize;
        dest += offs;
        log_message(fe_log, "Size less than 32kB.  Aligning as close as possible to the 32kB boundary in 4kB blocks. (0x%06X-0x%06X)", (unsigned int)offs, (unsigned int)(offs + tsize));
    } else if (fsize < (size_t)CART_ROM_SIZE) {
        log_message(fe_log, "Size less than 512kB, padding.");
    } else if (fsize > (size_t)CART_ROM_SIZE) {
        fsize = CART_ROM_SIZE;
        log_message(fe_log, "Size larger than 512kB, truncating.");
    }
    if (fread(dest, fsize, 1, fd) < 1) {
        log_message(fe_log, "Failed to read image `%s'!", filename);
        zfile_fclose(fd);
        return -1;
    }
    zfile_fclose(fd);
    log_message(fe_log, "Read image `%s'.",
                filename);
    return 0;
}

int finalexpansion_bin_attach(const char *filename)
{
    BYTE *cart_flash;

    if (!cart_ram) {
        cart_ram = lib_malloc(CART_RAM_SIZE);
    }

    cart_flash = lib_malloc(CART_ROM_SIZE);
    if (cart_flash == NULL) {
        return -1;
    }

    /* flash040core_init() does not clear the flash */
    memset(cart_flash, 0xff, CART_ROM_SIZE);

    flash040core_init(&flash_state, maincpu_alarm_context, FLASH040_TYPE_B, cart_flash);

    util_string_set(&cartfile, filename);
    if (zfile_load(filename, flash_state.flash_data) < 0) {
        finalexpansion_detach();
        return -1;
    }

    mem_cart_blocks = VIC_CART_RAM123 |
                      VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 |
                      VIC_CART_IO3;
    mem_initialize_memory();

    finalexpansion_list_item = io_source_register(&finalexpansion_device);

    return 0;
}

void finalexpansion_detach(void)
{
    /* try to write back cartridge contents if write back is enabled
       and cartridge wasn't from a snapshot */
    if (finalexpansion_writeback && !cartridge_is_from_snapshot) {
        if (flash_state.flash_dirty) {
            int n;
            FILE *fd;

            n = 0;
            log_message(fe_log, "Flash dirty, trying to write back...");
            fd = fopen(cartfile, "wb");
            if (fd) {
                n = fwrite(flash_state.flash_data, (size_t)CART_ROM_SIZE, 1, fd);
                fclose(fd);
            }
            if (n < 1) {
                log_message(fe_log, "Failed to write back image `%s'!", cartfile);
            } else {
                log_message(fe_log, "Wrote back image `%s'.", cartfile);
            }
        } else {
            log_message(fe_log, "Flash clean, skipping write back.");
        }
    }

    mem_cart_blocks = 0;
    mem_initialize_memory();
    lib_free(flash_state.flash_data);
    flash040core_shutdown(&flash_state);
    lib_free(cart_ram);
    cart_ram = NULL;
    lib_free(cartfile);
    cartfile = NULL;

    if (finalexpansion_list_item != NULL) {
        io_source_unregister(finalexpansion_list_item);
        finalexpansion_list_item = NULL;
    }
}

/* ------------------------------------------------------------------------- */

static int set_finalexpansion_writeback(int val, void *param)
{
    finalexpansion_writeback = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "FinalExpansionWriteBack", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &finalexpansion_writeback, set_finalexpansion_writeback, NULL },
    { NULL }
};

int finalexpansion_resources_init(void)
{
    return resources_register_int(resources_int);
}

void finalexpansion_resources_shutdown(void)
{
}

static const cmdline_option_t cmdline_options[] =
{
    { "-fewriteback", SET_RESOURCE, 0,
      NULL, NULL, "FinalExpansionWriteBack", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_FINALEXPANSION_WRITEBACK,
      NULL, NULL },
    { "+fewriteback", SET_RESOURCE, 0,
      NULL, NULL, "FinalExpansionWriteBack", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_FINALEXPANSION_WRITEBACK,
      NULL, NULL },
    { NULL }
};

int finalexpansion_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

#define VIC20CART_DUMP_VER_MAJOR   2
#define VIC20CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "FINALEXPANSION"
#define FLASH_SNAP_MODULE_NAME  "FLASH040FE"

int finalexpansion_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, VIC20CART_DUMP_VER_MAJOR, VIC20CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, register_a) < 0)
        || (SMW_B(m, register_b) < 0)
        || (SMW_B(m, lock_bit) < 0)
        || (SMW_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMW_BA(m, flash_state.flash_data, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if ((flash040core_snapshot_write_module(s, &flash_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        return -1;
    }

    return 0;
}

int finalexpansion_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    BYTE *cart_flash = NULL;

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
    if (!cart_flash) {
        cart_flash = lib_malloc(CART_ROM_SIZE);
    }

    flash040core_init(&flash_state, maincpu_alarm_context, FLASH040_TYPE_B, cart_flash);

    if (0
        || (SMR_B(m, &register_a) < 0)
        || (SMR_B(m, &register_b) < 0)
        || (SMR_B(m, &lock_bit) < 0)
        || (SMR_BA(m, cart_ram, CART_RAM_SIZE) < 0)
        || (SMR_BA(m, flash_state.flash_data, CART_ROM_SIZE) < 0)) {
        snapshot_module_close(m);
        flash040core_shutdown(&flash_state);
        lib_free(cart_ram);
        lib_free(cart_flash);
        cart_ram = NULL;
        cart_flash = NULL;
        return -1;
    }

    snapshot_module_close(m);

    if ((flash040core_snapshot_read_module(s, &flash_state, FLASH_SNAP_MODULE_NAME) < 0)) {
        flash040core_shutdown(&flash_state);
        lib_free(cart_ram);
        lib_free(cart_flash);
        cart_ram = NULL;
        cart_flash = NULL;
        return -1;
    }

    mem_cart_blocks = VIC_CART_RAM123 |
                      VIC_CART_BLK1 | VIC_CART_BLK2 | VIC_CART_BLK3 | VIC_CART_BLK5 |
                      VIC_CART_IO2 | VIC_CART_IO3;
    mem_initialize_memory();

    return 0;
}

/* ------------------------------------------------------------------------- */

static const char *finalexpansion_acc_mode[] = {
    "off",
    "RAM",
    "ROM"
};

static void finalexpansion_mon_dump_blk(int blk)
{
    int mode;
    BYTE reg_mask;
    int sel;
    int bank_r, bank_w;
    WORD base;
    enum { ACC_OFF, ACC_RAM, ACC_FLASH } acc_mode_r, acc_mode_w;

    switch (blk) {
        case 1:
            reg_mask = REGA_BLK1_SEL;
            base = BLK1_BASE;
            break;
        case 2:
            reg_mask = REGA_BLK2_SEL;
            base = BLK2_BASE;
            break;
        case 3:
            reg_mask = REGA_BLK3_SEL;
            base = BLK3_BASE;
            break;
        case 5:
            reg_mask = REGA_BLK5_SEL;
            base = BLK5_BASE;
            break;
        default:
            /* ignore block 4 */
            return;
    }

    mon_out("BLK %i: ", blk);

    if (register_b & reg_mask) {
        mon_out("off\n");
        return;
    }

    mode = register_a & REGA_MODE_MASK;
    sel = register_a & reg_mask;

    bank_r = register_a & REGA_BANK_MASK;
    bank_w = bank_r;

    switch (mode) {
        default:
        case MODE_START:
            bank_r = 0;
            bank_w = 1;
            acc_mode_r = (blk == 5) ? ACC_FLASH : ACC_OFF;
            acc_mode_w = ACC_RAM;
            break;
        case MODE_FLASH:
            acc_mode_r = ACC_FLASH;
            acc_mode_w = ACC_FLASH;
            break;
        case MODE_SUPER_ROM:
#ifdef FE3_2_SUPER_ROM_BUG
            bank_w = 1 | (register_a & REGA_BANK_MASK);
#endif
            acc_mode_r = ACC_FLASH;
            acc_mode_w = ACC_RAM;
            break;
        case MODE_SUPER_RAM:
            acc_mode_r = ACC_RAM;
            acc_mode_w = ACC_RAM;
            break;
        case MODE_ROM_RAM:
            bank_r = 1;
            bank_w = sel ? 2 : 1;
            acc_mode_r = sel ? ACC_FLASH : ACC_RAM;
            acc_mode_w = sel ? ACC_RAM : ACC_OFF;
            break;
        case MODE_RAM1:
            bank_r = 1;
            bank_w = sel ? 2 : 1;
            acc_mode_r = ACC_RAM;
            acc_mode_w = ACC_RAM;
            break;
        case MODE_RAM2:
            bank_r = sel ? 2 : 1;
            bank_w = 1;
            acc_mode_r = ACC_RAM;
            acc_mode_w = ACC_RAM;
            break;
    }

    mon_out("\n  read %s ", finalexpansion_acc_mode[acc_mode_r]);

    if (acc_mode_r != ACC_OFF) {
        mon_out("bank $%02x (offset $%06x)", bank_r, calc_addr(0, bank_r, base));
    }

    mon_out("\n write %s ", finalexpansion_acc_mode[acc_mode_w]);

    if (acc_mode_w != ACC_OFF) {
        mon_out("bank $%02x (offset $%06x)", bank_w, calc_addr(0, bank_w, base));
    }

    mon_out("\n");
}

static const char *finalexpansion_mode_name[] = {
    "Start",
    "Flash",
    "Super ROM",
    "RAM/ROM",
    "RAM 1",
    "Super RAM",
    "RAM 2"
};

static int finalexpansion_mon_dump(void)
{
    BYTE mode;
    int blk, active, ro;

    mode = register_a & REGA_MODE_MASK;

    mon_out("Register A: $%02x, B: $%02x, lock bit %i\n", register_a, register_b, lock_bit);
    mon_out("Mode: %s\n", finalexpansion_mode_name[mode >> 5]);

    /* BLK 0 */
    mon_out("BLK 0: ");
    active = 0;
    ro = 0;

    if (!(register_b & REGB_BLK0_OFF)) {
        switch (mode) {
            case MODE_SUPER_ROM:
            case MODE_SUPER_RAM:
                active = 1;
                break;
            case MODE_ROM_RAM:
            case MODE_RAM1:
            case MODE_RAM2:
                active = 1;
                ro = register_a & REGA_BLK0_RO;
                break;
            default:
                break;
        }
    }

    if (active) {
        mon_out("RAM%s (offset $%06x)\n", ro ? " (read only)" : "", calc_addr(0, 0, BLK0_BASE));
    } else {
        mon_out("off\n");
    }

    /* BLK 1, 2, 3, 5 */
    for (blk = 1; blk <= 5; blk++) {
        finalexpansion_mon_dump_blk(blk);
    }

    return 0;
}
