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
#include "debug.h"
#include "log.h"
#include "machine.h"
#include "mainlock.h"
#include "vsyncapi.h"

/* This is lock coordinates access to VICE data structures */
static pthread_mutex_t  main_lock        = PTHREAD_MUTEX_INITIALIZER;

/* How many times the UI has recursively obtained the main lock*/
static int              main_lock_obtain_depth;

/* Used to coordinate access to data within mainlock.c */
static pthread_mutex_t  internal_lock    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   ui_waiting_cond  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t   ui_has_lock_cond = PTHREAD_COND_INITIALIZER;
static bool             ui_is_waiting    = false;
static pthread_t        vice_thread;
static bool             vice_thread_keepalive  = true;
static bool             vice_thread_is_running = false;

void mainlock_init(void)
{
}


void mainlock_set_vice_thread(void)
{
    pthread_mutex_lock(&internal_lock);
    vice_thread = pthread_self();
    vice_thread_is_running = true;
    pthread_mutex_unlock(&internal_lock);

    /* The vice thread owns this lock except when explicitly releasing it */
    pthread_mutex_lock(&main_lock);
}


static void consider_exit(void)
{
    /* NASTY - some emulation can continue on the main thread during shutdown. */
    if (!pthread_equal(pthread_self(), vice_thread)) {
        return;
    }

    /* Check if the vice thread has been told to die. */
    pthread_mutex_lock(&internal_lock);
    if (!vice_thread_keepalive) {

        /*
         * The VICE thread will exit!
         */

        /* Setting this lets the UI thread know not to wait for signals in future mainlock_obtain() calls */
        vice_thread_is_running = false;

        if (ui_is_waiting) {
            /* Wake up the UI thread, otherwise it will be waiting forever */
            pthread_cond_signal(&ui_waiting_cond);
        }

        pthread_mutex_unlock(&internal_lock);
        pthread_mutex_unlock(&main_lock);

        log_message(LOG_DEFAULT, "VICE thread is exiting");

        archdep_thread_shutdown();

        pthread_exit(NULL);

        /*
         * EXECUTION DOES NOT REACH HERE - pthread_exit() does not return
         */

        assert(false);

    } else {
        pthread_mutex_unlock(&internal_lock);
    }
}


void mainlock_initiate_shutdown(void)
{
    pthread_mutex_lock(&internal_lock);
    if (!vice_thread_keepalive) {
        /* Already initiated */
        pthread_mutex_unlock(&internal_lock);
        return;
    }
    vice_thread_keepalive = false;
    pthread_mutex_unlock(&internal_lock);

    log_message(LOG_DEFAULT, "VICE thread initiating shutdown");

    /* If called on the vice thread itself, run the exit code immediately */
    if (pthread_equal(pthread_self(), vice_thread)) {
        consider_exit();
        log_error(LOG_ERR, "VICE thread didn't immediately exit when it should have");
    }
}


/** \brief Yield the mainlock and attempt to regain it immediately
 */
void mainlock_yield(void)
{
    mainlock_yield_begin();
    mainlock_yield_end();
}


/** \brief Enter a period during which the mainlock can freely be obtained.
 */
void mainlock_yield_begin(void)
{
    mainlock_assert_is_vice_thread();

    pthread_mutex_unlock(&main_lock);

    /*
     * If the UI thread is already waiting for the mainlock, attempt
     * to wake it immediately and block until it has it. This ensures
     * that thread priorities don't affect sharing of this lock.
     */

    pthread_mutex_lock(&internal_lock);
    if (ui_is_waiting) {
        /* Wake up the UI thread */
        pthread_cond_signal(&ui_waiting_cond);
        /* Block until the UI has the main lock */
        pthread_cond_wait(&ui_has_lock_cond, &internal_lock);
    }
    pthread_mutex_unlock(&internal_lock);
}


/** \brief The vice thread takes back ownership of the mainlock.
 */
void mainlock_yield_end(void)
{
    pthread_mutex_lock(&main_lock);

    /* After the UI *might* have had the lock, check if we should exit. */
    consider_exit();
}

/** \brief Release the mainlock and sleep
 */
void mainlock_yield_and_sleep(tick_t ticks)
{
    mainlock_yield_begin();
    tick_sleep(ticks);
    mainlock_yield_end();
}

/****/

void mainlock_obtain(void)
{
#ifdef DEBUG
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
#endif

    /*
     * The UI may attempt to recursively obtain the mainlock during shutdown
     * and other cases where gtk event handlers recursively trigger other gtk
     * event handlers. This would be fine normally (if using a recursive mutex)
     * however during shutdown we can't be sure that the vice thread is there
     * to signal the condition we are about to block on.
     */

    if (main_lock_obtain_depth++ > 0) {
        return;
    }

    pthread_mutex_lock(&internal_lock);

    if (vice_thread_is_running) {
        /* Block until the VICE thread signals us */
        ui_is_waiting = true;
        pthread_cond_wait(&ui_waiting_cond, &internal_lock);
        ui_is_waiting = false;
    }

    pthread_mutex_unlock(&internal_lock);

    /* Get the main lock */
    pthread_mutex_lock(&main_lock);

    /* Let the VICE thread know we have the mainlock now */
    pthread_cond_signal(&ui_has_lock_cond);
}


bool mainlock_is_vice_thread(void)
{
    return pthread_equal(pthread_self(), vice_thread);
}


void mainlock_release(void)
{
#ifdef DEBUG
    if (pthread_equal(pthread_self(), vice_thread)) {
        /* See detailed comment in mainlock_obtain() */
        printf("FIXME! VICE thread is trying to release the mainlock!\n"); fflush(stdout);
        return;
    }
#endif

    pthread_mutex_unlock(&main_lock);

    main_lock_obtain_depth--;
}

#endif /* #ifdef USE_VICE_THREAD */
