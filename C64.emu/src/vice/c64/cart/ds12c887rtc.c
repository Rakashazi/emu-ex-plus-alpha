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

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "ds12c887.h"
#include "ds12c887rtc.h"
#include "export.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "rtc.h"
#include "sid.h"
#include "snapshot.h"
#include "uiapi.h"
#include "translate.h"

#define RTC_RUNMODE_HALTED    0
#define RTC_RUNMODE_RUNNING   1
#define RTC_RUNMODE_CURRENT   2

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

/* RTC run mode at first start */
static int ds12c887rtc_run_mode = -1;

/* RTC data save ?? */
static int ds12c887rtc_save;

static int ds12c887rtc_accessed = 0;

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static BYTE ds12c887rtc_read(WORD addr);
static void ds12c887rtc_store(WORD addr, BYTE byte);
static int ds12c887rtc_dump(void);

static io_source_t ds12c887rtc_device = {
    CARTRIDGE_NAME_DS12C887RTC,
    IO_DETACH_RESOURCE,
    "DS12C887RTC",
    0xde00, 0xde01, 0xff,
    0,
    ds12c887rtc_store,
    ds12c887rtc_read,
    ds12c887rtc_read,
    ds12c887rtc_dump,
    CARTRIDGE_DS12C887RTC,
    0,
    0
};

static io_source_list_t *ds12c887rtc_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_NAME_DS12C887RTC, 0, 0, &ds12c887rtc_device, NULL, CARTRIDGE_DS12C887RTC
};

/* ds12c887 context */
static rtc_ds12c887_t *ds12c887rtc_context = NULL;

/* ---------------------------------------------------------------------*/

int ds12c887rtc_cart_enabled(void)
{
    return ds12c887rtc_enabled;
}

static int set_ds12c887rtc_enabled(int value, void *param)
{
    int val = value ? 1 : 0;
    int runmode;

    if (!ds12c887rtc_enabled && val) {
        if (ds12c887rtc_accessed) {
            runmode = RTC_RUNMODE_CURRENT;
        } else {
            if (ds12c887rtc_run_mode == -1) {
                runmode = RTC_RUNMODE_RUNNING;
            } else {
                runmode = ds12c887rtc_run_mode;
            }
        }
        if (export_add(&export_res) < 0) {
            return -1;
        }
        ds12c887rtc_list_item = io_source_register(&ds12c887rtc_device);
        ds12c887rtc_context = ds12c887_init("DS12C887");
        if (runmode == RTC_RUNMODE_HALTED) {
            ds12c887rtc_context->clock_halt_latch = rtc_get_latch(0);
            ds12c887rtc_context->clock_halt = 1;
            ds12c887rtc_context->ctrl_regs[0] = 0;
        }
        ds12c887rtc_enabled = 1;
    } else if (ds12c887rtc_enabled && !val) {
        if (ds12c887rtc_list_item != NULL) {
            export_remove(&export_res);
            io_source_unregister(ds12c887rtc_list_item);
            ds12c887rtc_list_item = NULL;
            if (ds12c887rtc_context) {
                ds12c887_destroy(ds12c887rtc_context, ds12c887rtc_save);
                ds12c887rtc_context = NULL;
            }
        }
        ds12c887rtc_enabled = 0;
    }
    return 0;
}

static int set_ds12c887rtc_save(int val, void *param)
{
    ds12c887rtc_save = val ? 1 : 0;

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

static int set_ds12c887rtc_run_mode(int val, void *param)
{
    ds12c887rtc_run_mode = val ? 1 : 0;

    if (ds12c887rtc_enabled) {
        set_ds12c887rtc_enabled(0, NULL);
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

static int ds12c887rtc_dump(void)
{
    return ds12c887_dump(ds12c887rtc_context);
}

static BYTE ds12c887rtc_read(WORD addr)
{
    if (addr & 1) {
        ds12c887rtc_accessed = 1;
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
    ds12c887rtc_accessed = 1;
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "DS12C887RTC", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ds12c887rtc_enabled, set_ds12c887rtc_enabled, NULL },
    { "DS12C887RTCbase", 0xffff, RES_EVENT_NO, NULL,
      &ds12c887rtc_base_address, set_ds12c887rtc_base, NULL },
    { "DS12C887RTCRunMode", RTC_RUNMODE_RUNNING, RES_EVENT_NO, NULL,
      &ds12c887rtc_run_mode, set_ds12c887rtc_run_mode, NULL },
    { "DS12C887RTCSave", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ds12c887rtc_save, set_ds12c887rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int ds12c887rtc_resources_init(void)
{
    return resources_register_int(resources_int);
}

void ds12c887rtc_resources_shutdown(void)
{
    if (ds12c887rtc_context) {
        ds12c887_destroy(ds12c887rtc_context, ds12c887rtc_save);
        ds12c887rtc_context = NULL;
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
    { "-ds12c887rtchalted", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTCRunMode", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DS12C887RTC_RUNMODE_HALTED,
      NULL, NULL },
    { "-ds12c887rtcrunning", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTCRunMode", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DS12C887RTC_RUNMODE_RUNNING,
      NULL, NULL },
    { "-ds12c887rtcsave", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTCSave", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DS12C887RTC_SAVE,
      NULL, NULL },
    { "+ds12c887rtcsave", SET_RESOURCE, 0,
      NULL, NULL, "DS12C887RTCSave", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DS12C887RTC_SAVE,
      NULL, NULL },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-ds12c887rtcbase", SET_RESOURCE, 1,
      NULL, NULL, "DS12C887RTCbase", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_DS12C887RTC_BASE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int ds12c887rtc_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        base_cmdline_options[0].description = ". (0x9800/0x9C00)";
    } else if (machine_class == VICE_MACHINE_C128) {
        base_cmdline_options[0].description = ". (0xD700/0xDE00/0xDF00)";
    } else {
        base_cmdline_options[0].description = ". (0xD500/0xD600/0xD700/0xDE00/0xDF00)";
    }

    return cmdline_register_options(base_cmdline_options);
}

/* ---------------------------------------------------------------------*/

/* CARTDS12C887RTC snapshot module format:

   type  | name     | description
   ------------------------------
   DWORD | base     | base address of the RTC
 */

static char snap_module_name[] = "CARTDS12C887RTC";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ds12c887rtc_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_DW(m, (DWORD)ds12c887rtc_base_address) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return ds12c887_write_snapshot(ds12c887rtc_context, s);
}

int ds12c887rtc_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int temp_ds12c887rtc_address;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_DW_INT(m, &temp_ds12c887rtc_address) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    /* HACK set address to an invalid value, then use the function */
    ds12c887rtc_base_address = -1;
    set_ds12c887rtc_base(temp_ds12c887rtc_address, NULL);

    if (ds12c887rtc_enable() < 0) {
        return -1;
    }

    return ds12c887_read_snapshot(ds12c887rtc_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
