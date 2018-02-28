/*
 * sfx_soundexpander.c - SFX soundexpander cartridge emulation.
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
#include "export.h"
#include "fmopl.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "sfx_soundexpander.h"
#include "sid.h"
#include "snapshot.h"
#include "sound.h"
#include "uiapi.h"
#include "translate.h"

/*
    Note: this cartridge has a passthrough port, which for some odd reason does
          connect all lines 1:1 straight through, EXCEPT for these:

          A1 goes to A0 at the passthrough port
          R/!W goes to A1 at the passthrough port

    see http://www.zimmers.net/anonftp/pub/cbm/schematics/cartridges/c64/sfx/sfx-sch.gif
 */

/* Flag: What type of ym chip is used?  */
int sfx_soundexpander_chip = 3526;

static FM_OPL *YM3526_chip = NULL;
static FM_OPL *YM3812_chip = NULL;

/* ------------------------------------------------------------------------- */

/* some prototypes are needed */
static void sfx_soundexpander_sound_store(WORD addr, BYTE value);
static BYTE sfx_soundexpander_sound_read(WORD addr);
static BYTE sfx_soundexpander_sound_peek(WORD addr);
static BYTE sfx_soundexpander_piano_read(WORD addr);

static io_source_t sfx_soundexpander_sound_device = {
    CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
    IO_DETACH_RESOURCE,
    "SFXSoundExpander",
    0xdf00, 0xdfff, 0x7f,
    0,
    sfx_soundexpander_sound_store,
    sfx_soundexpander_sound_read,
    sfx_soundexpander_sound_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_SFX_SOUND_EXPANDER,
    0,
    0
};

static io_source_t sfx_soundexpander_piano_device = {
    CARTRIDGE_NAME_SFX_SOUND_EXPANDER,
    IO_DETACH_RESOURCE,
    "SFXSoundExpander",
    0xdf00, 0xdfff, 0x1f,
    0,
    NULL,
    sfx_soundexpander_piano_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    CARTRIDGE_SFX_SOUND_EXPANDER,
    0,
    0
};

static io_source_list_t *sfx_soundexpander_sound_list_item = NULL;
static io_source_list_t *sfx_soundexpander_piano_list_item = NULL;

static const export_resource_t export_res_sound = {
    CARTRIDGE_NAME_SFX_SOUND_EXPANDER, 0, 0, NULL, &sfx_soundexpander_sound_device, CARTRIDGE_SFX_SOUND_EXPANDER
};

static const export_resource_t export_res_piano = {
    CARTRIDGE_NAME_SFX_SOUND_EXPANDER, 0, 0, NULL, &sfx_soundexpander_piano_device, CARTRIDGE_SFX_SOUND_EXPANDER
};

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int sfx_soundexpander_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static void sfx_soundexpander_sound_machine_close(sound_t *psid);
static int sfx_soundexpander_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void sfx_soundexpander_sound_machine_store(sound_t *psid, WORD addr, BYTE val);
static BYTE sfx_soundexpander_sound_machine_read(sound_t *psid, WORD addr);
static void sfx_soundexpander_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int sfx_soundexpander_sound_machine_cycle_based(void)
{
    return 0;
}

static int sfx_soundexpander_sound_machine_channels(void)
{
    return 1;     /* FIXME: needs to become stereo for stereo capable ports */
}

static sound_chip_t sfx_soundexpander_sound_chip = {
    NULL, /* no open */
    sfx_soundexpander_sound_machine_init,
    sfx_soundexpander_sound_machine_close,
    sfx_soundexpander_sound_machine_calculate_samples,
    sfx_soundexpander_sound_machine_store,
    sfx_soundexpander_sound_machine_read,
    sfx_soundexpander_sound_reset,
    sfx_soundexpander_sound_machine_cycle_based,
    sfx_soundexpander_sound_machine_channels,
    0 /* chip enabled */
};

static WORD sfx_soundexpander_sound_chip_offset = 0;

void sfx_soundexpander_sound_chip_init(void)
{
    sfx_soundexpander_sound_chip_offset = sound_chip_register(&sfx_soundexpander_sound_chip);
}

/* ------------------------------------------------------------------------- */

static int sfx_soundexpander_io_swap = 0;

int sfx_soundexpander_cart_enabled(void)
{
    return sfx_soundexpander_sound_chip.chip_enabled;
}

static int set_sfx_soundexpander_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (sfx_soundexpander_sound_chip.chip_enabled != val) {
        if (val) {
            if (export_add(&export_res_sound) < 0) {
                return -1;
            }
            if (export_add(&export_res_piano) < 0) {
                return -1;
            }
            if (machine_class == VICE_MACHINE_VIC20) {
                if (sfx_soundexpander_io_swap) {
                    sfx_soundexpander_sound_device.start_address = 0x9800;
                    sfx_soundexpander_sound_device.end_address = 0x9bff;
                    sfx_soundexpander_piano_device.start_address = 0x9800;
                    sfx_soundexpander_piano_device.end_address = 0x9bff;
                } else {
                    sfx_soundexpander_sound_device.start_address = 0x9c00;
                    sfx_soundexpander_sound_device.end_address = 0x9fff;
                    sfx_soundexpander_piano_device.start_address = 0x9c00;
                    sfx_soundexpander_piano_device.end_address = 0x9fff;
                }
            }
            sfx_soundexpander_sound_list_item = io_source_register(&sfx_soundexpander_sound_device);
            sfx_soundexpander_piano_list_item = io_source_register(&sfx_soundexpander_piano_device);
            sfx_soundexpander_sound_chip.chip_enabled = 1;
        } else {
            export_remove(&export_res_sound);
            export_remove(&export_res_piano);
            io_source_unregister(sfx_soundexpander_sound_list_item);
            io_source_unregister(sfx_soundexpander_piano_list_item);
            sfx_soundexpander_sound_list_item = NULL;
            sfx_soundexpander_piano_list_item = NULL;
            sfx_soundexpander_sound_chip.chip_enabled = 0;
        }
    }
    return 0;
}

static int set_sfx_soundexpander_chip(int val, void *param)
{
    switch (val) {
        case 3526:
        case 3812:
            break;
        default:
            return -1;
    }

    if (val != sfx_soundexpander_chip) {
        sid_state_changed = 1;
        sfx_soundexpander_chip = val;
    }
    return 0;
}

static int set_sfx_soundexpander_io_swap(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val == sfx_soundexpander_io_swap) {
        return 0;
    }

    if (sfx_soundexpander_sound_chip.chip_enabled) {
        set_sfx_soundexpander_enabled(0, NULL);
        sfx_soundexpander_io_swap = val;
        set_sfx_soundexpander_enabled(1, NULL);
    } else {
        sfx_soundexpander_io_swap = val;
    }
    return 0;
}

void sfx_soundexpander_reset(void)
{
    /* TODO: do nothing ? */
}

int sfx_soundexpander_enable(void)
{
    return resources_set_int("SFXSoundExpander", 1);
}

void sfx_soundexpander_detach(void)
{
    resources_set_int("SFXSoundExpander", 0);
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_int[] = {
    { "SFXSoundExpander", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &sfx_soundexpander_sound_chip.chip_enabled, set_sfx_soundexpander_enabled, NULL },
    { "SFXSoundExpanderChip", 3526, RES_EVENT_STRICT, (resource_value_t)3526,
      &sfx_soundexpander_chip, set_sfx_soundexpander_chip, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_int_t resources_mascuerade_int[] = {
    { "SFXSoundExpanderIOSwap", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &sfx_soundexpander_io_swap, set_sfx_soundexpander_io_swap, NULL },
    RESOURCE_INT_LIST_END
};

int sfx_soundexpander_resources_init(void)
{
    if (machine_class == VICE_MACHINE_VIC20) {
        if (resources_register_int(resources_mascuerade_int) < 0) {
            return -1;
        }
    }
    return resources_register_int(resources_int);
}

void sfx_soundexpander_resources_shutdown(void)
{
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-sfxse", SET_RESOURCE, 0,
      NULL, NULL, "SFXSoundExpander", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SFX_SE,
      NULL, NULL },
    { "+sfxse", SET_RESOURCE, 0,
      NULL, NULL, "SFXSoundExpander", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SFX_SE,
      NULL, NULL },
    { "-sfxsetype", SET_RESOURCE, 1,
      NULL, NULL, "SFXSoundExpanderChip", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_SET_YM_CHIP_TYPE,
      NULL, NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t cmdline_mascuerade_options[] =
{
    { "-sfxseioswap", SET_RESOURCE, 0,
      NULL, NULL, "SFXSoundExpanderIOSwap", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MAP_CART_IO_2,
      NULL, NULL },
    { "+sfxseioswap", SET_RESOURCE, 0,
      NULL, NULL, "SFXSoundExpanderIOSwap", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MAP_CART_IO_3,
      NULL, NULL },
    CMDLINE_LIST_END
};

int sfx_soundexpander_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_VIC20) {
        if (cmdline_register_options(cmdline_mascuerade_options) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

struct sfx_soundexpander_sound_s {
    BYTE command;
};

static struct sfx_soundexpander_sound_s snd;

static int sfx_soundexpander_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    SWORD *buffer;

    buffer = lib_malloc(nr * 2);

    if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
        ym3812_update_one(YM3812_chip, buffer, nr);
    } else if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
        ym3526_update_one(YM3526_chip, buffer, nr);
    }

    for (i = 0; i < nr; i++) {
        pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], buffer[i]);
        if (soc > 1) {
            pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], buffer[i]);
        }
    }
    lib_free(buffer);

    return nr;
}

static int sfx_soundexpander_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    if (sfx_soundexpander_chip == 3812) {
        if (YM3812_chip != NULL) {
            ym3812_shutdown(YM3812_chip);
        }
        YM3812_chip = ym3812_init((UINT32)3579545, (UINT32)speed);
    } else {
        if (YM3526_chip != NULL) {
            ym3526_shutdown(YM3526_chip);
        }
        YM3526_chip = ym3526_init((UINT32)3579545, (UINT32)speed);
    }
    snd.command = 0;

    return 1;
}

static void sfx_soundexpander_sound_machine_close(sound_t *psid)
{
    if (YM3526_chip != NULL) {
        ym3526_shutdown(YM3526_chip);
        YM3526_chip = NULL;
    }
    if (YM3812_chip != NULL) {
        ym3812_shutdown(YM3812_chip);
        YM3812_chip = NULL;
    }
}

static void sfx_soundexpander_sound_machine_store(sound_t *psid, WORD addr, BYTE val)
{
    snd.command = val;

    if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
        ym3812_write(YM3812_chip, 1, val);
    } else if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
        ym3526_write(YM3526_chip, 1, val);
    }
}

static BYTE sfx_soundexpander_sound_machine_read(sound_t *psid, WORD addr)
{
    if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
        return ym3812_read(YM3812_chip, 1);
    }
    if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
        return ym3526_read(YM3526_chip, 1);
    }
    return 0;
}

static void sfx_soundexpander_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
        ym3812_reset_chip(YM3812_chip);
    } else if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
        ym3526_reset_chip(YM3526_chip);
    }
}

/* ---------------------------------------------------------------------*/

static void sfx_soundexpander_sound_store(WORD addr, BYTE value)
{
    if (addr == 0x40) {
        if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
            ym3812_write(YM3812_chip, 0, value);
        } else if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
            ym3526_write(YM3526_chip, 0, value);
        }
    }
    if (addr == 0x50) {
        sound_store(sfx_soundexpander_sound_chip_offset, value, 0);
    }
}

static BYTE sfx_soundexpander_sound_read(WORD addr)
{
    BYTE value = 0;

    sfx_soundexpander_sound_device.io_source_valid = 0;

    if (addr == 0x60) {
        if ((sfx_soundexpander_chip == 3812 && YM3812_chip)
            || (sfx_soundexpander_chip == 3526 && YM3526_chip)) {
            sfx_soundexpander_sound_device.io_source_valid = 1;
            value = sound_read(sfx_soundexpander_sound_chip_offset, 0);
        }
    }
    return value;
}

static BYTE sfx_soundexpander_sound_peek(WORD addr)
{
    BYTE value = 0;

    if (addr == 0x40) {
        if (sfx_soundexpander_chip == 3812 && YM3812_chip) {
            value = ym3812_peek(YM3812_chip, value);
        } else if (sfx_soundexpander_chip == 3526 && YM3526_chip) {
            value = ym3526_peek(YM3526_chip, value);
        }
    }
    return value;
}

/* No piano keyboard is emulated currently, so we return 0xff */
static BYTE sfx_soundexpander_piano_read(WORD addr)
{
    sfx_soundexpander_piano_device.io_source_valid = 0;
    if ((addr & 16) == 0 && (addr & 8) == 8) {
        sfx_soundexpander_piano_device.io_source_valid = 1;
    }
    return (BYTE)0xff;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

/* CARTSFXSE snapshot module format:

   type  | name                  | version | description
   -----------------------------------------------------
   BYTE  | IO swap               |   0.1   | VIC20 I/O swap flag
   DWORD | chip type             |   0.0+  | chip type
   BYTE  | sound command         |   0.0+  | sound command
   DWORD | ch 0 slot 0 ar        |   0.0+  | channel 0, slot 0, ar
   DWORD | ch 0 slot 0 dr        |   0.0+  | channel 0, slot 0, dr
   DWORD | ch 0 slot 0 rr        |   0.0+  | channel 0, slot 0, rr
   BYTE  | ch 0 slot 0 KSR       |   0.0+  | channel 0, slot 0, KSR
   BYTE  | ch 0 slot 0 ksl       |   0.0+  | channel 0, slot 0, ksl
   BYTE  | ch 0 slot 0 ksr       |   0.0+  | channel 0, slot 0, ksr
   BYTE  | ch 0 slot 0 mul       |   0.0+  | channel 0, slot 0, mul
   DWORD | ch 0 slot 0 Cnt       |   0.0+  | channel 0, slot 0, Cnt
   DWORD | ch 0 slot 0 Incr      |   0.0+  | channel 0, slot 0, Incr
   BYTE  | ch 0 slot 0 FB        |   0.0+  | channel 0, slot 0, FB
   DWORD | ch 0 slot 0 connect1  |   0.0+  | channel 0, slot 0, connect1
   DWORD | ch 0 slot 0 op1 out 0 |   0.0+  | channel 0, slot 0, op1 out 0
   DWORD | ch 0 slot 0 op1 out 1 |   0.0+  | channel 0, slot 0, op1 out 1
   BYTE  | ch 0 slot 0 CON       |   0.0+  | channel 0, slot 0, CON
   BYTE  | ch 0 slot 0 eg type   |   0.0+  | channel 0, slot 0, eg type
   BYTE  | ch 0 slot 0 state     |   0.0+  | channel 0, slot 0, state
   DWORD | ch 0 slot 0 TL        |   0.0+  | channel 0, slot 0, TL
   DWORD | ch 0 slot 0 TLL       |   0.0+  | channel 0, slot 0, TLL
   DWORD | ch 0 slot 0 volume    |   0.0+  | channel 0, slot 0, volume
   DWORD | ch 0 slot 0 sl        |   0.0+  | channel 0, slot 0, sl
   BYTE  | ch 0 slot 0 eg sh ar  |   0.0+  | channel 0, slot 0, eg sh ar
   BYTE  | ch 0 slot 0 eg sel ar |   0.0+  | channel 0, slot 0, eg sel ar
   BYTE  | ch 0 slot 0 eg sh dr  |   0.0+  | channel 0, slot 0, eg sh dr
   BYTE  | ch 0 slot 0 eg sel dr |   0.0+  | channel 0, slot 0, eg sel dr
   BYTE  | ch 0 slot 0 eg sh rr  |   0.0+  | channel 0, slot 0, eg sh rr
   BYTE  | ch 0 slot 0 eg sel rr |   0.0+  | channel 0, slot 0, eg sel rr
   DWORD | ch 0 slot 0 key       |   0.0+  | channel 0, slot 0, key
   DWORD | ch 0 slot 0 AMmask    |   0.0+  | channel 0, slot 0, AMmask
   BYTE  | ch 0 slot 0 vib       |   0.0+  | channel 0, slot 0, vib
   WORD  | ch 0 slot 0 wavetable |   0.0+  | channel 0, slot 0, wavetable
   DWORD | ch 0 slot 1 ar        |   0.0+  | channel 0, slot 1, ar
   DWORD | ch 0 slot 1 dr        |   0.0+  | channel 0, slot 1, dr
   DWORD | ch 0 slot 1 rr        |   0.0+  | channel 0, slot 1, rr
   BYTE  | ch 0 slot 1 KSR       |   0.0+  | channel 0, slot 1, KSR
   BYTE  | ch 0 slot 1 ksl       |   0.0+  | channel 0, slot 1, ksl
   BYTE  | ch 0 slot 1 ksr       |   0.0+  | channel 0, slot 1, ksr
   BYTE  | ch 0 slot 1 mul       |   0.0+  | channel 0, slot 1, mul
   DWORD | ch 0 slot 1 Cnt       |   0.0+  | channel 0, slot 1, Cnt
   DWORD | ch 0 slot 1 Incr      |   0.0+  | channel 0, slot 1, Incr
   BYTE  | ch 0 slot 1 FB        |   0.0+  | channel 0, slot 1, FB
   DWORD | ch 0 slot 1 connect1  |   0.0+  | channel 0, slot 1, connect1
   DWORD | ch 0 slot 1 op1 out 0 |   0.0+  | channel 0, slot 1, op1 out 0
   DWORD | ch 0 slot 1 op1 out 1 |   0.0+  | channel 0, slot 1, op1 out 1
   BYTE  | ch 0 slot 1 CON       |   0.0+  | channel 0, slot 1, CON
   BYTE  | ch 0 slot 1 eg type   |   0.0+  | channel 0, slot 1, eg type
   BYTE  | ch 0 slot 1 state     |   0.0+  | channel 0, slot 1, state
   DWORD | ch 0 slot 1 TL        |   0.0+  | channel 0, slot 1, TL
   DWORD | ch 0 slot 1 TLL       |   0.0+  | channel 0, slot 1, TLL
   DWORD | ch 0 slot 1 volume    |   0.0+  | channel 0, slot 1, volume
   DWORD | ch 0 slot 1 sl        |   0.0+  | channel 0, slot 1, sl
   BYTE  | ch 0 slot 1 eg sh ar  |   0.0+  | channel 0, slot 1, eg sh ar
   BYTE  | ch 0 slot 1 eg sel ar |   0.0+  | channel 0, slot 1, eg sel ar
   BYTE  | ch 0 slot 1 eg sh dr  |   0.0+  | channel 0, slot 1, eg sh dr
   BYTE  | ch 0 slot 1 eg sel dr |   0.0+  | channel 0, slot 1, eg sel dr
   BYTE  | ch 0 slot 1 eg sh rr  |   0.0+  | channel 0, slot 1, eg sh rr
   BYTE  | ch 0 slot 1 eg sel rr |   0.0+  | channel 0, slot 1, eg sel rr
   DWORD | ch 0 slot 1 key       |   0.0+  | channel 0, slot 1, key
   DWORD | ch 0 slot 1 AMmask    |   0.0+  | channel 0, slot 1, AMmask
   BYTE  | ch 0 slot 1 vib       |   0.0+  | channel 0, slot 1, vib
   WORD  | ch 0 slot 1 wavetable |   0.0+  | channel 0, slot 1, wavetable
   DWORD | ch 0 block fnum       |   0.0+  | channel 0, block fnum
   DWORD | ch 0 fc               |   0.0+  | channel 0, fc
   DWORD | ch 0 ksl base         |   0.0+  | channel 0, ksl base
   BYTE  | ch 0 kcode            |   0.0+  | channel 0, kcode
   DWORD | ch 1 slot 0 ar        |   0.0+  | channel 1, slot 0, ar
   DWORD | ch 1 slot 0 dr        |   0.0+  | channel 1, slot 0, dr
   DWORD | ch 1 slot 0 rr        |   0.0+  | channel 1, slot 0, rr
   BYTE  | ch 1 slot 0 KSR       |   0.0+  | channel 1, slot 0, KSR
   BYTE  | ch 1 slot 0 ksl       |   0.0+  | channel 1, slot 0, ksl
   BYTE  | ch 1 slot 0 ksr       |   0.0+  | channel 1, slot 0, ksr
   BYTE  | ch 1 slot 0 mul       |   0.0+  | channel 1, slot 0, mul
   DWORD | ch 1 slot 0 Cnt       |   0.0+  | channel 1, slot 0, Cnt
   DWORD | ch 1 slot 0 Incr      |   0.0+  | channel 1, slot 0, Incr
   BYTE  | ch 1 slot 0 FB        |   0.0+  | channel 1, slot 0, FB
   DWORD | ch 1 slot 0 connect1  |   0.0+  | channel 1, slot 0, connect1
   DWORD | ch 1 slot 0 op1 out 0 |   0.0+  | channel 1, slot 0, op1 out 0
   DWORD | ch 1 slot 0 op1 out 1 |   0.0+  | channel 1, slot 0, op1 out 1
   BYTE  | ch 1 slot 0 CON       |   0.0+  | channel 1, slot 0, CON
   BYTE  | ch 1 slot 0 eg type   |   0.0+  | channel 1, slot 0, eg type
   BYTE  | ch 1 slot 0 state     |   0.0+  | channel 1, slot 0, state
   DWORD | ch 1 slot 0 TL        |   0.0+  | channel 1, slot 0, TL
   DWORD | ch 1 slot 0 TLL       |   0.0+  | channel 1, slot 0, TLL
   DWORD | ch 1 slot 0 volume    |   0.0+  | channel 1, slot 0, volume
   DWORD | ch 1 slot 0 sl        |   0.0+  | channel 1, slot 0, sl
   BYTE  | ch 1 slot 0 eg sh ar  |   0.0+  | channel 1, slot 0, eg sh ar
   BYTE  | ch 1 slot 0 eg sel ar |   0.0+  | channel 1, slot 0, eg sel ar
   BYTE  | ch 1 slot 0 eg sh dr  |   0.0+  | channel 1, slot 0, eg sh dr
   BYTE  | ch 1 slot 0 eg sel dr |   0.0+  | channel 1, slot 0, eg sel dr
   BYTE  | ch 1 slot 0 eg sh rr  |   0.0+  | channel 1, slot 0, eg sh rr
   BYTE  | ch 1 slot 0 eg sel rr |   0.0+  | channel 1, slot 0, eg sel rr
   DWORD | ch 1 slot 0 key       |   0.0+  | channel 1, slot 0, key
   DWORD | ch 1 slot 0 AMmask    |   0.0+  | channel 1, slot 0, AMmask
   BYTE  | ch 1 slot 0 vib       |   0.0+  | channel 1, slot 0, vib
   WORD  | ch 1 slot 0 wavetable |   0.0+  | channel 1, slot 0, wavetable
   DWORD | ch 1 slot 1 ar        |   0.0+  | channel 1, slot 1, ar
   DWORD | ch 1 slot 1 dr        |   0.0+  | channel 1, slot 1, dr
   DWORD | ch 1 slot 1 rr        |   0.0+  | channel 1, slot 1, rr
   BYTE  | ch 1 slot 1 KSR       |   0.0+  | channel 1, slot 1, KSR
   BYTE  | ch 1 slot 1 ksl       |   0.0+  | channel 1, slot 1, ksl
   BYTE  | ch 1 slot 1 ksr       |   0.0+  | channel 1, slot 1, ksr
   BYTE  | ch 1 slot 1 mul       |   0.0+  | channel 1, slot 1, mul
   DWORD | ch 1 slot 1 Cnt       |   0.0+  | channel 1, slot 1, Cnt
   DWORD | ch 1 slot 1 Incr      |   0.0+  | channel 1, slot 1, Incr
   BYTE  | ch 1 slot 1 FB        |   0.0+  | channel 1, slot 1, FB
   DWORD | ch 1 slot 1 connect1  |   0.0+  | channel 1, slot 1, connect1
   DWORD | ch 1 slot 1 op1 out 0 |   0.0+  | channel 1, slot 1, op1 out 0
   DWORD | ch 1 slot 1 op1 out 1 |   0.0+  | channel 1, slot 1, op1 out 1
   BYTE  | ch 1 slot 1 CON       |   0.0+  | channel 1, slot 1, CON
   BYTE  | ch 1 slot 1 eg type   |   0.0+  | channel 1, slot 1, eg type
   BYTE  | ch 1 slot 1 state     |   0.0+  | channel 1, slot 1, state
   DWORD | ch 1 slot 1 TL        |   0.0+  | channel 1, slot 1, TL
   DWORD | ch 1 slot 1 TLL       |   0.0+  | channel 1, slot 1, TLL
   DWORD | ch 1 slot 1 volume    |   0.0+  | channel 1, slot 1, volume
   DWORD | ch 1 slot 1 sl        |   0.0+  | channel 1, slot 1, sl
   BYTE  | ch 1 slot 1 eg sh ar  |   0.0+  | channel 1, slot 1, eg sh ar
   BYTE  | ch 1 slot 1 eg sel ar |   0.0+  | channel 1, slot 1, eg sel ar
   BYTE  | ch 1 slot 1 eg sh dr  |   0.0+  | channel 1, slot 1, eg sh dr
   BYTE  | ch 1 slot 1 eg sel dr |   0.0+  | channel 1, slot 1, eg sel dr
   BYTE  | ch 1 slot 1 eg sh rr  |   0.0+  | channel 1, slot 1, eg sh rr
   BYTE  | ch 1 slot 1 eg sel rr |   0.0+  | channel 1, slot 1, eg sel rr
   DWORD | ch 1 slot 1 key       |   0.0+  | channel 1, slot 1, key
   DWORD | ch 1 slot 1 AMmask    |   0.0+  | channel 1, slot 1, AMmask
   BYTE  | ch 1 slot 1 vib       |   0.0+  | channel 1, slot 1, vib
   WORD  | ch 1 slot 1 wavetable |   0.0+  | channel 1, slot 1, wavetable
   DWORD | ch 1 block fnum       |   0.0+  | channel 1, block fnum
   DWORD | ch 1 fc               |   0.0+  | channel 1, fc
   DWORD | ch 1 ksl base         |   0.0+  | channel 1, ksl base
   BYTE  | ch 1 kcode            |   0.0+  | channel 1, kcode
   DWORD | ch 2 slot 0 ar        |   0.0+  | channel 2, slot 0, ar
   DWORD | ch 2 slot 0 dr        |   0.0+  | channel 2, slot 0, dr
   DWORD | ch 2 slot 0 rr        |   0.0+  | channel 2, slot 0, rr
   BYTE  | ch 2 slot 0 KSR       |   0.0+  | channel 2, slot 0, KSR
   BYTE  | ch 2 slot 0 ksl       |   0.0+  | channel 2, slot 0, ksl
   BYTE  | ch 2 slot 0 ksr       |   0.0+  | channel 2, slot 0, ksr
   BYTE  | ch 2 slot 0 mul       |   0.0+  | channel 2, slot 0, mul
   DWORD | ch 2 slot 0 Cnt       |   0.0+  | channel 2, slot 0, Cnt
   DWORD | ch 2 slot 0 Incr      |   0.0+  | channel 2, slot 0, Incr
   BYTE  | ch 2 slot 0 FB        |   0.0+  | channel 2, slot 0, FB
   DWORD | ch 2 slot 0 connect1  |   0.0+  | channel 2, slot 0, connect1
   DWORD | ch 2 slot 0 op1 out 0 |   0.0+  | channel 2, slot 0, op1 out 0
   DWORD | ch 2 slot 0 op1 out 1 |   0.0+  | channel 2, slot 0, op1 out 1
   BYTE  | ch 2 slot 0 CON       |   0.0+  | channel 2, slot 0, CON
   BYTE  | ch 2 slot 0 eg type   |   0.0+  | channel 2, slot 0, eg type
   BYTE  | ch 2 slot 0 state     |   0.0+  | channel 2, slot 0, state
   DWORD | ch 2 slot 0 TL        |   0.0+  | channel 2, slot 0, TL
   DWORD | ch 2 slot 0 TLL       |   0.0+  | channel 2, slot 0, TLL
   DWORD | ch 2 slot 0 volume    |   0.0+  | channel 2, slot 0, volume
   DWORD | ch 2 slot 0 sl        |   0.0+  | channel 2, slot 0, sl
   BYTE  | ch 2 slot 0 eg sh ar  |   0.0+  | channel 2, slot 0, eg sh ar
   BYTE  | ch 2 slot 0 eg sel ar |   0.0+  | channel 2, slot 0, eg sel ar
   BYTE  | ch 2 slot 0 eg sh dr  |   0.0+  | channel 2, slot 0, eg sh dr
   BYTE  | ch 2 slot 0 eg sel dr |   0.0+  | channel 2, slot 0, eg sel dr
   BYTE  | ch 2 slot 0 eg sh rr  |   0.0+  | channel 2, slot 0, eg sh rr
   BYTE  | ch 2 slot 0 eg sel rr |   0.0+  | channel 2, slot 0, eg sel rr
   DWORD | ch 2 slot 0 key       |   0.0+  | channel 2, slot 0, key
   DWORD | ch 2 slot 0 AMmask    |   0.0+  | channel 2, slot 0, AMmask
   BYTE  | ch 2 slot 0 vib       |   0.0+  | channel 2, slot 0, vib
   WORD  | ch 2 slot 0 wavetable |   0.0+  | channel 2, slot 0, wavetable
   DWORD | ch 2 slot 1 ar        |   0.0+  | channel 2, slot 1, ar
   DWORD | ch 2 slot 1 dr        |   0.0+  | channel 2, slot 1, dr
   DWORD | ch 2 slot 1 rr        |   0.0+  | channel 2, slot 1, rr
   BYTE  | ch 2 slot 1 KSR       |   0.0+  | channel 2, slot 1, KSR
   BYTE  | ch 2 slot 1 ksl       |   0.0+  | channel 2, slot 1, ksl
   BYTE  | ch 2 slot 1 ksr       |   0.0+  | channel 2, slot 1, ksr
   BYTE  | ch 2 slot 1 mul       |   0.0+  | channel 2, slot 1, mul
   DWORD | ch 2 slot 1 Cnt       |   0.0+  | channel 2, slot 1, Cnt
   DWORD | ch 2 slot 1 Incr      |   0.0+  | channel 2, slot 1, Incr
   BYTE  | ch 2 slot 1 FB        |   0.0+  | channel 2, slot 1, FB
   DWORD | ch 2 slot 1 connect1  |   0.0+  | channel 2, slot 1, connect1
   DWORD | ch 2 slot 1 op1 out 0 |   0.0+  | channel 2, slot 1, op1 out 0
   DWORD | ch 2 slot 1 op1 out 1 |   0.0+  | channel 2, slot 1, op1 out 1
   BYTE  | ch 2 slot 1 CON       |   0.0+  | channel 2, slot 1, CON
   BYTE  | ch 2 slot 1 eg type   |   0.0+  | channel 2, slot 1, eg type
   BYTE  | ch 2 slot 1 state     |   0.0+  | channel 2, slot 1, state
   DWORD | ch 2 slot 1 TL        |   0.0+  | channel 2, slot 1, TL
   DWORD | ch 2 slot 1 TLL       |   0.0+  | channel 2, slot 1, TLL
   DWORD | ch 2 slot 1 volume    |   0.0+  | channel 2, slot 1, volume
   DWORD | ch 2 slot 1 sl        |   0.0+  | channel 2, slot 1, sl
   BYTE  | ch 2 slot 1 eg sh ar  |   0.0+  | channel 2, slot 1, eg sh ar
   BYTE  | ch 2 slot 1 eg sel ar |   0.0+  | channel 2, slot 1, eg sel ar
   BYTE  | ch 2 slot 1 eg sh dr  |   0.0+  | channel 2, slot 1, eg sh dr
   BYTE  | ch 2 slot 1 eg sel dr |   0.0+  | channel 2, slot 1, eg sel dr
   BYTE  | ch 2 slot 1 eg sh rr  |   0.0+  | channel 2, slot 1, eg sh rr
   BYTE  | ch 2 slot 1 eg sel rr |   0.0+  | channel 2, slot 1, eg sel rr
   DWORD | ch 2 slot 1 key       |   0.0+  | channel 2, slot 1, key
   DWORD | ch 2 slot 1 AMmask    |   0.0+  | channel 2, slot 1, AMmask
   BYTE  | ch 2 slot 1 vib       |   0.0+  | channel 2, slot 1, vib
   WORD  | ch 2 slot 1 wavetable |   0.0+  | channel 2, slot 1, wavetable
   DWORD | ch 2 block fnum       |   0.0+  | channel 2, block fnum
   DWORD | ch 2 fc               |   0.0+  | channel 2, fc
   DWORD | ch 2 ksl base         |   0.0+  | channel 2, ksl base
   BYTE  | ch 2 kcode            |   0.0+  | channel 2, kcode
   DWORD | ch 3 slot 0 ar        |   0.0+  | channel 3, slot 0, ar
   DWORD | ch 3 slot 0 dr        |   0.0+  | channel 3, slot 0, dr
   DWORD | ch 3 slot 0 rr        |   0.0+  | channel 3, slot 0, rr
   BYTE  | ch 3 slot 0 KSR       |   0.0+  | channel 3, slot 0, KSR
   BYTE  | ch 3 slot 0 ksl       |   0.0+  | channel 3, slot 0, ksl
   BYTE  | ch 3 slot 0 ksr       |   0.0+  | channel 3, slot 0, ksr
   BYTE  | ch 3 slot 0 mul       |   0.0+  | channel 3, slot 0, mul
   DWORD | ch 3 slot 0 Cnt       |   0.0+  | channel 3, slot 0, Cnt
   DWORD | ch 3 slot 0 Incr      |   0.0+  | channel 3, slot 0, Incr
   BYTE  | ch 3 slot 0 FB        |   0.0+  | channel 3, slot 0, FB
   DWORD | ch 3 slot 0 connect1  |   0.0+  | channel 3, slot 0, connect1
   DWORD | ch 3 slot 0 op1 out 0 |   0.0+  | channel 3, slot 0, op1 out 0
   DWORD | ch 3 slot 0 op1 out 1 |   0.0+  | channel 3, slot 0, op1 out 1
   BYTE  | ch 3 slot 0 CON       |   0.0+  | channel 3, slot 0, CON
   BYTE  | ch 3 slot 0 eg type   |   0.0+  | channel 3, slot 0, eg type
   BYTE  | ch 3 slot 0 state     |   0.0+  | channel 3, slot 0, state
   DWORD | ch 3 slot 0 TL        |   0.0+  | channel 3, slot 0, TL
   DWORD | ch 3 slot 0 TLL       |   0.0+  | channel 3, slot 0, TLL
   DWORD | ch 3 slot 0 volume    |   0.0+  | channel 3, slot 0, volume
   DWORD | ch 3 slot 0 sl        |   0.0+  | channel 3, slot 0, sl
   BYTE  | ch 3 slot 0 eg sh ar  |   0.0+  | channel 3, slot 0, eg sh ar
   BYTE  | ch 3 slot 0 eg sel ar |   0.0+  | channel 3, slot 0, eg sel ar
   BYTE  | ch 3 slot 0 eg sh dr  |   0.0+  | channel 3, slot 0, eg sh dr
   BYTE  | ch 3 slot 0 eg sel dr |   0.0+  | channel 3, slot 0, eg sel dr
   BYTE  | ch 3 slot 0 eg sh rr  |   0.0+  | channel 3, slot 0, eg sh rr
   BYTE  | ch 3 slot 0 eg sel rr |   0.0+  | channel 3, slot 0, eg sel rr
   DWORD | ch 3 slot 0 key       |   0.0+  | channel 3, slot 0, key
   DWORD | ch 3 slot 0 AMmask    |   0.0+  | channel 3, slot 0, AMmask
   BYTE  | ch 3 slot 0 vib       |   0.0+  | channel 3, slot 0, vib
   WORD  | ch 3 slot 0 wavetable |   0.0+  | channel 3, slot 0, wavetable
   DWORD | ch 3 slot 1 ar        |   0.0+  | channel 3, slot 1, ar
   DWORD | ch 3 slot 1 dr        |   0.0+  | channel 3, slot 1, dr
   DWORD | ch 3 slot 1 rr        |   0.0+  | channel 3, slot 1, rr
   BYTE  | ch 3 slot 1 KSR       |   0.0+  | channel 3, slot 1, KSR
   BYTE  | ch 3 slot 1 ksl       |   0.0+  | channel 3, slot 1, ksl
   BYTE  | ch 3 slot 1 ksr       |   0.0+  | channel 3, slot 1, ksr
   BYTE  | ch 3 slot 1 mul       |   0.0+  | channel 3, slot 1, mul
   DWORD | ch 3 slot 1 Cnt       |   0.0+  | channel 3, slot 1, Cnt
   DWORD | ch 3 slot 1 Incr      |   0.0+  | channel 3, slot 1, Incr
   BYTE  | ch 3 slot 1 FB        |   0.0+  | channel 3, slot 1, FB
   DWORD | ch 3 slot 1 connect1  |   0.0+  | channel 3, slot 1, connect1
   DWORD | ch 3 slot 1 op1 out 0 |   0.0+  | channel 3, slot 1, op1 out 0
   DWORD | ch 3 slot 1 op1 out 1 |   0.0+  | channel 3, slot 1, op1 out 1
   BYTE  | ch 3 slot 1 CON       |   0.0+  | channel 3, slot 1, CON
   BYTE  | ch 3 slot 1 eg type   |   0.0+  | channel 3, slot 1, eg type
   BYTE  | ch 3 slot 1 state     |   0.0+  | channel 3, slot 1, state
   DWORD | ch 3 slot 1 TL        |   0.0+  | channel 3, slot 1, TL
   DWORD | ch 3 slot 1 TLL       |   0.0+  | channel 3, slot 1, TLL
   DWORD | ch 3 slot 1 volume    |   0.0+  | channel 3, slot 1, volume
   DWORD | ch 3 slot 1 sl        |   0.0+  | channel 3, slot 1, sl
   BYTE  | ch 3 slot 1 eg sh ar  |   0.0+  | channel 3, slot 1, eg sh ar
   BYTE  | ch 3 slot 1 eg sel ar |   0.0+  | channel 3, slot 1, eg sel ar
   BYTE  | ch 3 slot 1 eg sh dr  |   0.0+  | channel 3, slot 1, eg sh dr
   BYTE  | ch 3 slot 1 eg sel dr |   0.0+  | channel 3, slot 1, eg sel dr
   BYTE  | ch 3 slot 1 eg sh rr  |   0.0+  | channel 3, slot 1, eg sh rr
   BYTE  | ch 3 slot 1 eg sel rr |   0.0+  | channel 3, slot 1, eg sel rr
   DWORD | ch 3 slot 1 key       |   0.0+  | channel 3, slot 1, key
   DWORD | ch 3 slot 1 AMmask    |   0.0+  | channel 3, slot 1, AMmask
   BYTE  | ch 3 slot 1 vib       |   0.0+  | channel 3, slot 1, vib
   WORD  | ch 3 slot 1 wavetable |   0.0+  | channel 3, slot 1, wavetable
   DWORD | ch 3 block fnum       |   0.0+  | channel 3, block fnum
   DWORD | ch 3 fc               |   0.0+  | channel 3, fc
   DWORD | ch 3 ksl base         |   0.0+  | channel 3, ksl base
   BYTE  | ch 3 kcode            |   0.0+  | channel 3, kcode
   DWORD | ch 4 slot 0 ar        |   0.0+  | channel 4, slot 0, ar
   DWORD | ch 4 slot 0 dr        |   0.0+  | channel 4, slot 0, dr
   DWORD | ch 4 slot 0 rr        |   0.0+  | channel 4, slot 0, rr
   BYTE  | ch 4 slot 0 KSR       |   0.0+  | channel 4, slot 0, KSR
   BYTE  | ch 4 slot 0 ksl       |   0.0+  | channel 4, slot 0, ksl
   BYTE  | ch 4 slot 0 ksr       |   0.0+  | channel 4, slot 0, ksr
   BYTE  | ch 4 slot 0 mul       |   0.0+  | channel 4, slot 0, mul
   DWORD | ch 4 slot 0 Cnt       |   0.0+  | channel 4, slot 0, Cnt
   DWORD | ch 4 slot 0 Incr      |   0.0+  | channel 4, slot 0, Incr
   BYTE  | ch 4 slot 0 FB        |   0.0+  | channel 4, slot 0, FB
   DWORD | ch 4 slot 0 connect1  |   0.0+  | channel 4, slot 0, connect1
   DWORD | ch 4 slot 0 op1 out 0 |   0.0+  | channel 4, slot 0, op1 out 0
   DWORD | ch 4 slot 0 op1 out 1 |   0.0+  | channel 4, slot 0, op1 out 1
   BYTE  | ch 4 slot 0 CON       |   0.0+  | channel 4, slot 0, CON
   BYTE  | ch 4 slot 0 eg type   |   0.0+  | channel 4, slot 0, eg type
   BYTE  | ch 4 slot 0 state     |   0.0+  | channel 4, slot 0, state
   DWORD | ch 4 slot 0 TL        |   0.0+  | channel 4, slot 0, TL
   DWORD | ch 4 slot 0 TLL       |   0.0+  | channel 4, slot 0, TLL
   DWORD | ch 4 slot 0 volume    |   0.0+  | channel 4, slot 0, volume
   DWORD | ch 4 slot 0 sl        |   0.0+  | channel 4, slot 0, sl
   BYTE  | ch 4 slot 0 eg sh ar  |   0.0+  | channel 4, slot 0, eg sh ar
   BYTE  | ch 4 slot 0 eg sel ar |   0.0+  | channel 4, slot 0, eg sel ar
   BYTE  | ch 4 slot 0 eg sh dr  |   0.0+  | channel 4, slot 0, eg sh dr
   BYTE  | ch 4 slot 0 eg sel dr |   0.0+  | channel 4, slot 0, eg sel dr
   BYTE  | ch 4 slot 0 eg sh rr  |   0.0+  | channel 4, slot 0, eg sh rr
   BYTE  | ch 4 slot 0 eg sel rr |   0.0+  | channel 4, slot 0, eg sel rr
   DWORD | ch 4 slot 0 key       |   0.0+  | channel 4, slot 0, key
   DWORD | ch 4 slot 0 AMmask    |   0.0+  | channel 4, slot 0, AMmask
   BYTE  | ch 4 slot 0 vib       |   0.0+  | channel 4, slot 0, vib
   WORD  | ch 4 slot 0 wavetable |   0.0+  | channel 4, slot 0, wavetable
   DWORD | ch 4 slot 1 ar        |   0.0+  | channel 4, slot 1, ar
   DWORD | ch 4 slot 1 dr        |   0.0+  | channel 4, slot 1, dr
   DWORD | ch 4 slot 1 rr        |   0.0+  | channel 4, slot 1, rr
   BYTE  | ch 4 slot 1 KSR       |   0.0+  | channel 4, slot 1, KSR
   BYTE  | ch 4 slot 1 ksl       |   0.0+  | channel 4, slot 1, ksl
   BYTE  | ch 4 slot 1 ksr       |   0.0+  | channel 4, slot 1, ksr
   BYTE  | ch 4 slot 1 mul       |   0.0+  | channel 4, slot 1, mul
   DWORD | ch 4 slot 1 Cnt       |   0.0+  | channel 4, slot 1, Cnt
   DWORD | ch 4 slot 1 Incr      |   0.0+  | channel 4, slot 1, Incr
   BYTE  | ch 4 slot 1 FB        |   0.0+  | channel 4, slot 1, FB
   DWORD | ch 4 slot 1 connect1  |   0.0+  | channel 4, slot 1, connect1
   DWORD | ch 4 slot 1 op1 out 0 |   0.0+  | channel 4, slot 1, op1 out 0
   DWORD | ch 4 slot 1 op1 out 1 |   0.0+  | channel 4, slot 1, op1 out 1
   BYTE  | ch 4 slot 1 CON       |   0.0+  | channel 4, slot 1, CON
   BYTE  | ch 4 slot 1 eg type   |   0.0+  | channel 4, slot 1, eg type
   BYTE  | ch 4 slot 1 state     |   0.0+  | channel 4, slot 1, state
   DWORD | ch 4 slot 1 TL        |   0.0+  | channel 4, slot 1, TL
   DWORD | ch 4 slot 1 TLL       |   0.0+  | channel 4, slot 1, TLL
   DWORD | ch 4 slot 1 volume    |   0.0+  | channel 4, slot 1, volume
   DWORD | ch 4 slot 1 sl        |   0.0+  | channel 4, slot 1, sl
   BYTE  | ch 4 slot 1 eg sh ar  |   0.0+  | channel 4, slot 1, eg sh ar
   BYTE  | ch 4 slot 1 eg sel ar |   0.0+  | channel 4, slot 1, eg sel ar
   BYTE  | ch 4 slot 1 eg sh dr  |   0.0+  | channel 4, slot 1, eg sh dr
   BYTE  | ch 4 slot 1 eg sel dr |   0.0+  | channel 4, slot 1, eg sel dr
   BYTE  | ch 4 slot 1 eg sh rr  |   0.0+  | channel 4, slot 1, eg sh rr
   BYTE  | ch 4 slot 1 eg sel rr |   0.0+  | channel 4, slot 1, eg sel rr
   DWORD | ch 4 slot 1 key       |   0.0+  | channel 4, slot 1, key
   DWORD | ch 4 slot 1 AMmask    |   0.0+  | channel 4, slot 1, AMmask
   BYTE  | ch 4 slot 1 vib       |   0.0+  | channel 4, slot 1, vib
   WORD  | ch 4 slot 1 wavetable |   0.0+  | channel 4, slot 1, wavetable
   DWORD | ch 4 block fnum       |   0.0+  | channel 4, block fnum
   DWORD | ch 4 fc               |   0.0+  | channel 4, fc
   DWORD | ch 4 ksl base         |   0.0+  | channel 4, ksl base
   BYTE  | ch 4 kcode            |   0.0+  | channel 4, kcode
   DWORD | ch 5 slot 0 ar        |   0.0+  | channel 5, slot 0, ar
   DWORD | ch 5 slot 0 dr        |   0.0+  | channel 5, slot 0, dr
   DWORD | ch 5 slot 0 rr        |   0.0+  | channel 5, slot 0, rr
   BYTE  | ch 5 slot 0 KSR       |   0.0+  | channel 5, slot 0, KSR
   BYTE  | ch 5 slot 0 ksl       |   0.0+  | channel 5, slot 0, ksl
   BYTE  | ch 5 slot 0 ksr       |   0.0+  | channel 5, slot 0, ksr
   BYTE  | ch 5 slot 0 mul       |   0.0+  | channel 5, slot 0, mul
   DWORD | ch 5 slot 0 Cnt       |   0.0+  | channel 5, slot 0, Cnt
   DWORD | ch 5 slot 0 Incr      |   0.0+  | channel 5, slot 0, Incr
   BYTE  | ch 5 slot 0 FB        |   0.0+  | channel 5, slot 0, FB
   DWORD | ch 5 slot 0 connect1  |   0.0+  | channel 5, slot 0, connect1
   DWORD | ch 5 slot 0 op1 out 0 |   0.0+  | channel 5, slot 0, op1 out 0
   DWORD | ch 5 slot 0 op1 out 1 |   0.0+  | channel 5, slot 0, op1 out 1
   BYTE  | ch 5 slot 0 CON       |   0.0+  | channel 5, slot 0, CON
   BYTE  | ch 5 slot 0 eg type   |   0.0+  | channel 5, slot 0, eg type
   BYTE  | ch 5 slot 0 state     |   0.0+  | channel 5, slot 0, state
   DWORD | ch 5 slot 0 TL        |   0.0+  | channel 5, slot 0, TL
   DWORD | ch 5 slot 0 TLL       |   0.0+  | channel 5, slot 0, TLL
   DWORD | ch 5 slot 0 volume    |   0.0+  | channel 5, slot 0, volume
   DWORD | ch 5 slot 0 sl        |   0.0+  | channel 5, slot 0, sl
   BYTE  | ch 5 slot 0 eg sh ar  |   0.0+  | channel 5, slot 0, eg sh ar
   BYTE  | ch 5 slot 0 eg sel ar |   0.0+  | channel 5, slot 0, eg sel ar
   BYTE  | ch 5 slot 0 eg sh dr  |   0.0+  | channel 5, slot 0, eg sh dr
   BYTE  | ch 5 slot 0 eg sel dr |   0.0+  | channel 5, slot 0, eg sel dr
   BYTE  | ch 5 slot 0 eg sh rr  |   0.0+  | channel 5, slot 0, eg sh rr
   BYTE  | ch 5 slot 0 eg sel rr |   0.0+  | channel 5, slot 0, eg sel rr
   DWORD | ch 5 slot 0 key       |   0.0+  | channel 5, slot 0, key
   DWORD | ch 5 slot 0 AMmask    |   0.0+  | channel 5, slot 0, AMmask
   BYTE  | ch 5 slot 0 vib       |   0.0+  | channel 5, slot 0, vib
   WORD  | ch 5 slot 0 wavetable |   0.0+  | channel 5, slot 0, wavetable
   DWORD | ch 5 slot 1 ar        |   0.0+  | channel 5, slot 1, ar
   DWORD | ch 5 slot 1 dr        |   0.0+  | channel 5, slot 1, dr
   DWORD | ch 5 slot 1 rr        |   0.0+  | channel 5, slot 1, rr
   BYTE  | ch 5 slot 1 KSR       |   0.0+  | channel 5, slot 1, KSR
   BYTE  | ch 5 slot 1 ksl       |   0.0+  | channel 5, slot 1, ksl
   BYTE  | ch 5 slot 1 ksr       |   0.0+  | channel 5, slot 1, ksr
   BYTE  | ch 5 slot 1 mul       |   0.0+  | channel 5, slot 1, mul
   DWORD | ch 5 slot 1 Cnt       |   0.0+  | channel 5, slot 1, Cnt
   DWORD | ch 5 slot 1 Incr      |   0.0+  | channel 5, slot 1, Incr
   BYTE  | ch 5 slot 1 FB        |   0.0+  | channel 5, slot 1, FB
   DWORD | ch 5 slot 1 connect1  |   0.0+  | channel 5, slot 1, connect1
   DWORD | ch 5 slot 1 op1 out 0 |   0.0+  | channel 5, slot 1, op1 out 0
   DWORD | ch 5 slot 1 op1 out 1 |   0.0+  | channel 5, slot 1, op1 out 1
   BYTE  | ch 5 slot 1 CON       |   0.0+  | channel 5, slot 1, CON
   BYTE  | ch 5 slot 1 eg type   |   0.0+  | channel 5, slot 1, eg type
   BYTE  | ch 5 slot 1 state     |   0.0+  | channel 5, slot 1, state
   DWORD | ch 5 slot 1 TL        |   0.0+  | channel 5, slot 1, TL
   DWORD | ch 5 slot 1 TLL       |   0.0+  | channel 5, slot 1, TLL
   DWORD | ch 5 slot 1 volume    |   0.0+  | channel 5, slot 1, volume
   DWORD | ch 5 slot 1 sl        |   0.0+  | channel 5, slot 1, sl
   BYTE  | ch 5 slot 1 eg sh ar  |   0.0+  | channel 5, slot 1, eg sh ar
   BYTE  | ch 5 slot 1 eg sel ar |   0.0+  | channel 5, slot 1, eg sel ar
   BYTE  | ch 5 slot 1 eg sh dr  |   0.0+  | channel 5, slot 1, eg sh dr
   BYTE  | ch 5 slot 1 eg sel dr |   0.0+  | channel 5, slot 1, eg sel dr
   BYTE  | ch 5 slot 1 eg sh rr  |   0.0+  | channel 5, slot 1, eg sh rr
   BYTE  | ch 5 slot 1 eg sel rr |   0.0+  | channel 5, slot 1, eg sel rr
   DWORD | ch 5 slot 1 key       |   0.0+  | channel 5, slot 1, key
   DWORD | ch 5 slot 1 AMmask    |   0.0+  | channel 5, slot 1, AMmask
   BYTE  | ch 5 slot 1 vib       |   0.0+  | channel 5, slot 1, vib
   WORD  | ch 5 slot 1 wavetable |   0.0+  | channel 5, slot 1, wavetable
   DWORD | ch 5 block fnum       |   0.0+  | channel 5, block fnum
   DWORD | ch 5 fc               |   0.0+  | channel 5, fc
   DWORD | ch 5 ksl base         |   0.0+  | channel 5, ksl base
   BYTE  | ch 5 kcode            |   0.0+  | channel 5, kcode
   DWORD | ch 6 slot 0 ar        |   0.0+  | channel 6, slot 0, ar
   DWORD | ch 6 slot 0 dr        |   0.0+  | channel 6, slot 0, dr
   DWORD | ch 6 slot 0 rr        |   0.0+  | channel 6, slot 0, rr
   BYTE  | ch 6 slot 0 KSR       |   0.0+  | channel 6, slot 0, KSR
   BYTE  | ch 6 slot 0 ksl       |   0.0+  | channel 6, slot 0, ksl
   BYTE  | ch 6 slot 0 ksr       |   0.0+  | channel 6, slot 0, ksr
   BYTE  | ch 6 slot 0 mul       |   0.0+  | channel 6, slot 0, mul
   DWORD | ch 6 slot 0 Cnt       |   0.0+  | channel 6, slot 0, Cnt
   DWORD | ch 6 slot 0 Incr      |   0.0+  | channel 6, slot 0, Incr
   BYTE  | ch 6 slot 0 FB        |   0.0+  | channel 6, slot 0, FB
   DWORD | ch 6 slot 0 connect1  |   0.0+  | channel 6, slot 0, connect1
   DWORD | ch 6 slot 0 op1 out 0 |   0.0+  | channel 6, slot 0, op1 out 0
   DWORD | ch 6 slot 0 op1 out 1 |   0.0+  | channel 6, slot 0, op1 out 1
   BYTE  | ch 6 slot 0 CON       |   0.0+  | channel 6, slot 0, CON
   BYTE  | ch 6 slot 0 eg type   |   0.0+  | channel 6, slot 0, eg type
   BYTE  | ch 6 slot 0 state     |   0.0+  | channel 6, slot 0, state
   DWORD | ch 6 slot 0 TL        |   0.0+  | channel 6, slot 0, TL
   DWORD | ch 6 slot 0 TLL       |   0.0+  | channel 6, slot 0, TLL
   DWORD | ch 6 slot 0 volume    |   0.0+  | channel 6, slot 0, volume
   DWORD | ch 6 slot 0 sl        |   0.0+  | channel 6, slot 0, sl
   BYTE  | ch 6 slot 0 eg sh ar  |   0.0+  | channel 6, slot 0, eg sh ar
   BYTE  | ch 6 slot 0 eg sel ar |   0.0+  | channel 6, slot 0, eg sel ar
   BYTE  | ch 6 slot 0 eg sh dr  |   0.0+  | channel 6, slot 0, eg sh dr
   BYTE  | ch 6 slot 0 eg sel dr |   0.0+  | channel 6, slot 0, eg sel dr
   BYTE  | ch 6 slot 0 eg sh rr  |   0.0+  | channel 6, slot 0, eg sh rr
   BYTE  | ch 6 slot 0 eg sel rr |   0.0+  | channel 6, slot 0, eg sel rr
   DWORD | ch 6 slot 0 key       |   0.0+  | channel 6, slot 0, key
   DWORD | ch 6 slot 0 AMmask    |   0.0+  | channel 6, slot 0, AMmask
   BYTE  | ch 6 slot 0 vib       |   0.0+  | channel 6, slot 0, vib
   WORD  | ch 6 slot 0 wavetable |   0.0+  | channel 6, slot 0, wavetable
   DWORD | ch 6 slot 1 ar        |   0.0+  | channel 6, slot 1, ar
   DWORD | ch 6 slot 1 dr        |   0.0+  | channel 6, slot 1, dr
   DWORD | ch 6 slot 1 rr        |   0.0+  | channel 6, slot 1, rr
   BYTE  | ch 6 slot 1 KSR       |   0.0+  | channel 6, slot 1, KSR
   BYTE  | ch 6 slot 1 ksl       |   0.0+  | channel 6, slot 1, ksl
   BYTE  | ch 6 slot 1 ksr       |   0.0+  | channel 6, slot 1, ksr
   BYTE  | ch 6 slot 1 mul       |   0.0+  | channel 6, slot 1, mul
   DWORD | ch 6 slot 1 Cnt       |   0.0+  | channel 6, slot 1, Cnt
   DWORD | ch 6 slot 1 Incr      |   0.0+  | channel 6, slot 1, Incr
   BYTE  | ch 6 slot 1 FB        |   0.0+  | channel 6, slot 1, FB
   DWORD | ch 6 slot 1 connect1  |   0.0+  | channel 6, slot 1, connect1
   DWORD | ch 6 slot 1 op1 out 0 |   0.0+  | channel 6, slot 1, op1 out 0
   DWORD | ch 6 slot 1 op1 out 1 |   0.0+  | channel 6, slot 1, op1 out 1
   BYTE  | ch 6 slot 1 CON       |   0.0+  | channel 6, slot 1, CON
   BYTE  | ch 6 slot 1 eg type   |   0.0+  | channel 6, slot 1, eg type
   BYTE  | ch 6 slot 1 state     |   0.0+  | channel 6, slot 1, state
   DWORD | ch 6 slot 1 TL        |   0.0+  | channel 6, slot 1, TL
   DWORD | ch 6 slot 1 TLL       |   0.0+  | channel 6, slot 1, TLL
   DWORD | ch 6 slot 1 volume    |   0.0+  | channel 6, slot 1, volume
   DWORD | ch 6 slot 1 sl        |   0.0+  | channel 6, slot 1, sl
   BYTE  | ch 6 slot 1 eg sh ar  |   0.0+  | channel 6, slot 1, eg sh ar
   BYTE  | ch 6 slot 1 eg sel ar |   0.0+  | channel 6, slot 1, eg sel ar
   BYTE  | ch 6 slot 1 eg sh dr  |   0.0+  | channel 6, slot 1, eg sh dr
   BYTE  | ch 6 slot 1 eg sel dr |   0.0+  | channel 6, slot 1, eg sel dr
   BYTE  | ch 6 slot 1 eg sh rr  |   0.0+  | channel 6, slot 1, eg sh rr
   BYTE  | ch 6 slot 1 eg sel rr |   0.0+  | channel 6, slot 1, eg sel rr
   DWORD | ch 6 slot 1 key       |   0.0+  | channel 6, slot 1, key
   DWORD | ch 6 slot 1 AMmask    |   0.0+  | channel 6, slot 1, AMmask
   BYTE  | ch 6 slot 1 vib       |   0.0+  | channel 6, slot 1, vib
   WORD  | ch 6 slot 1 wavetable |   0.0+  | channel 6, slot 1, wavetable
   DWORD | ch 6 block fnum       |   0.0+  | channel 6, block fnum
   DWORD | ch 6 fc               |   0.0+  | channel 6, fc
   DWORD | ch 6 ksl base         |   0.0+  | channel 6, ksl base
   BYTE  | ch 6 kcode            |   0.0+  | channel 6, kcode
   DWORD | ch 7 slot 0 ar        |   0.0+  | channel 7, slot 0, ar
   DWORD | ch 7 slot 0 dr        |   0.0+  | channel 7, slot 0, dr
   DWORD | ch 7 slot 0 rr        |   0.0+  | channel 7, slot 0, rr
   BYTE  | ch 7 slot 0 KSR       |   0.0+  | channel 7, slot 0, KSR
   BYTE  | ch 7 slot 0 ksl       |   0.0+  | channel 7, slot 0, ksl
   BYTE  | ch 7 slot 0 ksr       |   0.0+  | channel 7, slot 0, ksr
   BYTE  | ch 7 slot 0 mul       |   0.0+  | channel 7, slot 0, mul
   DWORD | ch 7 slot 0 Cnt       |   0.0+  | channel 7, slot 0, Cnt
   DWORD | ch 7 slot 0 Incr      |   0.0+  | channel 7, slot 0, Incr
   BYTE  | ch 7 slot 0 FB        |   0.0+  | channel 7, slot 0, FB
   DWORD | ch 7 slot 0 connect1  |   0.0+  | channel 7, slot 0, connect1
   DWORD | ch 7 slot 0 op1 out 0 |   0.0+  | channel 7, slot 0, op1 out 0
   DWORD | ch 7 slot 0 op1 out 1 |   0.0+  | channel 7, slot 0, op1 out 1
   BYTE  | ch 7 slot 0 CON       |   0.0+  | channel 7, slot 0, CON
   BYTE  | ch 7 slot 0 eg type   |   0.0+  | channel 7, slot 0, eg type
   BYTE  | ch 7 slot 0 state     |   0.0+  | channel 7, slot 0, state
   DWORD | ch 7 slot 0 TL        |   0.0+  | channel 7, slot 0, TL
   DWORD | ch 7 slot 0 TLL       |   0.0+  | channel 7, slot 0, TLL
   DWORD | ch 7 slot 0 volume    |   0.0+  | channel 7, slot 0, volume
   DWORD | ch 7 slot 0 sl        |   0.0+  | channel 7, slot 0, sl
   BYTE  | ch 7 slot 0 eg sh ar  |   0.0+  | channel 7, slot 0, eg sh ar
   BYTE  | ch 7 slot 0 eg sel ar |   0.0+  | channel 7, slot 0, eg sel ar
   BYTE  | ch 7 slot 0 eg sh dr  |   0.0+  | channel 7, slot 0, eg sh dr
   BYTE  | ch 7 slot 0 eg sel dr |   0.0+  | channel 7, slot 0, eg sel dr
   BYTE  | ch 7 slot 0 eg sh rr  |   0.0+  | channel 7, slot 0, eg sh rr
   BYTE  | ch 7 slot 0 eg sel rr |   0.0+  | channel 7, slot 0, eg sel rr
   DWORD | ch 7 slot 0 key       |   0.0+  | channel 7, slot 0, key
   DWORD | ch 7 slot 0 AMmask    |   0.0+  | channel 7, slot 0, AMmask
   BYTE  | ch 7 slot 0 vib       |   0.0+  | channel 7, slot 0, vib
   WORD  | ch 7 slot 0 wavetable |   0.0+  | channel 7, slot 0, wavetable
   DWORD | ch 7 slot 1 ar        |   0.0+  | channel 7, slot 1, ar
   DWORD | ch 7 slot 1 dr        |   0.0+  | channel 7, slot 1, dr
   DWORD | ch 7 slot 1 rr        |   0.0+  | channel 7, slot 1, rr
   BYTE  | ch 7 slot 1 KSR       |   0.0+  | channel 7, slot 1, KSR
   BYTE  | ch 7 slot 1 ksl       |   0.0+  | channel 7, slot 1, ksl
   BYTE  | ch 7 slot 1 ksr       |   0.0+  | channel 7, slot 1, ksr
   BYTE  | ch 7 slot 1 mul       |   0.0+  | channel 7, slot 1, mul
   DWORD | ch 7 slot 1 Cnt       |   0.0+  | channel 7, slot 1, Cnt
   DWORD | ch 7 slot 1 Incr      |   0.0+  | channel 7, slot 1, Incr
   BYTE  | ch 7 slot 1 FB        |   0.0+  | channel 7, slot 1, FB
   DWORD | ch 7 slot 1 connect1  |   0.0+  | channel 7, slot 1, connect1
   DWORD | ch 7 slot 1 op1 out 0 |   0.0+  | channel 7, slot 1, op1 out 0
   DWORD | ch 7 slot 1 op1 out 1 |   0.0+  | channel 7, slot 1, op1 out 1
   BYTE  | ch 7 slot 1 CON       |   0.0+  | channel 7, slot 1, CON
   BYTE  | ch 7 slot 1 eg type   |   0.0+  | channel 7, slot 1, eg type
   BYTE  | ch 7 slot 1 state     |   0.0+  | channel 7, slot 1, state
   DWORD | ch 7 slot 1 TL        |   0.0+  | channel 7, slot 1, TL
   DWORD | ch 7 slot 1 TLL       |   0.0+  | channel 7, slot 1, TLL
   DWORD | ch 7 slot 1 volume    |   0.0+  | channel 7, slot 1, volume
   DWORD | ch 7 slot 1 sl        |   0.0+  | channel 7, slot 1, sl
   BYTE  | ch 7 slot 1 eg sh ar  |   0.0+  | channel 7, slot 1, eg sh ar
   BYTE  | ch 7 slot 1 eg sel ar |   0.0+  | channel 7, slot 1, eg sel ar
   BYTE  | ch 7 slot 1 eg sh dr  |   0.0+  | channel 7, slot 1, eg sh dr
   BYTE  | ch 7 slot 1 eg sel dr |   0.0+  | channel 7, slot 1, eg sel dr
   BYTE  | ch 7 slot 1 eg sh rr  |   0.0+  | channel 7, slot 1, eg sh rr
   BYTE  | ch 7 slot 1 eg sel rr |   0.0+  | channel 7, slot 1, eg sel rr
   DWORD | ch 7 slot 1 key       |   0.0+  | channel 7, slot 1, key
   DWORD | ch 7 slot 1 AMmask    |   0.0+  | channel 7, slot 1, AMmask
   BYTE  | ch 7 slot 1 vib       |   0.0+  | channel 7, slot 1, vib
   WORD  | ch 7 slot 1 wavetable |   0.0+  | channel 7, slot 1, wavetable
   DWORD | ch 7 block fnum       |   0.0+  | channel 7, block fnum
   DWORD | ch 7 fc               |   0.0+  | channel 7, fc
   DWORD | ch 7 ksl base         |   0.0+  | channel 7, ksl base
   BYTE  | ch 7 kcode            |   0.0+  | channel 7, kcode
   DWORD | ch 8 slot 0 ar        |   0.0+  | channel 8, slot 0, ar
   DWORD | ch 8 slot 0 dr        |   0.0+  | channel 8, slot 0, dr
   DWORD | ch 8 slot 0 rr        |   0.0+  | channel 8, slot 0, rr
   BYTE  | ch 8 slot 0 KSR       |   0.0+  | channel 8, slot 0, KSR
   BYTE  | ch 8 slot 0 ksl       |   0.0+  | channel 8, slot 0, ksl
   BYTE  | ch 8 slot 0 ksr       |   0.0+  | channel 8, slot 0, ksr
   BYTE  | ch 8 slot 0 mul       |   0.0+  | channel 8, slot 0, mul
   DWORD | ch 8 slot 0 Cnt       |   0.0+  | channel 8, slot 0, Cnt
   DWORD | ch 8 slot 0 Incr      |   0.0+  | channel 8, slot 0, Incr
   BYTE  | ch 8 slot 0 FB        |   0.0+  | channel 8, slot 0, FB
   DWORD | ch 8 slot 0 connect1  |   0.0+  | channel 8, slot 0, connect1
   DWORD | ch 8 slot 0 op1 out 0 |   0.0+  | channel 8, slot 0, op1 out 0
   DWORD | ch 8 slot 0 op1 out 1 |   0.0+  | channel 8, slot 0, op1 out 1
   BYTE  | ch 8 slot 0 CON       |   0.0+  | channel 8, slot 0, CON
   BYTE  | ch 8 slot 0 eg type   |   0.0+  | channel 8, slot 0, eg type
   BYTE  | ch 8 slot 0 state     |   0.0+  | channel 8, slot 0, state
   DWORD | ch 8 slot 0 TL        |   0.0+  | channel 8, slot 0, TL
   DWORD | ch 8 slot 0 TLL       |   0.0+  | channel 8, slot 0, TLL
   DWORD | ch 8 slot 0 volume    |   0.0+  | channel 8, slot 0, volume
   DWORD | ch 8 slot 0 sl        |   0.0+  | channel 8, slot 0, sl
   BYTE  | ch 8 slot 0 eg sh ar  |   0.0+  | channel 8, slot 0, eg sh ar
   BYTE  | ch 8 slot 0 eg sel ar |   0.0+  | channel 8, slot 0, eg sel ar
   BYTE  | ch 8 slot 0 eg sh dr  |   0.0+  | channel 8, slot 0, eg sh dr
   BYTE  | ch 8 slot 0 eg sel dr |   0.0+  | channel 8, slot 0, eg sel dr
   BYTE  | ch 8 slot 0 eg sh rr  |   0.0+  | channel 8, slot 0, eg sh rr
   BYTE  | ch 8 slot 0 eg sel rr |   0.0+  | channel 8, slot 0, eg sel rr
   DWORD | ch 8 slot 0 key       |   0.0+  | channel 8, slot 0, key
   DWORD | ch 8 slot 0 AMmask    |   0.0+  | channel 8, slot 0, AMmask
   BYTE  | ch 8 slot 0 vib       |   0.0+  | channel 8, slot 0, vib
   WORD  | ch 8 slot 0 wavetable |   0.0+  | channel 8, slot 0, wavetable
   DWORD | ch 8 slot 1 ar        |   0.0+  | channel 8, slot 1, ar
   DWORD | ch 8 slot 1 dr        |   0.0+  | channel 8, slot 1, dr
   DWORD | ch 8 slot 1 rr        |   0.0+  | channel 8, slot 1, rr
   BYTE  | ch 8 slot 1 KSR       |   0.0+  | channel 8, slot 1, KSR
   BYTE  | ch 8 slot 1 ksl       |   0.0+  | channel 8, slot 1, ksl
   BYTE  | ch 8 slot 1 ksr       |   0.0+  | channel 8, slot 1, ksr
   BYTE  | ch 8 slot 1 mul       |   0.0+  | channel 8, slot 1, mul
   DWORD | ch 8 slot 1 Cnt       |   0.0+  | channel 8, slot 1, Cnt
   DWORD | ch 8 slot 1 Incr      |   0.0+  | channel 8, slot 1, Incr
   BYTE  | ch 8 slot 1 FB        |   0.0+  | channel 8, slot 1, FB
   DWORD | ch 8 slot 1 connect1  |   0.0+  | channel 8, slot 1, connect1
   DWORD | ch 8 slot 1 op1 out 0 |   0.0+  | channel 8, slot 1, op1 out 0
   DWORD | ch 8 slot 1 op1 out 1 |   0.0+  | channel 8, slot 1, op1 out 1
   BYTE  | ch 8 slot 1 CON       |   0.0+  | channel 8, slot 1, CON
   BYTE  | ch 8 slot 1 eg type   |   0.0+  | channel 8, slot 1, eg type
   BYTE  | ch 8 slot 1 state     |   0.0+  | channel 8, slot 1, state
   DWORD | ch 8 slot 1 TL        |   0.0+  | channel 8, slot 1, TL
   DWORD | ch 8 slot 1 TLL       |   0.0+  | channel 8, slot 1, TLL
   DWORD | ch 8 slot 1 volume    |   0.0+  | channel 8, slot 1, volume
   DWORD | ch 8 slot 1 sl        |   0.0+  | channel 8, slot 1, sl
   BYTE  | ch 8 slot 1 eg sh ar  |   0.0+  | channel 8, slot 1, eg sh ar
   BYTE  | ch 8 slot 1 eg sel ar |   0.0+  | channel 8, slot 1, eg sel ar
   BYTE  | ch 8 slot 1 eg sh dr  |   0.0+  | channel 8, slot 1, eg sh dr
   BYTE  | ch 8 slot 1 eg sel dr |   0.0+  | channel 8, slot 1, eg sel dr
   BYTE  | ch 8 slot 1 eg sh rr  |   0.0+  | channel 8, slot 1, eg sh rr
   BYTE  | ch 8 slot 1 eg sel rr |   0.0+  | channel 8, slot 1, eg sel rr
   DWORD | ch 8 slot 1 key       |   0.0+  | channel 8, slot 1, key
   DWORD | ch 8 slot 1 AMmask    |   0.0+  | channel 8, slot 1, AMmask
   BYTE  | ch 8 slot 1 vib       |   0.0+  | channel 8, slot 1, vib
   WORD  | ch 8 slot 1 wavetable |   0.0+  | channel 8, slot 1, wavetable
   DWORD | ch 8 block fnum       |   0.0+  | channel 8, block fnum
   DWORD | ch 8 fc               |   0.0+  | channel 8, fc
   DWORD | ch 8 ksl base         |   0.0+  | channel 8, ksl base
   BYTE  | ch 8 kcode            |   0.0+  | channel 8, kcode
   DWORD | eg cnt                |   0.0+  | eg cnt
   DWORD | eg timer              |   0.0+  | eg timer
   DWORD | eg timer add          |   0.0+  | eg timer add
   DWORD | eg timer overflow     |   0.0+  | eg timer overflow
   BYTE  | rhythm                |   0.0+  | rhythm
   ARRAY | fn table              |   0.0+  | 1024 DWORDS of fn table data
   BYTE  | lfo am depth          |   0.0+  | lfo am depth
   BYTE  | lfo pm depth range    |   0.0+  | lfo pm depth range
   DWORD | lfo am cnt            |   0.0+  | lfo am cnt
   DWORD | lfo am inc            |   0.0+  | lfo am inc
   DWORD | lfo pm cnt            |   0.0+  | lfo pm cnt
   DWORD | lfo pm inc            |   0.0+  | lfo pm inc
   DWORD | noise rng             |   0.0+  | noise rng
   DWORD | noise p               |   0.0+  | noise p
   DWORD | noise f               |   0.0+  | noise f
   BYTE  | wavesel               |   0.0+  | wavesel
   DWORD | T 0                   |   0.0+  | T 0
   DWORD | T 1                   |   0.0+  | T 1
   BYTE  | st 0                  |   0.0+  | st 0
   BYTE  | st 1                  |   0.0+  | st 1
   BYTE  | type                  |   0.0+  | type
   BYTE  | address               |   0.0+  | address
   BYTE  | status                |   0.0+  | status
   BYTE  | status mask           |   0.0+  | status mask
   BYTE  | mode                  |   0.0+  | mode
   DWORD | clock                 |   0.0+  | clock
   DWORD | rate                  |   0.0+  | rate
   DBL   | freqbase              |   0.0+  | freqbase
 */

static char snap_module_name[] = "CARTSFXSE";
#define SNAP_MAJOR   0
#define SNAP_MINOR   1

int sfx_soundexpander_snapshot_write_module(snapshot_t *s)
{
    FM_OPL *chip = (sfx_soundexpander_chip == 3526) ? YM3526_chip : YM3812_chip;
    snapshot_module_t *m;
    int x, y;

    if (chip == NULL) {
        return 0;
    }

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)sfx_soundexpander_io_swap) < 0
        || SMW_DW(m, (DWORD)sfx_soundexpander_chip) < 0
        || SMW_B(m, (BYTE)snd.command) < 0) {
        goto fail;
    }

    for (x = 0; x < 9; x++) {
        for (y = 0; y < 2; y++) {
            if (0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].ar) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].dr) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].rr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].KSR) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].ksl) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].ksr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].mul) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].Cnt) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].Incr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].FB) < 0
                || SMW_DW(m, (DWORD)connect1_is_output0(chip->P_CH[x].SLOT[y].connect1)) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].op1_out[0]) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].op1_out[1]) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].CON) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_type) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].state) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].TL) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].TLL) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].volume) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].sl) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sh_ar) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sel_ar) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sh_dr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sel_dr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sh_rr) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].eg_sel_rr) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].key) < 0
                || SMW_DW(m, (DWORD)chip->P_CH[x].SLOT[y].AMmask) < 0
                || SMW_B(m, (BYTE)chip->P_CH[x].SLOT[y].vib) < 0
                || SMW_W(m, (WORD)chip->P_CH[x].SLOT[y].wavetable) < 0) {
                goto fail;
            }
        }
        if (0
            || SMW_DW(m, (DWORD)chip->P_CH[x].block_fnum) < 0
            || SMW_DW(m, (DWORD)chip->P_CH[x].fc) < 0
            || SMW_DW(m, (DWORD)chip->P_CH[x].ksl_base) < 0
            || SMW_B(m, (BYTE)chip->P_CH[x].kcode) < 0) {
            goto fail;
        }
    }

    if (0
        || SMW_DW(m, (DWORD)chip->eg_cnt) < 0
        || SMW_DW(m, (DWORD)chip->eg_timer) < 0
        || SMW_DW(m, (DWORD)chip->eg_timer_add) < 0
        || SMW_DW(m, (DWORD)chip->eg_timer_overflow) < 0
        || SMW_B(m, (BYTE)chip->rhythm) < 0) {
        goto fail;
    }

    for (x = 0; x < 1024; x++) {
        if (SMW_DW(m, (DWORD)chip->fn_tab[x]) < 0) {
            goto fail;
        }
    }

    if (0
        || SMW_B(m, (BYTE)chip->lfo_am_depth) < 0
        || SMW_B(m, (BYTE)chip->lfo_pm_depth_range) < 0
        || SMW_DW(m, (DWORD)chip->lfo_am_cnt) < 0
        || SMW_DW(m, (DWORD)chip->lfo_am_inc) < 0
        || SMW_DW(m, (DWORD)chip->lfo_pm_cnt) < 0
        || SMW_DW(m, (DWORD)chip->lfo_pm_inc) < 0
        || SMW_DW(m, (DWORD)chip->noise_rng) < 0
        || SMW_DW(m, (DWORD)chip->noise_p) < 0
        || SMW_DW(m, (DWORD)chip->noise_f) < 0
        || SMW_B(m, (BYTE)chip->wavesel) < 0
        || SMW_DW(m, (DWORD)chip->T[0]) < 0
        || SMW_DW(m, (DWORD)chip->T[1]) < 0
        || SMW_B(m, (BYTE)chip->st[0]) < 0
        || SMW_B(m, (BYTE)chip->st[1]) < 0
        || SMW_B(m, (BYTE)chip->type) < 0
        || SMW_B(m, (BYTE)chip->address) < 0
        || SMW_B(m, (BYTE)chip->status) < 0
        || SMW_B(m, (BYTE)chip->statusmask) < 0
        || SMW_B(m, (BYTE)chip->mode) < 0
        || SMW_DW(m, (DWORD)chip->clock) < 0
        || SMW_DW(m, (DWORD)chip->rate) < 0
        || SMW_DB(m, (double)chip->freqbase) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}

int sfx_soundexpander_snapshot_read_module(snapshot_t *s)
{
    BYTE vmajor, vminor;
    snapshot_module_t *m;
    int temp_chip;
    FM_OPL *chip = NULL;
    int temp_connect1;
    int x, y;

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
        if (SMR_B_INT(m, &sfx_soundexpander_io_swap) < 0) {
            goto fail;
        }
    } else {
        sfx_soundexpander_io_swap = 0;
    }

    if (SMR_DW_INT(m, &temp_chip) < 0) {
        goto fail;
    }

    if (sfx_soundexpander_sound_chip.chip_enabled) {
        set_sfx_soundexpander_enabled(0, NULL);
    }
    set_sfx_soundexpander_chip(temp_chip, NULL);
    set_sfx_soundexpander_enabled(1, NULL);
    chip = (temp_chip == 3526) ? YM3526_chip : YM3812_chip;

    if (SMR_B(m, &snd.command) < 0) {
        goto fail;
    }

    for (x = 0; x < 9; x++) {
        for (y = 0; y < 2; y++) {
            if (0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].ar) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].dr) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].rr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].KSR) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].ksl) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].ksr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].mul) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].Cnt) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].Incr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].FB) < 0
                || SMR_DW_INT(m, &temp_connect1) < 0
                || SMR_DW_INT(m, &chip->P_CH[x].SLOT[y].op1_out[0]) < 0
                || SMR_DW_INT(m, &chip->P_CH[x].SLOT[y].op1_out[1]) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].CON) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_type) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].state) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].TL) < 0
                || SMR_DW_INT(m, &chip->P_CH[x].SLOT[y].TLL) < 0
                || SMR_DW_INT(m, &chip->P_CH[x].SLOT[y].volume) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].sl) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sh_ar) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sel_ar) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sh_dr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sel_dr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sh_rr) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].eg_sel_rr) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].key) < 0
                || SMR_DW_UINT(m, &chip->P_CH[x].SLOT[y].AMmask) < 0
                || SMR_B(m, &chip->P_CH[x].SLOT[y].vib) < 0
                || SMR_W(m, &chip->P_CH[x].SLOT[y].wavetable) < 0) {
                goto fail;
            }
            set_connect1(chip, x, y, temp_connect1);
        }
        if (0
            || SMR_DW_UINT(m, &chip->P_CH[x].block_fnum) < 0
            || SMR_DW_UINT(m, &chip->P_CH[x].fc) < 0
            || SMR_DW_UINT(m, &chip->P_CH[x].ksl_base) < 0
            || SMR_B(m, &chip->P_CH[x].kcode) < 0) {
            goto fail;
        }
    }
    if (0
        || SMR_DW_UINT(m, &chip->eg_cnt) < 0
        || SMR_DW_UINT(m, &chip->eg_timer) < 0
        || SMR_DW_UINT(m, &chip->eg_timer_add) < 0
        || SMR_DW_UINT(m, &chip->eg_timer_overflow) < 0
        || SMR_B(m, &chip->rhythm) < 0) {
        goto fail;
    }
    for (x = 0; x < 1024; x++) {
        if (SMR_DW_UINT(m, &chip->fn_tab[x]) < 0) {
            goto fail;
        }
    }
    if (0
        || SMR_B(m, &chip->lfo_am_depth) < 0
        || SMR_B(m, &chip->lfo_pm_depth_range) < 0
        || SMR_DW_UINT(m, &chip->lfo_am_cnt) < 0
        || SMR_DW_UINT(m, &chip->lfo_am_inc) < 0
        || SMR_DW_UINT(m, &chip->lfo_pm_cnt) < 0
        || SMR_DW_UINT(m, &chip->lfo_pm_inc) < 0
        || SMR_DW_UINT(m, &chip->noise_rng) < 0
        || SMR_DW_UINT(m, &chip->noise_p) < 0
        || SMR_DW_UINT(m, &chip->noise_f) < 0
        || SMR_B(m, &chip->wavesel) < 0
        || SMR_DW_UINT(m, &chip->T[0]) < 0
        || SMR_DW_UINT(m, &chip->T[1]) < 0
        || SMR_B(m, &chip->st[0]) < 0
        || SMR_B(m, &chip->st[1]) < 0
        || SMR_B(m, &chip->type) < 0
        || SMR_B(m, &chip->address) < 0
        || SMR_B(m, &chip->status) < 0
        || SMR_B(m, &chip->statusmask) < 0
        || SMR_B(m, &chip->mode) < 0
        || SMR_DW_UINT(m, &chip->clock) < 0
        || SMR_DW_UINT(m, &chip->rate) < 0
        || SMR_DB(m, &chip->freqbase) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
