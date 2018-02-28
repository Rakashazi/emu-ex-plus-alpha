/*
 * stardos.c - Cartridge handling, StarDOS cart.
 *
 * (w)2008 Groepaz/Hitmen
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

#include "alarm.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64mem.h"
#include "c64memrom.h"
#include "c64pla.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "stardos.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*  the stardos hardware is kindof perverted. it has two "registers", which
    are nothing more than the IO1 and/or IO2 line connected to a capacitor.
    the caps are then connected to a flipflop. now multiple reads of one of
    the "registers" charges a capacitor, which then when its charged enough
    causes the flipflop to switch. the output of the flipflop then controls
    the GAME line, ie it switches a rom bank at $8000 on or off.

    the original stardos code reads either $de61 or $dfa1 256 times in a loop
    to succesfully switch.

    the second rom bank contains a kernal replacement. the necessary select
    signal comes from a clip that has to be installed inside of the c64.
*/

/* #define DBGSTARDOS  */
/* #define DBGSTARDOSC */

#ifdef DBGSTARDOS
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static int roml_enable;

/* ---------------------------------------------------------------------*/

#define CHARGEMAX               5000000 /* 5.0v */
#define LOWTHRESHOLD            1400000 /* 1.4v */
#define CHARGEMAXIDLE           2000000 /* 2.0v */
#define HIGHTHRESHOLD           2700000 /* 2.7v */
#define CYCLES_CHARGE           64
#define CYCLES_DECHARGE         64
#define CYCLES_CHARGE_IDLE      2500000

struct alarm_s *stardos_alarm;
static CLOCK stardos_alarm_time;
static int cap_voltage = 0;

#ifdef DBGSTARDOSC
static int dbglast = 0;
#endif

static void flipflop(void)
{
#ifdef DBGSTARDOS
    int old = roml_enable;
#endif
    if (cap_voltage < LOWTHRESHOLD) {
        roml_enable = 0;
    }
    if (cap_voltage > HIGHTHRESHOLD) {
        roml_enable = 1;
    }
#ifdef DBGSTARDOS
    if (old != roml_enable) {
        DBG(("STARDOS: flipflop (%d)\n", roml_enable));
    }
#endif
}

static void cap_trigger_access(void)
{
    alarm_unset(stardos_alarm);
    stardos_alarm_time = CLOCK_MAX;

    if (cap_voltage < CHARGEMAXIDLE) {
        stardos_alarm_time = maincpu_clk + 1;
        alarm_set(stardos_alarm, stardos_alarm_time);
    }
#ifdef DBGSTARDOSCC
    else if (dbglast != 4) {
        DBG(("STARDOS: charged (idle) (%d)\n", cap_voltage));
        dbglast = 4;
    }
#endif
}

static void stardos_alarm_handler(CLOCK offset, void *data)
{
    cap_voltage += (CHARGEMAX / CYCLES_CHARGE_IDLE);
    if (cap_voltage > CHARGEMAXIDLE) {
        cap_voltage = CHARGEMAXIDLE;
    }
#ifdef DBGSTARDOSC
    else if (dbglast != 1) {
        DBG(("STARDOS: charge idle (%d)\n", cap_voltage));
        dbglast = 1;
    }
#endif
    flipflop();
    cap_trigger_access();
}

static void cap_charge(void)
{
    cap_voltage += (CHARGEMAX / CYCLES_CHARGE);
    if (cap_voltage > CHARGEMAX) {
        cap_voltage = CHARGEMAX;
    }
#ifdef DBGSTARDOSC
    else if (dbglast != 2) {
        DBG(("STARDOS: charge (%d)\n", cap_voltage));
        dbglast = 2;
    }
#endif
    flipflop();
    cap_trigger_access();
}

static void cap_discharge(void)
{
    cap_voltage -= (CHARGEMAX / CYCLES_DECHARGE);
    if (cap_voltage < 0) {
        cap_voltage = 0;
    }
#ifdef DBGSTARDOSC
    else if (dbglast != 3) {
        DBG(("STARDOS: discharge (%d)\n", cap_voltage));
        dbglast = 3;
    }
#endif
    flipflop();
    cap_trigger_access();
}

static BYTE stardos_io1_read(WORD addr)
{
    cap_charge();
    return 0;
}

static void stardos_io1_store(WORD addr, BYTE value)
{
    cap_charge();
}

static BYTE stardos_io_peek(WORD addr)
{
    return roml_enable;
}

static BYTE stardos_io2_read(WORD addr)
{
    cap_discharge();
    return 0;
}

static void stardos_io2_store(WORD addr, BYTE value)
{
    cap_discharge();
}

static int stardos_dump(void)
{
    mon_out("$8000-$9FFF ROM: %s\n", (roml_enable) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t stardos_io1_device = {
    CARTRIDGE_NAME_STARDOS,
    IO_DETACH_CART,
    NULL,
/*    0xde61, 0xde61, 0x01, */
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    stardos_io1_store,
    stardos_io1_read,
    stardos_io_peek,
    stardos_dump,
    CARTRIDGE_STARDOS,
    0,
    0
};

static io_source_t stardos_io2_device = {
    CARTRIDGE_NAME_STARDOS,
    IO_DETACH_CART,
    NULL,
/*    0xdfa1, 0xdfa1, 0x01, */
    0xdf00, 0xdfff, 0xff,
    0, /* read is never valid */
    stardos_io2_store,
    stardos_io2_read,
    stardos_io_peek,
    stardos_dump,
    CARTRIDGE_STARDOS,
    0,
    0
};

static io_source_list_t *stardos_io1_list_item = NULL;
static io_source_list_t *stardos_io2_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_STARDOS, 1, 1, &stardos_io1_device, &stardos_io2_device, CARTRIDGE_STARDOS
};

/* ---------------------------------------------------------------------*/

BYTE stardos_roml_read(WORD addr)
{
    if (roml_enable) {
        if ((pport.data & 1) == 1) {
            return roml_banks[(addr & 0x1fff)];
        }
    }
    return mem_read_without_ultimax(addr);
}

BYTE stardos_romh_read(WORD addr)
{
    if ((pport.data & 2) == 2) {
        return romh_banks[(addr & 0x1fff)];
    }
    return mem_read_without_ultimax(addr);
}

int stardos_romh_phi1_read(WORD addr, BYTE *value)
{
    return CART_READ_C64MEM;
}

int stardos_romh_phi2_read(WORD addr, BYTE *value)
{
    return stardos_romh_phi1_read(addr, value);
}

int stardos_peek_mem(export_t *export, WORD addr, BYTE *value)
{
    if (roml_enable) {
        if (addr >= 0x8000 && addr <= 0x9fff) {
            *value = roml_banks[addr & 0x1fff];
            return CART_READ_VALID;
        }
    }
    if (addr >= 0xe000) {
        *value = romh_banks[addr & 0x1fff];
        return CART_READ_VALID;
    }
    return CART_READ_THROUGH;
}

void stardos_config_init(void)
{
    flipflop();
    cap_trigger_access();
    cart_config_changed_slotmain(2, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

/* not sure, the original hardware doesn't work like this.
   should probably be left out unless it will cause problems */
#if 0
void stardos_reset(void)
{
    cap_voltage = 0;
}
#endif

void stardos_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, &rawcart[0], 0x2000);
    memcpy(romh_banks, &rawcart[0x2000], 0x2000);

    cart_config_changed_slotmain(2, 3, CMODE_READ);
}

/* ---------------------------------------------------------------------*/

static int stardos_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    stardos_alarm = alarm_new(maincpu_alarm_context, "StardosRomAlarm", stardos_alarm_handler, NULL);
    stardos_alarm_time = CLOCK_MAX;

    stardos_io1_list_item = io_source_register(&stardos_io1_device);
    stardos_io2_list_item = io_source_register(&stardos_io2_device);

    return 0;
}

int stardos_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    return stardos_common_attach();
}

int stardos_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i < 2; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            return -1;
        }

        if (chip.size != 0x2000 || (chip.start != 0x8000 && chip.start != 0xe000)) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.start & 0x2000, &chip, fd)) {
            return -1;
        }
    }

    return stardos_common_attach();
}

void stardos_detach(void)
{
    alarm_destroy(stardos_alarm);
    export_remove(&export_res);
    io_source_unregister(stardos_io1_list_item);
    io_source_unregister(stardos_io2_list_item);
    stardos_io1_list_item = NULL;
    stardos_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "CARTSTARDOS"

int stardos_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_DW(m, stardos_alarm_time) < 0)
        || (SMW_DW(m, (DWORD)cap_voltage) < 0)
        || (SMW_B(m, (BYTE)roml_enable) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)
        || (SMW_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int stardos_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    CLOCK temp_clk;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_DW(m, &temp_clk) < 0)
        || (SMR_DW_INT(m, &cap_voltage) < 0)
        || (SMR_B_INT(m, &roml_enable) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)
        || (SMR_BA(m, romh_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    if (stardos_common_attach() < 0) {
        return -1;
    }

    if (temp_clk < CLOCK_MAX) {
        stardos_alarm_time = temp_clk;
        alarm_set(stardos_alarm, stardos_alarm_time);
    }

    return 0;
}
