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
#include "tick.h"
#include "vsyncapi.h"

static volatile bool vice_thread_keepalive = true;

static pthread_mutex_t lock;

static pthread_t vice_thread;
static bool vice_thread_is_running = false;

void mainlock_init(void)
{
    pthread_mutexattr_t lock_attributes;
    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_settype(&lock_attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock, &lock_attributes);
}


void mainlock_set_vice_thread(void)
{
    /* The vice thread owns this lock except when explicitly releasing it */
    pthread_mutex_lock(&lock);

    vice_thread = pthread_self();
    vice_thread_is_running = true;
}


static void consider_exit(void)
{
    /* NASTY - some emulation can continue on the main thread during shutdown. */
    if (!pthread_equal(pthread_self(), vice_thread)) {
        return;
    }

    /* Check if the vice thread has been told to die. */
    if (!vice_thread_keepalive) {
        
        /* This needs to be set before unlocking to avoid a shutdown race condition */
        vice_thread_is_running = false;
        
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
    pthread_mutex_unlock(&lock);
}


/** \brief The vice thread takes back ownership of the mainlock.
 */
void mainlock_yield_end(void)
{
    pthread_mutex_lock(&lock);
    
    /* After the UI *might* have had the lock, check if we should exit. */
    consider_exit();
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

    pthread_mutex_lock(&lock);
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
    
    pthread_mutex_unlock(&lock);
}

#endif /* #ifdef USE_VICE_THREAD */
