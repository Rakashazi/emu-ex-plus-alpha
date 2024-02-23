/** \file   archdep_rtc_get_centisecond.c
 * \brief   get centiseconds time when gettimeofday() is not available
 * \author  groepaz <groepaz@gmx.net>
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

#ifdef WINDOWS_COMPILE
#include <windows.h>
#endif

#include "archdep_rtc_get_centisecond.h"

#ifdef WINDOWS_COMPILE
int archdep_rtc_get_centisecond(void)
{
    SYSTEMTIME t;

    GetSystemTime(&t);
    return (int)(t.wMilliseconds / 10);
}

#else /* other OS */

#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

int archdep_rtc_get_centisecond(void)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval t;
    gettimeofday(&t, NULL);
    return (int)(t.tv_usec / 10000);
#else
#warning "archdep_rtc_get_centisecond implementation missing"
    return 0;
#endif
}

#endif
