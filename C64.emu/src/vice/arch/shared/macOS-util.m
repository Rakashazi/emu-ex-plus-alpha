/**
 * \file macOS-util.m
 * \brief A collection of little macOS helpers.
 *
 * Calling Objective-C code from C is possible, but not very readable.
 * Nicer to just plop a few readable functions in here.
 *
 * \author David Hogan <david.q.hogan@gmail.com>
 */

/* This file is part of VICE, the Versatile Commodore Emulator.
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

#import "vice.h"

#import "macOS-util.h"

#import <Cocoa/Cocoa.h>
#import <mach/mach.h>
#import <mach/mach_time.h>
#import <pthread.h>
#import <os/log.h>
#import <os/signpost.h>

#import "log.h"
#import "resources.h"

#define NANOS_PER_MICRO 1000ULL
#define MICROS_PER_SEC  1000000ULL

#ifdef USE_GTK3UI
/* For some reason this isn't in the GDK quartz headers */
id gdk_quartz_window_get_nswindow (GdkWindow *window);
#endif

void vice_macos_set_main_thread(void)
{
    /*
     * Cocoa doesn't behave if it doesn't know that it's multithreaded.
     * Starting a new thread via the NSThread interface is enough to flip
     * the bit.
     */

    [NSThread detachNewThreadSelector: @selector(class)
                             toTarget: [NSObject class]
                           withObject: nil];

    /*
     * Disable app-nap
     */
    [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityUserInitiated | NSActivityLatencyCritical
                                                   reason: @"Accurate emulation requires low latency access to resources."];

    /* The main thread benefits from interactive response levels */
    [[NSThread currentThread] setQualityOfService: NSQualityOfServiceUserInteractive];
}

static void move_pthread_to_normal_scheduling_class(pthread_t pthread)
{
    /*
     * See Chromium source file platform_thread_mac.mm for more examples
     */

    kern_return_t result;
    mach_port_t thread_id = pthread_mach_thread_np(pthread);

    thread_extended_policy_data_t extended_policy;
    extended_policy.timeshare = 1;
    result =
        thread_policy_set(
            thread_id,
            THREAD_EXTENDED_POLICY,
            (thread_policy_t)&extended_policy,
            THREAD_EXTENDED_POLICY_COUNT);

    if (result != KERN_SUCCESS) {
        mach_error("thread_policy_set:", result);
    }

    thread_standard_policy_data_t standard_policy;

    result =
        thread_policy_set(
            thread_id,
            THREAD_STANDARD_POLICY,
            (thread_policy_t)&standard_policy,
            THREAD_STANDARD_POLICY_COUNT);

    if (result != KERN_SUCCESS) {
        mach_error("thread_policy_set:", result);
    }
}

static void move_pthread_to_realtime_scheduling_class(pthread_t pthread, int period_microseconds, int typical_work_microseconds, int max_work_microseconds)
{
    /*
     * See Chromium source file platform_thread_mac.mm for more examples
     */

    kern_return_t result;
    mach_port_t thread_id = pthread_mach_thread_np(pthread);

    thread_extended_policy_data_t extended_policy;
    extended_policy.timeshare = 0;
    result =
        thread_policy_set(
            thread_id,
            THREAD_EXTENDED_POLICY,
            (thread_policy_t)&extended_policy,
            THREAD_EXTENDED_POLICY_COUNT);

    if (result != KERN_SUCCESS) {
        mach_error("thread_policy_set:", result);
    }

    /*
     * https://developer.apple.com/library/archive/technotes/tn2169/_index.html
     */

    mach_timebase_info_data_t timebase_info;
    mach_timebase_info(&timebase_info);

    double clock2abs = ((double)timebase_info.denom / (double)timebase_info.numer) * NANOS_PER_MICRO;

    thread_time_constraint_policy_data_t realtime_policy;
    realtime_policy.period      = (uint32_t)(clock2abs * period_microseconds);
    realtime_policy.computation = (uint32_t)(clock2abs * typical_work_microseconds);
    realtime_policy.constraint  = (uint32_t)(clock2abs * max_work_microseconds);
    realtime_policy.preemptible = FALSE;

    result =
        thread_policy_set(
            thread_id,
            THREAD_TIME_CONSTRAINT_POLICY,
            (thread_policy_t)&realtime_policy,
            THREAD_TIME_CONSTRAINT_POLICY_COUNT);

    if (result != KERN_SUCCESS) {
        mach_error("thread_policy_set:", result);
    }
}

void vice_macos_set_vice_thread_priority(bool warp_enabled)
{
    [[NSThread currentThread] setThreadPriority: 1.0];

    if (warp_enabled) {
        /* macOS doesn't like us warping in realtime, drifts down to 50% performance */
        move_pthread_to_normal_scheduling_class(pthread_self());
    } else {
        /* typically vice thread will run for a couple ms before blocking, but lets not penalise it for running longer */
        move_pthread_to_realtime_scheduling_class(pthread_self(), 0, MICROS_PER_SEC / 60, MICROS_PER_SEC / 60);
    }
}

void vice_macos_set_render_thread_priority(void)
{
    [[NSThread currentThread] setThreadPriority: 1.0];

    /* Likely a 60fps system, passing rendered buffer to OpenGL shouldn't take more than a couple of ms. */
    move_pthread_to_realtime_scheduling_class(pthread_self(), 0, MICROS_PER_SEC / 1000 * 5, MICROS_PER_SEC / 1000 * 17);
}

#ifdef USE_GTK3UI
void vice_macos_get_widget_frame_and_content_rect(GtkWidget *widget, CGRect *native_frame, CGRect *content_rect)
{
    id native_window  = gdk_quartz_window_get_nswindow(gtk_widget_get_window(widget));
    id content_view   = [native_window contentView];

    *native_frame = [native_window frame];
    *content_rect = [content_view frame];
}
#endif
