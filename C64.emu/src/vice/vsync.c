/*
 * vsync.c - End-of-frame handling
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
 *  Dag Lem <resid@nimrod.no>
 *  Thomas Bretz <tbretz@gsi.de>
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

/* This does what has to be done at the end of each screen frame (50 times per
   second on PAL machines). */

/* NB! The timing code depends on two's complement arithmetic.
   unsigned long is used for the timer variables, and the difference
   between two time points a and b is calculated with (signed long)(b - a)
   This allows timer variables to overflow without any explicit
   overflow handling.
*/

#include "vice.h"

/* Port me... */

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "archdep.h"
#include "cmdline.h"
#include "debug.h"
#include "joystick.h"
#include "kbdbuf.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "machine.h"
#ifdef HAVE_NETWORK
#include "monitor_network.h"
#include "monitor_binary.h"
#endif
#include "network.h"
#include "resources.h"
#include "sound.h"
#include "types.h"
#include "videoarch.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "ui.h"

#ifdef MACOS_COMPILE
#include "macOS-util.h"
#endif

/* public metrics, updated every vsync */
static double vsync_metric_cpu_percent;
static double vsync_metric_emulated_fps;

#ifdef USE_VICE_THREAD
#   include <pthread.h>
    pthread_mutex_t vsync_metric_lock = PTHREAD_MUTEX_INITIALIZER;
#   define METRIC_LOCK() pthread_mutex_lock(&vsync_metric_lock)
#   define METRIC_UNLOCK() pthread_mutex_unlock(&vsync_metric_lock)
#else
#   define METRIC_LOCK()
#   define METRIC_UNLOCK()
#endif

/* ------------------------------------------------------------------------- */

typedef struct callback_queue_s {
    vsync_callback_t *queue;
    int size_max;
    int size;
} callback_queue_t;

static callback_queue_t callback_queues[2];
static callback_queue_t *callback_queue = callback_queues;
static int callback_queue_index;

/** \brief Call callback_func(callback_param) once at vsync time (or machine reset) */
void vsync_on_vsync_do(vsync_callback_func_t callback_func, void *callback_param)
{
    /* Grow the queue as needed */
    if (callback_queue->size == callback_queue->size_max) {
        callback_queue->size_max++;
        callback_queue->queue = lib_realloc(callback_queue->queue, callback_queue->size_max * sizeof(vsync_callback_t));
    }

    callback_queue->queue[callback_queue->size].callback = callback_func;
    callback_queue->queue[callback_queue->size].param = callback_param;

    callback_queue->size++;
}

/** \brief Keep executing on_vsync_do callbacks until none are scheduled. */
void execute_vsync_callbacks(void)
{
    int i;
    callback_queue_t *executing_queue;

    while (callback_queue->size) {

        /* We'll iterate over this queue */
        executing_queue = callback_queue;

        /* Flip to the other queue to collect any new callbacks queued within these callbacks */
        callback_queue_index = 1 - callback_queue_index;
        callback_queue = &callback_queues[callback_queue_index];

        for (i = 0; i < executing_queue->size; i++) {
            executing_queue->queue[i].callback(executing_queue->queue[i].param);
        }

        executing_queue->size = 0;
    }
}

/* ------------------------------------------------------------------------- */

static int set_timer_speed(int speed);

/* Relative speed of the emulation (%) (negative values target FPS rather than cpu %) */
static int relative_speed;

/* "Warp mode".  If nonzero, attempt to run as fast as possible. */
static int warp_enabled;

/* "InitialWarpMode" resource controlling whether warp should be enabled from launch. */
static int initial_warp_mode_resource;

/* warp mode cmdline arg controlling whether warp should be enabled from launch. Overrides InitialWarpMode resource. */
static int initial_warp_mode_cmdline = -1;

/* When the next frame should be rendered, not skipped, during warp. */
static tick_t warp_render_tick_interval;

/* Triggers the vice thread to update its priorty */
static volatile int update_thread_priority = 1;

static int set_relative_speed(int val, void *param)
{
    if (val == 0) {
        val = 100;
        log_warning(LOG_DEFAULT, "Setting speed to 0 is no longer supported - use warp instead.");
    }

    relative_speed = val;
    sound_set_relative_speed(relative_speed);
    set_timer_speed(relative_speed);

    return 0;
}

void vsync_set_warp_mode(int val)
{
    warp_enabled = val ? 1 : 0;

    sound_set_warp_mode(warp_enabled);
    vsync_suspend_speed_eval();

    update_thread_priority = 1;
}

int vsync_get_warp_mode(void)
{
    return warp_enabled;
}

static int set_initial_warp_mode_resource(int val, void *param)
{
    initial_warp_mode_resource = val ? 1 : 0;

    return 0;
}

/* Vsync-related resources. */
static const resource_int_t resources_int[] = {
    { "Speed", 100, RES_EVENT_SAME, NULL,
      &relative_speed, set_relative_speed, NULL },
    { "InitialWarpMode", 0, RES_EVENT_STRICT, (resource_value_t)0,
      /* FIXME: maybe RES_EVENT_NO */
      &initial_warp_mode_resource, set_initial_warp_mode_resource, NULL },
    RESOURCE_INT_LIST_END
};

int vsync_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static int set_initial_warp_mode_cmdline(const char *param, void *extra_param)
{
    /*
     * We don't want -warp / +warp to end up in the config file,
     * so we don't use a resource.
     */

    initial_warp_mode_cmdline = vice_ptr_to_int(extra_param) ? 1 : 0;

    return 0;
}

/* Vsync-related command-line options. */
static const cmdline_option_t cmdline_options[] =
{
    { "-speed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Speed", NULL,
      "<percent or negative fps>", "Limit emulation speed to specified value" },
    { "-warp", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_initial_warp_mode_cmdline, int_to_void_ptr(1), NULL, NULL,
      NULL, "Initially enable warp mode" },
    { "+warp", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_initial_warp_mode_cmdline, int_to_void_ptr(0), NULL, NULL,
      NULL, "Do not initially enable warp mode (default)" },
    CMDLINE_LIST_END
};

int vsync_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* Maximum consecutive length of time we can skip rendering frames when
   adjusting the refresh rate dynamically for slow host CPU situations. */
#define MAX_RENDER_SKIP_MS (1000 / 10)

/* Number of frames per second on the real machine. */
static double refresh_frequency;

/* Number of clock cycles per seconds on the real machine. */
static long cycles_per_sec;

/* Function to call at the end of every screen frame. */
void (*vsync_hook)(void);

/* ------------------------------------------------------------------------- */

/* static guarantees zero values. */
static double ticks_per_frame;
static double cycles_per_frame;
static double emulated_clk_per_second;

static tick_t last_sync_emulated_tick;
static tick_t last_sync_tick;
static CLOCK last_sync_clk;

static tick_t sync_target_tick;

static int timer_speed = 0;
static bool sync_reset = true;
static bool metrics_reset = false;

/* Initialize vsync timers and set relative speed of emulation in percent. */
static int set_timer_speed(int speed)
{
    double cpu_percent;

    timer_speed = speed;

    vsync_suspend_speed_eval();

    if (refresh_frequency <= 0) {
        /* Happens during init */
        return -1;
    }

    if (speed < 0) {
        /* negative speeds are fps targets */
        cpu_percent = 100.0 * ((0 - speed) / refresh_frequency);
    } else {
        /* positive speeds are cpu percent targets */
        cpu_percent = speed;
    }

    ticks_per_frame = tick_per_second() * 100.0 / cpu_percent / refresh_frequency;
    emulated_clk_per_second = cycles_per_sec * cpu_percent / 100.0;

    return 0;
}

/* ------------------------------------------------------------------------- */

void vsync_set_machine_parameter(double refresh, long cycles)
{
    refresh_frequency = refresh;
    cycles_per_sec = cycles;
    cycles_per_frame = (double)cycles / refresh;
    set_timer_speed(relative_speed);
    vsyncarch_refresh_frequency_changed(refresh_frequency);
}

double vsync_get_refresh_frequency(void)
{
    return refresh_frequency;
}

void vsync_init(void (*hook)(void))
{
    /* Set the initial warp state. */
    if (initial_warp_mode_cmdline != -1) {
        /* Command line overrides config resource setting. */
        vsync_set_warp_mode(initial_warp_mode_cmdline);
    } else {
        vsync_set_warp_mode(initial_warp_mode_resource);
    }

    /* Limit warp rendering to 10fps */
    warp_render_tick_interval = tick_per_second() / 10.0;

    vsync_hook = hook;
    vsync_suspend_speed_eval();
}

void vsync_shutdown(void)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (callback_queues[i].queue) {
            lib_free(callback_queues[i].queue);
            callback_queues[i].queue = NULL;
        }
    }
}

/* This should be called whenever something that has nothing to do with the
   emulation happens, so that we don't display bogus speed values. */
void vsync_suspend_speed_eval(void)
{
    /* TODO - Is this needed any more now that late vsync is detected
       in vsync_do_vsync() */
    network_suspend();
    sync_reset = true;
}

void vsync_reset_hook(void)
{
    execute_vsync_callbacks();

    vsync_suspend_speed_eval();
}

void vsyncarch_get_metrics(double *cpu_percent, double *emulated_fps, int *is_warp_enabled)
{
    METRIC_LOCK();

    *cpu_percent = vsync_metric_cpu_percent;
    *emulated_fps = vsync_metric_emulated_fps;
    *is_warp_enabled = warp_enabled;

    METRIC_UNLOCK();
}

/*
 * TODO: Grow measurements array as needed so 5 seconds can be stored.
 * This will allow warp measurements to be stablise!
 */
#define MEASUREMENT_SMOOTH_FACTOR 0.99
#define MEASUREMENT_FRAME_WINDOW  250

static int measurement_count;
static int next_measurement_index;

/* For knowing the relevant timespan */
static tick_t last_tick;
static tick_t tick_deltas[MEASUREMENT_FRAME_WINDOW];
static uint64_t cumulative_tick_delta;

/* For measuring emulator cpu cycles per second */
static CLOCK last_clock;
static CLOCK clock_deltas[MEASUREMENT_FRAME_WINDOW];
static uint64_t cumulative_clock_delta;

static void reset_performance_metrics(tick_t frame_tick)
{
    /*
     * The emulator is just starting, or resuming from pause, or entering
     * warp, or exiting warp. So we reset the fps calculations.
     */

    last_tick = frame_tick;
    last_clock = maincpu_clk;

    measurement_count = 0;
    next_measurement_index = 0;

    cumulative_tick_delta = 0;
    cumulative_clock_delta = 0;

    METRIC_LOCK();

    /* The final smoothing function requires that we initialise the public metrics. */
    if (timer_speed > 0) {
        vsync_metric_emulated_fps = (double)timer_speed * refresh_frequency / 100.0;
        vsync_metric_cpu_percent  = timer_speed;
    } else {
        vsync_metric_emulated_fps = (0.0 - timer_speed);
        vsync_metric_cpu_percent  = (0.0 - timer_speed) / refresh_frequency * 100;
    }

    METRIC_UNLOCK();
}

static void update_performance_metrics(tick_t frame_tick)
{
    /* how many seconds of wallclock time the measurement window covers */
    double frame_timespan_seconds;

    /* how many emulated seconds of cpu time have been emulated */
    double clock_delta_seconds;

    CLOCK main_cpu_clock = maincpu_clk;

    if (metrics_reset) {
        metrics_reset = false;
        reset_performance_metrics(frame_tick);
        return;
    }

    if (measurement_count == MEASUREMENT_FRAME_WINDOW) {
        /* Remove the oldest measurement */
        cumulative_tick_delta -= tick_deltas[next_measurement_index];
        cumulative_clock_delta -= clock_deltas[next_measurement_index];
    } else {
        measurement_count++;
    }

    /* Add this frame's measurement */
    tick_deltas[next_measurement_index] = frame_tick - last_tick;
    clock_deltas[next_measurement_index] = main_cpu_clock - last_clock;

    cumulative_tick_delta += tick_deltas[next_measurement_index];
    cumulative_clock_delta += clock_deltas[next_measurement_index];

    last_tick = frame_tick;
    last_clock = main_cpu_clock;

    /* Calculate our final metrics */
    frame_timespan_seconds = (double)cumulative_tick_delta / tick_per_second();
    clock_delta_seconds = (double)cumulative_clock_delta / cycles_per_sec;

    METRIC_LOCK();

    /* smooth and make public */
    vsync_metric_cpu_percent  = (MEASUREMENT_SMOOTH_FACTOR * vsync_metric_cpu_percent)  + (1.0 - MEASUREMENT_SMOOTH_FACTOR) * (clock_delta_seconds / frame_timespan_seconds * 100.0);
    vsync_metric_emulated_fps = (MEASUREMENT_SMOOTH_FACTOR * vsync_metric_emulated_fps) + (1.0 - MEASUREMENT_SMOOTH_FACTOR) * ((double)measurement_count / frame_timespan_seconds);

    /* printf("%.3f seconds - %0.3f%% cpu, %.3f fps (CLOCK delta: %u)\n", frame_timespan_seconds, vsync_metric_cpu_percent, vsync_metric_emulated_fps, clock_deltas[next_measurement_index]); fflush(stdout); */

    METRIC_UNLOCK();

    /* Get ready for next invoke */
    if (++next_measurement_index == MEASUREMENT_FRAME_WINDOW) {
        next_measurement_index = 0;
    }
}

#ifndef EMU_EX_PLATFORM
void vsync_do_end_of_line(void)
{
    const int microseconds_between_sync = 2 * 1000;

    tick_t tick_between_sync = tick_per_second() / (1000000 / microseconds_between_sync);
    tick_t tick_now;
    tick_t tick_delta;
    tick_t ticks_until_target;

    bool tick_based_sync_timing;

    CLOCK main_cpu_clock = maincpu_clk;
    CLOCK sync_clk_delta;
    double sync_emulated_ticks;

    /* used to preserve the fractional ticks betwen calls */
    static double sync_emulated_ticks_offset;

    /*
     * Ideally the vic chip draw alarm wouldn't be triggered
     * during shutdown but here we are - apply workaround.
     */

    if (archdep_is_exiting()) {
        /* UI thread shutdown code can end up here ... :( */
        if (mainlock_is_vice_thread()) {
            mainlock_yield();
        }

        return;
    }

    /* deal with any accumulated sound immediately */
    tick_based_sync_timing = sound_flush();

    tick_now = tick_now_after(last_sync_tick);

    if (sync_reset) {
        log_message(LOG_DEFAULT, "Sync reset");
        sync_reset = false;
        metrics_reset = true;

        last_sync_emulated_tick = tick_now;
        last_sync_tick = tick_now;
        last_sync_clk = main_cpu_clock;
        sync_target_tick = tick_now;

        return;
    }

    /* how many host ticks since last sync. */
    tick_delta = tick_now - last_sync_tick;

    /* is it time to consider keyboard, joystick ? */
    if (tick_delta >= tick_between_sync) {

        if (warp_enabled) {
            /* During warp we need to periodically allow the UI a chance with the mainlock */
            mainlock_yield();
        } else {
            /*
             * Compare the emulated time vs host time.
             *
             * First, add the emulated clock cycles since last sync.
             */

            sync_clk_delta = main_cpu_clock - last_sync_clk;

            /* amount of host ticks that represents the emulated duration */
            sync_emulated_ticks = (double)tick_per_second() * sync_clk_delta / emulated_clk_per_second;

            /* combine with leftover offset from last sync */
            sync_emulated_ticks += sync_emulated_ticks_offset;

            /* calculate the ideal host tick representing the emulated tick */
            sync_target_tick += sync_emulated_ticks;

            /* Preserve the fractional component for next time */
            sync_emulated_ticks_offset = sync_emulated_ticks - (tick_t)sync_emulated_ticks;

            /*
             * How many host ticks to catch up with emulator?
             *
             * If we are behind the emulator, this will be a giant number
             */

            ticks_until_target = sync_target_tick - tick_now;

            if (ticks_until_target < tick_per_second()) {
                /* Emulation timing / sync is OK. */

                /* If we can't rely on the audio device for timing, slow down here. */
                if (tick_based_sync_timing) {
                    mainlock_yield_and_sleep(ticks_until_target);
                }
            } else if ((tick_t)0 - ticks_until_target > tick_per_second()) {
                /* We are more than a second behind, reset sync and accept that we're not running at full speed. */

                mainlock_yield();

                log_warning(LOG_DEFAULT, "Sync is %.3f ms behind", (double)TICK_TO_MICRO((tick_t)0 - ticks_until_target) / 1000);
                sync_reset = true;
            } else {
                /* We are running slow - make sure we still yield to the UI thread if it's waiting */

                mainlock_yield();
            }
        }

        /* deal with pending user input */
        joystick();

        last_sync_tick = tick_now;
        last_sync_clk = main_cpu_clock;
    }

    /* Do we need to update the thread priority? */
    if (update_thread_priority) {
        update_thread_priority = 0;

#if defined(MACOS_COMPILE)
        vice_macos_set_vice_thread_priority(warp_enabled);
#elif defined(LINUX_COMPILE)
        /* TODO: Linux thread prio stuff, need root or some 'capability' though */
#else
        /* TODO: BSD thread prio stuff */
#endif
    }
}

bool vsync_should_skip_frame(struct video_canvas_s *canvas)
{
    tick_t now = tick_now();

    /*
     * Ideally the draw alarm wouldn't be triggered
     * during shutdown but here we are - apply workaround.
     */

    if (archdep_is_exiting()) {
        /* skip this frame */
        return true;
    }

    /*
     * Limit rendering fps if we're in warp mode.
     * It's ugly enough for dqh to weep but makes warp faster.
     */

    if (warp_enabled) {
        if (now < canvas->warp_next_render_tick) {
            if (now < canvas->warp_next_render_tick - warp_render_tick_interval) {
                /* next render tick is further ahead than it should be */
                canvas->warp_next_render_tick = now + warp_render_tick_interval;
            }
            /* skip this frame */
            return true;
        } else {
            canvas->warp_next_render_tick += warp_render_tick_interval;

            if (canvas->warp_next_render_tick < now) {
                /* Warp rendering is behind, catch up */
                canvas->warp_next_render_tick = now + warp_render_tick_interval;
            }
            /* render this frame */
            return false;
        }
    }

    /* render this frame */
    return false;
}

/* This is called at the end of each screen frame. */
void vsync_do_vsync(struct video_canvas_s *c)
{
    static tick_t last_vsync;

    tick_t now;
    tick_t network_hook_time = 0;

    monitor_vsync_hook();

    /*
     * process everything wich should be done before the synchronisation
     * e.g. OS/2: exit the programm if trigger_shutdown set
     */
    vsyncarch_presync();

    /* Run vsync jobs. */
    if (network_connected()) {
        network_hook_time = tick_now();
    }

    vsync_hook();

    if (network_connected()) {
        /* TODO - re-eval if any of this network stuff makes sense */
        network_hook_time = tick_now_delta(network_hook_time);

        if (network_hook_time > (tick_t)ticks_per_frame) {
            /* next_frame_start += network_hook_time; */
            last_vsync += network_hook_time;
        }
    }

#ifdef DEBUG
    /* switch between recording and playback in history debug mode */
    debug_check_autoplay_mode();
#endif

    now = tick_now_after(last_vsync);
    update_performance_metrics(now);

    vsyncarch_postsync();

#ifdef VSYNC_DEBUG
    log_debug("vsync: start:%lu  delay:%ld  sound-delay:%lf  end:%lu  next-frame:%lu  frame-ticks:%lu",
                now, delay, sound_delay * 1000000, tick_now(), next_frame_start, ticks_per_frame);
#endif

    execute_vsync_callbacks();

    kbdbuf_flush();

    last_vsync = now;
}

#endif
