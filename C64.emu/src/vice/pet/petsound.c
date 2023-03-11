/*
 * petsound.c - implementation of PET sound code
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Olaf Seibert <rhialto@falu.nl>
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
#include "maincpu.h"
#include "petsound.h"
#include "sid.h"
#include "sidcart.h"
#include "resources.h"
#include "sound.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

/* #define DBG(...)        fprintf(stderr, __VA_ARGS__) */
#define DBG(...)

/*
 * Enable a low-pass filter directly on the CB2 output.
 * Disabling this may reduce the CPU usage somewhat.
 * In that case, the CB2 signal is averaged for each output sample
 * and a low-pass filter is applied on that.
 */
#define LOWPASS_CB2     1
#define HIGHPASS        0       /* Experimental so far; default off */

#define MAX_SAMPLE      4095

/*
 * Low-pass filter.
 */
#define ALPHA_SCALE     65536
#define LP_SCALE        65536
#define LP_TABLESZ        256   /* At 8000 Hz we have 1 000 000/8 000 = 125
                                   clocks per sample. Doubling that for this
                                   table should be more than enough. */

typedef int16_t sample_t;       /* Dictated from outside; range [0,4096> */
typedef uint32_t big_sample_t;  /* Our samples with extra bits for precision
                                   during low-pass filtering */
typedef uint32_t big_samples_t; /* Big enough for multiplying big samples,
                                   i.e. (LP_SCALE-1) * (LP_SCALE-1) */
typedef int64_t big_sample_diff_t; /* Only needs 33 bits, really */

/* Some prototypes are needed */

static int pet_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int pet_sound_machine_calculate_samples(sound_t **psid, sample_t *pbuf, int nr, int sound_output_channels, int sound_chip_channels, CLOCK *delta_t);
static void pet_sound_reset(sound_t *psid, CLOCK cpu_clk);
static void create_intermediate_samples(CLOCK rclk);

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
    .open = NULL,                      /* NO sound chip open function */
    .init = pet_sound_machine_init,    /* sound chip init function */
    .close = NULL,                     /* NO sound chip close function */
    .calculate_samples = pet_sound_machine_calculate_samples,
    .store = NULL,                     /* NO sound chip store function */
    .read = NULL,                      /* NO sound chip read function */
    .reset = pet_sound_reset,          /* sound chip reset function */
    .cycle_based = pet_sound_machine_cycle_based,
                                       /* chip is NOT cycle based */
    .channels = pet_sound_machine_channels,/* sound chip has 1 channel */
    .chip_enabled = false,             /* chip is enabled after init */
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
    bool on;            /* are we even making sound? */

    int speed;          /* sample rate * 100 / speed_percent */
    int cycles_per_sec;

    bool manual;        /* 1 if CB2 set to manual control "high", 0 otherwise */

    CLOCK next_sample_time;     /* start time of sample under construction */
    CLOCK end_of_sample_time;   /* end time of same */
    CLOCK latest_bit_time;      /* last time constructing sample was updated */

    int first_sample_index;     /* where the consumer gets samples */
    int next_sample_index;      /* the producer is creating this sample */

    int clocks_per_sample;
    int fracs_per_sample;       /* fixed-point */
    int end_of_sample_frac;
#define FRAC_BITS 16
#define FRAC_MASK ((1 << FRAC_BITS) - 1)
    int lowpass_prev;           /* lowpass filter, prev sample */
#define NSAMPLES 256            /* 5 is usually enough... */
    big_sample_t samples[NSAMPLES];
#if LOWPASS_CB2
    /* Increasing weights for the newer sample as time goes on */
    big_sample_t exponential_moving_average[LP_TABLESZ];
#else
    int alpha;
#endif /* LOWPASS_CB2 */
#if HIGHPASS
    int highpass_alpha;
    int highpass_prev;          /* highpass filter, prev sample;
                                   keep separate from lowpass_prev */
#endif
};

static struct pet_sound_s snd = {
    .speed = 48000,
    .on = false,
    .clocks_per_sample = 20,
};

#if LOWPASS_CB2

static inline big_sample_t lowpass(big_sample_t alpha, big_sample_t prev, big_sample_t next)
{
    prev += ((big_sample_diff_t)alpha * ((big_sample_diff_t)next - (big_sample_diff_t)prev)) / ALPHA_SCALE;
    return prev;

    /*
       return ((ALPHA_SCALE - alpha) * (big_samples_t)prev +
                              alpha  * (big_samples_t)next) / ALPHA_SCALE;
     *
     * prev = ((ALPHA_SCALE - alpha) * prev + alpha * next) / ALPHA_SCALE;
     * prev = (ALPHA_SCALE*prev - alpha*prev + alpha * next) / ALPHA_SCALE;
     * prev = prev + (- alpha * prev + alpha * next) / ALPHA_SCALE;
     * prev = prev + (alpha * next - alpha * prev) / ALPHA_SCALE;
     * prev = prev + (alpha * (next - prev)) / ALPHA_SCALE;
     * prev += (alpha * (next - prev)) / ALPHA_SCALE;
     */
}

static inline double dlowpass(big_sample_t alpha, double prev, double next)
{
    return ((ALPHA_SCALE - alpha) * prev +
                           alpha  * next) / ALPHA_SCALE;
}

static void init_lowpass_table(int alpha)
{
    double sample = 0;

    for (int i = 0; i < LP_TABLESZ; i++) {
        snd.exponential_moving_average[i] = sample + 0.5;
        if (i < 10) {
            DBG("%d\t%f\n", i, sample);
        }

        sample = dlowpass(alpha, sample, LP_SCALE);
    }
}

/*
 * This function is equivalent with calling lowpass() repeatedly
 * with the same alpha and next sample (the case where the CB2 output
 * remains constant for a while):
 *
 * for (int t = 0; t < time; t++)
 *     prev = lowpass(ALPHA, prev, next);
 *
 * Instead of repeating, it uses a table lookup. The table was prepared
 * in init_lowpass_table() for the desired value of ALPHA.
 *
 * https://www.embeddedrelated.com/showarticle/779.php
 * https://helpful.knobs-dials.com/index.php/Low-pass_filter
 * https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
 */
static inline int lowpass_repeated(big_sample_t prev, big_sample_t next, int time)
{
    if (prev == next) {
        return next;
    }

    if (time < 0 || time >= LP_TABLESZ) {
        time  = LP_TABLESZ - 1;
    }

    int alpha = snd.exponential_moving_average[time];
    prev = lowpass(alpha, prev, next);

    return prev;
}

#endif /* LOWPASS_CB2 */

/*
 * Collect a sample from the ring buffer.
 *
#if LOWPASS_CB2
 * The samples have been processed through a low-pass filter when
 * they were created.
 * We need to drop the extra bits used to make the filtering more precise.
#else
 * The samples have been created by averaging the CB2 signal over
 * the time of the sample. Here we apply a low-pass filter on that.
#endif
 */
static sample_t pet_makesample(void)
{
    if (snd.first_sample_index != snd.next_sample_index) {
        int sample = snd.samples[snd.first_sample_index];
        snd.first_sample_index++;
        snd.first_sample_index %= NSAMPLES;

#if HIGHPASS
        /* The highpass value is scaled with the same factor
         * as the sample. */
        snd.highpass_prev += (snd.highpass_alpha * (sample - snd.highpass_prev))
                             / ALPHA_SCALE;
#endif /* HIGHPASS */
#if LOWPASS_CB2
        /*
         * Reduce the range from [0, LP_SCALE> to [0, MAX_SAMPLE].
         */
# if HIGHPASS
        /* Subtract highpass value here, so it gets scaled with the
         * already low-passed sample. */
        sample -= snd.highpass_prev;
# endif /* HIGHPASS */
        sample = sample * (MAX_SAMPLE+1) / LP_SCALE;
        snd.lowpass_prev = sample;      /* Only used when samples run out */
#else /* LOWPASS_CB2 */
        /* Low-pass filtering on the averaged CB2 signal */
        snd.lowpass_prev += (snd.alpha * (sample - snd.lowpass_prev))
                            / ALPHA_SCALE;
        sample = snd.lowpass_prev;
# if HIGHPASS
        /* Now that the sample has been low-passed, subtract the
         * high-pass value */
        sample -= snd.highpass_prev;
# endif /* HIGHPASS */
#endif /* LOWPASS_CB2 */

        return sample;
    }

    /* No more samples available... */
    DBG("*");
#if HIGHPASS
    if (snd.lowpass_prev != 0) {
        snd.lowpass_prev += (snd.highpass_alpha * (0 - snd.lowpass_prev))
                           / ALPHA_SCALE;
    }
#endif /* HIGHPASS */
    return snd.lowpass_prev;
}

static int pet_sound_machine_calculate_samples(sound_t **psid, sample_t *pbuf, int nr, int soc, int scc, CLOCK *delta_t)
{
    int i;
    sample_t v = 0;

    create_intermediate_samples(maincpu_clk);

    for (i = 0; i < nr; i++) {
        v = pet_makesample();

        /* pbuf[i * soc] = v; */
        pbuf[i * soc] = sound_audio_mix(pbuf[i * soc], (sample_t)v);
#if 0
        if (soc > 1) {
            pbuf[(i * soc) + 1] = sound_audio_mix(pbuf[(i * soc) + 1], (sample_t)v);
        }
#endif

    }
    return nr;
}

/*
 * This function works together with petvia.c to turn off the sound
 * if the shift register is under control of Timer 2, but the timer
 * value is 0. On real hardware, the result is inaudible,  but here
 * we would get annoying background noise. Even the low-pass filter
 * doesn't get rid of it, so we do it by simply disabling the sound.
 */
void petsound_store_onoff(bool value)
{
    if (pet_sound_chip.chip_enabled) {
        create_intermediate_samples(maincpu_clk);
    }

    snd.on = value;
    if (!snd.on) {
        snd.manual = false;
    }
}

static void create_intermediate_samples(CLOCK rclk)
{
    while (rclk >= snd.end_of_sample_time) {
#if LOWPASS_CB2
        /*
         * Now that the CB2 signal changes, we know how long
         * the previous state lasted, and can process that period.
         */
        CLOCK time = snd.end_of_sample_time - snd.latest_bit_time;
        big_sample_t sample = snd.samples[snd.next_sample_index];
        big_sample_t newsample = snd.manual ? LP_SCALE-1 : 0;
        sample = lowpass_repeated(sample, newsample, (int)time);
        snd.samples[snd.next_sample_index] = sample;
#else /* LOWPASS_CB2 */
        if (snd.manual) {
            CLOCK time = snd.end_of_sample_time - snd.latest_bit_time;
            int contribution = time /* * snd.manual */;
            snd.samples[snd.next_sample_index] += contribution;
        }

        /*
         * Because sometimes a sample can be a cycle longer due to
         * accumulated rounding (fracs_per_sample), we must average and
         * scale the sample value here, where we still know the real time
         * in clocks.
         * fracs_per_sample is significant enough that we can't ignore it.
         */
        snd.samples[snd.next_sample_index] =
            (snd.samples[snd.next_sample_index] * MAX_SAMPLE) /
            (snd.end_of_sample_time - snd.next_sample_time);
#endif /* LOWPASS_CB2 */

        snd.next_sample_index++;
        snd.next_sample_index %= NSAMPLES;
#if LOWPASS_CB2
        snd.samples[snd.next_sample_index] = sample;
#else
        snd.samples[snd.next_sample_index] = 0;
#endif

        snd.next_sample_time = snd.end_of_sample_time;
        snd.latest_bit_time = snd.end_of_sample_time;
        snd.end_of_sample_time += snd.clocks_per_sample;
        snd.end_of_sample_frac += snd.fracs_per_sample;
        snd.end_of_sample_time += snd.end_of_sample_frac >> FRAC_BITS;
        snd.end_of_sample_frac &= FRAC_MASK;
    }
}

/* For manual control of CB2 sound using $E84C */
void petsound_store_manual(bool value, CLOCK rclk)
{
    /* DBG("%c", value ? ':' : '.'); */
    /* Only do something when the signal changes */
    if (!snd.on || value == snd.manual) {
        return;
    }

    if (pet_sound_chip.chip_enabled) {
        create_intermediate_samples(rclk);

        /*
         * Now that the CB2 signal changes, we know how long
         * the previous state lasted, and can process that period.
         */
#if LOWPASS_CB2
        CLOCK time = rclk - snd.latest_bit_time;
        big_sample_t sample = snd.samples[snd.next_sample_index];
        big_sample_t newsample = snd.manual ? LP_SCALE-1 : 0;
        sample = lowpass_repeated(sample, newsample, (int)time);
        snd.samples[snd.next_sample_index] = sample;
#else /* LOWPASS_CB2 */
        if (snd.manual) {
            CLOCK time = rclk - snd.latest_bit_time;
            int contribution = time /* * snd.manual */;
            snd.samples[snd.next_sample_index] += contribution;
        }
#endif /* LOWPASS_CB2 */
    }

    /* Remember when this CB2 state started (i.e. this moment) */
    snd.latest_bit_time = rclk;
    snd.manual = value;
}

/*
 * Calculate the alpha parameter for the low-pass filter.
 * Multiply it with scale_factor to make it more precise, in fixed-point
 * (the samples themselves can use a different fixed-point offset).
 */
static int calculate_alpha(int sample_freq, int limit_freq, int scale_factor)
{
    double delta_t = 1.0 / sample_freq;
    double tau = 1.0 / (2.0 * 3.141592654 * limit_freq);
    double falpha = delta_t / (delta_t + tau);
    int alpha = falpha * scale_factor + 0.5;

    return alpha;
}

static int pet_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    DBG("### pet_sound_machine_init: speed %d cycles_per_sec %d\n", speed, cycles_per_sec);
    pet_sound_reset(psid, maincpu_clk);

    snd.cycles_per_sec = cycles_per_sec;
    snd.speed = speed;

    CLOCK clocks_per_sample = ((CLOCK)snd.cycles_per_sec << FRAC_BITS)
                            / snd.speed;

    snd.clocks_per_sample = (int)(clocks_per_sample >> FRAC_BITS);
    snd.fracs_per_sample  = clocks_per_sample & FRAC_MASK;
    snd.next_sample_time = maincpu_clk;
    snd.end_of_sample_time = snd.next_sample_time + snd.clocks_per_sample;
    snd.end_of_sample_frac = snd.fracs_per_sample;

    DBG("### pet_sound_machine_init: clocks_per_sample %d %d\n", snd.clocks_per_sample, snd.fracs_per_sample);
    /*
     * Calculate the value of ALPHA for the given CB2Lowpass which is
     * the knee frequency or the 3dB-down-frequency (depending on source).
     */
    int cb2_lowpass_freq;
    resources_get_int("CB2Lowpass", &cb2_lowpass_freq);
#if LOWPASS_CB2
    int alpha = calculate_alpha(cycles_per_sec, cb2_lowpass_freq, ALPHA_SCALE);
    DBG("### pet_sound_machine_init: alpha = %d\n", alpha);
    init_lowpass_table(alpha);
#else
    snd.alpha = calculate_alpha(speed, cb2_lowpass_freq, ALPHA_SCALE);
#endif /* LOWPASS_CB2 */
#if HIGHPASS
    snd.highpass_alpha = calculate_alpha(speed, 160, ALPHA_SCALE);
    snd.highpass_prev = 0;
    DBG("### pet_sound_machine_init: highpass alpha = %d\n", snd.highpass_alpha);
#endif

    pet_sound_chip.chip_enabled = true;

    return 1;
}

static void pet_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    DBG("### pet_sound_reset: cpu_clk %lu\n", cpu_clk);
    snd.first_sample_index = 0;
    snd.next_sample_index = 0;
    snd.next_sample_time = cpu_clk;
    snd.end_of_sample_time = snd.next_sample_time + snd.clocks_per_sample;
}

void petsound_reset(sound_t *psid, CLOCK cpu_clk)
{
    sound_reset();
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
