/** \file   archdep_socketpeek.c
 * \brief   socketpeek function to avoid ioctl()'s
 *
 * \author  pottendo <pottendo@gmx.net>
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

#include "archdep_socketpeek.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_WINSOCK_H
#include <windows.h>
#include <winsock.h>
#endif

#if defined(HAVE_WINSOCK_H) && defined(FIONREAD)

int archdep_socketpeek(int fd, int *bytes_available)
{
    return ioctlsocket((SOCKET) fd, FIONREAD, (u_long *)bytes_available);
}

#elif defined(HAVE_SYS_IOCTL_H) && defined(FIONREAD)

int archdep_socketpeek(int fd, int *bytes_available)
{
    return ioctl(fd, FIONREAD, bytes_available);
}

#else

int archdep_socketpeek(int fd, int *bytes_available)
{
    return -1; /* not implemented */
}

#endif
