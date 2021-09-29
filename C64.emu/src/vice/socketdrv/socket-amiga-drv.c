/*
 * socket-amiga-drv.c
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

#define __USE_INLINE__

#include "archdep.h"

#include "socketimpl.h"

#ifndef AMIGA_OS4
struct Library *SocketBase;
#endif

int archdep_network_init(void)
{
#ifndef AMIGA_OS4
    if (SocketBase == NULL) {
        SocketBase = OpenLibrary("bsdsocket.library", 3);
        if (SocketBase == NULL) {
            return -1;
        }
    }
#endif

    return 0;
}

void archdep_network_shutdown(void)
{
#ifndef AMIGA_OS4
    if (SocketBase != NULL) {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
#endif
}
