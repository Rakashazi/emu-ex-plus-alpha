/*
 * digimax.c - Digimax DAC cartridge emulation.
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
#include "digimax.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sid.h"
#include "snapshot.h"
#include "sound.h"
#include "uiapi.h"
#include "util.h"
#include "translate.h"

/*
    Digimax Cartridge

    This cartridge is an 8bit 4-channel digital sound output
    interface.

    When inserted into the cart port the cart uses 4 registers,
    one for each channel. The base address can be relocated
    through the entire I/O-1 and I/O-2 range in 0x20 increments.

    TODO: Userport pin description.
*/

/* DIGIMAX address */
int digimax_address;
sound_dac_t digimax_dac[4];

static char *digimax_address_list = NULL;

/* ---------------------------------------------------------------------*/

static io_source_t digimax_device = {
    CARTRIDGE_NAME_DIGIMAX,
    IO_DETACH_RESOURCE,
    "DIGIMAX",
    0xde00, 0xde03, 0x03,
    1, /* read is always valid */
    digimax_sound_store,
    digimax_sound_read,
    digimax_sound_read,
    NULL,
    CARTRIDGE_DIGIMAX,
    0,
    0
};

static io_source_list_t * digimax_list_item = NULL;

static c64export_resource_t export_res = {
    CARTRIDGE_NAME_DIGIMAX, 0, 0, &digimax_device, NULL, CARTRIDGE_DIGIMAX
};

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static int digimax_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int digimax_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void digimax_sound_machine_store(sound_t *psid, WORD addr, BYTE val);
static BYTE digimax_sound_machine_read(sound_t *psid, WORD addr);
static void digimax_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int digimax_sound_machine_cycle_based(void)
{
    return 0;
}

static int digimax_sound_machine_channels(void)
{
    return 1;     /* FIXME: needs to become stereo for stereo capable ports */
}

static sound_chip_t digimax_sound_chip = {
    NULL, /* no open */
    digimax_sound_machine_init,
    NULL, /* no close */
    digimax_sound_machine_calculate_samples,
    digimax_sound_machine_store,
    digimax_sound_machine_read,
    digimax_sound_reset,
    digimax_sound_machine_cycle_based,
    digimax_sound_machine_channels,
    0 /* chip enabled */
};

static WORD digimax_sound_chip_offset = 0;

void digimax_sound_chip_init(void)
{
    digimax_sound_chip_offset = sound_chip_register(&digimax_sound_chip);
}

/* ---------------------------------------------------------------------*/

int digimax_cart_enabled(void)
{
    return digimax_sound_chip.chip_enabled;
}

static BYTE digimax_sound_data[4];

int digimax_is_userport(void)
{
    return (digimax_address == 0xdd00);
}

void digimax_sound_store(WORD addr, BYTE value)
{
    digimax_sound_data[addr] = value;
    sound_store((WORD)(digimax_sound_chip_offset | addr), value, 0);
}

BYTE digimax_sound_read(WORD addr)
{
    BYTE value = sound_read((WORD)(digimax_sound_chip_offset | addr), 0);

    return value;
}

/* ---------------------------------------------------------------------*/

static int set_digimax_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!digimax_sound_chip.chip_enabled && val) {
        if (!digimax_is_userport()) {
            if (c64export_add(&export_res) < 0) {
                return -1;
            }
            digimax_list_item = io_source_register(&digimax_device);
        }
        digimax_sound_chip.chip_enabled = 1;
    } else if (digimax_sound_chip.chip_enabled && !val) {
        if (digimax_list_item != NULL) {
            c64export_remove(&export_res);
            io_source_unregister(digimax_list_item);
            digimax_list_item = NULL;
        }
        digimax_sound_chip.chip_enabled = 0;
    }
    return 0;
}

static int set_digimax_base(int val, void *param)
{
    int addr = val;
    int old = digimax_sound_chip.chip_enabled;

    if (val == digimax_address) {
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
        set_digimax_enabled(0, NULL);
    }

    switch (addr) {
        case 0xdd00:   /* special case, userport interface */
            break;
        case 0xde00:
        case 0xde20:
        case 0xde40:
        case 0xde60:
        case 0xde80:
        case 0xdea0:
        case 0xdec0:
        case 0xdee0:
            if (machine_class != VICE_MACHINE_VIC20) {
                digimax_device.start_address = (WORD)addr;
                digimax_device.end_address = (WORD)(addr + 3);
                export_res.io1 = &digimax_device;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0xdf00:
        case 0xdf20:
        case 0xdf40:
        case 0xdf60:
        case 0xdf80:
        case 0xdfa0:
        case 0xdfc0:
        case 0xdfe0:
            if (machine_class != VICE_MACHINE_VIC20) {
                digimax_device.start_address = (WORD)addr;
                digimax_device.end_address = (WORD)(addr + 3);
                export_res.io1 = NULL;
                export_res.io2 = &digimax_device;
            } else {
                return -1;
            }
            break;
        case 0x9800:
        case 0x9820:
        case 0x9840:
        case 0x9860:
        case 0x9880:
        case 0x98a0:
        case 0x98c0:
        case 0x98e0:
        case 0x9c00:
        case 0x9c20:
        case 0x9c40:
        case 0x9c60:
        case 0x9c80:
        case 0x9ca0:
        case 0x9cc0:
        case 0x9ce0:
            if (machine_class == VICE_MACHINE_VIC20) {
                digimax_device.start_address = (WORD)addr;
                digimax_device.end_address = (WORD)(addr + 3);
            } else {
                return -1;
            }
            break;
        default:
            return -1;
    }

    digimax_address = val;

    if (old) {
        set_digimax_enabled(1, NULL);
    }
    return 0;
}

void digimax_reset(void)
{
}

int digimax_enable(void)
{
    return resources_set_int("DIGIMAX", 1);
}

void digimax_detach(void)
{
    resources_set_int("DIGIMAX", 0);
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "DIGIMAX", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &digimax_sound_chip.chip_enabled, set_digimax_enabled, NULL },
    { "DIGIMAXbase", 0xffff, RES_EVENT_NO, NULL,
      &digimax_address, set_digimax_base, NULL },
    { NULL }
};

int digimax_resources_init(void)
{
    return resources_register_int(resources_int);
}

void digimax_resources_shutdown(void)
{
    if (digimax_address_list) {
        lib_free(digimax_address_list);
    }
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-digimax", SET_RESOURCE, 0,
      NULL, NULL, "DIGIMAX", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DIGIMAX,
      NULL, NULL },
    { "+digimax", SET_RESOURCE, 0,
      NULL, NULL, "DIGIMAX", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DIGIMAX,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-digimaxbase", SET_RESOURCE, 1,
      NULL, NULL, "DIGIMAXbase", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_DIGIMAX_BASE,
      NULL, NULL },
    { NULL }
};

int digimax_cmdline_options_init(void)
{
    char *temp1, *temp2;

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        temp1 = util_gen_hex_address_list(0x9800, 0x9900, 0x20);
        temp2 = util_gen_hex_address_list(0x9c00, 0x9d00, 0x20);
        digimax_address_list = util_concat(". (", temp1, "/", temp2, ")", NULL);        
        lib_free(temp2);
    } else {
        temp1 = util_gen_hex_address_list(0xde00, 0xe000, 0x20);
        digimax_address_list = util_concat(". (0xDD00: Userport/", temp1, ")", NULL);
    }
    lib_free(temp1);

    base_cmdline_options[0].description = digimax_address_list;

    return cmdline_register_options(base_cmdline_options);
}

/* ---------------------------------------------------------------------*/

struct digimax_sound_s {
    BYTE voice0;
    BYTE voice1;
    BYTE voice2;
    BYTE voice3;
};

static struct digimax_sound_s snd;

static int digimax_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    sound_dac_calculate_samples(&digimax_dac[0], pbuf, (int)snd.voice0 * 64, nr, soc, 1);
    sound_dac_calculate_samples(&digimax_dac[1], pbuf, (int)snd.voice1 * 64, nr, soc, (soc > 1) ? 2 : 1);
    sound_dac_calculate_samples(&digimax_dac[2], pbuf, (int)snd.voice2 * 64, nr, soc, 1);
    sound_dac_calculate_samples(&digimax_dac[3], pbuf, (int)snd.voice3 * 64, nr, soc, (soc > 1) ? 2 : 1);
    return nr;
}

static int digimax_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sound_dac_init(&digimax_dac[0], speed);
    sound_dac_init(&digimax_dac[1], speed);
    sound_dac_init(&digimax_dac[2], speed);
    sound_dac_init(&digimax_dac[3], speed);
    snd.voice0 = 0;
    snd.voice1 = 0;
    snd.voice2 = 0;
    snd.voice3 = 0;

    return 1;
}

static void digimax_sound_machine_store(sound_t *psid, WORD addr, BYTE val)
{
    switch (addr & 3) {
        case 0:
            snd.voice0 = val;
            break;
        case 1:
            snd.voice1 = val;
            break;
        case 2:
            snd.voice2 = val;
            break;
        case 3:
            snd.voice3 = val;
            break;
    }
}

static BYTE digimax_sound_machine_read(sound_t *psid, WORD addr)
{
    return digimax_sound_data[addr & 3];
}

static void digimax_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    snd.voice0 = 0;
    snd.voice1 = 0;
    snd.voice2 = 0;
    snd.voice3 = 0;
    digimax_sound_data[0] = 0;
    digimax_sound_data[1] = 0;
    digimax_sound_data[2] = 0;
    digimax_sound_data[3] = 0;
}

/* ---------------------------------------------------------------------*/

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTDIGIMAX"

int digimax_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_DW(m, (DWORD)digimax_address) < 0)
/* FIXME: implement userport part in userport_digimax.c */
#if 0
        || (SMW_B(m, digimax_userport_address) < 0)
        || (SMW_B(m, digimax_userport_direction_A) < 0)
        || (SMW_B(m, digimax_userport_direction_B) < 0)
#endif
        || (SMW_BA(m, digimax_sound_data, 4) < 0)
        || (SMW_B(m, snd.voice0) < 0)
        || (SMW_B(m, snd.voice1) < 0)
        || (SMW_B(m, snd.voice2) < 0)
        || (SMW_B(m, snd.voice3) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int digimax_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int temp_digimax_address;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_DW_INT(m, &temp_digimax_address) < 0)
/* FIXME: Implement the userport part in userport_digimax.c */
#if 0
        || (SMR_B(m, &digimax_userport_address) < 0)
        || (SMR_B(m, &digimax_userport_direction_A) < 0)
        || (SMR_B(m, &digimax_userport_direction_B) < 0)
#endif
        || (SMR_BA(m, digimax_sound_data, 4) < 0)
        || (SMR_B(m, &snd.voice0) < 0)
        || (SMR_B(m, &snd.voice1) < 0)
        || (SMR_B(m, &snd.voice2) < 0)
        || (SMR_B(m, &snd.voice3) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    /* HACK set address to an invalid value, then use the function */
    digimax_address = -1;
    set_digimax_base(temp_digimax_address, NULL);

    return digimax_enable();
}
