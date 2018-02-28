/*
 * epyxfastload.c - Cartridge handling, EPYX Fastload cart.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Fixed by
 *  Ingo Korb <ingo@akana.de>
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
#include "cartio.h"
#include "cartridge.h"
#include "epyxfastload.h"
#include "export.h"
#include "maincpu.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    "Epyx Fastload"

    - 8kb ROM, mapped to $8000 in 8k game config (if enabled)
    - last page of the rom is visible in io2

    The Epyx FastLoad cart uses a simple capacitor to toggle the ROM on and off:

    the capacitor is discharged, and 8k game config enabled, by
    - reading ROML
    - reading io1

    if none of that happens the capacitor will charge, and if it is charged
    then the ROM will get disabled.
*/

/* This constant defines the number of cycles it takes to recharge it.     */
#define EPYX_ROM_CYCLES 512

struct alarm_s *epyxrom_alarm;

static CLOCK epyxrom_alarm_time;

static int epyxrom_active = 0;

static void epyxfastload_trigger_access(void)
{
    /* Discharge virtual capacitor, enable rom */
    alarm_unset(epyxrom_alarm);
    epyxrom_alarm_time = maincpu_clk + EPYX_ROM_CYCLES;
    alarm_set(epyxrom_alarm, epyxrom_alarm_time);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    epyxrom_active = 1;
}

static void epyxfastload_alarm_handler(CLOCK offset, void *data)
{
    /* Virtual capacitor charged, disable rom */
    alarm_unset(epyxrom_alarm);
    epyxrom_alarm_time = CLOCK_MAX;
    cart_config_changed_slotmain(2, 2, CMODE_READ);
    epyxrom_active = 0;
}

/* ---------------------------------------------------------------------*/

static BYTE epyxfastload_io1_read(WORD addr)
{
    /* IO1 discharges the capacitor, but does nothing else */
    epyxfastload_trigger_access();
    return 0;
}

static BYTE epyxfastload_io1_peek(WORD addr)
{
    return 0;
}

static BYTE epyxfastload_io2_read(WORD addr)
{
    /* IO2 allows access to the last 256 bytes of the rom */
    return roml_banks[0x1f00 + (addr & 0xff)];
}

static int epyxfastload_dump(void)
{
    mon_out("ROM at $8000-$9FFF: %s\n", (epyxrom_active) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t epyxfastload_io1_device = {
    CARTRIDGE_NAME_EPYX_FASTLOAD,
    IO_DETACH_CART,
    NULL,
    0xde00, 0xdeff, 0xff,
    0, /* read is never valid */
    NULL,
    epyxfastload_io1_read,
    epyxfastload_io1_peek,
    epyxfastload_dump,
    CARTRIDGE_EPYX_FASTLOAD,
    0,
    0
};

static io_source_t epyxfastload_io2_device = {
    CARTRIDGE_NAME_EPYX_FASTLOAD,
    IO_DETACH_CART,
    NULL,
    0xdf00, 0xdfff, 0xff,
    1, /* read is always valid */
    NULL,
    epyxfastload_io2_read,
    epyxfastload_io2_read,
    epyxfastload_dump,
    CARTRIDGE_EPYX_FASTLOAD,
    0,
    0
};

static io_source_list_t *epyxfastload_io1_list_item = NULL;
static io_source_list_t *epyxfastload_io2_list_item = NULL;

static const export_resource_t export_res_epyx = {
    CARTRIDGE_NAME_EPYX_FASTLOAD, 0, 1, &epyxfastload_io1_device, &epyxfastload_io2_device, CARTRIDGE_EPYX_FASTLOAD
};

/* ---------------------------------------------------------------------*/

BYTE epyxfastload_roml_read(WORD addr)
{
    /* ROML accesses also discharge the capacitor */
    epyxfastload_trigger_access();

    return roml_banks[(addr & 0x1fff)];
}

/* ---------------------------------------------------------------------*/

void epyxfastload_reset(void)
{
    /* RESET discharges the capacitor so the rom is visible */
    epyxfastload_trigger_access();
}

void epyxfastload_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    epyxrom_active = 1;
}

void epyxfastload_config_setup(BYTE *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    epyxrom_active = 1;
}

/* ---------------------------------------------------------------------*/

static int epyxfastload_common_attach(void)
{
    if (export_add(&export_res_epyx) < 0) {
        return -1;
    }

    epyxrom_alarm = alarm_new(maincpu_alarm_context, "EPYXCartRomAlarm", epyxfastload_alarm_handler, NULL);
    epyxrom_alarm_time = CLOCK_MAX;

    epyxfastload_io1_list_item = io_source_register(&epyxfastload_io1_device);
    epyxfastload_io2_list_item = io_source_register(&epyxfastload_io2_device);

    return 0;
}

int epyxfastload_bin_attach(const char *filename, BYTE *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return epyxfastload_common_attach();
}

int epyxfastload_crt_attach(FILE *fd, BYTE *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return epyxfastload_common_attach();
}

void epyxfastload_detach(void)
{
    alarm_destroy(epyxrom_alarm);
    export_remove(&export_res_epyx);
    io_source_unregister(epyxfastload_io1_list_item);
    io_source_unregister(epyxfastload_io2_list_item);
    epyxfastload_io1_list_item = NULL;
    epyxfastload_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTEPYX snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | active |   0.1   | cartridge active flag
   DWORD | alarm  |   0.0+  | alarm time
   ARRAY | ROML   |   0.0+  | 8192 BYTES of ROML data
 */

static char snap_module_name[] = "CARTEPYX";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int epyxfastload_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (BYTE)epyxrom_active) < 0)
        || (SMW_DW(m, epyxrom_alarm_time) < 0)
        || (SMW_BA(m, roml_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int epyxfastload_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    CLOCK temp_clk;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* new in 0.1 */
    if (SNAPVAL(vmajor, vminor, 0, 1)) {
        if (SMR_B_INT(m, &epyxrom_active) < 0) {
            goto fail;
        }
    } else {
        epyxrom_active = 0;
    }

    if (0
        || (SMR_DW(m, &temp_clk) < 0)
        || (SMR_BA(m, roml_banks, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    if (epyxfastload_common_attach() < 0) {
        return -1;
    }

    if (temp_clk < CLOCK_MAX) {
        epyxrom_alarm_time = temp_clk;
        alarm_set(epyxrom_alarm, epyxrom_alarm_time);
    }

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
