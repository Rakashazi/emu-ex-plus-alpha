/*
 * datasette.c - CBM cassette implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

/* #define DEBUG_TAPE */

#include "vice.h"

#include <stdio.h>
#include <math.h>

#include "alarm.h"
#include "autostart.h"
#include "cmdline.h"
#include "datasette.h"
#include "datasette-sound.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "network.h"
#include "resources.h"
#include "snapshot.h"
#include "tap.h"
#include "tape.h"
#include "tape-snapshot.h"
#include "tapeport.h"
#include "types.h"
#include "uiapi.h"
#include "vice-event.h"

#ifdef DEBUG_TAPE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#define MOTOR_DELAY         32000   /* for PLAY and RECORD */
#define MOTOR_DELAY_FAST     1000   /* for fast forward/reverse */
#define TAP_BUFFER_LENGTH   100000

/* at least every DATASETTE_MAX_GAP cycle there should be an alarm */
#define DATASETTE_MAX_GAP   100000


/* Attached TAP tape image.  */
static tap_t *current_image[TAPEPORT_MAX_PORTS];

/* Buffer for the TAP */
static uint8_t tap_buffer[TAPEPORT_MAX_PORTS][TAP_BUFFER_LENGTH];

/* Pointer and length of the tap-buffer */
static long next_tap[TAPEPORT_MAX_PORTS], last_tap[TAPEPORT_MAX_PORTS];

/* State of the datasette motor.  */
static int datasette_motor[TAPEPORT_MAX_PORTS];

/* Last time we have recorded a flux change.  */
static CLOCK last_write_clk[TAPEPORT_MAX_PORTS];

/* last state of the write line */
static int last_write_bit[TAPEPORT_MAX_PORTS] = { -1, -1 };

/* Motor stop is delayed.  */
static CLOCK motor_stop_clk[TAPEPORT_MAX_PORTS];

static alarm_t *datasette_alarm[TAPEPORT_MAX_PORTS];

static int datasette_alarm_pending[TAPEPORT_MAX_PORTS];

static CLOCK datasette_long_gap_pending[TAPEPORT_MAX_PORTS];

static CLOCK datasette_long_gap_elapsed[TAPEPORT_MAX_PORTS];

static int datasette_last_direction[TAPEPORT_MAX_PORTS];

static long datasette_cycles_per_second;

/* Remember the reset of tape-counter.  */
static int datasette_counter_offset[TAPEPORT_MAX_PORTS];

/* app_resource datasette */
/* shall the datasette reset when the CPU does? */
static int reset_datasette_with_maincpu;

/* how long to wait, if a zero occurs in the tap ? */
static int datasette_zero_gap_delay;

/* finetuning for speed of motor */
static int datasette_speed_tuning;

/* status when no tape image is present */
static int notape_mode[TAPEPORT_MAX_PORTS] = { DATASETTE_CONTROL_STOP,
                                               DATASETTE_CONTROL_STOP };

/* Low/high wave indicator for C16 TAPs. */
static unsigned int fullwave[TAPEPORT_MAX_PORTS];
static CLOCK fullwave_gap[TAPEPORT_MAX_PORTS];

/* tape wobble parameters */
static int datasette_tape_wobble_amplitude;
static int datasette_tape_wobble_frequency;
/* amount of random azimuth error */
static int datasette_tape_azimuth_error;

/* datasette device enable */
static int datasette_enabled[TAPEPORT_MAX_PORTS] = { 0, 0 };

/* audible sound from datasette device */
int datasette_sound_emulation = 1;

/* volume of sound from datasette device */
int datasette_sound_emulation_volume;

static log_t datasette_log = LOG_ERR;

static void datasette_internal_reset(int port);
static void datasette_event_record(int command);
static void datasette_control_internal(int port, int command);

static void datasette_set_motor(int port, int flag);
static void datasette_toggle_write_bit(int port, int write_bit);

static int datasette_write_snapshot(int port, snapshot_t *s, int write_image);
static int datasette_read_snapshot(int port, snapshot_t *s);

static int datasette_enable(int port, int val);

static tapeport_device_t datasette_device = {
    "Datasette",                /* device name */
    TAPEPORT_DEVICE_TYPE_TAPE,  /* device is a 'tape' type of device */
    VICE_MACHINE_ALL,           /* device works on all machines */
    TAPEPORT_PORT_ALL_MASK,     /* device works on all ports */
    datasette_enable,           /* device enable function */
    NULL,                       /* NO device specific reset function */
    NULL,                       /* NO device shutdown function */
    datasette_set_motor,        /* set motor line function */
    datasette_toggle_write_bit, /* set write line function */
    NULL,                       /* NO set sense line function */
    NULL,                       /* NO set read line function */
    datasette_write_snapshot,   /* device snapshot write function */
    datasette_read_snapshot     /* device snapshot read function */
};

void datasette_set_tape_sense(int port, int sense)
{
    if (datasette_enabled[port]) {
        tapeport_set_tape_sense(sense, port);
    }
}

/*******************************************************************************
    Resources
 ******************************************************************************/

static int set_reset_datasette_with_maincpu(int val, void *param)
{
    reset_datasette_with_maincpu = val ? 1 : 0;

    return 0;
}

static int set_datasette_zero_gap_delay(int val, void *param)
{
    if ((val < 0) || (val > TAP_ZERO_GAP_DELAY_MAX)) {
        return -1;
    }
    datasette_zero_gap_delay = val;

    return 0;
}

static int set_datasette_speed_tuning(int val, void *param)
{
    if ((val < -TAP_SPEED_TUNING_MAX) || (val > TAP_SPEED_TUNING_MAX)) {
        return -1;
    }

    datasette_speed_tuning = val;

    return 0;
}

static int set_datasette_tape_wobble_frequency(int val, void *param)
{
    if ((val < 0) || (val > TAP_WOBBLE_FREQ_MAX)) {
        return -1;
    }

    datasette_tape_wobble_frequency = val;

    return 0;
}

static int set_datasette_tape_wobble_amplitude(int val, void *param)
{
    if ((val < 0) || (val > TAP_WOBBLE_AMPLITUDE_MAX)) {
        return -1;
    }

    datasette_tape_wobble_amplitude = val;

    return 0;
}

static int set_datasette_tape_azimuth_error(int val, void *param)
{
    if ((val < 0) || (val > TAP_AZIMUTH_ERROR_MAX)) {
        return -1;
    }

    datasette_tape_azimuth_error = val;

    return 0;
}

static int datasette_enable(int port, int value)
{
    int val = value ? 1 : 0;

    DBG(("set_datasette_enable: %d", value));

    datasette_enabled[port] = val;

    return 0;
}

static int set_datasette_sound_emulation(int val, void *param)
{
    datasette_sound_emulation = val ? 1 : 0;

    return 0;
}

static int set_datasette_sound_emulation_volume(int val, void *param)
{
    if ((val < 0) || (val > TAPE_SOUND_VOLUME_MAX)) {
        return -1;
    }

    datasette_sound_emulation_volume = val;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DatasetteResetWithCPU", 1, RES_EVENT_SAME, NULL,
      &reset_datasette_with_maincpu,
      set_reset_datasette_with_maincpu, NULL },
    /* mtap uses 2500, so we use the same */
    { "DatasetteZeroGapDelay", TAP_ZERO_GAP_DELAY_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_zero_gap_delay,
      set_datasette_zero_gap_delay, NULL },
    { "DatasetteSpeedTuning", TAP_SPEED_TUNING_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_speed_tuning,
      set_datasette_speed_tuning, NULL },
    { "DatasetteTapeWobbleFrequency", TAP_WOBBLE_FREQ_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_tape_wobble_frequency,
      set_datasette_tape_wobble_frequency, NULL },
    { "DatasetteTapeWobbleAmplitude", TAP_WOBBLE_AMPLITUDE_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_tape_wobble_amplitude,
      set_datasette_tape_wobble_amplitude, NULL },
    { "DatasetteTapeAzimuthError", TAP_AZIMUTH_ERROR_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_tape_azimuth_error,
      set_datasette_tape_azimuth_error, NULL },
    { "DatasetteSound", 0, RES_EVENT_SAME, NULL,
      &datasette_sound_emulation,
      set_datasette_sound_emulation, NULL },
    { "DatasetteSoundVolume", TAPE_SOUND_VOLUME_DEFAULT, RES_EVENT_SAME, NULL,
      &datasette_sound_emulation_volume,
      set_datasette_sound_emulation_volume, NULL },
    RESOURCE_INT_LIST_END
};

int datasette_resources_init(int amount)
{
    if (tapeport_device_register(TAPEPORT_DEVICE_DATASETTE, &datasette_device) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*******************************************************************************
    Commandline options
 ******************************************************************************/

static const cmdline_option_t cmdline_options[] =
{
    { "-dsresetwithcpu", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DatasetteResetWithCPU", (resource_value_t)1,
      NULL, "Enable automatic Datasette-Reset" },
    { "+dsresetwithcpu", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DatasetteResetWithCPU", (resource_value_t)0,
      NULL, "Disable automatic Datasette-Reset" },
    { "-dszerogapdelay", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteZeroGapDelay", NULL,
      "<value>", "Set delay in cycles for a zero in a v0 tap file" },
    { "-dsspeedtuning", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteSpeedTuning", NULL,
      "<value>", "Set constant deviation from correct motor speed" },
    { "-dstapewobblefreq", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteTapeWobbleFrequency", NULL,
      "<value>", "Set tape wobble frequency" },
    { "-dstapewobbleamp", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteTapeWobbleAmplitude", NULL,
      "<value>", "Set tape wobble amplitude" },
    { "-dstapeerror", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteTapeAzimuthError", NULL,
      "<value>", "Set amount of azimuth error (misalignment)" },
    { "-datasettesound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DatasetteSound", (resource_value_t)1,
      NULL, "Enable Datasette sound" },
    { "+datasettesound", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "DatasetteSound", (resource_value_t)0,
      NULL, "Disable Datasette sound" },
    { "-dssoundvolume", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "DatasetteSoundVolume", NULL,
      "<value>", "Set volume of Datasette sound" },
    CMDLINE_LIST_END
};

int datasette_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/*---------------------------------------------------------------------*/

/* constants to make the counter-calculation a little faster */
/* see datasette.h for the complete formular                 */
static const double ds_c1 = DS_V_PLAY / DS_D / PI;
static const double ds_c2 = (DS_R * DS_R) / (DS_D * DS_D);
static const double ds_c3 = DS_R / DS_D;

static void datasette_update_ui_counter(int port)
{
    if (current_image[port] == NULL) {
        /* FIXME: this is not quite correct, on a real datasette the counter
                  would also count when no tape is inserted */
        ui_display_tape_counter(port, 1000 - datasette_counter_offset[port]);
    } else {
        current_image[port]->counter = (1000 - datasette_counter_offset[port] +
                                (int) (DS_G *
                                        (sqrt((current_image[port]->cycle_counter
                                                / (datasette_cycles_per_second / 8.0)
                                                * ds_c1) + ds_c2) - ds_c3))) % 1000;
        ui_display_tape_counter(port, current_image[port]->counter);
    }
}


void datasette_reset_counter(int port)
{
    if (current_image[port] == NULL) {
        datasette_counter_offset[port] = (1000);
    } else {
        datasette_counter_offset[port] = (1000 + (int) (DS_G *
                                                (sqrt((current_image[port]->cycle_counter
                                                        / (datasette_cycles_per_second / 8.0)
                                                        * ds_c1) + ds_c2) - ds_c3))) % 1000;
    }
    datasette_update_ui_counter(port);
}


inline static int datasette_move_buffer_forward(int port, int offset)
{
    /* reads buffer to fit the next gap-read
       tap_buffer[port][next_tap[port]] ~ current_file_seek_position
    */
    if (next_tap[port] + offset >= last_tap[port]) {
        if (fseek(current_image[port]->fd, current_image[port]->current_file_seek_position
                  + current_image[port]->offset, SEEK_SET)) {
            log_error(datasette_log, "Cannot read in tap-file.");
            return 0;
        }
        last_tap[port] = (long)fread(tap_buffer[port], 1, TAP_BUFFER_LENGTH, current_image[port]->fd);
        next_tap[port] = 0;
        if (next_tap[port] >= last_tap[port]) {
            return 0;
        }
    }
    return 1;
}

inline static int datasette_move_buffer_back(int port, int offset)
{
    /* reads buffer to fit the next gap-read at current_file_seek_position-1
       tap_buffer[port][next_tap[port]] ~ current_file_seek_position
    */
    if (next_tap[port] + offset < 0) {
        if (current_image[port]->current_file_seek_position >= TAP_BUFFER_LENGTH) {
            next_tap[port] = TAP_BUFFER_LENGTH;
        } else {
            next_tap[port] = current_image[port]->current_file_seek_position;
        }
        if (fseek(current_image[port]->fd, current_image[port]->current_file_seek_position
                  - next_tap[port] + current_image[port]->offset, SEEK_SET)) {
            log_error(datasette_log, "Cannot read in tap-file.");
            return 0;
        }
        last_tap[port] = (long)fread(tap_buffer[port], 1, TAP_BUFFER_LENGTH, current_image[port]->fd);
        if (next_tap[port] > last_tap[port]) {
            return 0;
        }
    }
    return 1;
}

/* calculate tape wobble, add speed tuning */
static CLOCK tape_do_wobble(int port, CLOCK gap)
{
    /* cpu cycles since last call */
    static float wobble_sin_count;
    float wobble_factor;
    CLOCK cpu_cycles = current_image[port]->cycle_counter_total;
    signed long newgap;
    float newgapf;
    static float restf = 0.0f;
    float amplitude;
    float tuning;
    float freq;

    if (((datasette_tape_wobble_frequency == 0) || (datasette_tape_wobble_amplitude == 0)) &&
        (datasette_speed_tuning == 0)) {
        return gap;
    }

    /* convert resource values to floats */
    freq = (float)datasette_tape_wobble_frequency / (float)TAP_WOBBLE_FREQ_ONE;
    amplitude = (float)datasette_tape_wobble_amplitude / (float)TAP_WOBBLE_AMPLITUDE_ONE;
    tuning = (float)datasette_speed_tuning / (float)TAP_SPEED_TUNING_ONE;

    wobble_sin_count += freq * (((uint64_t)cpu_cycles / ((float)datasette_cycles_per_second / 1000000.0f)) / (10000000000.0f * (2.0f * M_PI)));

    if (wobble_sin_count > (2 * M_PI)) {
        wobble_sin_count -= (2 * M_PI);
    }

    wobble_factor = 1.0f + (sinf(wobble_sin_count) * amplitude) + tuning;

    newgapf = restf + (wobble_factor * gap);
    newgap = (int)(newgapf + 0.5f);

    if (newgap < 1) {
        newgap = 1;
    }
    restf = newgapf - newgap;
#if 0
    printf("gap: %4d factor: % 1.4f newgapf: % 3.2f rest: % 4.2f newgap: %4d cycles: %8d sincnt: % 2.2f  freq: % 2.2f amplitude:% 2.2f tuning:% 2.2f\r",
           (int)gap, (float)wobble_factor, (float)newgapf, (float)restf, (int)newgap,
           (int)cpu_cycles, (float)wobble_sin_count, (float)freq, amplitude, tuning);
#endif
    return (CLOCK)newgap;
}

static CLOCK tape_do_misalignment(CLOCK gap)
{
    static CLOCK resterror = 0;
    CLOCK newgap, newgapf, tapeerror;

    if (datasette_tape_azimuth_error == 0) {
        return gap;
    }

    tapeerror = lib_unsigned_rand(-datasette_tape_azimuth_error, datasette_tape_azimuth_error);
    newgapf = (gap * TAP_AZIMUTH_ERROR_ONE) + tapeerror + resterror;
    newgap = (newgapf + 500) / TAP_AZIMUTH_ERROR_ONE;
    if (newgap < 1) {
        newgap = 1;
    }
    resterror = newgapf - (newgap * TAP_AZIMUTH_ERROR_ONE);
#if 0
    printf("gap: %4d tapeerror: % 4d resterror: % 4d newgap: %4d\n", (int)gap, tapeerror, resterror, newgap);
#endif
    return newgap;
}

inline static int fetch_gap(int port, CLOCK *gap, int *direction, long read_tap)
{
    if ((read_tap >= last_tap[port]) || (read_tap < 0)) {
        return -1;
    }

    *gap = tap_buffer[port][read_tap];

    if ((current_image[port]->version == 0) || *gap) {
        /* in v0 tap files gaps > 255 produced an overflow and generally
           produced 0 - which needs to be reinterpreted as a "long" gap.
           "mtap" by Marcus Brenner, which probably the majority of tap v0 files
           are created with, uses 2500.
           We also add a constant number of cycles (default:1) to compensate
           for tape speed variations. */
        *gap = (*gap ? (CLOCK)(*gap * 8) : (CLOCK)datasette_zero_gap_delay);
    } else {
        if (read_tap >= last_tap[port] - 3) {
            return -1;
        }
        *direction *= 4;
        *gap = tap_buffer[port][read_tap + 1]
               + (tap_buffer[port][read_tap + 2] << 8)
               + (tap_buffer[port][read_tap + 3] << 16);
        if (!(*gap)) {
            *gap = (CLOCK)datasette_zero_gap_delay;
        }
    }

    *gap = tape_do_wobble(port, *gap);
    *gap = tape_do_misalignment(*gap);
    return 0;
}

inline static void read_gap_forward(int port, long *read_tap)
{
    *read_tap = next_tap[port];
}

inline static void read_gap_backward_v0(int port, long *read_tap)
{
    *read_tap = next_tap[port] - 1;
}

inline static int read_gap_backward_v1(int port, long *read_tap)
{
    /* examine, if previous gap was long
       by rewinding until 3 non-zero-values
       in a row found, then reading forward (FIXME???)
    */
    int non_zeros_in_a_row = 0;
    long remember_file_seek_position;

    remember_file_seek_position = current_image[port]->current_file_seek_position;

    current_image[port]->current_file_seek_position -= 4;
    next_tap[port] -= 4;

    while ((non_zeros_in_a_row < 3) && current_image[port]->current_file_seek_position) {
        if (!datasette_move_buffer_back(port, -1)) {
            return 1;
        }
        current_image[port]->current_file_seek_position--;
        next_tap[port]--;
        if (tap_buffer[port][next_tap[port]]) {
            non_zeros_in_a_row++;
        } else {
            non_zeros_in_a_row = 0;
        }
    }

    /* now forward */
    while (current_image[port]->current_file_seek_position < remember_file_seek_position - 4) {
        if (!datasette_move_buffer_forward(port, 1)) {
            return -1;
        }
        if (tap_buffer[port][next_tap[port]]) {
            current_image[port]->current_file_seek_position++;
            next_tap[port]++;
        } else {
            current_image[port]->current_file_seek_position += 4;
            next_tap[port] += 4;
        }
    }
    if (!datasette_move_buffer_forward(port, 4)) {
        return -1;
    }

    *read_tap = next_tap[port];
    next_tap[port] += (remember_file_seek_position - current_image[port]->current_file_seek_position);
    current_image[port]->current_file_seek_position = (int)remember_file_seek_position;

    return 0;
}

static CLOCK datasette_read_gap(int port, int direction)
{
    /* direction 1: forward, -1: rewind */
    long read_tap = 0;
    CLOCK gap = 0;

/*    if (current_image[port]->system != 2 || current_image[port]->version != 1
        || !fullwave[port]) {*/
    if (machine_tape_behaviour() != TAPE_BEHAVIOUR_C16) {
        /* regular tape behaviour */
        if ((direction < 0) && !datasette_move_buffer_back(port, direction * 4)) {
            return 0;
        }
        if ((direction > 0 ) && !datasette_move_buffer_forward(port, direction * 4)) {
            return 0;
        }

        if (direction > 0) {
            read_gap_forward(port, &read_tap);
        } else {
            if ((current_image[port]->version == 0) || (next_tap[port] < 4) || tap_buffer[port][next_tap[port] - 4]) {
                read_gap_backward_v0(port, &read_tap);
            } else {
                if (read_gap_backward_v1(port, &read_tap) < 0) {
                    return 0;
                }
            }
        }
        if (fetch_gap(port, &gap, &direction, read_tap) < 0) {
            return 0;
        }
        next_tap[port] += direction;
        current_image[port]->current_file_seek_position += direction;
    } else if (current_image[port]->version == 1) {
        /* C16 v1 behaviour */
        if (!fullwave[port]) {
            if ((direction < 0) && !datasette_move_buffer_back(port, direction * 4)) {
                return 0;
            }
            if ((direction > 0 ) && !datasette_move_buffer_forward(port, direction * 4)) {
                return 0;
            }

            if (direction > 0) {
                read_gap_forward(port, &read_tap);
            } else {
                if ((current_image[port]->version == 0) || (next_tap[port] < 4) || tap_buffer[port][next_tap[port] - 4]) {
                    read_gap_backward_v0(port, &read_tap);
                } else {
                    if (read_gap_backward_v1(port, &read_tap) < 0) {
                        return 0;
                    }
                }
            }
            if (fetch_gap(port, &gap, &direction, read_tap) < 0) {
                return 0;
            }

            fullwave_gap[port] = gap;
            next_tap[port] += direction;
            current_image[port]->current_file_seek_position += direction;
        } else {
            gap = fullwave_gap[port];
        }
        fullwave[port] ^= 1;
    } else if (current_image[port]->version == 2) {
        /* C16 v2 behaviour */
        if ((direction < 0) && !datasette_move_buffer_back(port, direction * 4)) {
            return 0;
        }
        if ((direction > 0 ) && !datasette_move_buffer_forward(port, direction * 4)) {
            return 0;
        }

        if (direction > 0) {
            read_gap_forward(port, &read_tap);
        } else {
            if ((current_image[port]->version == 0) || (next_tap[port] < 4) || tap_buffer[port][next_tap[port] - 4]) {
                read_gap_backward_v0(port, &read_tap);
            } else {
                if (read_gap_backward_v1(port, &read_tap) < 0) {
                    return 0;
                }
            }
        }
        if (fetch_gap(port, &gap, &direction, read_tap) < 0) {
            return 0;
        }
        gap *= 2;
        fullwave[port] ^= 1;
        next_tap[port] += direction;
        current_image[port]->current_file_seek_position += direction;
    }
    return gap;
}

static void datasette_alarm_set(int port, CLOCK offset)
{
#ifdef DEBUG_TAPE
    if (!datasette_alarm_pending[port]) {
        log_debug("datasette_alarm_set: %"PRIu64"", offset);
    } else {
        log_debug("datasette_alarm_set: %"PRIu64" (WARNING: another alarm was pending!)", offset);
    }
#endif
    alarm_set(datasette_alarm[port], offset);
    datasette_alarm_pending[port] = 1;
}

static void datasette_alarm_unset(int port)
{
    alarm_unset(datasette_alarm[port]);
    datasette_alarm_pending[port] = 0;
}

/* FIXME: the .tap header specifies the system and video system of the computer
          the .tap was captured with, which implicitly tells the clockrate a
          "tap byte" refers to. this is no problem as long as the .tap is played
          on the same system it was created with - however when eg a PAL tap is
          played back on a NTSC machine, the pulses must be scaled accordingly
          - which is not happening right now. */

/* this is the alarm function */
static void datasette_read_bit(CLOCK offset, void *data)
{
    double speed_of_tape = DS_V_PLAY;
    int direction = 1;
    long gap;
    int port = vice_ptr_to_int(data);

    datasette_alarm_unset(port);

    DBG(("datasette_read_bit(motor:%d) maincpu_clk: 0x%"PRIx64" motor_stop_clk: 0x%"PRIx64" (image present:%s)",
         datasette_motor[port], maincpu_clk, motor_stop_clk[port], current_image[port] ? "yes" : "no"));

    /* check for delay of motor stop */
    if (motor_stop_clk[port] > 0) {
        if (maincpu_clk >= motor_stop_clk[port]) {
            motor_stop_clk[port] = 0;
            ui_display_tape_motor_status(port, 0);
            datasette_motor[port] = 0;
        } else {
            /* we cleared the alarm above, setup a new one further into the
               future that will trigger the motor stop */
            datasette_alarm_set(port, motor_stop_clk[port]);
        }
    }
    DBG(("datasette_read_bit(motor:%d)", datasette_motor[port]));

    if (!datasette_motor[port]) {
        return;
    }

    /* there is no image attached */
    if (current_image[port] == NULL) {
        switch (notape_mode[port]) {
            case DATASETTE_CONTROL_START:
            case DATASETTE_CONTROL_FORWARD:
            case DATASETTE_CONTROL_REWIND:
            case DATASETTE_CONTROL_RECORD:
                break;
            case DATASETTE_CONTROL_STOP:
                if (motor_stop_clk[port] > 0) {
                    datasette_alarm_set(port, motor_stop_clk[port]);
                }
                break;
        }
        datasette_update_ui_counter(port);
        return;
    }

    /* an image is attached */
    switch (current_image[port]->mode) {
        case DATASETTE_CONTROL_START:
            direction = 1;
            speed_of_tape = DS_V_PLAY;
            if (!datasette_long_gap_pending[port]) {
                if (datasette_enabled[port]) {
                    tapeport_trigger_flux_change(fullwave[port], port);
                }
            }
            break;
        case DATASETTE_CONTROL_FORWARD:
            direction = 1;
            speed_of_tape = DS_RPS_FAST / DS_G
                            * sqrt(4 * PI * DS_D
                                   * DS_V_PLAY / datasette_cycles_per_second * 8
                                   * current_image[port]->cycle_counter
                                   + 4 * PI * PI * DS_R * DS_R);
            break;
        case DATASETTE_CONTROL_REWIND:
            direction = -1;
            speed_of_tape = DS_RPS_FAST / DS_G
                            * sqrt(4 * PI * DS_D
                                   * DS_V_PLAY / datasette_cycles_per_second * 8
                                   * (current_image[port]->cycle_counter_total
                                      - current_image[port]->cycle_counter)
                                   + 4 * PI * PI * DS_R * DS_R);
            break;
        case DATASETTE_CONTROL_RECORD:
        case DATASETTE_CONTROL_STOP:
            return;
        default:
            log_error(datasette_log, "Unknown datasette mode.");
            return;
    }

    if (direction + datasette_last_direction[port] == 0) {
        /* the direction changed; read the gap from file,
        but use only the elapsed gap */
        gap = datasette_read_gap(port, direction);
        datasette_long_gap_pending[port] = datasette_long_gap_elapsed[port];
        datasette_long_gap_elapsed[port] = (CLOCK)(gap - datasette_long_gap_elapsed[port]);
    }
    if (datasette_long_gap_pending[port]) {
        gap = datasette_long_gap_pending[port];
        datasette_long_gap_pending[port] = 0;
    } else {
        gap = datasette_read_gap(port, direction);
        if (gap) {
            datasette_long_gap_elapsed[port] = 0;
        }
    }
    if (!gap) {
        datasette_control(port, DATASETTE_CONTROL_STOP);
        return;
    }
    if (gap > DATASETTE_MAX_GAP) {
        datasette_long_gap_pending[port] = (CLOCK)(gap - DATASETTE_MAX_GAP);
        gap = DATASETTE_MAX_GAP;
    }
    datasette_long_gap_elapsed[port] += gap;
    datasette_last_direction[port] = direction;

    if (direction > 0) {
        current_image[port]->cycle_counter += gap / 8;
    } else {
        current_image[port]->cycle_counter -= gap / 8;
    }

    if (current_image[port]->mode == DATASETTE_CONTROL_START) {
        datasette_sound_add_to_circular_buffer((CLOCK)gap);
    }

    gap -= offset;

    if (gap > 0) {
        datasette_alarm_set(port, maincpu_clk + (CLOCK)(gap * (DS_V_PLAY / speed_of_tape)));
    } else {
        /* If the offset is geater than the gap to the next flux
           change, the change happend during DMA.  Schedule it now.  */
        datasette_alarm_set(port, maincpu_clk);
    }
    datasette_update_ui_counter(port);
}

void datasette_init(void)
{
    int i;

    DBG(("datasette_init"));
    datasette_log = log_open("Datasette");

    for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
        datasette_alarm[i] = alarm_new(maincpu_alarm_context, "Datasette",
                                       datasette_read_bit, int_to_void_ptr(i));
    }

    datasette_cycles_per_second = machine_get_cycles_per_second();
    if (!datasette_cycles_per_second) {
        log_error(datasette_log,
                  "Cannot get cycles per second for this machine.");
        datasette_cycles_per_second = 985248;
    }

    for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
        datasette_set_tape_image(i, NULL);
    }
}

void datasette_set_tape_image(int port, tap_t *image)
{
    CLOCK gap;

    DBG(("datasette_set_tape_image (image present:%s)", image ? "yes" : "no"));

    current_image[port] = image;
    last_tap[port] = next_tap[port] = 0;
    datasette_internal_reset(port);

    if (image != NULL) {
        /* We need the length of tape for realistic counter. */
        current_image[port]->cycle_counter_total = 0;
        do {
            gap = datasette_read_gap(port, 1);
            current_image[port]->cycle_counter_total += gap / 8;
        } while (gap);
        current_image[port]->current_file_seek_position = 0;
        datasette_sound_set_halfwaves(current_image[port]->version == 2);
    }
    if (datasette_enabled[port]) {
        tapeport_set_tape_sense(0, port);
    }

    last_tap[port] = next_tap[port] = 0;
    fullwave[port] = 0;

    ui_set_tape_status(port, current_image[port] ? 1 : 0);
}


static void datasette_forward(int port)
{
    int mode = current_image[port] ? current_image[port]->mode : notape_mode[port];

    DBG(("datasette_forward"));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_REWIND) {
        datasette_alarm_unset(port);
    }
    datasette_alarm_set(port, maincpu_clk + MOTOR_DELAY_FAST);
}

static void datasette_rewind(int port)
{
    int mode = current_image[port] ? current_image[port]->mode : notape_mode[port];

    DBG(("datasette_rewind"));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_FORWARD) {
        datasette_alarm_unset(port);
    }
    datasette_alarm_set(port, maincpu_clk + MOTOR_DELAY_FAST);
}


static void datasette_internal_reset(int port)
{
    int mode = current_image[port] ? current_image[port]->mode : notape_mode[port];

    if (!tapeport_valid_port(port)) {
        return;
    }

    DBG(("datasette_internal_reset (mode:%d)", mode));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_FORWARD ||
        mode == DATASETTE_CONTROL_REWIND) {
        datasette_alarm_unset(port);
    }
    datasette_control(port, current_image[port] ? DATASETTE_CONTROL_STOP : notape_mode[port]);
    if (current_image[port] != NULL) {
        if (!autostart_ignore_reset) {
            tap_seek_start(current_image[port]);
        }
        current_image[port]->cycle_counter = 0;
    }
    datasette_counter_offset[port] = 0;
    datasette_long_gap_pending[port] = 0;
    datasette_long_gap_elapsed[port] = 0;
    datasette_last_direction[port] = 0;
    motor_stop_clk[port] = 0;
    fullwave[port] = 0;
    /* update the UI */
    datasette_update_ui_counter(port);
    ui_display_tape_motor_status(port, 0);
    ui_display_reset(port + 1, 0);
}

void datasette_reset(void)
{
    int i;
    int ds_reset = 0;
    DBG(("datasette_reset"));
    resources_get_int("DatasetteResetWithCPU", &ds_reset);

    if (ds_reset) {
        for (i = 0; i < TAPEPORT_MAX_PORTS; i++) {
            datasette_internal_reset(i);
        }
    }
}

static void datasette_start_motor(int port)
{
    DBG(("datasette_start_motor (image present:%s)", current_image[port] ? "yes" : "no"));
    if (current_image[port]) {
        fseek(current_image[port]->fd, current_image[port]->current_file_seek_position + current_image[port]->offset, SEEK_SET);
    }
    if (!datasette_alarm_pending[port]) {
        datasette_alarm_set(port, maincpu_clk + MOTOR_DELAY);
    }

    if (last_write_clk[port] == (CLOCK)0) {
        /* FIXME: the first pulse should start when the motor is actually running
                  (after the MOTOR_DELAY - however, due to how the delay is currently
                  implemented at motor start, this causes weird side effects) */
        DBG(("datasette_start_motor: first pulse starts at maincpu_clk: 0x%"PRIx64"", maincpu_clk));
        last_write_clk[port] = maincpu_clk;
        /* HACK: we start "as if" the write line was 0 when recording started, this
                 way the first written 1 will always produce a (likely "long") gap
                 at the start of the tape */
        last_write_bit[port] = 0;
    }
}

#ifdef DEBUG_TAPE
static char *cmdstr[8] = {
    "DATASETTE_CONTROL_STOP",
    "DATASETTE_CONTROL_START",
    "DATASETTE_CONTROL_FORWARD",
    "DATASETTE_CONTROL_REWIND",
    "DATASETTE_CONTROL_RECORD",
    "DATASETTE_CONTROL_RESET",
    "DATASETTE_CONTROL_RESET_COUNTER"
};
#endif

static void tap_reset_gap(int port)
{
    last_write_clk[port] = (CLOCK)0;
}

/* "press" the buttons on the c2n */
static void datasette_control_internal(int port, int command)
{
    DBG(("datasette_control_internal (%s) (image present:%s)", cmdstr[command], current_image[port] ? "yes" : "no"));
    if (current_image[port]) {
        switch (command) {
            case DATASETTE_CONTROL_RESET_COUNTER:
                datasette_reset_counter(port);
                break;
            case DATASETTE_CONTROL_RESET:
                datasette_internal_reset(port);
                /* falls through */
            case DATASETTE_CONTROL_STOP:
                current_image[port]->mode = DATASETTE_CONTROL_STOP;
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(0, port);
                }
                tap_reset_gap(port);
                break;
            case DATASETTE_CONTROL_START:
                current_image[port]->mode = DATASETTE_CONTROL_START;
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_FORWARD:
                current_image[port]->mode = DATASETTE_CONTROL_FORWARD;
                datasette_forward(port);
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_REWIND:
                current_image[port]->mode = DATASETTE_CONTROL_REWIND;
                datasette_rewind(port);
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_RECORD:
                if (current_image[port]->read_only == 0) {
                    current_image[port]->mode = DATASETTE_CONTROL_RECORD;
                    if (datasette_enabled[port]) {
                        tapeport_set_tape_sense(1, port);
                    }
                    tap_reset_gap(port);
                }
                break;
        }
        ui_display_tape_control_status(port, current_image[port]->mode);
    } else {
       switch (command) {
            case DATASETTE_CONTROL_RESET_COUNTER:
                datasette_reset_counter(port);
                break;
            case DATASETTE_CONTROL_RESET:
                datasette_internal_reset(port);
                /* falls through */
            case DATASETTE_CONTROL_STOP:
                notape_mode[port] = DATASETTE_CONTROL_STOP;
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(0, port);
                }
                tap_reset_gap(port);
                break;
            case DATASETTE_CONTROL_START:
                notape_mode[port] = DATASETTE_CONTROL_START;
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_FORWARD:
                notape_mode[port] = DATASETTE_CONTROL_FORWARD;
                datasette_forward(port);
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_REWIND:
                notape_mode[port] = DATASETTE_CONTROL_REWIND;
                datasette_rewind(port);
                if (datasette_enabled[port]) {
                    tapeport_set_tape_sense(1, port);
                }
                tap_reset_gap(port);
                if (datasette_motor[port]) {
                    datasette_start_motor(port);
                }
                break;
            case DATASETTE_CONTROL_RECORD:
                /* record can usually not be pressed when no tape is present */
                break;
        }
        ui_display_tape_control_status(port, notape_mode[port]);
    }
    /* clear the tap-buffer */
    last_tap[port] = next_tap[port] = 0;
}

void datasette_control(int port, int command)
{
    if (event_playback_active()) {
        return;
    }

    datasette_event_record(command);
    if (!network_connected()) {
        datasette_control_internal(port, command);
    }
}

/* "set motor line" function used by tapeport API */
static void datasette_set_motor(int port, int motor)
{
    DBG(("datasette_set_motor(%d) (image present:%s) datasette_motor: %d motor_stop_clk: 0x%"PRIx64"",
         motor, current_image[port] ? "yes" : "no", datasette_motor[port], motor_stop_clk[port]));

    if (datasette_alarm[port] == NULL) {
        DBG(("datasette_set_motor (datasette_alarm[port] == NULL)"));
        return;
    }

    if (motor) {
        /* abort pending motor stop */
        motor_stop_clk[port] = 0;
        DBG(("datasette_set_motor(motor=1 maincpu_clk: 0x%"PRIx64" motor_stop_clk: 0x%"PRIx64")", maincpu_clk, motor_stop_clk[port]));
        if (!datasette_motor[port]) {
            tap_reset_gap(port);
            datasette_start_motor(port);
            ui_display_tape_motor_status(port, 1);
            datasette_motor[port] = 1;
        } else {
            DBG(("datasette_set_motor() not starting motor"));
        }
    } else {
        if (datasette_motor[port] && motor_stop_clk[port] == 0) {
            motor_stop_clk[port] = maincpu_clk + MOTOR_DELAY;
            DBG(("datasette_set_motor(motor=0 maincpu_clk: 0x%"PRIx64" motor_stop_clk: 0x%"PRIx64")", maincpu_clk, motor_stop_clk[port]));
            if (!datasette_alarm_pending[port]) {
                /* make sure that the motor will stop */
                datasette_alarm_set(port, motor_stop_clk[port]);
            }
        } else {
            DBG(("datasette_set_motor() not stopping motor"));
        }
    }
}

/* FIXME: right now we always write v1 .tap files (falling edges only), even for xplus4 */

inline static void bit_write(int port)
{
    CLOCK write_time;
    uint8_t write_gap;

    write_time = maincpu_clk - last_write_clk[port];
    DBG(("bit_write last_write_clk: 0x%"PRIx64" maincpu_clk: 0x%"PRIx64" write_time: 0x%"PRIx64" tap: 0x%"PRIx64"",
         last_write_clk[port], maincpu_clk, write_time,
         (write_time / (CLOCK)8) > 0xff ? write_time : (write_time / (CLOCK)8)));
    last_write_clk[port] = maincpu_clk;

    /* C16 TAPs use half the machine clock as base cycle */
    if (machine_class == VICE_MACHINE_PLUS4) {
        write_time = write_time / 2;
    }

    if (write_time < (CLOCK)7) {
        return;
    }

    if (write_time < (CLOCK)(255 * 8 + 7)) {
        /* this is a normal short/one byte gap */
        write_gap = (uint8_t)(write_time / (CLOCK)8);
        if (fwrite(&write_gap, 1, 1, current_image[port]->fd) < 1) {
            log_error(datasette_log, "datasette bit_write failed (stopping tape).");
            datasette_control(port, DATASETTE_CONTROL_STOP);
            return;
        }
        DBG(("bit_write v0 value 0x%02x at position 0x%04x",
             write_gap, (unsigned int)current_image[port]->current_file_seek_position));
        current_image[port]->current_file_seek_position++;
    } else {
        /* this is a long gap, v0 tap only stores a zero for this, in v1 the zero
           is followed by the exact length - so write the zero first */
        write_gap = 0;
        if (fwrite(&write_gap, 1, 1, current_image[port]->fd) != 1) {
            log_error(datasette_log, "datasette bit_write failed (stopping tape).");
            datasette_control(port, DATASETTE_CONTROL_STOP);
            return;
        }
        current_image[port]->current_file_seek_position++;
        /* in v1/v2 .tap the next 3 bytes are the exact length of the gap in cycles */
        if (current_image[port]->version >= 1) {
            uint8_t long_gap[3];
            int bytes_written;
            long_gap[0] = (uint8_t)(write_time & 0xff);
            long_gap[1] = (uint8_t)((write_time >> 8) & 0xff);
            long_gap[2] = (uint8_t)((write_time >> 16) & 0xff);
            write_time &= 0xffffff;
            bytes_written = (int)fwrite(long_gap, 1, 3, current_image[port]->fd);
            DBG(("bit_write v1 gap 0x%"PRIx64" at position 0x%04x",
                write_time, (unsigned int)current_image[port]->current_file_seek_position - 1));
            current_image[port]->current_file_seek_position += bytes_written;
            if (bytes_written < 3) {
                log_error(datasette_log, "datasette bit_write failed (stopping tape).");
                datasette_control(port, DATASETTE_CONTROL_STOP);
                return;
            }
        }
    }
    /* adjust file size */
    if (current_image[port]->size < current_image[port]->current_file_seek_position) {
        current_image[port]->size = current_image[port]->current_file_seek_position;
    }

    current_image[port]->cycle_counter += write_time / 8;

    /* Correct for C16 TAPs so the counter is the same during record/play */
    if (machine_class == VICE_MACHINE_PLUS4) {
        current_image[port]->cycle_counter += write_time / 8;
    }

    if (current_image[port]->cycle_counter_total < current_image[port]->cycle_counter) {
        current_image[port]->cycle_counter_total = current_image[port]->cycle_counter;
    }
    current_image[port]->has_changed = 1;
    datasette_update_ui_counter(port);
}

/* BUG: when the motor is started when the write line is 0, then the first
        pulse will not end up in the .tap file

        see https://sourceforge.net/p/vice-emu/bugs/1598/ */

static void datasette_toggle_write_bit(int port, int write_bit)
{
    DBG(("datasette_toggle_write_bit: last_write_bit:%d write_bit:%d maincpu_clk: 0x%"PRIx64"",
         last_write_bit[port] ? 1 : 0, write_bit ? 1 : 0, maincpu_clk));
    if ((current_image[port] != NULL) && /* there is a tape image */
        (current_image[port]->mode == DATASETTE_CONTROL_RECORD) && /* record is pressed */
        (datasette_motor[port] != 0)) { /* motor is running */

        /* FIXME: right now we always write v1 .tap files (falling edges only),
                  even for xplus4 */

        /* check if this is a falling edge (0->1 transtion on the write line) */
        if ((last_write_bit[port] == 0) && (write_bit != 0)) {
            if (last_write_clk[port] >= maincpu_clk) {
                /* HACK: should the last write time be in the future, adjust it to be now */
                DBG(("datasette_toggle_write_bit: first pulse starts at maincpu_clk: 0x%"PRIx64"", maincpu_clk));
                last_write_clk[port] = maincpu_clk;
            } else {
                bit_write(port);
            }
        }
    }
    last_write_bit[port] = write_bit;
}

/*******************************************************************************
    Event recording
 ******************************************************************************/

static void datasette_event_record(int command)
{
    uint32_t rec_cmd;

    rec_cmd = (uint32_t)command;

    if (network_connected()) {
        network_event_record(EVENT_DATASETTE, (void *)&rec_cmd, sizeof(uint32_t));
    } else {
        event_record(EVENT_DATASETTE, (void *)&rec_cmd, sizeof(uint32_t));
    }
}

void datasette_event_playback_port1(CLOCK offset, void *data)
{
    int command;

    command = (int)(*(uint32_t *)data);

    datasette_control_internal(TAPEPORT_PORT_1, command);
}

void datasette_event_playback_port2(CLOCK offset, void *data)
{
    int command;

    command = (int)(*(uint32_t *)data);

    datasette_control_internal(TAPEPORT_PORT_2, command);
}

/*******************************************************************************
    Snapshot support
 ******************************************************************************/

#define DATASETTE_SNAP_MAJOR 1
#define DATASETTE_SNAP_MINOR 5

static int datasette_write_snapshot(int port, snapshot_t *s, int write_image)
{
    snapshot_module_t *m;
    CLOCK alarm_clk = CLOCK_MAX;

    m = snapshot_module_create(s, "DATASETTE", DATASETTE_SNAP_MAJOR,
                               DATASETTE_SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (datasette_alarm_pending[port]) {
        alarm_clk = datasette_alarm[port]->context->pending_alarms[datasette_alarm[port]->pending_idx].clk;
    }

    if (0
        || SMW_B(m, (uint8_t)datasette_motor[port]) < 0
        || SMW_B(m, (uint8_t)notape_mode[port]) < 0
        || SMW_CLOCK(m, last_write_clk[port]) < 0
        || SMW_CLOCK(m, motor_stop_clk[port]) < 0
        || SMW_B(m, (uint8_t)datasette_alarm_pending[port]) < 0
        || SMW_CLOCK(m, alarm_clk) < 0
        || SMW_CLOCK(m, datasette_long_gap_pending[port]) < 0
        || SMW_CLOCK(m, datasette_long_gap_elapsed[port]) < 0
        || SMW_B(m, (uint8_t)datasette_last_direction[port]) < 0
        || SMW_DW(m, datasette_counter_offset[port]) < 0
        || SMW_B(m, (uint8_t)reset_datasette_with_maincpu) < 0
        || SMW_DW(m, datasette_zero_gap_delay) < 0
        || SMW_DW(m, datasette_speed_tuning) < 0
        || SMW_DW(m, datasette_tape_wobble_frequency) < 0
        || SMW_DW(m, datasette_tape_wobble_amplitude) < 0
        || SMW_DW(m, datasette_tape_azimuth_error) < 0
        || SMW_B(m, (uint8_t)fullwave[port]) < 0
        || SMW_CLOCK(m, fullwave_gap[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return tape_snapshot_write_module(port, s, write_image);
}

static int datasette_read_snapshot(int port, snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    CLOCK alarm_clk;

    m = snapshot_module_open(s, "DATASETTE",
                             &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    if (0
        || SMR_B_INT(m, &datasette_motor[port]) < 0
        || SMR_B_INT(m, &notape_mode[port]) < 0
        || SMR_CLOCK(m, &last_write_clk[port]) < 0
        || SMR_CLOCK(m, &motor_stop_clk[port]) < 0
        || SMR_B_INT(m, &datasette_alarm_pending[port]) < 0
        || SMR_CLOCK(m, &alarm_clk) < 0
        || SMR_CLOCK(m, &datasette_long_gap_pending[port]) < 0
        || SMR_CLOCK(m, &datasette_long_gap_elapsed[port]) < 0
        || SMR_B_INT(m, &datasette_last_direction[port]) < 0
        || SMR_DW_INT(m, &datasette_counter_offset[port]) < 0
        || SMR_B_INT(m, &reset_datasette_with_maincpu) < 0
        || SMR_DW_INT(m, &datasette_zero_gap_delay) < 0
        || SMR_DW_INT(m, &datasette_speed_tuning) < 0
        || SMR_DW_INT(m, &datasette_tape_wobble_frequency) < 0
        || SMR_DW_INT(m, &datasette_tape_wobble_amplitude) < 0
        || SMR_DW_INT(m, &datasette_tape_azimuth_error) < 0
        || SMR_B_INT(m, (int *)&fullwave[port]) < 0
        || SMR_CLOCK(m, &fullwave_gap[port]) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (datasette_alarm_pending[port]) {
        alarm_set(datasette_alarm[port], alarm_clk);
    } else {
        alarm_unset(datasette_alarm[port]);
    }

    ui_set_tape_status(port, current_image[port] ? 1 : 0);
    datasette_update_ui_counter(port);
    ui_display_tape_motor_status(port, datasette_motor[port]);
    if (current_image[port]) {
        ui_display_tape_control_status(port, current_image[port]->mode);

        if (current_image[port]->mode > 0) {
            if (datasette_enabled[port]) {
                tapeport_set_tape_sense(1, port);
            }
        } else {
            if (datasette_enabled[port]) {
                tapeport_set_tape_sense(0, port);
            }
        }
    }

    /* reset buffer */
    next_tap[port] = last_tap[port] = 0;

    snapshot_module_close(m);

    return tape_snapshot_read_module(port, s);
}
