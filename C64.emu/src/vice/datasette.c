/*
 * datasette.c - CBM cassette implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andreas Matthies <andreas.matthies@gmx.net>
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
#include "clkguard.h"
#include "cmdline.h"
#include "datasette.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "network.h"
#include "resources.h"
#include "snapshot.h"
#include "tap.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "vice-event.h"

#ifdef DEBUG_TAPE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#define MOTOR_DELAY         32000
#define TAP_BUFFER_LENGTH   100000

/* at least every DATASETTE_MAX_GAP cycle there should be an alarm */
#define DATASETTE_MAX_GAP   100000


/* Attached TAP tape image.  */
static tap_t *current_image = NULL;

/* Buffer for the TAP */
static BYTE tap_buffer[TAP_BUFFER_LENGTH];

/* Pointer and length of the tap-buffer */
static long next_tap, last_tap;

/* State of the datasette motor.  */
static int datasette_motor = 0;

/* Last time we have recorded a flux change.  */
static CLOCK last_write_clk = (CLOCK)0;

/* Motor stop is delayed.  */
static CLOCK motor_stop_clk = (CLOCK)0;

static alarm_t *datasette_alarm = NULL;

static int datasette_alarm_pending = 0;

static CLOCK datasette_long_gap_pending = 0;

static CLOCK datasette_long_gap_elapsed = 0;

static int datasette_last_direction = 0;

static long datasette_cycles_per_second;

/* Remember the reset of tape-counter.  */
static int datasette_counter_offset = 0;

/* app_resource datasette */
/* shall the datasette reset when the CPU does? */
static int reset_datasette_with_maincpu;

/* how long to wait, if a zero occurs in the tap ? */
static int datasette_zero_gap_delay;

/* finetuning for speed of motor */
static int datasette_speed_tuning;

/* status when no tape image is present */
static int notape_mode = DATASETTE_CONTROL_STOP;

/* Low/high wave indicator for C16 TAPs. */
static unsigned int fullwave = 0;
static CLOCK fullwave_gap;

/* random wobble to be added to tape pulses */
static int datasette_tape_wobble = 0;

static log_t datasette_log = LOG_ERR;

static void datasette_internal_reset(void);
static void datasette_event_record(int command);
static void datasette_control_internal(int command);

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
    if (val < 0) {
        return -1;
    }
    datasette_zero_gap_delay = val;

    return 0;
}

static int set_datasette_speed_tuning(int val, void *param)
{
    if (val < 0) {
        return -1;
    }

    datasette_speed_tuning = val;

    return 0;
}

static int set_datasette_tape_wobble(int val, void *param)
{
    if (val < 0) {
        return -1;
    }

    datasette_tape_wobble = val;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DatasetteResetWithCPU", 1, RES_EVENT_SAME, NULL,
      &reset_datasette_with_maincpu,
      set_reset_datasette_with_maincpu, NULL },
    { "DatasetteZeroGapDelay", 20000, RES_EVENT_SAME, NULL,
      &datasette_zero_gap_delay,
      set_datasette_zero_gap_delay, NULL },
    { "DatasetteSpeedTuning", 1, RES_EVENT_SAME, NULL,
      &datasette_speed_tuning,
      set_datasette_speed_tuning, NULL },
    { "DatasetteTapeWobble", 10, RES_EVENT_SAME, NULL,
      &datasette_tape_wobble,
      set_datasette_tape_wobble, NULL },
    { NULL }
};

int datasette_resources_init(void)
{
    return resources_register_int(resources_int);
}

/*******************************************************************************
    Commandline options
 ******************************************************************************/

static const cmdline_option_t cmdline_options[] = {
    { "-dsresetwithcpu", SET_RESOURCE, 0,
      NULL, NULL, "DatasetteResetWithCPU", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_AUTO_DATASETTE_RESET,
      NULL, NULL },
    { "+dsresetwithcpu", SET_RESOURCE, 0,
      NULL, NULL, "DatasetteResetWithCPU", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_AUTO_DATASETTE_RESET,
      NULL, NULL },
    { "-dszerogapdelay", SET_RESOURCE, 1,
      NULL, NULL, "DatasetteZeroGapDelay", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_ZERO_TAP_DELAY,
      NULL, NULL },
    { "-dsspeedtuning", SET_RESOURCE, 1,
      NULL, NULL, "DatasetteSpeedTuning", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_CYCLES_ADDED_GAP_TAP,
      NULL, NULL },
    { "-dstapewobble", SET_RESOURCE, 1,
      NULL, NULL, "DatasetteTapeWobble", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_TAPE_WOBBLE,
      NULL, NULL },
    { NULL }
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

static void datasette_update_ui_counter(void)
{
    if (current_image == NULL) {
        /* FIXME: this is not quite correct, on a real datasette the counter
                  would also count when no tape is inserted */
        ui_display_tape_counter(1000 - datasette_counter_offset);
    } else {
        current_image->counter = (1000 - datasette_counter_offset +
                                (int) (DS_G *
                                        (sqrt((current_image->cycle_counter
                                                / (datasette_cycles_per_second / 8.0)
                                                * ds_c1) + ds_c2) - ds_c3))) % 1000;
        ui_display_tape_counter(current_image->counter);
    }
}


void datasette_reset_counter(void)
{
    if (current_image == NULL) {
        datasette_counter_offset = (1000);
    } else {
        datasette_counter_offset = (1000 + (int) (DS_G *
                                                (sqrt((current_image->cycle_counter
                                                        / (datasette_cycles_per_second / 8.0)
                                                        * ds_c1) + ds_c2) - ds_c3))) % 1000;
    }
    datasette_update_ui_counter();
}


inline static int datasette_move_buffer_forward(int offset)
{
    /* reads buffer to fit the next gap-read
       tap_buffer[next_tap] ~ current_file_seek_position
    */
    if (next_tap + offset >= last_tap) {
        if (fseek(current_image->fd, current_image->current_file_seek_position
                  + current_image->offset, SEEK_SET)) {
            log_error(datasette_log, "Cannot read in tap-file.");
            return 0;
        }
        last_tap = (long)fread(tap_buffer, 1, TAP_BUFFER_LENGTH, current_image->fd);
        next_tap = 0;
        if (next_tap >= last_tap) {
            return 0;
        }
    }
    return 1;
}

inline static int datasette_move_buffer_back(int offset)
{
    /* reads buffer to fit the next gap-read at current_file_seek_position-1
       tap_buffer[next_tap] ~ current_file_seek_position
    */
    if (next_tap + offset < 0) {
        if (current_image->current_file_seek_position >= TAP_BUFFER_LENGTH) {
            next_tap = TAP_BUFFER_LENGTH;
        } else {
            next_tap = current_image->current_file_seek_position;
        }
        if (fseek(current_image->fd, current_image->current_file_seek_position
                  - next_tap + current_image->offset, SEEK_SET)) {
            log_error(datasette_log, "Cannot read in tap-file.");
            return 0;
        }
        last_tap = (long)fread(tap_buffer, 1, TAP_BUFFER_LENGTH, current_image->fd);
        if (next_tap > last_tap) {
            return 0;
        }
    }
    return 1;
}

inline static int fetch_gap(CLOCK *gap, int *direction, long read_tap)
{
    int wobble;

    if ((read_tap >= last_tap) || (read_tap < 0)) {
        return -1;
    }

    *gap = tap_buffer[read_tap];

    if ((current_image->version == 0) || *gap) {
        *gap = (*gap ? (CLOCK)(*gap * 8) : (CLOCK)datasette_zero_gap_delay)
               + (CLOCK)datasette_speed_tuning;
    } else {
        if (read_tap >= last_tap - 3) {
            return -1;
        }
        *direction *= 4;
        *gap = tap_buffer[read_tap + 1]
               + (tap_buffer[read_tap + 2] << 8)
               + (tap_buffer[read_tap + 3] << 16);
        if (!(*gap)) {
            *gap = (CLOCK)datasette_zero_gap_delay;
        }
    }
    /* add some random wobble */
    if (datasette_tape_wobble) {
        wobble = lib_unsigned_rand(-datasette_tape_wobble, datasette_tape_wobble);
        if ((wobble >= 0) || (*gap > (CLOCK)-wobble)) {
            *gap += wobble;
        } else {
            *gap = 0;
        }
    }
    return 0;
}

inline static void read_gap_forward(long *read_tap)
{
    *read_tap = next_tap;
}

inline static void read_gap_backward_v0(long *read_tap)
{
    *read_tap = next_tap - 1;
}

inline static int read_gap_backward_v1(long *read_tap)
{
    /* examine, if previous gap was long
       by rewinding until 3 non-zero-values
       in a row found, then reading forward (FIXME???)
    */
    int non_zeros_in_a_row = 0;
    long remember_file_seek_position;

    remember_file_seek_position = current_image->current_file_seek_position;

    current_image->current_file_seek_position -= 4;
    next_tap -= 4;

    while ((non_zeros_in_a_row < 3) && current_image->current_file_seek_position) {
        if (!datasette_move_buffer_back(-1)) {
            return 1;
        }
        current_image->current_file_seek_position--;
        next_tap--;
        if (tap_buffer[next_tap]) {
            non_zeros_in_a_row++;
        } else {
            non_zeros_in_a_row = 0;
        }
    }

    /* now forward */
    while (current_image->current_file_seek_position < remember_file_seek_position - 4) {
        if (!datasette_move_buffer_forward(1)) {
            return -1;
        }
        if (tap_buffer[next_tap]) {
            current_image->current_file_seek_position++;
            next_tap++;
        } else {
            current_image->current_file_seek_position += 4;
            next_tap += 4;
        }
    }
    if (!datasette_move_buffer_forward(4)) {
        return -1;
    }

    *read_tap = next_tap;
    next_tap += (remember_file_seek_position - current_image->current_file_seek_position);
    current_image->current_file_seek_position = remember_file_seek_position;

    return 0;
}

static CLOCK datasette_read_gap(int direction)
{
    /* direction 1: forward, -1: rewind */
    long read_tap = 0;
    CLOCK gap = 0;

/*    if (current_image->system != 2 || current_image->version != 1
        || !fullwave) {*/
    if (current_image->system != 2) {
        if ((direction < 0) && !datasette_move_buffer_back(direction * 4)) {
            return 0;
        }
        if ((direction > 0 ) && !datasette_move_buffer_forward(direction * 4)) {
            return 0;
        }

        if (direction > 0) {
            read_gap_forward(&read_tap);
        } else {
            if ((current_image->version == 0) || (next_tap < 4) || tap_buffer[next_tap - 4]) {
                read_gap_backward_v0(&read_tap);
            } else {
                if (read_gap_backward_v1(&read_tap) < 0) {
                    return 0;
                }
            }
        }
        if (fetch_gap(&gap, &direction, read_tap) < 0) {
            return 0;
        }
        next_tap += direction;
        current_image->current_file_seek_position += direction;
    }

    if (current_image->system == 2 && current_image->version == 1) {
        if (!fullwave) {
            if ((direction < 0) && !datasette_move_buffer_back(direction * 4)) {
                return 0;
            }
            if ((direction > 0 ) && !datasette_move_buffer_forward(direction * 4)) {
                return 0;
            }

            if (direction > 0) {
                read_gap_forward(&read_tap);
            } else {
                if ((current_image->version == 0) || (next_tap < 4) || tap_buffer[next_tap - 4]) {
                    read_gap_backward_v0(&read_tap);
                } else {
                    if (read_gap_backward_v1(&read_tap) < 0) {
                        return 0;
                    }
                }
            }
            if (fetch_gap(&gap, &direction, read_tap) < 0) {
                return 0;
            }

            fullwave_gap = gap;
            next_tap += direction;
            current_image->current_file_seek_position += direction;
        } else {
            gap = fullwave_gap;
        }
        fullwave ^= 1;
    } else if (current_image->system == 2 && current_image->version == 2) {
        if ((direction < 0) && !datasette_move_buffer_back(direction * 4)) {
            return 0;
        }
        if ((direction > 0 ) && !datasette_move_buffer_forward(direction * 4)) {
            return 0;
        }

        if (direction > 0) {
            read_gap_forward(&read_tap);
        } else {
            if ((current_image->version == 0) || (next_tap < 4) || tap_buffer[next_tap - 4]) {
                read_gap_backward_v0(&read_tap);
            } else {
                if (read_gap_backward_v1(&read_tap) < 0) {
                    return 0;
                }
            }
        }
        if (fetch_gap(&gap, &direction, read_tap) < 0) {
            return 0;
        }
        gap *= 2;
        fullwave ^= 1;
        next_tap += direction;
        current_image->current_file_seek_position += direction;
    }
    return gap;
}

/* this is the alarm function */
static void datasette_read_bit(CLOCK offset, void *data)
{
    double speed_of_tape = DS_V_PLAY;
    int direction = 1;
    long gap;

    alarm_unset(datasette_alarm);
    datasette_alarm_pending = 0;

    DBG(("datasette_read_bit(motor:%d) %d>=%d (image present:%s)", datasette_motor, maincpu_clk, motor_stop_clk, current_image ? "yes" : "no"));

    /* check for delay of motor stop */
    if (motor_stop_clk > 0 && maincpu_clk >= motor_stop_clk) {
        motor_stop_clk = 0;
        ui_display_tape_motor_status(0);
        datasette_motor = 0;
    }
    DBG(("datasette_read_bit(motor:%d)", datasette_motor));

    if (!datasette_motor) {
        return;
    }

    if (current_image == NULL) {
        switch (notape_mode) {
            case DATASETTE_CONTROL_START:
            case DATASETTE_CONTROL_FORWARD:
            case DATASETTE_CONTROL_REWIND:
            case DATASETTE_CONTROL_RECORD:
                break;
            case DATASETTE_CONTROL_STOP:
                if (motor_stop_clk > 0) {
                    alarm_set(datasette_alarm, motor_stop_clk);
                    datasette_alarm_pending = 1;
                }
                break;
        }
        datasette_update_ui_counter();
        return;
    }

    switch (current_image->mode) {
        case DATASETTE_CONTROL_START:
            direction = 1;
            speed_of_tape = DS_V_PLAY;
            if (!datasette_long_gap_pending) {
                datasette_trigger_flux_change(fullwave);
            }
            break;
        case DATASETTE_CONTROL_FORWARD:
            direction = 1;
            speed_of_tape = DS_RPS_FAST / DS_G
                            * sqrt(4 * PI * DS_D
                                   * DS_V_PLAY / datasette_cycles_per_second * 8
                                   * current_image->cycle_counter
                                   + 4 * PI * PI * DS_R * DS_R);
            break;
        case DATASETTE_CONTROL_REWIND:
            direction = -1;
            speed_of_tape = DS_RPS_FAST / DS_G
                            * sqrt(4 * PI * DS_D
                                   * DS_V_PLAY / datasette_cycles_per_second * 8
                                   * (current_image->cycle_counter_total
                                      - current_image->cycle_counter)
                                   + 4 * PI * PI * DS_R * DS_R);
            break;
        case DATASETTE_CONTROL_RECORD:
        case DATASETTE_CONTROL_STOP:
            return;
        default:
            log_error(datasette_log, "Unknown datasette mode.");
            return;
    }

    if (direction + datasette_last_direction == 0) {
        /* the direction changed; read the gap from file,
        but use use only the elapsed gap */
        gap = datasette_read_gap(direction);
        datasette_long_gap_pending = datasette_long_gap_elapsed;
        datasette_long_gap_elapsed = gap - datasette_long_gap_elapsed;
    }
    if (datasette_long_gap_pending) {
        gap = datasette_long_gap_pending;
        datasette_long_gap_pending = 0;
    } else {
        gap = datasette_read_gap(direction);
        if (gap) {
            datasette_long_gap_elapsed = 0;
        }
    }
    if (!gap) {
        datasette_control(DATASETTE_CONTROL_STOP);
        return;
    }
    if (gap > DATASETTE_MAX_GAP) {
        datasette_long_gap_pending = gap - DATASETTE_MAX_GAP;
        gap = DATASETTE_MAX_GAP;
    }
    datasette_long_gap_elapsed += gap;
    datasette_last_direction = direction;

    if (direction > 0) {
        current_image->cycle_counter += gap / 8;
    } else {
        current_image->cycle_counter -= gap / 8;
    }

    gap -= offset;

    if (gap > 0) {
        alarm_set(datasette_alarm, maincpu_clk +
                  (CLOCK)(gap * (DS_V_PLAY / speed_of_tape)));
        datasette_alarm_pending = 1;
    } else {
        /* If the offset is geater than the gap to the next flux
           change, the change happend during DMA.  Schedule it now.  */
        alarm_set(datasette_alarm, maincpu_clk);
        datasette_alarm_pending = 1;
    }
    datasette_update_ui_counter();
}


static void clk_overflow_callback(CLOCK sub, void *data)
{
    DBG(("clk_overflow_callback"));
    if (last_write_clk > (CLOCK)0) {
        last_write_clk -= sub;
    }
    if (motor_stop_clk > (CLOCK)0) {
        motor_stop_clk -= sub;
    }
}

void datasette_init(void)
{
    DBG(("datasette_init"));
    datasette_log = log_open("Datasette");

    datasette_alarm = alarm_new(maincpu_alarm_context, "Datasette",
                                datasette_read_bit, NULL);

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    datasette_cycles_per_second = machine_get_cycles_per_second();
    if (!datasette_cycles_per_second) {
        log_error(datasette_log,
                  "Cannot get cycles per second for this machine.");
        datasette_cycles_per_second = 985248;
    }
    datasette_set_tape_image(NULL);
}

void datasette_set_tape_image(tap_t *image)
{
    CLOCK gap;

    DBG(("datasette_set_tape_image (image present:%s)", image ? "yes" : "no"));

    current_image = image;
    last_tap = next_tap = 0;
    datasette_internal_reset();

    if (image != NULL) {
        /* We need the length of tape for realistic counter. */
        current_image->cycle_counter_total = 0;
        do {
            gap = datasette_read_gap(1);
            current_image->cycle_counter_total += gap / 8;
        } while (gap);
        current_image->current_file_seek_position = 0;
    }
    datasette_set_tape_sense(0);

    last_tap = next_tap = 0;
    fullwave = 0;

    ui_set_tape_status(current_image ? 1 : 0);
}


static void datasette_forward(void)
{
    int mode = current_image ? current_image->mode : notape_mode;

    DBG(("datasette_forward"));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_REWIND) {
        alarm_unset(datasette_alarm);
        datasette_alarm_pending = 0;
    }
    alarm_set(datasette_alarm, maincpu_clk + 1000);
    datasette_alarm_pending = 1;
}

static void datasette_rewind(void)
{
    int mode = current_image ? current_image->mode : notape_mode;

    DBG(("datasette_rewind"));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_FORWARD) {
        alarm_unset(datasette_alarm);
        datasette_alarm_pending = 0;
    }
    alarm_set(datasette_alarm, maincpu_clk + 1000);
    datasette_alarm_pending = 1;
}


static void datasette_internal_reset(void)
{
    int mode = current_image ? current_image->mode : notape_mode;

    DBG(("datasette_internal_reset (mode:%d)", mode));

    if (mode == DATASETTE_CONTROL_START ||
        mode == DATASETTE_CONTROL_FORWARD ||
        mode == DATASETTE_CONTROL_REWIND) {
        alarm_unset(datasette_alarm);
        datasette_alarm_pending = 0;
    }
    datasette_control(current_image ? DATASETTE_CONTROL_STOP : notape_mode);
    if (current_image != NULL) {
        if (!autostart_ignore_reset) {
            tap_seek_start(current_image);
        }
        current_image->cycle_counter = 0;
    }
    datasette_counter_offset = 0;
    datasette_long_gap_pending = 0;
    datasette_long_gap_elapsed = 0;
    datasette_last_direction = 0;
    motor_stop_clk = 0;
    datasette_update_ui_counter();
    fullwave = 0;
}

void datasette_reset(void)
{
    int ds_reset;
    DBG(("datasette_reset"));
    resources_get_int("DatasetteResetWithCPU", &ds_reset);

    if (ds_reset) {
        datasette_internal_reset();
    }
}

static void datasette_start_motor(void)
{
    DBG(("datasette_start_motor (image present:%s)", current_image ? "yes" : "no"));
    if (current_image) {
        fseek(current_image->fd, current_image->current_file_seek_position + current_image->offset, SEEK_SET);
    }
    if (!datasette_alarm_pending) {
        alarm_set(datasette_alarm, maincpu_clk + MOTOR_DELAY);
        datasette_alarm_pending = 1;
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

static void datasette_control_internal(int command)
{
    DBG(("datasette_control_internal (%s) (image present:%s)", cmdstr[command], current_image ? "yes" : "no"));
    if (current_image) {
        switch (command) {
            case DATASETTE_CONTROL_RESET_COUNTER:
                datasette_reset_counter();
                break;
            case DATASETTE_CONTROL_RESET:
                datasette_internal_reset();
            case DATASETTE_CONTROL_STOP:
                current_image->mode = DATASETTE_CONTROL_STOP;
                datasette_set_tape_sense(0);
                last_write_clk = (CLOCK)0;
                break;
            case DATASETTE_CONTROL_START:
                current_image->mode = DATASETTE_CONTROL_START;
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_FORWARD:
                current_image->mode = DATASETTE_CONTROL_FORWARD;
                datasette_forward();
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_REWIND:
                current_image->mode = DATASETTE_CONTROL_REWIND;
                datasette_rewind();
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_RECORD:
                if (current_image->read_only == 0) {
                    current_image->mode = DATASETTE_CONTROL_RECORD;
                    datasette_set_tape_sense(1);
                    last_write_clk = (CLOCK)0;
                }
                break;
        }
        ui_display_tape_control_status(current_image->mode);
    } else {
       switch (command) {
            case DATASETTE_CONTROL_RESET_COUNTER:
                datasette_reset_counter();
                break;
            case DATASETTE_CONTROL_RESET:
                datasette_internal_reset();
            case DATASETTE_CONTROL_STOP:
                notape_mode = DATASETTE_CONTROL_STOP;
                datasette_set_tape_sense(0);
                last_write_clk = (CLOCK)0;
                break;
            case DATASETTE_CONTROL_START:
                notape_mode = DATASETTE_CONTROL_START;
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_FORWARD:
                notape_mode = DATASETTE_CONTROL_FORWARD;
                datasette_forward();
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_REWIND:
                notape_mode = DATASETTE_CONTROL_REWIND;
                datasette_rewind();
                datasette_set_tape_sense(1);
                last_write_clk = (CLOCK)0;
                if (datasette_motor) {
                    datasette_start_motor();
                }
                break;
            case DATASETTE_CONTROL_RECORD:
                /* record can usually not be pressed when no tape is present */
                break;
        }
        ui_display_tape_control_status(notape_mode);
    }
    /* clear the tap-buffer */
    last_tap = next_tap = 0;
}

void datasette_control(int command)
{
    if (event_playback_active()) {
        return;
    }

    datasette_event_record(command);
    if (!network_connected()) {
        datasette_control_internal(command);
    }
}

void datasette_set_motor(int flag)
{
    DBG(("datasette_set_motor(%d) (image present:%s)", flag, current_image ? "yes" : "no"));

    if (datasette_alarm == NULL) {
        DBG(("datasette_set_motor (datasette_alarm == NULL)"));
        return;
    }

    if (flag) {
        /* abort pending motor stop */
        motor_stop_clk = 0;
        if (!datasette_motor) {
            last_write_clk = (CLOCK)0;
            datasette_start_motor();
            ui_display_tape_motor_status(1);
            datasette_motor = 1;
        }
    }
    if (!flag && datasette_motor && motor_stop_clk == 0) {
        motor_stop_clk = maincpu_clk + MOTOR_DELAY;
        DBG(("datasette_set_motor(maincpu_clk:%d motor_stop_clk:%d)", maincpu_clk, motor_stop_clk));
        if (!datasette_alarm_pending) {
            /* make sure that the motor will stop */
            alarm_set(datasette_alarm, motor_stop_clk);
            datasette_alarm_pending = 1;
        }
    }
}

inline static void bit_write(void)
{
    CLOCK write_time;
    BYTE write_gap;

    write_time = maincpu_clk - last_write_clk;
    last_write_clk = maincpu_clk;

    /* C16 TAPs use half the machine clock as base cycle */
    if (machine_class == VICE_MACHINE_PLUS4) {
        write_time = write_time / 2;
    }

    if (write_time < (CLOCK)7) {
        return;
    }

    if (write_time < (CLOCK)(255 * 8 + 7)) {
        write_gap = (BYTE)(write_time / (CLOCK)8);
        if (fwrite(&write_gap, 1, 1, current_image->fd) < 1) {
            datasette_control(DATASETTE_CONTROL_STOP);
            return;
        }
        current_image->current_file_seek_position++;
    } else {
        write_gap = 0;
        if (fwrite(&write_gap, 1, 1, current_image->fd) != 1) {
            log_debug("datasette bit_write failed.");
        }
        current_image->current_file_seek_position++;
        if (current_image->version >= 1) {
            BYTE long_gap[3];
            int bytes_written;
            long_gap[0] = (BYTE)(write_time & 0xff);
            long_gap[1] = (BYTE)((write_time >> 8) & 0xff);
            long_gap[2] = (BYTE)((write_time >> 16) & 0xff);
            write_time &= 0xffffff;
            bytes_written = (int)fwrite(long_gap, 1, 3, current_image->fd);
            current_image->current_file_seek_position += bytes_written;
            if (bytes_written < 3) {
                datasette_control(DATASETTE_CONTROL_STOP);
                return;
            }
        }
    }
    if (current_image->size < current_image->current_file_seek_position) {
        current_image->size = current_image->current_file_seek_position;
    }

    current_image->cycle_counter += write_time / 8;

    /* Correct for C16 TAPs so the counter is the same during record/play */
    if (machine_class == VICE_MACHINE_PLUS4) {
        current_image->cycle_counter += write_time / 8;
    }

    if (current_image->cycle_counter_total < current_image->cycle_counter) {
        current_image->cycle_counter_total = current_image->cycle_counter;
    }
    current_image->has_changed = 1;
    datasette_update_ui_counter();
}

void datasette_toggle_write_bit(int write_bit)
{
    if (current_image != NULL && write_bit
        && current_image->mode == DATASETTE_CONTROL_RECORD) {
        if (datasette_motor) {
            if (last_write_clk == (CLOCK)0) {
                last_write_clk = maincpu_clk;
            } else {
                bit_write();
            }
        }
    }
}

/*******************************************************************************
    Event recording
 ******************************************************************************/

static void datasette_event_record(int command)
{
    DWORD rec_cmd;

    rec_cmd = (DWORD)command;

    if (network_connected()) {
        network_event_record(EVENT_DATASETTE, (void *)&rec_cmd, sizeof(DWORD));
    } else {
        event_record(EVENT_DATASETTE, (void *)&rec_cmd, sizeof(DWORD));
    }
}

void datasette_event_playback(CLOCK offset, void *data)
{
    int command;

    command = (int)(*(DWORD *)data);

    datasette_control_internal(command);
}

/*******************************************************************************
    Snapshot support
 ******************************************************************************/

#define DATASETTE_SNAP_MAJOR 1
#define DATASETTE_SNAP_MINOR 2

int datasette_write_snapshot(snapshot_t *s)
{
    snapshot_module_t *m;
    DWORD alarm_clk = CLOCK_MAX;

    m = snapshot_module_create(s, "DATASETTE", DATASETTE_SNAP_MAJOR,
                               DATASETTE_SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (datasette_alarm_pending) {
        alarm_clk = datasette_alarm->context->pending_alarms[datasette_alarm->pending_idx].clk;
    }

    if (0
        || SMW_B(m, (BYTE)datasette_motor) < 0
        || SMW_B(m, (BYTE)notape_mode) < 0
        || SMW_DW(m, last_write_clk) < 0
        || SMW_DW(m, motor_stop_clk) < 0
        || SMW_B(m, (BYTE)datasette_alarm_pending) < 0
        || SMW_DW(m, alarm_clk) < 0
        || SMW_DW(m, datasette_long_gap_pending) < 0
        || SMW_DW(m, datasette_long_gap_elapsed) < 0
        || SMW_B(m, (BYTE)datasette_last_direction) < 0
        || SMW_DW(m, datasette_counter_offset) < 0
        || SMW_B(m, (BYTE)reset_datasette_with_maincpu) < 0
        || SMW_DW(m, datasette_zero_gap_delay) < 0
        || SMW_DW(m, datasette_speed_tuning) < 0
        || SMW_DW(m, datasette_tape_wobble) < 0
        || SMW_B(m, (BYTE)fullwave) < 0
        || SMW_DW(m, fullwave_gap) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}

int datasette_read_snapshot(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    DWORD alarm_clk;

    m = snapshot_module_open(s, "DATASETTE",
                             &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    if (0
        || SMR_B_INT(m, &datasette_motor) < 0
        || SMR_B_INT(m, &notape_mode) < 0
        || SMR_DW(m, &last_write_clk) < 0
        || SMR_DW(m, &motor_stop_clk) < 0
        || SMR_B_INT(m, &datasette_alarm_pending) < 0
        || SMR_DW(m, &alarm_clk) < 0
        || SMR_DW(m, &datasette_long_gap_pending) < 0
        || SMR_DW(m, (DWORD *)&datasette_long_gap_elapsed) < 0
        || SMR_B_INT(m, &datasette_last_direction) < 0
        || SMR_DW_INT(m, &datasette_counter_offset) < 0
        || SMR_B_INT(m, &reset_datasette_with_maincpu) < 0
        || SMR_DW_INT(m, &datasette_zero_gap_delay) < 0
        || SMR_DW_INT(m, &datasette_speed_tuning) < 0
        || SMR_DW_INT(m, &datasette_tape_wobble) < 0
        || SMR_B_INT(m, (int *)&fullwave) < 0
        || SMR_DW(m, &fullwave_gap) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (datasette_alarm_pending) {
        alarm_set(datasette_alarm, alarm_clk);
    } else {
        alarm_unset(datasette_alarm);
    }

    ui_set_tape_status(current_image ? 1 : 0);
    datasette_update_ui_counter();
    ui_display_tape_motor_status(datasette_motor);
    if (current_image) {
        ui_display_tape_control_status(current_image->mode);

        if (current_image->mode > 0) {
            datasette_set_tape_sense(1);
        } else {
            datasette_set_tape_sense(0);
        }
    }

    /* reset buffer */
    next_tap = last_tap = 0;

    snapshot_module_close(m);
    return 0;
}
