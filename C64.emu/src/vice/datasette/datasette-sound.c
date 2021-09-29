/*
 * datasette-sound.c - Sound of a cassette being played by the Datasette
 *
 * Written by
 *  Fabrizio Gennari <fabrizio.ge@tiscali.it>
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

#include "sound.h"
#include "maincpu.h"

static sound_chip_t datasette_sound;

static CLOCK gap_circular_buffer[200];
static const int gap_circular_buffer_size =
    sizeof(gap_circular_buffer) / sizeof(gap_circular_buffer[0]);
static int gap_circular_buffer_start = 0;
static int gap_circular_buffer_end = 0;
static char last_was_split_in_two = 0;
static CLOCK sound_start_maincpu_clk;
static char datasette_square_sign = 1;
static char datasette_halfwaves;


/* resources */
extern int datasette_sound_emulation;
extern int datasette_sound_emulation_volume;

static void datasette_sound_flush_circular_buffer(void)
{
    datasette_sound.chip_enabled = 0;
    gap_circular_buffer_end = gap_circular_buffer_start;
}

void datasette_sound_set_halfwaves(char halfwaves)
{
    datasette_halfwaves = halfwaves;
}

void datasette_sound_add_to_circular_buffer(CLOCK gap)
{
    if (!datasette_sound_emulation) {
        datasette_sound_flush_circular_buffer();
        return;
    }
    gap_circular_buffer[gap_circular_buffer_end] = gap;
    if (datasette_sound.chip_enabled == 0) {
        sound_start_maincpu_clk = maincpu_clk;
        datasette_sound.chip_enabled = 1;
    }
    gap_circular_buffer_end =
        (gap_circular_buffer_end + 1) % gap_circular_buffer_size;
    if (gap_circular_buffer_end == gap_circular_buffer_start) {
        gap_circular_buffer_start =
            (gap_circular_buffer_start + 1) % gap_circular_buffer_size;
    }
}

static CLOCK datasette_sound_remove_from_circular_buffer(
    CLOCK max_amount_to_remove, char divide_by_two, char *must_flip)
{
    CLOCK gap = 0;
    *must_flip = 0;
    if (gap_circular_buffer_end != gap_circular_buffer_start) {
        char try_again;
        do {
            try_again = 0;
            gap = gap_circular_buffer[gap_circular_buffer_start];
            if (divide_by_two && !last_was_split_in_two) {
                gap /= 2;
            }
            if (gap > max_amount_to_remove) {
                if (divide_by_two && !last_was_split_in_two) {
                    if (gap_circular_buffer_start == ((gap_circular_buffer_end + 1)
                        % gap_circular_buffer_size)) {
                        gap_circular_buffer_start = (gap_circular_buffer_start + 1)
                            % gap_circular_buffer_size;
                        try_again = 1;
                        continue;
                    }
                    gap_circular_buffer[gap_circular_buffer_start] -= gap;
                    gap_circular_buffer_start = gap_circular_buffer_start
                        ? gap_circular_buffer_start - 1
                        : gap_circular_buffer_size - 1;
                    gap_circular_buffer[gap_circular_buffer_start] = gap;
                    last_was_split_in_two = 1;
                }
                gap = max_amount_to_remove;
            } else {
                *must_flip = 1;
                last_was_split_in_two = 0;
            }
        } while (try_again);
        gap_circular_buffer[gap_circular_buffer_start] -= gap;
        if (!gap_circular_buffer[gap_circular_buffer_start]) {
            gap_circular_buffer_start = (gap_circular_buffer_start + 1)
                % gap_circular_buffer_size;
            if (gap_circular_buffer_end == gap_circular_buffer_start) {
                datasette_sound.chip_enabled = 0;
            }
        }
    }
    return gap;
}

static int datasette_sound_machine_calculate_samples(sound_t **psid,
    int16_t *pbuf, int nr, int soc, int scc, int *delta_t)
{
    int i = 0, j, num_samples;
    int cycles_to_be_consumed = *delta_t;
    double factor = (double)cycles_to_be_consumed / nr;
    char must_flip;

    if (sound_start_maincpu_clk) {
        int initial_zero_samples =
            (cycles_to_be_consumed + sound_start_maincpu_clk - maincpu_clk) / factor;
        while (i < initial_zero_samples) {
            pbuf[i++] = 0;
        }
        cycles_to_be_consumed = maincpu_clk - sound_start_maincpu_clk;
        sound_start_maincpu_clk = 0;
    }
    while (cycles_to_be_consumed) {
        CLOCK max_amount_to_consume = cycles_to_be_consumed;
        CLOCK cycles_to_consume_now =
            datasette_sound_remove_from_circular_buffer(max_amount_to_consume,
                datasette_square_sign == 1 && !datasette_halfwaves, &must_flip);
        if (!cycles_to_consume_now) {
            break;
        }
        cycles_to_be_consumed -= cycles_to_consume_now;
        if (i < nr) {
            if (cycles_to_be_consumed == 0) {
                num_samples = nr - i;
            } else {
                num_samples = cycles_to_consume_now / factor;
                if (i + num_samples < nr - 1
                    && cycles_to_be_consumed * 1.0 / (nr-i-num_samples) < factor) {
                    num_samples++;
                }
            }
            for (j = 0; j < num_samples; j++) {
                pbuf[i++] =
                    datasette_sound_emulation_volume * datasette_square_sign;
            }
        }
        if (must_flip)
            datasette_square_sign = -datasette_square_sign;
    }
    while (i < nr) {
        pbuf[i++] = 0;
    }
    return nr;
}

static int datasette_sound_machine_cycle_based(void)
{
    return 0;
}

static int datasette_sound_machine_channels(void)
{
    return 1;
}

/* Drive sound 'chip', emulates the sound of a 1541 disk drive */
static sound_chip_t datasette_sound = {
    NULL,                                      /* NO sound chip open function */ 
    NULL,                                      /* NO sound chip init function */
    NULL,                                      /* NO sound chip close function */
    datasette_sound_machine_calculate_samples, /* sound chip calculate samples function */
    NULL,                                      /* NO sound chip store function */
    NULL,                                      /* NO sound chip read function */
    NULL,                                      /* NO sound chip reset function */
    datasette_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, chip is NOT cycle based */
    datasette_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, sound chip has 1 channel */
    0                                          /* sound chip enabled flag, toggled upon device (de-)activation */
};

void datasette_sound_init(void)
{
    sound_chip_register(&datasette_sound);
}
