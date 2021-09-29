/*
 * petsound.c - implementation of PET sound code
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
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
#include <math.h>

#include "lib.h"
#include "machine.h"
#include "petsound.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-resources.h"
#include "sound.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

/* Some prototypes are needed */
static int pet_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int pet_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void pet_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val);
static void pet_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int pet_sound_machine_cycle_based(void)
{
    return 0;
}

static int pet_sound_machine_channels(void)
{
    return 1;
}

/* PET userport sound device */
static sound_chip_t pet_sound_chip = {
    NULL,                                /* NO sound chip open function */ 
    pet_sound_machine_init,              /* sound chip init function */
    NULL,                                /* NO sound chip close function */
    pet_sound_machine_calculate_samples, /* sound chip calculate samples function */
    pet_sound_machine_store,             /* sound chip store function */
    NULL,                                /* NO sound chip read function */
    pet_sound_reset,                     /* sound chip reset function */
    pet_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, chip is NOT cycle based */
    pet_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 1 channel */
    1                                    /* sound chip enabled flag, chip is always enabled */
};

static uint16_t pet_sound_chip_offset = 0;

void pet_sound_chip_init(void)
{
    pet_sound_chip_offset = sound_chip_register(&pet_sound_chip);
}

/* ------------------------------------------------------------------------- */

/* dummy function for now */
int machine_sid2_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid3_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid4_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid5_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid6_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid7_check_range(unsigned int sid_adr)
{
    return 0;
}

/* dummy function for now */
int machine_sid8_check_range(unsigned int sid_adr)
{
    return 0;
}

void machine_sid2_enable(int val)
{
}

struct pet_sound_s {
    int on;
    CLOCK t;
    uint8_t sample;

    double b;
    double bs;

    int speed;
    int cycles_per_sec;

    int manual;       /* 1 if CB2 set to manual control "high", 0 otherwise */
};

static uint8_t snddata[5];
static struct pet_sound_s snd;

/* XXX: this is not correct */
static uint16_t pet_makesample(double s, double e, uint8_t sample)
{
    double v;
    int sc, sf, ef, i, nr;

    sc = (int)ceil(s);
    sf = (int)floor(s);
    ef = (int)floor(e);
    nr = 0;

    for (i = sc; i < ef; i++) {
        if (sample & (1 << (i % 8))) {
            nr++;
        }
    }

    v = nr;

    if (sample & (1 << (sf % 8))) {
        v += sc - s;
    }
    if (sample & (1 << (ef % 8))) {
        v += e - ef;
    }

    return ((uint16_t)(v * 4095.0 / (e - s)));
}

static int pet_sound_machine_calculate_samples(sound_t **psid, int16_t *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i;
    uint16_t v = 0;

    for (i = 0; i < nr; i++) {
        if (snd.on) {
            v = pet_makesample(snd.b, snd.b + snd.bs, snd.sample);
        } else if (snd.manual) {
            v = 20000;
        }

        pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], (int16_t)v);
        if (soc > 1) {
            pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], (int16_t)v);
        }

        snd.b += snd.bs;
        while (snd.b >= 8.0) {
            snd.b -= 8.0;
        }
    }
    return nr;
}

void petsound_store_onoff(int value)
{
    snddata[0] = value;
    sound_store(pet_sound_chip_offset, snddata[0], 0);
}

void petsound_store_rate(CLOCK t)
{
    snddata[2] = (uint8_t)(t & 0xff);
    snddata[3] = (uint8_t)((t >> 8) & 0xff);
    sound_store((uint16_t)(pet_sound_chip_offset | 2), snddata[2], 0);
    sound_store((uint16_t)(pet_sound_chip_offset | 3), snddata[3], 0);
}

void petsound_store_sample(uint8_t sample)
{
    snddata[1] = sample;
    sound_store((uint16_t)(pet_sound_chip_offset | 1), snddata[1], 0);
}

/* For manual control of CB2 sound using $E84C */
void petsound_store_manual(int value)
{
    snddata[4] = value;
    sound_store((uint16_t)(pet_sound_chip_offset | 4), snddata[4], 0);
}

static void pet_sound_machine_store(sound_t *psid, uint16_t addr, uint8_t val)
{
    switch (addr) {
        case 0:
            snd.on = val;
            break;
        case 1:
            snd.sample = val;
            while (snd.b >= 1.0) {
                snd.b -= 1.0;
            }
            break;
        case 2:
            snd.t = val;
            break;
        case 3:
            snd.t = (snd.t & 0xff) | (val << 8);
            snd.bs = (double)snd.cycles_per_sec / (snd.t * snd.speed);
            break;
        case 4:
            snd.manual = val;
            break;
        default:
            abort();
    }
}

static int pet_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    uint16_t i;

    snd.speed = speed;
    snd.cycles_per_sec = cycles_per_sec;
    if (!snd.t) {
        snd.t = 32;
    }
    snd.b = 0.0;
    snd.bs = (double)snd.cycles_per_sec / (snd.t * snd.speed);

    snddata[0] = 0;
    snddata[1] = 0;
    snddata[2] = 4;
    snddata[3] = 0;
    snddata[4] = 0;

    for (i = 0; i < 5; i++) {
        pet_sound_machine_store(psid, i, snddata[i]);
    }

    return 1;
}

static void pet_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    petsound_store_onoff(0);
}

void petsound_reset(sound_t *psid, CLOCK cpu_clk)
{
    sound_reset();
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
