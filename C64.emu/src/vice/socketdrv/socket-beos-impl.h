/*! \file socket-beos-impl.h \n
 *  \author Spiro Trikaliotis\n
 *  \brief  Abstraction from network sockets.
 *
 * socket-beos-impl.h - Abstraction from network sockets. BeOS implementation.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 *
 * based on code from network.c written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifndef VICE_SOCKET_BEOS_IMPL_H
#define VICE_SOCKET_BEOS_IMPL_H

#ifdef HAVE_NETWORK
 
#include <sys/time.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <ByteOrder.h>

#ifdef __HAIKU__
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

typedef int SOCKET;
typedef struct timeval TIMEVAL;

/* HACK: Haiku doesn't declare closesocket() in any standard header, so the
 *       following hack takes care of that. Since Haiku is fairly POSIX
 *       compatible this works. But feel free to solve this properly (compyx)
 */
#ifdef __HAIKU__
#  ifndef closesocket
#    include <unistd.h>
#    define closesocket(FD) close(FD)
#  endif
#endif

#ifndef __HAIKU__
typedef unsigned long in_addr_t;
#endif

#define PF_INET AF_INET
#define INVALID_SOCKET -1
#define HAVE_HTONS
#define HAVE_HTONL

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)-1)
#endif

#endif /* #ifdef HAVE_NETWORK */

#endif /* #ifndef VICE_SOCKET_BEOS_IMPL_H */
