/*
 * vic20-sidcart.c - VIC20 specific SID cart emulation.
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

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "export.h"
#include "resources.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sid-snapshot.h"
#include "snapshot.h"
#include "translate.h"
#include "vic20.h"

/* ---------------------------------------------------------------------*/

static io_source_t sidcart_device = {
    CARTRIDGE_VIC20_NAME_SIDCART,
    IO_DETACH_RESOURCE,
    "SidCart",
    0x9800, 0x9bff, 0x3ff,
    1, /* read is always valid */
    sid_store,
    sid_read,
    NULL, /* TODO: peek */
    sid_dump,
    CARTRIDGE_VIC20_SIDCART,
    0,
    0
};

static io_source_list_t *sidcart_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_VIC20_NAME_SIDCART, 0, 0, &sidcart_device, NULL, CARTRIDGE_VIC20_SIDCART
};

/* ---------------------------------------------------------------------*/

static int sidcart_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    if (!sidcart_clock && cycles_per_sec == VIC20_PAL_CYCLES_PER_SEC) {
        return sid_sound_machine_init_vbr(psid, speed, cycles_per_sec, 1125);
    } else {
        return sid_sound_machine_init(psid, speed, cycles_per_sec);
    }
}

static sound_chip_t sidcart_sound_chip = {
    sid_sound_machine_open,
    sidcart_sound_machine_init,
    sid_sound_machine_close,
    sid_sound_machine_calculate_samples,
    sid_sound_machine_store,
    sid_sound_machine_read,
    sid_sound_machine_reset,
    sid_sound_machine_cycle_based,
    sid_sound_machine_channels,
    0 /* chip enabled */
};

static WORD sidcart_sound_chip_offset = 0;

void sidcart_sound_chip_init(void)
{
    sidcart_sound_chip_offset = sound_chip_register(&sidcart_sound_chip);
}

/* ---------------------------------------------------------------------*/

int sidcart_address;
int sidcart_clock;

int sidcart_enabled(void)
{
    return sidcart_sound_chip.chip_enabled;
}

static int sidcart_enable(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    sidcart_list_item = io_source_register(&sidcart_device);
    return 0;
}

static void sidcart_disable(void)
{
    if (sidcart_list_item != NULL) {
        export_remove(&export_res);
        io_source_unregister(sidcart_list_item);
        sidcart_list_item = NULL;
    }
}

static int set_sidcart_address(int val)
{
    WORD address = (WORD)val;

    switch (val) {
        case 0x9800:
        case 0x9c00:
            break;
        default:
            return -1;
    }

    if (sidcart_list_item != NULL) {
        sidcart_disable();
        sidcart_device.start_address = address;
        sidcart_device.end_address = address + 0x3ff;
        return sidcart_enable();
    }

    sidcart_device.start_address = address;
    sidcart_device.end_address = address + 0x3ff;

    return 0;
}

static int set_sidcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val != sidcart_sound_chip.chip_enabled) {
        if (val) {
            if (sidcart_enable() < 0) {
                return -1;
            }
        } else {
            sidcart_disable();
        }
        sidcart_sound_chip.chip_enabled = val;
        sound_state_changed = 1;
    }
    return 0;
}

static int set_sid_address(int val, void *param)
{
    if (val != sidcart_address) {
        sidcart_address = val;
        return set_sidcart_address(val);
    }
    return 0;
}

static int set_sid_clock(int val, void *param)
{
    switch (val) {
        case SIDCART_CLOCK_C64:
        case SIDCART_CLOCK_NATIVE:
            break;
        default:
            return -1;
    }

    if (val != sidcart_clock) {
        sidcart_clock = val;
        sid_state_changed = 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

static const resource_int_t sidcart_resources_int[] = {
    { "SidCart", 0, RES_EVENT_SAME, NULL,
      &sidcart_sound_chip.chip_enabled, set_sidcart_enabled, NULL },
    { "SidAddress", 0x9800, RES_EVENT_SAME, NULL,
      &sidcart_address, set_sid_address, NULL },
    { "SidClock", SIDCART_CLOCK_NATIVE, RES_EVENT_SAME, NULL,
      &sidcart_clock, set_sid_clock, NULL },
    { NULL }
};

int sidcart_resources_init(void)
{
    if (sid_resources_init() < 0) {
        return -1;
    }
    return resources_register_int(sidcart_resources_int);
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t sidcart_cmdline_options[] = {
    { "-sidcart", SET_RESOURCE, 1,
      NULL, NULL, "SidCart", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SIDCART,
      NULL, NULL },
    { "+sidcart", SET_RESOURCE, 0,
      NULL, NULL, "SidCart", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SIDCART,
      NULL, NULL },
    { "-sidcartaddress", SET_RESOURCE, 1,
      NULL, NULL, "SidAddress", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_ADDRESS, IDCLS_VIC20_SIDCART_ADDRESS,
      NULL, NULL },
    { "-sidcartclock", SET_RESOURCE, 1,
      NULL, NULL, "SidClock", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_CLOCK, IDCLS_PLUS4_SIDCART_CLOCK,
      NULL, NULL },
    { NULL }
};

int sidcart_cmdline_options_init(void)
{
    if (sid_cmdline_options_init() < 0) {
        return -1;
    }
    return cmdline_register_options(sidcart_cmdline_options);
}

/* ---------------------------------------------------------------------*/

void sidcart_detach(void)
{
    set_sidcart_enabled(0, NULL);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

/* SIDCART snapshot module format:

   type  | name    | description
   -----------------------------
   WORD  | address | sidcart address
   BYTE  | clock   | sidcart clock
 */

static char snap_module_name[] = "SIDCART";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int sidcart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_W(m, (WORD)sidcart_address) < 0
        || SMW_B(m, (BYTE)sidcart_clock) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return sid_snapshot_write_module(s);
}

int sidcart_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int tmp_address;
    int tmp_clock;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not allow versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    set_sidcart_enabled(0, NULL);

    if (0
        || SMR_W_INT(m, &tmp_address) < 0
        || SMR_B_INT(m, &tmp_clock) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    set_sid_address(tmp_address, NULL);
    set_sid_clock(tmp_clock, NULL);
    set_sidcart_enabled(1, NULL);

    return sid_snapshot_read_module(s);

fail:
    snapshot_module_close(m);
    return -1;
}
