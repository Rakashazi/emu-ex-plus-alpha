/** \file   signals.c
 * \brief   Architecture-dependant signal handling
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "archdep.h"
#include "archdep_signals.h"
#include "signals.h"

/** \brief  Initialize signal handling
 *
 *  Used once at init time to setup all signal handlers.
 *
 *  \param[in]  do_core_dumps   do core dumps. appearantly
*/
void signals_init(int do_core_dumps)
{
    archdep_signals_init(do_core_dumps);
}

/*
 * These two are used for socket send/recv. in this case we might
 * get SIGPIPE if the connection is unexpectedly closed.
 */


/** \brief  Call another function
 */
void signals_pipe_set(void)
{
    archdep_signals_pipe_set();
}


/** \brief  Call another function
 */
void signals_pipe_unset(void)
{
    archdep_signals_pipe_unset();
}
