/** \file   tick.c
 * \brief   Relating to the management of time.
 *
 * \author  David Hogan <david.q.hogan@gmail.com>
 */

/*
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

#include "mainlock.h"
#include "tick.h"

#ifdef WIN32_COMPILE
#   include <windows.h>
#elif defined(HAVE_NANOSLEEP)
#   include <time.h>
#else
#   include <unistd.h>
#   include <errno.h>
#   include <sys/time.h>
#endif

#ifdef MACOSX_SUPPORT
#   include <mach/mach.h>
#   include <mach/mach_time.h>
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

/* ------------------------------------------------------------------------- */

#ifndef WIN32_COMPILE
#   ifdef HAVE_NANOSLEEP
#       define TICKSPERSECOND  1000000000L  /* Nanoseconds resolution. */
#       define TICKSPERMSEC    1000000L
#       define TICKSPERUSEC    1000L
#       define TICKSPERNSEC    1L
#   else
#       define TICKSPERSECOND  1000000L     /* Microseconds resolution. */
#       define TICKSPERMSEC    1000L
#       define TICKSPERUSEC    1L
#   endif
#endif

#ifdef WIN32_COMPILE
static LARGE_INTEGER timer_frequency;
static HANDLE wait_timer;
#endif

void tick_init(void)
{
#ifdef WIN32_COMPILE
    QueryPerformanceFrequency(&timer_frequency);

    wait_timer = CreateWaitableTimer(NULL, TRUE, NULL);
#endif
}

unsigned long tick_per_second(void)
{
#ifdef WIN32_COMPILE
    return timer_frequency.QuadPart;
#else
    return TICKSPERSECOND;
#endif
}

/* Get time in timer units. */
unsigned long tick_now(void)
{
#ifdef WIN32_COMPILE
    LARGE_INTEGER time_now;
    
    QueryPerformanceCounter(&time_now);

    return time_now.QuadPart;

#elif defined(HAVE_NANOSLEEP)
#   ifdef MACOSX_SUPPORT
        static uint64_t factor = 0;
        uint64_t time = mach_absolute_time();
        if (!factor) {
            mach_timebase_info_data_t info;
            mach_timebase_info(&info);
            factor = info.numer / info.denom;
        }
        return time * factor;
#   else
        struct timespec now;
#       if defined(__linux__)
            clock_gettime(CLOCK_MONOTONIC_RAW, &now);
#       elif defined(__FreeBSD__)
            clock_gettime(CLOCK_MONOTONIC_PRECISE, &now);
#       else
            clock_gettime(CLOCK_MONOTONIC, &now);
#       endif
        return (TICKSPERSECOND * now.tv_sec) + (TICKSPERNSEC * now.tv_nsec);
#   endif
#else
    /* this is really really bad, we should never use the wallclock
       see: https://blog.habets.se/2010/09/gettimeofday-should-never-be-used-to-measure-time.html */
    struct timeval now;
    gettimeofday(&now, NULL);
    return (TICKSPERSECOND * now.tv_sec) + (TICKSPERUSEC * now.tv_usec);
#endif
}

#if 0
/* Sleep a number of timer units. */
void tick_sleep(unsigned long ticks)
{
    /* do this asap. */
    unsigned long before_yield_tick = tick_now();
    
    unsigned long target_tick = before_yield_tick + ticks;
    unsigned long after_yield_tick;
    unsigned long adjusted_tick;

    /* Since we're about to sleep, give another thread a go of the lock */
    mainlock_yield_once();

    after_yield_tick = tick_after(before_yield_tick);

    /* Adjust ticks to account for the yield time */
    adjusted_tick = target_tick - after_yield_tick;

    /* Since we yielded the lock to the UI, maybe for a while, Check if we still need to sleep */
    if (adjusted_tick > ticks) {
        return;
    }
    
#ifdef WIN32_COMPILE
    LARGE_INTEGER timeout;

    timeout.QuadPart = 0LL - adjusted_tick;

    SetWaitableTimer(wait_timer, &timeout, 0, NULL, NULL, 0);
    WaitForSingleObject(wait_timer, INFINITE);

#elif defined(HAVE_NANOSLEEP)
    struct timespec ts;

    if (ticks < TICKSPERSECOND) {
        ts.tv_sec = 0;
        ts.tv_nsec = ticks;
    } else {
        ts.tv_sec = ticks / TICKSPERSECOND;
        ts.tv_nsec = ticks % TICKSPERSECOND;
    }

    nanosleep(&ts, NULL);

#else
    if (usleep(ticks) == -EINVAL) {
        usleep(999999);
    }
#endif
}
#endif

unsigned long tick_after(unsigned long previous_tick)
{
    /*
     * Fark, high performance counters, called from different threads / cpus, can be off by 1 tick.
     * 
     *    "When you compare performance counter results that are acquired from different
     *     threads, consider values that differ by ± 1 tick to have an ambiguous ordering.
     *     If the time stamps are taken from the same thread, this ± 1 tick uncertainty
     *     doesn't apply. In this context, the term tick refers to a period of time equal
     *     to 1 ÷ (the frequency of the performance counter obtained from
     *     QueryPerformanceFrequency)."
     * 
     * https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps#guidance-for-acquiring-time-stamps
     */

    unsigned long after = tick_now();

    if (after == previous_tick - 1) {
        after = previous_tick;
    }
    
    return after;
}

unsigned long tick_delta(unsigned long previous_tick)
{
    return tick_after(previous_tick) - previous_tick;
}
