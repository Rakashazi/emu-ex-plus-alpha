/** \file   archdep_usleep.c
 * \brief   usleep replacements for OS'es that don't support usleep
 *
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

#include <stdint.h>

#ifdef WINDOWS_COMPILE
# include <windows.h>
#endif

#ifdef UNIX_COMPILE
# include <time.h>
#endif

#include "archdep_usleep.h"

#ifdef WINDOWS_COMPILE

/* Provide a usleep replacement */
void archdep_usleep(uint64_t usec)
{
    uint64_t time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *) &time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do {
        QueryPerformanceCounter((LARGE_INTEGER *) &time2);
    } while((time2-time1) < usec);
}

#endif


#ifdef UNIX_COMPILE

void archdep_usleep(uint64_t usec)
{
    struct timespec req = { 0, (long)usec * 1000 };

    nanosleep(&req, NULL);
}

#endif
