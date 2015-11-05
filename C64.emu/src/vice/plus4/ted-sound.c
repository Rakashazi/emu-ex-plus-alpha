/*
 * ted-sound.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown @ axelero . hu>
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

#include "digiblaster.h"
#include "lib.h"
#include "maincpu.h"
#include "plus4.h"
#include "plus4speech.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-resources.h"
#include "sound.h"
#include "ted-sound.h"

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int ted_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int ted_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void ted_sound_machine_store(sound_t *psid, WORD addr, BYTE val);
static BYTE ted_sound_machine_read(sound_t *psid, WORD addr);

static int ted_sound_machine_cycle_based(void)
{
    return 0;
}

static int ted_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t ted_sound_chip = {
    NULL, /* no open */
    ted_sound_machine_init,
    NULL, /* no close */
    ted_sound_machine_calculate_samples,
    ted_sound_machine_store,
    ted_sound_machine_read,
    ted_sound_reset,
    ted_sound_machine_cycle_based,
    ted_sound_machine_channels,
    1 /* chip enabled */
};

static WORD ted_sound_chip_offset = 0;

void ted_sound_chip_init(void)
{
    ted_sound_chip_offset = sound_chip_register(&ted_sound_chip);
}

/* ------------------------------------------------------------------------- */

static BYTE plus4_sound_data[5];

/* dummy function for now */
int machine_sid2_check_range(unsigned int sid2_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid3_check_range(unsigned int sid3_adr)
{
    return 0;
}

void machine_sid2_enable(int val)
{
}

struct plus4_sound_s {
    /* Voice 0 collect number of cycles elapsed */
    DWORD voice0_accu;
    /* Voice 0 toggle sign and reload accu if accu reached 0 */
    DWORD voice0_reload;
    /* Voice 0 sign of the square wave */
    SWORD voice0_sign;
    BYTE voice0_output_enabled;

    /* Voice 1 collect number of cycles elapsed */
    DWORD voice1_accu;
    /* Voice 1 toggle sign and reload accu if accu reached 0 */
    DWORD voice1_reload;
    /* Voice 1 sign of the square wave */
    SWORD voice1_sign;
    BYTE voice1_output_enabled;

    /* Volume multiplier  */
    SWORD volume;
    /* 8 cycles units per sample  */
    DWORD speed;
    DWORD sample_position_integer;
    DWORD sample_position_remainder;
    DWORD sample_length_integer;
    DWORD sample_length_remainder;
    /* Digital output?  */
    BYTE digital;
    /* Noise generator active?  */
    BYTE noise;
    BYTE noise_shift_register;
};

static struct plus4_sound_s snd;

/* FIXME: Find proper volume multiplier.  */
static const SWORD volume_tab[16] = {
    0x0000, 0x0800, 0x1000, 0x1800, 0x2000, 0x2800, 0x3000, 0x3800,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff
};

static int ted_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    int j;
    SWORD volume;

    if (snd.digital) {
        for (i = 0; i < nr; i++) {
            pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], (snd.volume * (snd.voice0_output_enabled + snd.voice1_output_enabled)));
            if (soc > 1) {
                pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], (snd.volume * (snd.voice0_output_enabled + snd.voice1_output_enabled)));
            }
        }
    } else {
        for (i = 0; i < nr; i++) {
            snd.sample_position_remainder += snd.sample_length_remainder;
            if (snd.sample_position_remainder >= snd.speed) {
                snd.sample_position_remainder -= snd.speed;
                snd.sample_position_integer++;
            }
            snd.sample_position_integer += snd.sample_length_integer;
            if (snd.sample_position_integer >= 8) {
                /* Advance state engine */
                DWORD ticks = snd.sample_position_integer >> 3;
                if (snd.voice0_accu <= ticks) {
                    DWORD delay = ticks - snd.voice0_accu;
                    snd.voice0_sign ^= 1;
                    snd.voice0_accu = 1023 - snd.voice0_reload;
                    if (snd.voice0_accu == 0) {
                        snd.voice0_accu = 1024;
                    }
                    if (delay >= snd.voice0_accu) {
                        snd.voice0_sign = ((delay / snd.voice0_accu)
                                           & 1) ? snd.voice0_sign ^ 1
                                          : snd.voice0_sign;
                        snd.voice0_accu = snd.voice0_accu - (delay % snd.voice0_accu);
                    } else {
                        snd.voice0_accu -= delay;
                    }
                } else {
                    snd.voice0_accu -= ticks;
                }

                if (snd.voice1_accu <= ticks) {
                    DWORD delay = ticks - snd.voice1_accu;
                    snd.voice1_sign ^= 1;
                    snd.noise_shift_register
                        = (snd.noise_shift_register << 1) +
                          ( 1 ^ ((snd.noise_shift_register >> 7) & 1) ^
                            ((snd.noise_shift_register >> 5) & 1) ^
                            ((snd.noise_shift_register >> 4) & 1) ^
                            ((snd.noise_shift_register >> 1) & 1));
                    snd.voice1_accu = 1023 - snd.voice1_reload;
                    if (snd.voice1_accu == 0) {
                        snd.voice1_accu = 1024;
                    }
                    if (delay >= snd.voice1_accu) {
                        snd.voice1_sign = ((delay / snd.voice1_accu)
                                           & 1) ? snd.voice1_sign ^ 1
                                          : snd.voice1_sign;
                        for (j = 0; j < (int)(delay / snd.voice1_accu);
                             j++) {
                            snd.noise_shift_register
                                = (snd.noise_shift_register << 1) +
                                  ( 1 ^ ((snd.noise_shift_register >> 7) & 1) ^
                                    ((snd.noise_shift_register >> 5) & 1) ^
                                    ((snd.noise_shift_register >> 4) & 1) ^
                                    ((snd.noise_shift_register >> 1) & 1));
                        }
                        snd.voice1_accu = snd.voice1_accu - (delay % snd.voice1_accu);
                    } else {
                        snd.voice1_accu -= delay;
                    }
                } else {
                    snd.voice1_accu -= ticks;
                }
            }
            snd.sample_position_integer = snd.sample_position_integer & 7;

            volume = 0;

            if (snd.voice0_output_enabled && snd.voice0_sign) {
                volume += snd.volume;
            }
            if (snd.voice1_output_enabled && !snd.noise && snd.voice1_sign) {
                volume += snd.volume;
            }
            if (snd.voice1_output_enabled && snd.noise && (!(snd.noise_shift_register & 1))) {
                volume += snd.volume;
            }

            pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], volume);
            if (soc > 1) {
                pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], volume);
            }
        }
    }
    return nr;
}

static int ted_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    BYTE val;

    snd.speed = speed;
    snd.sample_length_integer = cycles_per_sec / speed;
    snd.sample_length_remainder = cycles_per_sec % speed;
    snd.sample_position_integer = 0;
    snd.sample_position_remainder = 0;

    snd.voice0_reload = (plus4_sound_data[0] | (plus4_sound_data[4] << 8));
    snd.voice1_reload = (plus4_sound_data[1] | (plus4_sound_data[2] << 8));
    val = plus4_sound_data[3];
    snd.volume = volume_tab[val & 0x0f];
    snd.voice0_output_enabled = (val & 0x10) ? 1 : 0;
    snd.voice1_output_enabled = (val & 0x60) ? 1 : 0;
    snd.noise = ((val & 0x60) == 0x40) ? 1 : 0;
    snd.digital = val & 0x80;
    if (snd.digital) {
        snd.voice0_sign = 1;
        snd.voice0_accu = 0;
        snd.voice1_sign = 1;
        snd.voice1_accu = 0;
        snd.noise_shift_register = 0;
    }

    return 1;
}

static void ted_sound_machine_store(sound_t *psid, WORD addr, BYTE val)
{
    switch (addr) {
        case 0x0e:
            plus4_sound_data[0] = val;
            snd.voice0_reload = (plus4_sound_data[0] | (plus4_sound_data[4] << 8));
            break;
        case 0x0f:
            plus4_sound_data[1] = val;
            snd.voice1_reload = (plus4_sound_data[1] | (plus4_sound_data[2] << 8));
            break;
        case 0x10:
            plus4_sound_data[2] = val & 3;
            snd.voice1_reload = (plus4_sound_data[1] | (plus4_sound_data[2] << 8));
            break;
        case 0x11:
            snd.volume = volume_tab[val & 0x0f];
            snd.voice0_output_enabled = (val & 0x10) ? 1 : 0;
            snd.voice1_output_enabled = (val & 0x60) ? 1 : 0;
            snd.noise = ((val & 0x60) == 0x40) ? 1 : 0;
            snd.digital = val & 0x80;
            if (snd.digital) {
                snd.voice0_sign = 1;
                snd.voice0_accu = 0;
                snd.voice1_sign = 1;
                snd.voice1_accu = 0;
                snd.noise_shift_register = 0;
            }
            plus4_sound_data[3] = val;
            break;
        case 0x12:
            plus4_sound_data[4] = val & 3;
            snd.voice0_reload = (plus4_sound_data[0] | (plus4_sound_data[4] << 8));
            break;
    }
}

static BYTE ted_sound_machine_read(sound_t *psid, WORD addr)
{
    switch (addr) {
        case 0x0e:
            return plus4_sound_data[0];
        case 0x0f:
            return plus4_sound_data[1];
        case 0x10:
            return plus4_sound_data[2] | 0xc0;
        case 0x11:
            return plus4_sound_data[3];
        case 0x12:
            return plus4_sound_data[4];
    }

    return 0;
}

void ted_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    WORD i;

    snd.noise_shift_register = 0;
    for (i = 0x0e; i <= 0x12; i++) {
        ted_sound_store(i, 0);
    }
}

/* ---------------------------------------------------------------------*/

void ted_sound_store(WORD addr, BYTE value)
{
    sound_store((WORD)(ted_sound_chip_offset | addr), value, 0);
}

BYTE ted_sound_read(WORD addr)
{
    BYTE value;

    value = sound_read((WORD)(ted_sound_chip_offset | addr), 0);

    if (addr == 0x12) {
        value &= 3;
    }

    return value;
}

void sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
    sid_sound_machine_prevent_clk_overflow(psid, sub);
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
