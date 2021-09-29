/** \file   mainlock.c
 * \brief   VICE mutex used to synchronise access to the VICE api and data
 *
 * The mutex is held most of the time by the thread spawned to run VICE in the background.
 * It is frequently unlocked and relocked to allow the UI thread an opportunity to safely
 * call vice functions and access vice data structures.
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

#ifdef USE_VICE_THREAD

/* #define VICE_MAINLOCK_DEBUG */

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "archdep.h"
#include "log.h"
#include "machine.h"
#include "mainlock.h"
#include "tick.h"
#include "vsyncapi.h"

static volatile bool vice_thread_keepalive = true;

static int ui_thread_waiting = 0;
static int ui_thread_lock_count = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* Used to hand control to the UI */
static pthread_cond_t yield_condition = PTHREAD_COND_INITIALIZER;

/* Used to release control to VICE */
static pthread_cond_t return_condition = PTHREAD_COND_INITIALIZER;

static pthread_t vice_thread;
static bool vice_thread_is_running = false;

static unsigned long tick_per_ms;
static unsigned long start_time;

void mainlock_init(void)
{
    pthread_mutex_lock(&lock);

    vice_thread = pthread_self();
    vice_thread_is_running = true;

    tick_per_ms = tick_per_second() / 1000;
    start_time = tick_now();

    pthread_mutex_unlock(&lock);
}


static void consider_exit(void)
{
    /* Check if the vice thread has been told to die. */
    if (!vice_thread_keepalive) {
        
        /* This needs to be set before unlocking to avoid a shutdown race condition */
        vice_thread_is_running = false;
        
        /* Signal the yield condition in case the main thread is currently waiting for it */
        pthread_cond_signal(&yield_condition);
        pthread_mutex_unlock(&lock);
        
        log_message(LOG_DEFAULT, "VICE thread is exiting");

        archdep_thread_shutdown();

        /* Execution ends here - this function does not return. */
        pthread_exit(NULL);
    }
}


void mainlock_initiate_shutdown(void)
{
    pthread_mutex_lock(&lock);

    if (!vice_thread_keepalive) {
        pthread_mutex_unlock(&lock);
        return;
    }

    log_message(LOG_DEFAULT, "VICE thread initiating shutdown");

    vice_thread_keepalive = false;

    pthread_mutex_unlock(&lock);

    /* If called on the vice thread itself, run the exit code immediately */
    if (pthread_equal(pthread_self(), vice_thread)) {
        consider_exit();
        log_error(LOG_ERR, "VICE thread didn't immediately exit when it should have");
    }
}


/** \brief Provide the UI thread a safe opportunity to call into VICE.
 */
void mainlock_yield_once(void)
{
#ifdef VICE_MAINLOCK_DEBUG
    unsigned long yield_tick = tick_now();
    unsigned long yield_tick_delta_ms;
#endif

    pthread_mutex_lock(&lock);

    /* If the UI thread is not waiting for VICE, we're done here. */
    if (!ui_thread_waiting) {
        pthread_mutex_unlock(&lock);
        return;
    }

    /*
     * The ui thread is waiting to do some work.
     */

    /* Signal the main thread that it can take control of vice. */
    pthread_cond_signal(&yield_condition);

    /* Atomically release the lock and wait for the release signal from the main thread. */
    pthread_cond_wait(&return_condition, &lock);
    
    /* After the UI has had the lock, check if we should exit. */
    consider_exit();
    
    pthread_mutex_unlock(&lock);

#ifdef VICE_MAINLOCK_DEBUG
    yield_tick_delta_ms = tick_delta(yield_tick) / tick_per_ms;
    if (yield_tick_delta_ms > 0) {
        printf("Yielded for %lu ms\n", yield_tick_delta_ms);
    }
#endif
}

/** \brief Enter a period during which the mainlock can freely be obtained.
 */
void mainlock_yield_begin(void)
{
    pthread_mutex_lock(&lock);

    ui_thread_lock_count++;
    
    if (ui_thread_waiting) {
        /*
         * The ui thread is waiting to do some work, wake it up
         */
        pthread_cond_signal(&yield_condition);
    }

    pthread_mutex_unlock(&lock);
}


/** \brief The vice thread takes back ownership of the mainlock.
 */
void mainlock_yield_end(void)
{
    pthread_mutex_lock(&lock);

    ui_thread_lock_count--;

    if (ui_thread_lock_count) {
        pthread_cond_wait(&return_condition, &lock);
    }
    
    /* After the UI *might* have had the lock, check if we should exit. */
    consider_exit();
    
    pthread_mutex_unlock(&lock);
}

/****/

void mainlock_obtain(void)
{
    if (pthread_equal(pthread_self(), vice_thread)) {
        /*
         * Bad - likely the vice thread directly triggered some UI code.
         * That UI code then generated a signal which is then synchronously
         * pushed through to the handler, which tries to obtain the lock.
         * 
         * The solution is ALWAYS to make VICE asynchronously trigger the
         * UI code.
         */
        printf("FIXME! VICE thread is trying to obtain the mainlock!\n"); fflush(stderr);
        return;
    }

    pthread_mutex_lock(&lock);

    /* If we have already obtained the lock, we're done */
    if (ui_thread_lock_count) {
        ui_thread_lock_count++;
        /* printf("lock count now %d, (already locked)\n", ui_thread_lock_count); */

        pthread_mutex_unlock(&lock);
        return;
    }

    /* If there is no vice thread running, we're done */
    if (!vice_thread_is_running) {
        ui_thread_lock_count++;
        pthread_mutex_unlock(&lock);
        return;
    }

    /*
     * There is a vice thread running, we need to wait for it to set the yield condition
     * before we know it's safe to to some work from the ui thread.
     */

    ui_thread_waiting = 1;

    /* Release yield mutex, wait for the VICE thread to signal, and re-obtain the mutex */
    pthread_cond_wait(&yield_condition, &lock);

    /* OK, the UI thread has control of VICE until it signals the return condition via mainlock_release() */
    ui_thread_waiting = 0;
    ui_thread_lock_count++;

    pthread_mutex_unlock(&lock);
}


bool mainlock_is_vice_thread(void)
{
    return pthread_equal(pthread_self(), vice_thread);
}


void mainlock_assert_is_not_vice_thread(void)
{
    assert(!mainlock_is_vice_thread());
}


void mainlock_assert_lock_obtained(void)
{
    pthread_mutex_lock(&lock);

    /* If there is no vice thread running yet, we're ok */
    if (!vice_thread_is_running) {
        pthread_mutex_unlock(&lock);
        return;
    }

    if (pthread_equal(pthread_self(), vice_thread)) {
        /* See detailed comment in mainlock_obtain() */
        printf("FIXME! VICE thread is trying to assert_obtained the mainlock!\n"); fflush(stdout);
        pthread_mutex_unlock(&lock);
        return;
    }

    /* If we have already obtained the lock, we're good */
    if (ui_thread_lock_count) {
        pthread_mutex_unlock(&lock);
        return;
    }

    /* Bad. The UI thread is doing something without holding the mainlock. */
    assert(0);

    pthread_mutex_unlock(&lock);
}


void mainlock_release(void)
{
    pthread_mutex_lock(&lock);

    if (pthread_equal(pthread_self(), vice_thread)) {
        /* See detailed comment in mainlock_obtain() */
        printf("FIXME! VICE thread is trying to release the mainlock!\n"); fflush(stdout);
        return;
    }

    ui_thread_lock_count--;

    if (ui_thread_lock_count) {
        /* printf("lock count now %d, (still locked)\n", ui_thread_lock_count); */
        pthread_mutex_unlock(&lock);
        return;
    }

    pthread_cond_signal(&return_condition);

    pthread_mutex_unlock(&lock);
}

#endif /* #ifdef USE_VICE_THREAD */
