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

#include "archdep_exit.h"
#include "clkguard.h"
#include "cmdline.h"
#include "debug.h"
#include "joy.h"
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
#include "tick.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "ui.h"

#ifdef MACOSX_SUPPORT
#include "macOS-util.h"
#endif

/* public metrics, updated every vsync */
static double vsync_metric_cpu_percent;
static double vsync_metric_emulated_fps;
static int    vsync_metric_warp_enabled;

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

static vsync_callback_t *vsync_callback_queue;
static int vsync_callback_queue_size_max;
static int vsync_callback_queue_size;

/** \brief Call callback_func(callback_param) once at vsync time (or machine reset) */
void vsync_on_vsync_do(vsync_callback_func_t callback_func, void *callback_param)
{
    mainlock_assert_lock_obtained();
    
    /* Grow the queue as needed */
    if (vsync_callback_queue_size == vsync_callback_queue_size_max) {
        vsync_callback_queue_size_max += 1;
        vsync_callback_queue = lib_realloc(vsync_callback_queue, vsync_callback_queue_size_max * sizeof(vsync_callback_t));
    }

    vsync_callback_queue[vsync_callback_queue_size].callback = callback_func;
    vsync_callback_queue[vsync_callback_queue_size].param = callback_param;

    vsync_callback_queue_size++;
}

static void execute_vsync_callbacks(void)
{
    int i;
    
    /* Execute each callback in turn. */
    if (vsync_callback_queue_size) {
        for (i = 0; i < vsync_callback_queue_size; i++) {
            vsync_callback_queue[i].callback(vsync_callback_queue[i].param);
        }
        vsync_callback_queue_size = 0;
    }
}

/* ------------------------------------------------------------------------- */

static int set_timer_speed(int speed);

/* Relative speed of the emulation (%) (negative values target FPS rather than cpu %) */
static int relative_speed;

/* "Warp mode".  If nonzero, attempt to run as fast as possible. */
static int warp_enabled;
static unsigned long warp_render_tick_interval;
static unsigned long warp_next_render_tick;

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

static int set_warp_mode(int val, void *param)
{
    warp_enabled = val ? 1 : 0;

    sound_set_warp_mode(warp_enabled);
    vsync_suspend_speed_eval();
    
    if (warp_enabled) {
        warp_next_render_tick = tick_now() + warp_render_tick_interval;
    }

    update_thread_priority = 1;
    
    return 0;
}


/* Vsync-related resources. */
static const resource_int_t resources_int[] = {
    { "Speed", 100, RES_EVENT_SAME, NULL,
      &relative_speed, set_relative_speed, NULL },
    { "WarpMode", 0, RES_EVENT_STRICT, (resource_value_t)0,
      /* FIXME: maybe RES_EVENT_NO */
      &warp_enabled, set_warp_mode, NULL },
    RESOURCE_INT_LIST_END
};


int vsync_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

/* Vsync-related command-line options. */
static const cmdline_option_t cmdline_options[] =
{
    { "-speed", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Speed", NULL,
      "<percent or negative fps>", "Limit emulation speed to specified value" },
    { "-warp", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WarpMode", (resource_value_t)1,
      NULL, "Enable warp mode" },
    { "+warp", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "WarpMode", (resource_value_t)0,
      NULL, "Disable warp mode" },
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
static double emulated_clk_per_second;

static unsigned long last_sync_emulated_tick;
static unsigned long last_sync_tick;
static CLOCK last_sync_clk;

static unsigned long sync_target_tick;

static int timer_speed = 0;
static int speed_eval_suspended = 1;
static int sync_reset = 1;

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

    ticks_per_frame = tick_per_second() * 100 / cpu_percent / refresh_frequency;
    emulated_clk_per_second = cycles_per_sec * cpu_percent / 100;

    return 0;
}

/* ------------------------------------------------------------------------- */

void vsync_set_machine_parameter(double refresh, long cycles)
{
    refresh_frequency = refresh;
    cycles_per_sec = cycles;
    set_timer_speed(relative_speed);
    vsyncarch_refresh_frequency_changed(refresh_frequency);
}

double vsync_get_refresh_frequency(void)
{
    return refresh_frequency;
}

void vsync_init(void (*hook)(void))
{
    /* Limit warp rendering to 10fps */
    warp_render_tick_interval = tick_per_second() / 10.0;
    
    vsync_hook = hook;
    vsync_suspend_speed_eval();
}

/* FIXME: This function is not needed here anymore, however it is
   called from sound.c and can only be removed if all other ports are
   changed to use similar vsync code. */
int vsync_disable_timer(void)
{
    return 0;
}

/* This should be called whenever something that has nothing to do with the
   emulation happens, so that we don't display bogus speed values. */
void vsync_suspend_speed_eval(void)
{
    /* TODO - Is this needed any more now that late vsync is detected
       in vsync_do_vsync() */
    network_suspend();
    speed_eval_suspended = 1;
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
    *is_warp_enabled = vsync_metric_warp_enabled;
    
    METRIC_UNLOCK();
}

#define MEASUREMENT_SMOOTH_FACTOR 0.99
#define MEASUREMENT_FRAME_WINDOW  250

static void update_performance_metrics(unsigned long frame_time)
{
    static int oldest_measurement_index;
    static int next_measurement_index;
    static int frames_counted;

    /* For knowing the relevant timespan */
    static unsigned long frame_times[MEASUREMENT_FRAME_WINDOW];
    
    /* For measuring emulator cpu cycles per second */
    static CLOCK clocks[MEASUREMENT_FRAME_WINDOW];
    
    /* how many seconds of wallclock time the measurement window covers */
    double frame_timespan_seconds;
    
    /* how many emulated seconds of cpu time have been emulated */
    double clock_delta_seconds;
    
    METRIC_LOCK();
    
    if (sync_reset) {
        /*
         * The emulator is just starting, or resuming from pause, or entering
         * warp, or exiting warp. So we reset the fps calculations. We also
         * throw away this initial measurement, because the emulator is just
         * about to skip the timing sleep and immediately produce the next frame.
         *
         * TODO: Don't reset numbers on unpause, and account for the gap.
         */
        sync_reset = 0;
        
        frame_times[0] = frame_time;
        clocks[0] = maincpu_clk;
        
        next_measurement_index = 1;
        
        if (warp_enabled) {
            /* Don't bother with seed measurements when entering warp mode, just reset */
            oldest_measurement_index = 0;
            frames_counted = 1;
        } else {
            int i;
            /*
             * For normal speed changes, exiting warp etc, initialise with a full set
             * of fake perfect measurements
             */
            for (i = 1; i < MEASUREMENT_FRAME_WINDOW; i++) {
                frame_times[i] = frame_time - ((MEASUREMENT_FRAME_WINDOW - i) * ticks_per_frame);
                clocks[i] = maincpu_clk - (CLOCK)((MEASUREMENT_FRAME_WINDOW - i) * cycles_per_sec / refresh_frequency);
            }
            
            oldest_measurement_index = 1;
            frames_counted = MEASUREMENT_FRAME_WINDOW;
        }
    
        /* The final smoothing function requires that we initialise the public metrics */
        if (timer_speed > 0) {
            vsync_metric_emulated_fps = (double)timer_speed * refresh_frequency / 100.0;
            vsync_metric_cpu_percent  = timer_speed;
        } else {
            vsync_metric_emulated_fps = (0.0 - timer_speed);
            vsync_metric_cpu_percent  = (0.0 - timer_speed) / refresh_frequency * 100;
        }
        
        /* printf("INIT frames_counted %d %.3f seconds - %0.3f%% cpu, %.3f fps\n", frames_counted, frame_timespan_seconds, vsync_metric_cpu_percent, vsync_metric_emulated_fps); fflush(stdout); */

        METRIC_UNLOCK();
        
        return;
    }
    
    /* Capure this frame's measurements */
    frame_times[next_measurement_index] = frame_time;
    clocks[next_measurement_index] = maincpu_clk;

    if(frames_counted == MEASUREMENT_FRAME_WINDOW) {
        if (++oldest_measurement_index == MEASUREMENT_FRAME_WINDOW) {
            oldest_measurement_index = 0;
        }
    } else {
        frames_counted++;
    }
    
    /* Calculate our final metrics */
    frame_timespan_seconds = (double)(frame_time - frame_times[oldest_measurement_index]) / tick_per_second();
    clock_delta_seconds = (double)(maincpu_clk - clocks[oldest_measurement_index]) / cycles_per_sec;

    /* smooth and make public */
    vsync_metric_cpu_percent  = (MEASUREMENT_SMOOTH_FACTOR * vsync_metric_cpu_percent)  + (1.0 - MEASUREMENT_SMOOTH_FACTOR) * (clock_delta_seconds / frame_timespan_seconds * 100.0);
    vsync_metric_emulated_fps = (MEASUREMENT_SMOOTH_FACTOR * vsync_metric_emulated_fps) + (1.0 - MEASUREMENT_SMOOTH_FACTOR) * ((double)(frames_counted - 1) / frame_timespan_seconds);
    vsync_metric_warp_enabled = warp_enabled;
    
    /* printf("frames_counted %d %.3f seconds - %0.3f%% cpu, %.3f fps\n", frames_counted, frame_timespan_seconds, vsync_metric_cpu_percent, vsync_metric_emulated_fps); fflush(stdout); */
    
    /* Get ready for next invoke */
    if (++next_measurement_index == MEASUREMENT_FRAME_WINDOW) {
        next_measurement_index = 0;
    }
    
    METRIC_UNLOCK();
}

#ifndef EMU_EX_PLATFORM
void vsync_do_end_of_line(void)
{
    const int microseconds_between_sync = 2 * 1000;
    
    unsigned long tick_between_sync = tick_per_second() * microseconds_between_sync / 1000000;
    unsigned long tick_now;
    unsigned long tick_delta;
    
    bool tick_based_sync_timing;

    CLOCK sync_clk_delta;
    unsigned long sync_emulated_ticks;
    
    /*
     * Ideally the vic chip draw alarm wouldn't be triggered
     * during shutdown but here we are - apply workaround.
     */
    
    if (archdep_is_exiting()) {
        mainlock_yield_once();
        return;
    }

    /* deal with any accumulated sound immediately */
    tick_based_sync_timing = sound_flush();
    
    tick_now = tick_after(last_sync_tick);

    /*
     * If it's been a long time between scanlines, (such as when paused in
     * debuggeretc) then reset speed eval and sync. Otherwise, the emulator
     * will warp until it catches up, which is rarely good when this far out
     * of sync. This means the emulator will run slower overall than hoped.
     */

    if (!speed_eval_suspended && (tick_now - last_sync_tick) > ticks_per_frame * 5) {
        if (last_sync_tick != 0) {
            log_warning(LOG_DEFAULT, "sync is far too late, resetting sync");
        }
        vsync_suspend_speed_eval();
    }

    if (speed_eval_suspended) {
        last_sync_emulated_tick = tick_now;
        last_sync_tick = tick_now;
        last_sync_clk = maincpu_clk;
        sync_target_tick = tick_now;

        speed_eval_suspended = 0;
        sync_reset = 1;
        return;
    }

    /* how many host ticks since last sync. */
    tick_delta = tick_now - last_sync_tick;
    
    /* is it time to consider keyboard, joystick ? */
    if (tick_delta >= tick_between_sync) {
        
        /* deal with user input */
        joystick();
                
        /* Do we need to slow down the emulation here or can we rely on the audio device? */
        if (tick_based_sync_timing && !warp_enabled) {
            
            /* add the emulated clock cycles since last sync. */
            sync_clk_delta = maincpu_clk - last_sync_clk;
            
            /* amount of host ticks that represents the emulated duration */
            sync_emulated_ticks = (double)tick_per_second() * sync_clk_delta / emulated_clk_per_second;

            /* 
             * We sleep so that our host is blocked for long enough to catch up
             * with how long the emulated machine would have taken.
             */

            sync_target_tick += sync_emulated_ticks;

            /* Some tricky wrap around cases to deal with */
            if (sync_target_tick - tick_now > 0 && sync_target_tick - tick_now < tick_per_second()) {
                tick_sleep(sync_target_tick - tick_now);
            }
        }
        
        last_sync_tick = tick_now;
        last_sync_clk = maincpu_clk;        
    }

    /* Do we need to update the thread priority? */
    if (update_thread_priority) {
        update_thread_priority = 0;

#if defined(MACOSX_SUPPORT)
        vice_macos_set_vice_thread_priority(warp_enabled);
#elif defined(__linux__)
        /* TODO: Linux thread prio stuff, need root or some 'capability' though */
#else
        /* TODO: BSD thread prio stuff */
#endif
    }
}

/* This is called at the end of each screen frame. */
int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
    // static unsigned long next_frame_start = 0;
    static int skipped_redraw_count = 0;
    static unsigned long last_vsync;

    unsigned long now;
    unsigned long network_hook_time = 0;
    // long delay;
    int skip_next_frame = 0;
    
    /*
     * Ideally the vic chip draw alarm wouldn't be triggered
     * during shutdown but here we are - apply workaround.
     */
    
    if (archdep_is_exiting()) {
        return 1;
    }

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
        network_hook_time = tick_delta(network_hook_time);

        if (network_hook_time > (unsigned long)ticks_per_frame) {
            // next_frame_start += network_hook_time;
            last_vsync += network_hook_time;
        }
    }

#ifdef DEBUG
    /* switch between recording and playback in history debug mode */
    debug_check_autoplay_mode();
#endif

    now = tick_after(last_vsync);
    update_performance_metrics(now);

    /*
     * Limit rendering fps if we're in warp mode.
     * It's ugly enough for dqh to weep but makes warp faster.
     */
    
    if (warp_enabled) {
        if (now < warp_next_render_tick) {
            skip_next_frame = 1;
            skipped_redraw_count++;
        } else {
            warp_next_render_tick += warp_render_tick_interval;
            skipped_redraw_count = 0;
        }
    }

    vsyncarch_postsync();

#ifdef VSYNC_DEBUG
    log_debug("vsync: start:%lu  delay:%ld  sound-delay:%lf  end:%lu  next-frame:%lu  frame-ticks:%lu", 
                now, delay, sound_delay * 1000000, tick_now(), next_frame_start, ticks_per_frame);
#endif
    
    execute_vsync_callbacks();
    
    kbdbuf_flush();
    
    last_vsync = now;

    return skip_next_frame;
}
#endif
