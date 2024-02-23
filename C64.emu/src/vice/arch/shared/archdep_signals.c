/** \file   archdep_signals.c
 * \brief   Signal handling
 *
 * \author  Ettore Perazzoli <ettore@comm2000.it>
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#include "log.h"

#include <stdlib.h>
#include <signal.h>

#include "archdep_defs.h"

#include "archdep_signals.h"

#ifdef UNIX_COMPILE

typedef void (*signal_handler_t)(int);

static signal_handler_t old_pipe_handler;


/* Seems RETSIGTYPE could just be void */

static RETSIGTYPE break64(int sig)
{
    log_message(LOG_DEFAULT, "VICE Received signal %d, exiting.", sig);
    exit(1);
}


/** \brief   Set up signal handlers at emu init
 *
 * \param[in]   do_core_dumps   Do core dumps
 */
void archdep_signals_init(int do_core_dumps)
{
    if (!do_core_dumps) {
        signal(SIGPIPE, break64);
    }
}


/*
    these two are used for socket send/recv. in this case we might
    get SIGPIPE if the connection is unexpectedly closed.
*/
void archdep_signals_pipe_set(void)
{
    old_pipe_handler = signal(SIGPIPE, SIG_IGN);
}


void archdep_signals_pipe_unset(void)
{
    signal(SIGPIPE, old_pipe_handler);

}

#else

/* Non-unix I suppose */

void archdep_signals_pipe_set(void) { /* NOP */ }
void archdep_signals_pipe_unset(void) { /* NOP */ }
void archdep_signals_init(int do_core_dumps) { /* NOP */ }

#endif


