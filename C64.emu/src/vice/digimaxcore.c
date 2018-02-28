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

/* ---------------------------------------------------------------------*/

static sound_dac_t digimax_dac[4];

static BYTE digimax_sound_data[4];

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
