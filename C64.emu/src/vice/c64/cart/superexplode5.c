/*
 * superexplode5.c - Cartridge handling, Super Explode V5 cart.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alarm.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "maincpu.h"
#include "monitor.h"
#include "snapshot.h"
#include "superexplode5.h"
#include "types.h"
#include "util.h"
#include "vicii-phi1.h"
#include "crt.h"

/*
    FIXME: this one has been implemented purely based on guesswork and by
           examining the cartridge rom dump.

    The Soft Group "Super Explode V5"

    - 2 ROM banks, 8k each == 16kb
    - one button (reset)

    ROM banks are always mapped to $8000
    the last page of the ROM bank is also visible at DFxx

    controlregister is $df00:
        bit 7 selects bank

    the ROM at $8000 is enabled / disabled the following way:
    - if either IO1, ROMLO or RESET are active (=0), then EXROM becomes active (=0)
    - if all of the above are inactive (=1) then a capacitor is being charged,
      and EXROM becomes inactive (=1) after roughly 300ms

    this is a very strange cartridge, almost no information about it seems
    to exist, from http://www.mayhem64.co.uk/cartpower.htm:

    Super Explode! version 5 is primarily a graphics cartridge. It is designed
    to capture, manipulate, and edit screens and then print them. Its color
    print capability includes recolorization, and it dumps to all but one
    available color printer. Its extensive ability to manipulate graphics
    images makes it the cartridge of choice for graphics buffs. (Note that
    Super Explode! interfaces with The Soft Group's Video Byte system, a low-
    cost video digitizer designed to capture full-color images from a VCR or
    live camera.)

    Super Explode! 5's modest utility repertoire includes a complete disk-turbo
    feature, directory list to screen, single-stroke disk commands, and easy
    access to the error channel. These commands are not implemented on function
    keys, nor are the function keys programmed. There is no BASIC toolkit,
    monitor, or disk-backup or archiving capability. There is a fast multiple-
    copy file routine, as well as an unnew command. The freeze button doubles
    as a reset.

    The manual is on disk (you must print it out) and is rather haphazard.
    Nonetheless, it contains a wealth of technical information. Topics include
    split screens, elementary and advanced file conversion (for Doodle, Koala,
    text screens, and custom character sets), sprite manipulation, and sprite
    overlay. If you require few utility functions but extensive graphics
    capability, Super Explode! 5 is for you.

*/

/* #define SE5_DEBUG */
/* #define SE5_DEBUG_RW */
/* #define SE5_DEBUG_CHARGE */

#ifdef SE5_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#ifdef SE5_DEBUG_RW
#define DBGRW(x)  printf x
#else
#define DBGRW(x)
#endif

#ifdef SE5_DEBUG_CHARGE
#define DBGCH(x)  printf x
#else
#define DBGCH(x)
#endif

#define SE5_CART_SIZE (2 * 0x2000)

/* ---------------------------------------------------------------------*/

static int se5_bank = 0;
static int se5_rom_enabled = 0;

static int se5_cap_charge = 0;

struct alarm_s *se5_alarm;
static CLOCK se5_alarm_time;
static CLOCK se5_charge_time;

#define CHARGEMAX           (90 * 3)            /* ~ 300ms */
#define DECHARGESTEPS       3                   /* should discharge in ~90 steps */
#define LOWTHRESHOLD        5
#define HIGHTHRESHOLD       (CHARGEMAX - 5)

static void flipflop(void)
{
#ifdef SE5_DEBUG
    int old = se5_rom_enabled;
#endif
    int mode;
    if (se5_cap_charge < LOWTHRESHOLD) {
        se5_rom_enabled = 1;
    }
    if (se5_cap_charge > HIGHTHRESHOLD) {
        se5_rom_enabled = 0;
    }
    mode = se5_rom_enabled ? CMODE_8KGAME : CMODE_RAM;
    mode |= se5_bank << CMODE_BANK_SHIFT;
    cart_config_changed_slotmain(mode, mode, CMODE_READ);
#ifdef SE5_DEBUG
    if (old != se5_rom_enabled) {
        DBG(("%08lx SE5: flipflop (rom:%d charge:%d)\n", maincpu_clk, se5_rom_enabled, se5_cap_charge));
    }
#endif
}

static void cap_trigger_access(void)
{
    alarm_unset(se5_alarm);
    se5_alarm_time = CLOCK_MAX;

    if (se5_cap_charge < CHARGEMAX) {
        se5_alarm_time = maincpu_clk + 1;
        alarm_set(se5_alarm, se5_alarm_time);
    }
    DBGCH(("%08lx SE5: is fully charged (idle) (rom:%d charge:%d)\n", maincpu_clk, se5_rom_enabled, se5_cap_charge));
}

static void se5_alarm_handler(CLOCK offset, void *data)
{
    if (maincpu_clk >= se5_charge_time) {
        se5_cap_charge++;
        if (se5_cap_charge > CHARGEMAX) {
            se5_cap_charge = CHARGEMAX;
        }
    }
    DBGCH(("%08lx SE5: charge idle (rom:%d charge:%d)\n", maincpu_clk, se5_rom_enabled, se5_cap_charge));
    flipflop();
    cap_trigger_access();
}

static void cap_discharge(void)
{
    se5_cap_charge -= DECHARGESTEPS;
    if (se5_cap_charge < 0) {
        se5_cap_charge = 0;
    }
    DBGCH(("%08lx SE5: discharge (rom:%d charge:%d)\n", maincpu_clk , se5_rom_enabled, se5_cap_charge));
    /* put first alarm a few cycles ahead, so we dont have to fiddle with alternating
       charge/discharge while the discharge loop is executed */
    se5_charge_time = maincpu_clk + 10;
    flipflop();
    cap_trigger_access();
}

/* ---------------------------------------------------------------------*/

static void se5_io1_store(uint16_t addr, uint8_t value)
{
    DBGRW(("%08lx io1 wr %04x %02x %d\n",maincpu_clk ,addr, value, se5_cap_charge));
    cap_discharge();
}

static uint8_t se5_io1_read(uint16_t addr)
{
    DBGRW(("%08lx io1 rd %04x %d\n",maincpu_clk, addr, se5_cap_charge));
    cap_discharge();
    return vicii_read_phi1();
}

static uint8_t se5_io1_peek(uint16_t addr)
{
    DBGRW(("io1 rd %04x\n", addr));
    return 0;
}

static void se5_io2_store(uint16_t addr, uint8_t value)
{
    DBGRW(("io2 wr %04x %02x\n", addr, value));
    se5_bank = (value & 0x80) ? 1 : 0;
    flipflop();
}

static uint8_t se5_io2_read(uint16_t addr)
{
    addr |= 0xdf00;
    return roml_banks[(addr & 0x1fff) + (se5_bank << 13)];
}

static int se5_dump(void)
{
    mon_out("Bank: %d\n", se5_bank);
    mon_out("ROM is %s\n", se5_rom_enabled ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t se5_io1_device = {
    CARTRIDGE_NAME_SUPER_EXPLODE_V5, /* name of the device */
    IO_DETACH_CART,                  /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,           /* does not use a resource for detach */
    0xde00, 0xdeff, 0x01,            /* range for the device, regs:$de00-$deff */
    1,                               /* read is always valid */
    se5_io1_store,                   /* store function */
    NULL,                            /* NO poke function */
    se5_io1_read,                    /* read function */
    se5_io1_peek,                    /* NO peek function */
    se5_dump,                        /* device state information dump function */
    CARTRIDGE_SUPER_EXPLODE_V5,      /* cartridge ID */
    IO_PRIO_NORMAL,                  /* normal priority, device read needs to be checked for collisions */
    0,                               /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                   /* NO mirroring */
};

static io_source_t se5_io2_device = {
    CARTRIDGE_NAME_SUPER_EXPLODE_V5, /* name of the device */
    IO_DETACH_CART,                  /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,           /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,            /* range for the device, regs:$df00-$dfff */
    1,                               /* read is always valid */
    se5_io2_store,                   /* store function */
    NULL,                            /* NO poke function */
    se5_io2_read,                    /* read function */
    NULL,                            /* NO peek function */
    se5_dump,                        /* device state information dump function */
    CARTRIDGE_SUPER_EXPLODE_V5,      /* cartridge ID */
    IO_PRIO_NORMAL,                  /* normal priority, device read needs to be checked for collisions */
    0,                               /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE                   /* NO mirroring */
};

static io_source_list_t *se5_io1_list_item = NULL;
static io_source_list_t *se5_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_SUPER_EXPLODE_V5, 1, 1, &se5_io1_device, &se5_io2_device, CARTRIDGE_SUPER_EXPLODE_V5
};

/* ---------------------------------------------------------------------*/

uint8_t se5_roml_read(uint16_t addr)
{
    DBGRW(("%08lx se5_roml_read %04x %d\n", maincpu_clk, addr, se5_cap_charge));
    cap_discharge();
    return roml_banks[(addr & 0x1fff) + (roml_bank << 13)];
}

/* ---------------------------------------------------------------------*/

void se5_reset(void)
{
    se5_cap_charge = 0;
    se5_charge_time = maincpu_clk + 10;
    flipflop();
    cap_trigger_access();
}

void se5_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    cart_romlbank_set_slotmain(0);
    se5_bank = 0;
}

void se5_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, SE5_CART_SIZE);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    cart_romlbank_set_slotmain(0);
    se5_bank = 0;
}

/* ---------------------------------------------------------------------*/

static int se5_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    se5_io1_list_item = io_source_register(&se5_io1_device);
    se5_io2_list_item = io_source_register(&se5_io2_device);

    se5_alarm = alarm_new(maincpu_alarm_context, "SE5RomAlarm", se5_alarm_handler, NULL);
    se5_alarm_time = CLOCK_MAX;

    return 0;
}

int se5_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, SE5_CART_SIZE, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return se5_common_attach();
}

int se5_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 0x01; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 0x1f || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }

    return se5_common_attach();
}

void se5_detach(void)
{
    alarm_destroy(se5_alarm);
    export_remove(&export_res);
    io_source_unregister(se5_io1_list_item);
    se5_io1_list_item = NULL;
    io_source_unregister(se5_io2_list_item);
    se5_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTSE5 snapshot module format:

   type  | name | version | description
   --------------------------------------
   BYTE  | bank |   0.1   | current bank
   ARRAY | ROML |   0.0+  | 16384 BYTES of ROML data
 */

static const char snap_module_name[] = "CARTSE5";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int se5_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)se5_bank) < 0
        || SMW_BA(m, roml_banks, SE5_CART_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int se5_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

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
        if (SMR_B_INT(m, &se5_bank) < 0) {
            goto fail;
        }
    } else {
        se5_bank = 0;
    }

    if (SMR_BA(m, roml_banks, SE5_CART_SIZE) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return se5_common_attach();

fail:
    snapshot_module_close(m);
    return -1;
}
