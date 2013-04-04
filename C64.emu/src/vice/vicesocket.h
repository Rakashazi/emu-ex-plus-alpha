/*! \file vicesocket.h \n
 *  \author Spiro Trikaliotis\n
 *  \brief  Abstraction from network sockets.
 *
 * socket.h - Abstraction from network sockets.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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

#ifndef VICE_SOCKET_H
#define VICE_SOCKET_H

#include "types.h"

typedef struct vice_network_socket_s vice_network_socket_t;

typedef struct vice_network_socket_address_s vice_network_socket_address_t;

vice_network_socket_t * vice_network_server(const vice_network_socket_address_t * server_address);
vice_network_socket_t * vice_network_client(const vice_network_socket_address_t * server_address);

vice_network_socket_address_t * vice_network_address_generate(const char * address, unsigned short port);
void vice_network_address_close(vice_network_socket_address_t *);

vice_network_socket_t * vice_network_accept(vice_network_socket_t * sockfd);

int vice_network_socket_close(vice_network_socket_t * sockfd);

int vice_network_send(vice_network_socket_t * sockfd, const void * buffer, size_t buffer_length, int flags);
int vice_network_receive(vice_network_socket_t * sockfd, void * buffer, size_t buffer_length, int flags);

int vice_network_select_poll_one(vice_network_socket_t * readsockfd);

int vice_network_get_errorcode(void);

#endif /* VICE_SOCKET_H */
