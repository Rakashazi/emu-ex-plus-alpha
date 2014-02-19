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

int vsync_frame_counter;

#include "vice.h"

/* Port me... */
#if !defined(__MSDOS__) || defined(USE_SDLUI)

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "clkguard.h"
#include "cmdline.h"
#include "debug.h"
#include "log.h"
#include "maincpu.h"
#include "machine.h"
#ifdef HAVE_NETWORK
#include "monitor_network.h"
#endif
#include "network.h"
#include "resources.h"
#include "sound.h"
#include "translate.h"
#include "types.h"
#if (defined(WIN32) || defined (HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
#include "videoarch.h"
#endif
#include "vsync.h"
#include "vsyncapi.h"

/* ------------------------------------------------------------------------- */

static int set_timer_speed(int speed);

/* Relative speed of the emulation (%).  0 means "don't limit speed". */
static int relative_speed;

/* Refresh rate.  0 means "auto". */
static int refresh_rate;

/* "Warp mode".  If nonzero, attempt to run as fast as possible. */
int warp_mode_enabled;

/* Dingoo overclocking mode */
#ifdef DINGOO_NATIVE
static int overclock_mode_enabled;
#endif

static int set_relative_speed(int val, void *param)
{
    relative_speed = val;
    sound_set_relative_speed(relative_speed);
    set_timer_speed(relative_speed);

    return 0;
}

static int set_refresh_rate(int val, void *param)
{
    if (val < 0) {
        return -1;
    }

    refresh_rate = val;

    return 0;
}

static int set_warp_mode(int val, void *param)
{
    warp_mode_enabled = val;
    sound_set_warp_mode(warp_mode_enabled);
    set_timer_speed(relative_speed);

    return 0;
}

/* FIXME: Why the hell is this here and not in archdep ? */
#ifdef DINGOO_NATIVE
static int set_overclock_mode(int val, void *param)
{
    overclock_mode_enabled = val;
    set_overclock(val);
    return 0;
}
#endif

/* Vsync-related resources. */
static const resource_int_t resources_int[] = {
    { "Speed", 100, RES_EVENT_SAME, NULL,
      &relative_speed, set_relative_speed, NULL },
    { "RefreshRate", 0, RES_EVENT_STRICT, (resource_value_t)1,
      &refresh_rate, set_refresh_rate, NULL },
    { "WarpMode", 0, RES_EVENT_STRICT, (resource_value_t)0,
      /* FIXME: maybe RES_EVENT_NO */
      &warp_mode_enabled, set_warp_mode, NULL },
#ifdef DINGOO_NATIVE
    { "OverClock", 0, RES_EVENT_STRICT, (resource_value_t)1,
      /* FIXME: maybe RES_EVENT_NO */
      &overclock_mode_enabled, set_overclock_mode, NULL },
#endif
    { NULL }
};

int vsync_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

/* Vsync-related command-line options. */
static const cmdline_option_t cmdline_options[] = {
    { "-speed", SET_RESOURCE, 1,
      NULL, NULL, "Speed", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PERCENT, IDCLS_LIMIT_SPEED_TO_VALUE,
      NULL, NULL },
    { "-refresh", SET_RESOURCE, 1,
      NULL, NULL, "RefreshRate", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_UPDATE_EVERY_VALUE_FRAMES,
      NULL, NULL },
    { "-warp", SET_RESOURCE, 0,
      NULL, NULL, "WarpMode", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_WARP_MODE,
      NULL, NULL },
    { "+warp", SET_RESOURCE, 0,
      NULL, NULL, "WarpMode", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_WARP_MODE,
      NULL, NULL },
    { NULL }
};

int vsync_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/* Maximum number of frames we can skip consecutively when adjusting the
   refresh rate dynamically.  */
#define MAX_SKIPPED_FRAMES        10

/* Number of frames per second on the real machine. */
static double refresh_frequency;

/* Number of clock cycles per seconds on the real machine. */
static long cycles_per_sec;

/* Function to call at the end of every screen frame. */
void (*vsync_hook)(void);

/* ------------------------------------------------------------------------- */

/* static guarantees zero values. */
static long vsyncarch_freq = 0;
static unsigned long now;
static unsigned long display_start;
static long frame_ticks, frame_ticks_orig;

static int timer_speed = 0;
static int speed_eval_suspended = 1;
static int sync_reset = 1;
static CLOCK speed_eval_prev_clk;

/* Initialize vsync timers and set relative speed of emulation in percent. */
static int set_timer_speed(int speed)
{
    speed_eval_suspended = 1;
    vsync_sync_reset();

    if (speed > 0 && refresh_frequency > 0) {
        timer_speed = speed;
        frame_ticks = (long)(vsyncarch_freq / refresh_frequency * 100 / speed);
        frame_ticks_orig = frame_ticks;
    } else {
        timer_speed = 0;
        frame_ticks = 0;
    }

    return 0;
}

/* Display speed (percentage) and frame rate (frames per second). */
static void display_speed(int num_frames)
{
    CLOCK diff_clk;
    double diff_sec;
    double speed_index;
    double frame_rate;

    /* Lie a bit by correcting for sound speed.  This yields a nice
       100%, 50 fps instead of e.g. 98%, 49fps if the sound hardware
       should have a slightly different understanding of time. */
    double factor = timer_speed ? (double)frame_ticks / frame_ticks_orig : 1.0;

    diff_clk = maincpu_clk - speed_eval_prev_clk;
    diff_sec = (double)(signed long)(now - display_start) / vsyncarch_freq
               / factor;
    frame_rate = num_frames / diff_sec;
    speed_index = 100.0 * diff_clk / (cycles_per_sec * diff_sec);

    if (!console_mode && machine_class != VICE_MACHINE_VSID) {
        vsyncarch_display_speed(speed_index, frame_rate, warp_mode_enabled);
    }

    speed_eval_prev_clk = maincpu_clk;
}

static void clk_overflow_callback(CLOCK amount, void *data)
{
    speed_eval_prev_clk -= amount;
}

/* ------------------------------------------------------------------------- */

void vsync_set_machine_parameter(double refresh_rate, long cycles)
{
    refresh_frequency = refresh_rate;
    cycles_per_sec = cycles;
    set_timer_speed(relative_speed);
}

double vsync_get_refresh_frequency(void)
{
    return refresh_frequency;
}

void vsync_init(void (*hook)(void))
{
    vsync_hook = hook;
    vsync_suspend_speed_eval();
    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    vsyncarch_init();

    vsyncarch_freq = vsyncarch_frequency();
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
    network_suspend();
    sound_suspend();
    vsync_sync_reset();
    speed_eval_suspended = 1;
}

/* This resets sync calculation after a "too slow" or "sound buffer
   drained" case. */
void vsync_sync_reset(void)
{
    sync_reset = 1;
}

#ifndef EMUFRAMEWORK_BUILD
/* This is called at the end of each screen frame. It flushes the
   audio buffer and keeps control of the emulation speed. */
int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
    static unsigned long next_frame_start = 0;
    unsigned long network_hook_time = 0;

    /*
     * these are the counters to show how many frames are skipped
     * since the last vsync_display_speed
     */
    static int frame_counter = 0;
    static int skipped_frames = 0;

    /*
     * This are the frames which are skipped in a row
     */
    static int skipped_redraw = 0;

    /* Adjustment of frame output frequency. */
    static unsigned long adjust_start;
    static int frames_adjust;
    static signed long avg_sdelay, prev_sdelay;

    double sound_delay;
    int skip_next_frame;

    signed long delay;
    long frame_ticks_remainder, frame_ticks_integer, compval;

#if (defined(HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
    float refresh_cmp;
    int refresh_div;
#endif

#ifdef HAVE_NETWORK
    /* check if someone wants to connect remotely to the monitor */
    monitor_check_remote();
#endif

    vsync_frame_counter++;

    /*
     * process everything wich should be done before the synchronisation
     * e.g. OS/2: exit the programm if trigger_shutdown set
     */
    vsyncarch_presync();

    /* Run vsync jobs. */
    if (network_connected()) {
        network_hook_time = vsyncarch_gettime();
    }

    vsync_hook();

    if (network_connected()) {
        network_hook_time = vsyncarch_gettime() - network_hook_time;

        if (network_hook_time > (unsigned long)frame_ticks) {
            next_frame_start += network_hook_time;
            now += network_hook_time;
        }
    }

#ifdef DEBUG
    /* switch between recording and playback in history debug mode */
    debug_check_autoplay_mode();
#endif

    /*
     * Update display every two second (pc system time)
     * This has some reasons:
     *  - we have a better statistic in case of a fastly running emulator
     *  - we don't slow down fast emulations by updating this value
     *    too often (eg more then 10 times a second)
     *  - I don't want to have a value jumping around for example
     *    between 99% and 101% if the user chooses 100% (s.above)
     *  - We need some statistict to get an avarage number for the
     *    frame-rate without staticstics it would also jump around
     */
    frame_counter++;

    if (!speed_eval_suspended &&
        (signed long)(now - display_start) >= 2 * vsyncarch_freq) {
        display_speed(frame_counter - skipped_frames);
        display_start = now;
        frame_counter = 0;
        skipped_frames = 0;
    }

    if (been_skipped) {
        skipped_frames++;
    }

    /* Flush sound buffer, get delay in seconds. */
    sound_delay = sound_flush();

    /* Get current time, directly after getting the sound delay. */
    now = vsyncarch_gettime();

    /* Start afresh after pause in frame output. */
    if (speed_eval_suspended) {
        speed_eval_suspended = 0;

        speed_eval_prev_clk = maincpu_clk;

        display_start = now;
        frame_counter = 0;
        skipped_frames = 0;

        next_frame_start = now;
        skipped_redraw = 0;
    }

    /* Start afresh after "out of sync" cases. */
    if (sync_reset) {
        sync_reset = 0;

        adjust_start = now;
        frames_adjust = 0;
        avg_sdelay = 0;
        prev_sdelay = 0;

        frame_ticks = (frame_ticks_orig + frame_ticks) / 2;
    }


    /* This is the time between the start of the next frame and now. */
    delay = (signed long)(now - next_frame_start);
#if (defined(HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
    refresh_cmp = (float)(c->refreshrate / refresh_frequency);
    refresh_div = (int)(refresh_cmp + 0.5f);
    refresh_cmp /= (float)refresh_div;

    if ((timer_speed == 100) && (!warp_mode_enabled) &&
        vsyncarch_vbl_sync_enabled() &&
        (refresh_cmp <= 1.02f) && (refresh_cmp > 0.98f) &&
        (refresh_div == 1)) {
        vsyncarch_verticalblank(c, c->refreshrate, refresh_div);
        skip_next_frame = 0;
        skipped_redraw = 0;
    } else {
#endif
    /*
     * We sleep until the start of the next frame, if:
     *  - warp_mode is disabled
     *  - a limiting speed is given
     *  - we have not reached next_frame_start yet
     *
     * We could optimize by sleeping only if a frame is to be output.
     */
    /*log_debug("vsync_do_vsync: sound_delay=%f  frame_ticks=%d  delay=%d", sound_delay, frame_ticks, delay);*/
    if (!warp_mode_enabled && timer_speed && delay < 0) {
        vsyncarch_sleep(-delay);
    }
#if (defined(HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
    vsyncarch_prepare_vbl();
#endif
    /*
     * Check whether we should skip the next frame or not.
     * Allow delay of up to one frame before skipping frames.
     * Frames are skipped:
     *  - only if maximum skipped frames are not reached
     *  - if warp_mode enabled
     *  - if speed is not limited or we are too slow and
     *    refresh rate is automatic or fixed and needs correction
     *
     * Remark: The time_deviation should be the equivalent of two
     *         frames and must be scaled to make sure, that we
     *         don't start skipping frames before the CPU reaches 100%.
     *         If we are becoming faster a small deviation because of
     *         threading results in a frame rate correction suddenly.
     */
    frame_ticks_remainder = frame_ticks % 100;
    frame_ticks_integer = frame_ticks / 100;
    compval = frame_ticks_integer * 3 * timer_speed
              + frame_ticks_remainder * 3 * timer_speed / 100;
    if (skipped_redraw < MAX_SKIPPED_FRAMES
        && (warp_mode_enabled
            || (skipped_redraw < refresh_rate - 1)
            || ((!timer_speed || delay > compval)
                && !refresh_rate
                )
            )
        ) {
        skip_next_frame = 1;
        skipped_redraw++;
    } else {
        skip_next_frame = 0;
        skipped_redraw = 0;
    }
#if (defined(HAVE_OPENGL_SYNC)) && !defined(USE_SDLUI)
}
#endif

    /*
     * Check whether the hardware can keep up.
     * Allow up to 0,25 second error before forcing a correction.
     */
    if ((signed long)(now - next_frame_start) >= vsyncarch_freq / 8) {
#if !defined(__OS2__) && !defined(DEBUG)
        if (!warp_mode_enabled && relative_speed) {
            log_warning(LOG_DEFAULT, "Your machine is too slow for current settings!");
        }
#endif
        vsync_sync_reset();
        next_frame_start = now;
    }

    /* Adjust frame output frequency to match sound speed.
       This only kicks in for cycle based sound and SOUND_ADJUST_EXACT. */
    if (frames_adjust < INT_MAX) {
        frames_adjust++;
    }

    /* Adjust audio-video sync */
    if (!network_connected()
        && (signed long)(now - adjust_start) >= vsyncarch_freq / 5) {
        signed long adjust;
        avg_sdelay /= frames_adjust;
        /* Account for both relative and absolute delay. */
        adjust = (avg_sdelay - prev_sdelay + avg_sdelay / 8) / frames_adjust;
        /* Maximum adjustment step 1%. */
        if (labs(adjust) > frame_ticks / 100) {
            adjust = adjust / labs(adjust) * frame_ticks / 100;
        }
        frame_ticks -= adjust;

        frames_adjust = 0;
        prev_sdelay = avg_sdelay;
        avg_sdelay = 0;

        adjust_start = now;
    } else {
        /* Actual sound delay is sound delay minus vsync delay. */
        signed long sdelay = (signed long)(sound_delay * vsyncarch_freq);
        avg_sdelay += sdelay;
    }

    next_frame_start += frame_ticks;

    vsyncarch_postsync();
#if 0
    FILE *fd = fopen("latencylog.txt", "a");
    fprintf(fd, "%d %ld %ld %lf\n",
            vsync_frame_counter, frame_ticks, delay, sound_delay * 1000000);
    fclose(fd);
#endif
    return skip_next_frame;
}
#endif

#if defined (HAVE_OPENGL_SYNC) && !defined(USE_SDLUI)

static unsigned long last = 0;
static unsigned long nosynccount = 0;

void vsyncarch_verticalblank(video_canvas_t *c, float rate, int frames)
{
    unsigned long nowi, lastx, max, frm, vbl;

    if (c->refreshrate <= 0.0f) {
        return;
    }

    nowi = vsyncarch_frequency();

    /* calculate counter cycles per frame */
    frm = (unsigned long)((float)(nowi * frames) / rate);

    nowi = vsyncarch_gettime();

    lastx = last - (frm * nosynccount);
    max = (frm * 7) >> 3;
    vbl = 0;
    while (max >= (nowi - lastx)) {
        vsyncarch_sync_with_raster(c);
        nowi = vsyncarch_gettime();
        vbl = 1;
    }
    if ((!vbl) && (nosynccount < 16)) {
        nosynccount++;
    } else {
        last = nowi;
        nosynccount = 0;
    }
}

void vsyncarch_prepare_vbl(void)
{
    /* keep vertical blank data prepared */
    last = vsyncarch_gettime();
    nosynccount = 0;
}

#endif /* defined (HAVE_OPENGL_SYNC) && !defined(USE_SDLUI) */

#endif
