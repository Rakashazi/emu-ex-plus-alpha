/*
 * ds12c887rtc.c - DS12C887 RTC based cartridge emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "ds12c887.h"
#include "ds12c887rtc.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sid.h"
#include "snapshot.h"
#include "uiapi.h"
#include "translate.h"

/*
    DS12C887 RTC Cartridge

    This cartridge is an RTC interface.

    When inserted into the cart port the cart uses 2 registers,
    one for setting the register being addressed and 1 for
    reading/writing the RTC register. The base address can be
    relocated to the following : $D500, $D600, $D700, $DE00, $DF00.
*/

/* RTC enabled ?? */
static int ds12c887rtc_enabled = 0;

/* RTC base address */
static int ds12c887rtc_base_address;

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static BYTE ds12c887rtc_read(WORD addr);
static void ds12c887rtc_store(WORD addr, BYTE byte);

static io_source_t ds12c887rtc_device = {
    CARTRIDGE_NAME_DS12C887RTC,
    IO_DETACH_RESOURCE,
    "DS12C887RTC",
    0xde00, 0xde01, 0xff,
    0,
    ds12c887rtc_store,
    ds12c887rtc_read,
    ds12c887rtc_read,
    NULL,
    CARTRIDGE_DS12C887RTC,
    0,
    0
};

static io_source_list_t *ds12c887rtc_list_item = NULL;

static c64export_resource_t export_res = {
    CARTRIDGE_NAME_DS12C887RTC, 0, 0, &ds12c887rtc_device, NULL, CARTRIDGE_DS12C887RTC
};

/* ds12c887 context */
static rtc_ds12c887_t *ds12c887rtc_context = NULL;

/* ds12c887 offset */
/* FIXME: Implement saving/setting/loading of the offset */
static time_t ds12c887rtc_offset = 0;

/* rtc ram */
/* FIXME: Implement saving/loading of the ram */
static char ds12c887rtc_ram[128];

/* ---------------------------------------------------------------------*/

int ds12c887rtc_cart_enabled(void)
{
    return ds12c887rtc_enabled;
}

static int set_ds12c887rtc_enabled(int val, void *param)
{
    if (!ds12c887rtc_enabled && val) {
        if (c64export_add(&export_res) < 0) {
            return -1;
        }
        ds12c887rtc_list_item = io_source_register(&ds12c887rtc_device);
        ds12c887rtc_context = ds12c887_init((BYTE *)ds12c887rtc_ram, &ds12c887rtc_offset);
        ds12c887rtc_enabled = 1;
    } else if (ds12c887rtc_enabled && !val) {
        if (ds12c887rtc_list_item != NULL) {
            c64export_remove(&export_res);
            io_source_unregister(ds12c887rtc_list_item);
            ds12c887rtc_list_item = NULL;
            ds12c887_destroy(ds12c887rtc_context);
        }
        ds12c887rtc_enabled = 0;
    }
    return 0;
}

static int set_ds12c887rtc_base(int val, void *param)
{
    int addr = val;
    int old = ds12c887rtc_enabled;

    if (val == ds12c887rtc_base_address) {
        return 0;
    }

    if (addr == 0xffff) {
        if (machine_class == VICE_MACHINE_VIC20) {
            addr = 0x9800;
        } else {
            addr = 0xde00;
        }
    }

    if (old) {
        set_ds12c887rtc_enabled(0, NULL);
    }

    switch (addr) {
        case 0xde00:
            if (machine_class != VICE_MACHINE_VIC20) {
                ds12c887rtc_device.start_address = (WORD)addr;
                ds12c887rtc_device.end_address = (WORD)(addr + 1);
                export_res.io1 = &ds12c887rtc_device;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0xdf00:
            if (machine_class != VICE_MACHINE_VIC20) {
                ds12c887rtc_device.start_address = (WORD)addr;
                ds12c887rtc_device.end_address = (WORD)(addr + 1);
                export_res.io1 = NULL;
                export_res.io2 = &ds12c887rtc_device;
            } else {
                return -1;
            }
            break;
/* FIXME: $d100/$d200/$d300 base address handling still needs to be implemented */
#if 0
        case 0xd100:
        case 0xd200:
        case 0xd300:
#endif
        case 0xd700:
            if (machine_class != VICE_MACHINE_VIC20) {
                ds12c887rtc_device.start_address = (WORD)addr;
                ds12c887rtc_device.end_address = (WORD)(addr + 1);
                export_res.io1 = NULL;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0xd500:
        case 0xd600:
            if (machine_class != VICE_MACHINE_VIC20 && machine_class != VICE_MACHINE_C128) {
                ds12c887rtc_device.start_address = (WORD)addr;
                ds12c887rtc_device.end_address = (WORD)(addr + 1);
                export_res.io1 = NULL;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0x9800:
        case 0x9c00:
            if (machine_class == VICE_MACHINE_VIC20) {
                ds12c887rtc_device.start_address = (WORD)addr;
                ds12c887rtc_device.end_address = (WORD)(addr + 1);
            } else {
                return -1;
            }
            break;
        default:
            return -1;
    }

    ds12c887rtc_base_address = val;

    if (old) {
        set_ds12c887rtc_enabled(1, NULL);
    }
    return 0;
}

void ds12c887rtc_reset(void)
{
    if (ds12c887rtc_context) {
        ds12c887_reset(ds12c887rtc_context);
    }
}

int ds12c887rtc_enable(void)
{
    return resources_set_int("DS12C887RTC", 1);
}

void ds12c887rtc_detach(void)
{
    resources_set_int("DS12C887RTC", 0);
}

/* ---------------------------------------------------------------------*/

static BYTE ds12c887rtc_read(WORD addr)
{
    if (addr & 1) {
        ds12c887rtc_device.io_source_valid = 1;
        return ds12c887_read(ds12c887rtc_context);
    }

    ds12c887rtc_device.io_source_valid = 0;

    return 0;
}

static void ds12c887rtc_store(WORD addr, BYTE byte)
{
    if (addr & 1) {
        ds12c887_store_data(ds12c887rtc_context, byte);
    } else {
        ds12c887_store_address(ds12c887rtc_context, byte);
    }
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "DS12C887RTC", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ds12c887rtc_enabled, set_ds12c887rtc_enabled, NULL },
    { "DS12C887RTCbase", 0xffff, RES_EVENT_NO, NULL,
      &ds12c887rtc_base_address, set_ds12c887rtc_base, NULL },
    { NULL }
};

int ds12c887rtc_resources_init(void)
{
    return resources_register_int(resources_int);
}

void ds12c887rtc_resources_shutdown(void)
{
    if (ds12c887rtc_context) {
        ds12c887_destroy(ds12c887rtc_context);
    }
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-ds12c887rtc", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTC", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DS12C887RTC,
      NULL, NULL },
    { "+ds12c887rtc", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTC", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DS12C887RTC,
      NULL, NULL },
    { "-ds12c887rtcbase", SET_RESOURCE, 1,
      NULL, NULL, "DS12C887RTCbase", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_BASE_ADDRESS, IDCLS_DS12C887RTC_BASE,
      NULL, NULL },
    { NULL }
};

int ds12c887rtc_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTDS12C887RTC"

int ds12c887rtc_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* FIXME: Implement the RTC snapshot part */
    if (0
        || (SMW_DW(m, (DWORD)ds12c887rtc_base_address) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int ds12c887rtc_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int temp_ds12c887rtc_address;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    /* FIXME: Implement the RTC snapshot part */
    if (0
        || (SMR_DW_INT(m, &temp_ds12c887rtc_address) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    /* HACK set address to an invalid value, then use the function */
    ds12c887rtc_base_address = -1;
    set_ds12c887rtc_base(temp_ds12c887rtc_address, NULL);

    return ds12c887rtc_enable();
}
