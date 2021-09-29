/*
 * socketdrv.c - Network socket driver
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#include <stdio.h>

#include "archdep.h"


#ifndef HAVE_NETWORK
#error No HAVE_NETWORK
#endif

#ifdef AMIGA_SUPPORT
#include "socket-amiga-drv.c"
#define NETWORK_SUPPORT_HANDLED
#endif

#ifdef WIN32_COMPILE
#include "socket-win32-drv.c"
#define NETWORK_SUPPORT_HANDLED
#endif

#ifndef NETWORK_SUPPORT_HANDLED
int archdep_network_init(void)
{
    /* Nothing to be done */
    return 0;
}

void archdep_network_shutdown(void)
{
    /* Nothing to be done */
}
#endif
