/*
 * digimaxcore.c - Digimax DAC device core emulation.
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

/* This source file contains the sound core for the cartridge,
   the shortbus and userport versions of the device, and is intended
   to be included from a specific digimax device. */


/* Some prototypes are needed */
static int digimax_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static void digimax_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val);
static uint8_t digimax_sound_machine_read(sound_t *psid, uint16_t addr);
static void digimax_sound_reset(sound_t *psid, CLOCK cpu_clk);

#ifdef SOUND_SYSTEM_FLOAT
static int digimax_sound_machine_calculate_samples(sound_t **psid, float *pbuf, int nr, int sound_chip_channels, CLOCK *delta_t);
#else
static int digimax_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
#endif

static int digimax_sound_machine_cycle_based(void)
{
    return 0;
}

static int digimax_sound_machine_channels(void)
{
    return 4;
}

#ifdef SOUND_SYSTEM_FLOAT
/* stereo mixing placement of the DigiMAX sound */
static sound_chip_mixing_spec_t digimax_sound_mixing_spec[SOUND_CHIP_CHANNELS_MAX] = {
    {
        100, /* DAC 1 left channel volume % in case of stereo output, default output to left only */
        0,   /* DAC 1 right channel volume % in case of stereo output, default output to left only */
    },
    {
        100, /* DAC 2 left channel volume % in case of stereo output, default output to left only */
        0,   /* DAC 2 right channel volume % in case of stereo output, default output to left only */
    },
    {
        0,   /* DAC 3 left channel volume % in case of stereo output, default output to right only */
        100, /* DAC 3 right channel volume % in case of stereo output, default output to right only */
    },
    {
        0,   /* DAC 4 left channel volume % in case of stereo output, default output to right only */
        100, /* DAC 4 right channel volume % in case of stereo output, default output to right only */
    }
};
#endif

/* DigiMAX sound chip, as used in the IDE64-shortbus DigiMAX device, userport DigiMAX device and c64/c128 DigiMAX cartridge */
static sound_chip_t digimax_sound_chip = {
    NULL,                                    /* NO sound chip open function */
    digimax_sound_machine_init,              /* sound chip init function */
    NULL,                                    /* NO sound chip close function */
    digimax_sound_machine_calculate_samples, /* sound chip calculate samples function */
    digimax_sound_machine_store,             /* sound chip store function */
    digimax_sound_machine_read,              /* sound chip read function */
    digimax_sound_reset,                     /* sound chip reset function */
    digimax_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, chip is NOT cycle based */
    digimax_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 4 channels */
#ifdef SOUND_SYSTEM_FLOAT
    digimax_sound_mixing_spec,               /* stereo mixing placement specs */
#endif
    0                                        /* sound chip enabled flag, toggled upon device (de-)activation */
};

static uint16_t digimax_sound_chip_offset = 0;

/* ---------------------------------------------------------------------*/

static sound_dac_t digimax_dac[4];

static uint8_t digimax_sound_data[4];

struct digimax_sound_s {
    uint8_t voice[4];
};

static struct digimax_sound_s snd;

#ifdef SOUND_SYSTEM_FLOAT
/* FIXME: fix this for multichannel output */
static int digimax_sound_machine_calculate_samples(sound_t **psid, float *pbuf, int nr, int scc, CLOCK *delta_t)
{
    sound_dac_calculate_samples(&digimax_dac[scc], pbuf, (int)snd.voice[scc] * 64, nr);

    return nr;
}
#else
static int digimax_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    sound_dac_calculate_samples(&digimax_dac[0], pbuf, (int)snd.voice[0] * 64, nr, soc, SOUND_CHANNEL_1);
    sound_dac_calculate_samples(&digimax_dac[1], pbuf, (int)snd.voice[1] * 64, nr, soc, (soc == SOUND_OUTPUT_STEREO) ? SOUND_CHANNEL_2 : SOUND_CHANNEL_1);
    sound_dac_calculate_samples(&digimax_dac[2], pbuf, (int)snd.voice[2] * 64, nr, soc, SOUND_CHANNEL_1);
    sound_dac_calculate_samples(&digimax_dac[3], pbuf, (int)snd.voice[3] * 64, nr, soc, (soc == SOUND_OUTPUT_STEREO) ? SOUND_CHANNEL_2 : SOUND_CHANNEL_1);
    return nr;
}
#endif

static int digimax_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sound_dac_init(&digimax_dac[0], speed);
    sound_dac_init(&digimax_dac[1], speed);
    sound_dac_init(&digimax_dac[2], speed);
    sound_dac_init(&digimax_dac[3], speed);
    snd.voice[0] = 0;
    snd.voice[1] = 0;
    snd.voice[2] = 0;
    snd.voice[3] = 0;

    return 1;
}

static void digimax_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val)
{
    snd.voice[addr & 3] = val;
}

static uint8_t digimax_sound_machine_read(sound_t *psid, uint16_t addr)
{
    return digimax_sound_data[addr & 3];
}

static void digimax_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    snd.voice[0] = 0;
    snd.voice[1] = 0;
    snd.voice[2] = 0;
    snd.voice[3] = 0;
    digimax_sound_data[0] = 0;
    digimax_sound_data[1] = 0;
    digimax_sound_data[2] = 0;
    digimax_sound_data[3] = 0;
}
