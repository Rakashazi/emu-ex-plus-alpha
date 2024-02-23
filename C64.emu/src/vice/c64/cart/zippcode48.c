/*
 * zippcode48.c - Cartridge handling, ZIPP-CODE 48 cart.
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
#include <stdlib.h>
#include <string.h>

#include "alarm.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "cartio.h"
#include "cartridge.h"
#include "zippcode48.h"
#include "export.h"
#include "maincpu.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"

/*
    "ZIPP-CODE-48"

    - 8k (?) ROM mapped to $8000 in 8k game mode (needs to be confirmed)
    - 9e00-9eff is also mapped to de00-deff
    - IO1 reads enable the cartridge
    - IO2 reads disable the cartridge
*/

/* #define ZIPP_USE_CAP */

#ifdef ZIPP_USE_CAP

/* This constant defines the number of cycles it takes to recharge it.     */
#define ZIPP_ROM_CYCLES (63*312*100)

struct alarm_s *zipprom_alarm;

static CLOCK zipprom_alarm_time;

#endif

static int zipprom_active = 1;

static void zippcode48_trigger_access(void)
{
#ifdef ZIPP_USE_CAP
    /* Discharge virtual capacitor, enable rom */
    alarm_unset(zipprom_alarm);
    zipprom_alarm_time = maincpu_clk + ZIPP_ROM_CYCLES;
    alarm_set(zipprom_alarm, zipprom_alarm_time);
#endif
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    zipprom_active = 1;
}

#ifdef ZIPP_USE_CAP
static void zippcode48_alarm_handler(CLOCK offset, void *data)
{
    /* Virtual capacitor charged, disable rom */
    alarm_unset(zipprom_alarm);
    zipprom_alarm_time = CLOCK_MAX;
    cart_config_changed_slotmain(2, 2, CMODE_READ);
    zipprom_active = 0;
}
#endif

static void zippcode48_trigger_noaccess(void)
{
#ifdef ZIPP_USE_CAP
    /* Virtual capacitor charged, disable rom */
    alarm_unset(zipprom_alarm);
    zipprom_alarm_time = CLOCK_MAX;
#endif
    cart_config_changed_slotmain(2, 2, CMODE_READ);
    zipprom_active = 0;
}

/* ---------------------------------------------------------------------*/

static uint8_t zippcode48_io1_read(uint16_t addr)
{
    /* IO1 discharges the capacitor, but does nothing else */
    zippcode48_trigger_access();
    /* IO1 allows access to the second last 256 bytes of the rom */
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static uint8_t zippcode48_io1_peek(uint16_t addr)
{
    /* IO1 allows access to the second last 256 bytes of the rom */
    return roml_banks[0x1e00 + (addr & 0xff)];
}

static uint8_t zippcode48_io2_read(uint16_t addr)
{
    zippcode48_trigger_noaccess();
    return 0;
}

static uint8_t zippcode48_io2_peek(uint16_t addr)
{
    return 0;
}

static int zippcode48_dump(void)
{
    mon_out("ROM at $8000-$9FFF: %s\n", (zipprom_active) ? "enabled" : "disabled");

    return 0;
}

/* ---------------------------------------------------------------------*/

static io_source_t zippcode48_io1_device = {
    CARTRIDGE_NAME_ZIPPCODE48,  /* name of the device */
    IO_DETACH_CART,             /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,      /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,       /* range for the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    1,                          /* read is always valid */
    NULL,                       /* NO store function */
    NULL,                       /* NO poke funtion */
    zippcode48_io1_read,        /* read function */
    zippcode48_io1_peek,        /* peek function */
    zippcode48_dump,            /* device state information dump function */
    CARTRIDGE_ZIPPCODE48,       /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0,                          /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE              /* NO mirroring */
};

static io_source_t zippcode48_io2_device = {
    CARTRIDGE_NAME_ZIPPCODE48,  /* name of the device */
    IO_DETACH_CART,             /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,      /* does not use a resource for detach */
    0xdf00, 0xdfff, 0xff,       /* range for the device, regs:$df00-$dfff */
    0,                          /* read is never valid */
    NULL,                       /* NO store function */
    NULL,                       /* NO poke function */
    zippcode48_io2_read,        /* read function */
    zippcode48_io2_peek,        /* peek function */
    zippcode48_dump,            /* device state information dump function */
    CARTRIDGE_ZIPPCODE48,       /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0,                          /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE              /* NO mirroring */
};

static io_source_list_t *zippcode48_io1_list_item = NULL;
static io_source_list_t *zippcode48_io2_list_item = NULL;

static const export_resource_t export_res_zipp = {
    CARTRIDGE_NAME_ZIPPCODE48, 0, 1, &zippcode48_io1_device, &zippcode48_io2_device, CARTRIDGE_ZIPPCODE48
};

/* ---------------------------------------------------------------------*/

uint8_t zippcode48_roml_read(uint16_t addr)
{
    /* ROML accesses also discharge the capacitor */
    zippcode48_trigger_access();

    return roml_banks[(addr & 0x1fff)];
}

/* ---------------------------------------------------------------------*/

void zippcode48_reset(void)
{
    /* RESET discharges the capacitor so the rom is visible */
    zippcode48_trigger_access();
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
}

void zippcode48_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    zipprom_active = 1;
}

void zippcode48_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    zipprom_active = 1;
}

/* ---------------------------------------------------------------------*/

static int zippcode48_common_attach(void)
{
    if (export_add(&export_res_zipp) < 0) {
        return -1;
    }

#ifdef ZIPP_USE_CAP
    zipprom_alarm = alarm_new(maincpu_alarm_context, "ZIPPCartRomAlarm", zippcode48_alarm_handler, NULL);
    zipprom_alarm_time = CLOCK_MAX;
#endif
    zippcode48_io1_list_item = io_source_register(&zippcode48_io1_device);
    zippcode48_io2_list_item = io_source_register(&zippcode48_io2_device);

    return 0;
}

int zippcode48_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return zippcode48_common_attach();
}

int zippcode48_crt_attach(FILE *fd, uint8_t *rawcart)
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

    return zippcode48_common_attach();
}

void zippcode48_detach(void)
{
#ifdef ZIPP_USE_CAP
    alarm_destroy(zipprom_alarm);
#endif
    export_remove(&export_res_zipp);
    io_source_unregister(zippcode48_io1_list_item);
    io_source_unregister(zippcode48_io2_list_item);
    zippcode48_io1_list_item = NULL;
    zippcode48_io2_list_item = NULL;
}

/* ---------------------------------------------------------------------*/

/* CARTZIPP snapshot module format:

   type  | name   | version | description
   --------------------------------------
   BYTE  | active |   0.1   | cartridge active flag
   DWORD | alarm  |   0.0+  | alarm time
   ARRAY | ROML   |   0.0+  | 8192 BYTES of ROML data
 */

static const char snap_module_name[] = "CARTZIPP";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int zippcode48_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)zipprom_active) < 0)
#ifdef ZIPP_USE_CAP
        || (SMW_DW(m, zipprom_alarm_time) < 0)
#endif
        || (SMW_BA(m, roml_banks, 0x2000) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int zippcode48_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

#ifdef ZIPP_USE_CAP
    CLOCK temp_clk;
#endif
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
        if (SMR_B_INT(m, &zipprom_active) < 0) {
            goto fail;
        }
    } else {
        zipprom_active = 0;
    }

    if (0
#ifdef ZIPP_USE_CAP
        || (SMR_DW(m, &temp_clk) < 0)
#endif
        || (SMR_BA(m, roml_banks, 0x2000) < 0)) {
        goto fail;
    }

    snapshot_module_close(m);

    if (zippcode48_common_attach() < 0) {
        return -1;
    }

#ifdef ZIPP_USE_CAP
    if (temp_clk < CLOCK_MAX) {
        zipprom_alarm_time = temp_clk;
        alarm_set(zipprom_alarm, zipprom_alarm_time);
    }
#endif
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
