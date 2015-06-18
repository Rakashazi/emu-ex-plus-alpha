/*! \file socket.c \n
 *  \author Spiro Trikaliotis\n
 *  \brief  Abstraction from network sockets.
 *
 * socket.c - Abstraction from network sockets.
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

#include "vice.h"

#ifdef HAVE_NETWORK

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "socketimpl.h"
#include "vicesocket.h"
#include "signals.h"

#ifndef HAVE_SOCKLEN_T
typedef size_t socklen_t;
#endif

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

/*! \brief determine the number of elements of an array

 \param _x
   The array of which the size is to be determined

 \remark
   The count of elements of the array

 \remark
   Make sure to really pass an array, not a pointer, as _x
*/
#define arraysize(_x) ( sizeof _x / sizeof _x[0] )

/*! \internal \brief Access to socket addresses
 *
 * This union is used to access socket addresses.
 * It replaces the casts needed otherwise.
 *
 * Furthermore, it ensures we have always enough
 * memory for any type needed. (sizeof struct sockaddr_in6
 * is bigger than struct sockaddr).
 *
 * Note, however, that there is no consensus if the C standard
 * guarantees that all union members start at the same
 * address. There are arguments for and against this.
 *
 * However, in practice, this approach works.
 */
union socket_addresses_u {
    struct sockaddr generic;     /*!< the generic type needed for calling the socket API */

#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    struct sockaddr_un local;    /*!< an Unix Domain Socket (file system) socket address */
#endif

    struct sockaddr_in ipv4;     /*!< an IPv4 socket address */

#ifdef HAVE_IPV6
    struct sockaddr_in6 ipv6;    /*!< an IPv6 socket address */
#endif
};

/*! \internal \brief opaque structure describing an address for use with socket functions
 *
 */
struct vice_network_socket_address_s {
    unsigned int used;      /*!< 1 if this entry is being used, 0 else.
                             * This is used for debugging the buffer
                             * allocation strategy.
                             */
    int domain;             /*!< the address family (AF_INET, ...) of this address */
    int protocol;           /*!< the protocol of this address. This can be used to distinguish between different types of an address family. */
#ifdef HAVE_SOCKLEN_T
    socklen_t len;          /*!< the length of the socket address */
#else
    int len;
#endif
    union socket_addresses_u address; /* the socket address */
};

/*! \internal \brief opaque structure describing a socket */
struct vice_network_socket_s {
    SOCKET sockfd;           /*!< the socket handle */
    vice_network_socket_address_t address; /*!< place for an address. It is updated on accept(). */
    unsigned int used;      /*!< 1 if this entry is being used, 0 else.
                             * This is used for debugging the buffer
                             * allocation strategy.
                             */
};

#ifndef HAVE_HTONL
# ifndef htonl
/*! \internal \brief convert a long from host order into network order

 \param ip
   The value in host order which is to be converted into network order

 \return
   The value in network order

 \remark
   This implementation is only used if the architecture does
   not already provide it.

*/
static unsigned int htonl(unsigned int ip)
{
#  ifdef WORDS_BIGENDIAN
    return ip;
#  else /* !WORDS_BIGENDIAN */
    unsigned int ip2;

    ip2 = ((ip >> 24) & 0xff) + (((ip >> 16) & 0xff) << 8) + (((ip >> 8) & 0xff) << 16) + ((ip & 0xff) << 24);
    return ip2;
#  endif /* WORDS_BIGENDIAN */
}
# endif /* !htonl */
#endif /* !HAVE_HTONL */

#ifndef HAVE_HTONS
# ifndef htons
/*! \internal \brief convert a short from host order into network order

 \param ip
   The value in host order which is to be converted into network order

 \return
   The value in network order

 \remark
   This implementation is only used if the architecture does
   not already provide it.

*/
static unsigned short htons(unsigned short ip)
{
#  ifdef WORDS_BIGENDIAN
    return ip;
#  else /* !WORDS_BIGENDIAN */
    unsigned short ip2;

    ip2 = ((ip >> 8) & 0xff) + ((ip & 0xff) << 8);
    return ip2;
#  endif /* WORDS_BIGENDIAN */
}
# endif /* !htons */
#endif /* !HAVE_HTONS */

/*! \internal \brief a memory pool for network addresses */
static vice_network_socket_address_t address_pool[16] = { { 0 } };
/*! \internal \brief usage bit pattern for address_pool */
static unsigned int address_pool_usage = 0;

/*! \internal \brief a memory pool for sockets */
static vice_network_socket_t socket_pool[16] = { { 0 } };
/*! \internal \brief usage bit pattern for socket_pool */
static unsigned int socket_pool_usage = 0;

/*! \internal \brief Get the next free entry of a pool

  \param PoolUsage
     Pointer to a bit pattern that contains the allocations
     of the pool.

  \return
     The next free pool entry, or -1 if there is none left.

  \remark
     A 1 bit marks a used pool entry, a 0 bit a free pool entry.

  \remark
     In the current implementation, this function is restricted
     to 16 entries.
*/
static int get_new_pool_entry(unsigned int * PoolUsage)
{
    int next_free = -1;

    static int nextentry[] = {
        0, 1, 0, 2, 0, 1, 0, 3,
        0, 1, 0, 2, 0, 1, 0, -1
    };

    next_free = nextentry[*PoolUsage & 0x0f];
    if (next_free < 0) {
        next_free = nextentry[(*PoolUsage >> 4) & 0x0f];
        if (next_free >= 0) {
            next_free += 4;
        } else {
            next_free = nextentry[(*PoolUsage >> 8) & 0x0f];
            if (next_free >= 0) {
                next_free += 8;
            } else {
                next_free = nextentry[(*PoolUsage >> 12) & 0x0f];
                if (next_free >= 0) {
                    next_free += 12;
                }
            }
        }
    }

    if (next_free >= 0) {
        *PoolUsage |= 1u << next_free;
    }

    return next_free;
}

/*! \internal \brief Get a free memory area for a socket

  \param sockfd
     socket handle as initialisation value.

  \return
     NULL on error;
     else, a pointer to an empty vice_network_socket_t structure

  \remark
     If not used anymore, the returned pointer must be freed
     with a call to vice_network_socket_close().
*/
static vice_network_socket_t * vice_network_alloc_new_socket(SOCKET sockfd)
{
    vice_network_socket_t * return_address = NULL;
    int i = get_new_pool_entry(&socket_pool_usage);

    if (i >= arraysize(socket_pool)) {
        i = -1;
    }

    assert(i >= 0);

    if (i >= 0) {
        assert(socket_pool[i].used == 0);

        return_address = &socket_pool[i];
        memset(return_address, 0, sizeof *return_address);
        return_address->used = 1;
        return_address->sockfd = sockfd;
    }

    return return_address;
}

/*! \internal \brief Initialise networking
 *
 * This function initialises networking. It ensures
 * that the initialisation is performed only once.
 *
 * \return
 *   0 on success, else -1.
 *
 * \remark
 *   If this function returns with an error, it is
 *   likely that all subsequent networking functions
 *   will fail.
 *
 * \todo
 *   Currently, archdep_network_shutdown() is never called!
 */
static int socket_init(void)
{
    static int init_done = 0;

    if (init_done) {
        return 0;
    }

    init_done = 1;

    return archdep_network_init();
}

/*! \internal \brief Initialise a socket address structure
 *
 * This function initialises a socket address structure.
 *
 * Regardless of the structure was used before or not, after
 * this call, the memory contents are as if the structure
 * was never used before.
 *
 * \param address
 *   Pointer to the memory structure to initialise
 *
 * \remark
 *   This function marks the memory structure as being used
 *   and initialises the length of the address field.
 */
static void initialize_socket_address(vice_network_socket_address_t * address)
{
    memset(address, 0, sizeof *address);
    address->used = 1;
    address->len = sizeof address->address;
}

/*! \internal \brief Get a free memory area for a socket address

  \return
     NULL on error;
     else, a pointer to an empty vice_network_socket_address_t structure

  \remark
     If not used anymore, the returned pointer must be freed
     with a call to vice_network_address_close().
*/
static vice_network_socket_address_t * vice_network_alloc_new_socket_address(void)
{
    vice_network_socket_address_t * return_address = NULL;
    int i = get_new_pool_entry(&address_pool_usage);

    if (i >= arraysize(address_pool)) {
        i = -1;
    }

    assert(i >= 0);

    if (i >= 0) {
        assert(address_pool[i].used == 0);

        return_address = &address_pool[i];
        initialize_socket_address(return_address);
    }

    return return_address;
}

/*! \brief Open a socket and initialise it for server operation

  \param server_address
     The address of the server to which to bind to.

  \return
     0 on error;
     else, a handle to the socket on success.

  \remark
     The server_address variable determines the type of
     socket to be used (IPv4, IPv6, Unix Domain Socket, ...)
     Thus, server_address must not be NULL.
*/
vice_network_socket_t * vice_network_server(const vice_network_socket_address_t * server_address)
{
    int socket_reuse_address = 1;
    int sockfd = INVALID_SOCKET;
    int error = 1;

    assert(server_address != NULL);

    do {
        if (socket_init() < 0) {
            break;
        }

        sockfd = (int)socket(server_address->domain, SOCK_STREAM, server_address->protocol);

        if (SOCKET_IS_INVALID(sockfd)) {
            sockfd = INVALID_SOCKET;
            break;
        }

        /*
            Fix the "Address In Use" error upon reconnecting to tcp socket monitor port
            by setting SO_REUSEPORT/ADDR options on socket before bind()

            Ignore setsockopt() failures - just continue - the socket is still valid.
        */
#ifdef HAVE_IPV6
        if ((server_address->domain == PF_INET) || (server_address->domain == PF_INET6)) {
#else
        if ((server_address->domain == PF_INET)) {
#endif
#ifndef WATCOM_COMPILE
#if defined(SO_REUSEPORT)
          setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void*)&socket_reuse_address, sizeof(socket_reuse_address));
#elif defined(SO_REUSEADDR)
          setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&socket_reuse_address, sizeof(socket_reuse_address));
#endif
#if defined(TCP_NODELAY)
          setsockopt(sockfd, SOL_TCP, TCP_NODELAY, (const void*)&error, sizeof(error)); /* just an integer with 1, not really an error */
#endif
#endif
        }
        if (bind(sockfd, &server_address->address.generic, server_address->len) < 0) {
            break;
        }
        if (listen(sockfd, 2) < 0) {
            break;
        }
        error = 0;
    } while (0);

    if (error) {
        if (!SOCKET_IS_INVALID(sockfd)) {
            closesocket(sockfd);
        }
        sockfd = INVALID_SOCKET;
    }

    return SOCKET_IS_INVALID(sockfd) ? NULL : vice_network_alloc_new_socket(sockfd);
}

/*! \brief Open a socket and initialise it for client operation

  \param server_address
     The address of the server to which to connect to.

  \return
     0 on error;
     else, a handle to the socket on success.

  \remark
     The server_address variable determines the type of
     socket to be used (IPv4, IPv6, Unix Domain Socket, ...)
*/
vice_network_socket_t * vice_network_client(const vice_network_socket_address_t * server_address)
{
    int sockfd = INVALID_SOCKET;
    int error = 1;

    assert(server_address != NULL);

    do {
        if (socket_init() < 0) {
            break;
        }

        sockfd = (int)socket(server_address->domain, SOCK_STREAM, server_address->protocol);

        if (SOCKET_IS_INVALID(sockfd)) {
            sockfd = INVALID_SOCKET;
            break;
        }

#ifndef WATCOM_COMPILE
#if defined(TCP_NODELAY)
        setsockopt(sockfd, SOL_TCP, TCP_NODELAY, (const void*)&error, sizeof(error)); /* just an integer with 1, not really an error */
#endif
#endif

        if (connect(sockfd, &server_address->address.generic, server_address->len) < 0) {
            break;
        }
        error = 0;
    } while (0);

    if (error) {
        if (!SOCKET_IS_INVALID(sockfd)) {
            closesocket(sockfd);
        }
        sockfd = INVALID_SOCKET;
    }

    return SOCKET_IS_INVALID(sockfd) ? NULL : vice_network_alloc_new_socket(sockfd);
}

/*! \internal \brief Generate an IPv4 socket address

  Initialises a socket address with an IPv4 address.

  \param socket_address
     Pointer to an empty socket address that will be initialized

  \param address_string
     A string describing the address to set.

  \param port
     A default port number. If address_string does not
     specify a port number, the number here will be used.
     If address_string specifies a port number, that one
     will be used instead.

  \return
     0 on success,
     else an error occurred.

  \remark
     address_string must be specified in the form
     &lt;host&gt;
     or
     &lt;host&gt;:&lt;port&gt;
     with &lt;host&gt; being the name or the IPv4 address of the host,
     and &lt;port&gt; being the port number. If the first form is used,
     the default port will be used.
*/
static int vice_network_address_generate_ipv4(vice_network_socket_address_t * socket_address, const char * address_string, unsigned short port)
{
    const char * address_part = address_string;
    int error = 1;

    do {
        /* preset the socket address with port and INADDR_ANY */

        memset(&socket_address->address, 0, sizeof socket_address->address);
        socket_address->domain = PF_INET;
        socket_address->protocol = IPPROTO_TCP;
        socket_address->len = sizeof socket_address->address.ipv4;
        socket_address->address.ipv4.sin_family = AF_INET;
        socket_address->address.ipv4.sin_port = htons(port);
        socket_address->address.ipv4.sin_addr.s_addr = INADDR_ANY;

        if (address_string) {
            /* an address string was specified, try to use it */
            struct hostent * host_entry;

            char * port_part = NULL;

            /* try to find out if a port has been specified */
            port_part = strchr(address_string, ':');

            if (port_part) {
                char * p;
                unsigned long new_port;

                /* yes, there is a port: Copy the part before, so we can modify it */
                p = lib_stralloc(address_string);

                p[port_part - address_string] = 0;
                address_part = p;

                ++port_part;

                new_port = strtoul(port_part, &p, 10);

                if (*p == 0) {
                    socket_address->address.ipv4.sin_port = htons((unsigned short) new_port);
                }
            }

            if (address_part[0] == 0) {
                /* there was no address give, do not try to process it. */
                error = 0;
                break;
            }

            host_entry = gethostbyname(address_part);

            if (host_entry != NULL && host_entry->h_addrtype == AF_INET) {
                if (host_entry->h_length != sizeof socket_address->address.ipv4.sin_addr.s_addr) {
                    /* something weird happened... SHOULD NOT HAPPEN! */
                    log_message(LOG_DEFAULT,
                                "gethostbyname() returned an IPv4 address, "
                                "but the length is wrong: %u", host_entry->h_length );
                    break;
                }

                memcpy(&socket_address->address.ipv4.sin_addr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);
            } else {
                /* Assume it is an IP address */

                if (address_part[0] != 0) {
#ifdef HAVE_INET_ATON
                    /*
                     * this implementation is preferable, as inet_addr() cannot
                     * return the broadcast address, as it has the same encoding
                     * as INADDR_NONE.
                     *
                     * Unfortunately, not all ports have inet_aton(), so, we must
                     * provide both implementations.
                     */
                    if (inet_aton(address_part, &socket_address->address.ipv4.sin_addr.s_addr) == 0) {
                        /* no valid IP address */
                        break;
                    }
#else
                    in_addr_t ip = inet_addr(address_part);

                    if (ip == INADDR_NONE) {
                        /* no valid IP address */
                        break;
                    }
                    socket_address->address.ipv4.sin_addr.s_addr = ip;
#endif
                }
            }

            error = 0;
        }
    } while (0);

    /* if we allocated memory for the address part
     * because a port was specified,
     * free that memory now.
     */
    if (address_part != address_string) {
        lib_free(address_part);
    }

    return error;
}

/*! \internal \brief Generate an IPv6 socket address

  Initialises a socket address with an IPv6 address.

  \param socket_address
     Pointer to an empty socket address that will be initialized

  \param address_string
     A string describing the address to set.

  \param port
     A default port number. If address_string does not
     specify a port number, the number here will be used.
     If address_string specifies a port number, that one
     will be used instead.

  \return
     0 on success,
     else an error occurred.

  \remark
     address_string must be specified in one of the forms
     &lt;host&gt;
     or
     [&lt;host&gt;]:&lt;port&gt;
     with &lt;hostname&gt; being the name of the host,
     &lt;hostipv6&gt; being the IP of the host, and
     &lt;host&gt; being the name of the host or its IPv6,
     and &lt;port&gt; being the port number.

     The extra braces [...] in case the port is specified are
     needed as IPv6 addresses themselves already contain colons,
     and it would be impossible to clearly distinguish between
     an IPv6 address, and an IPv6 address where a port has been
     specified. This format is a common one.

  \remark
     On platforms which do not support IPv6, this function
     returns -1 as error.

  \todo
     Specifying a port is not implemented yet.
*/
static int vice_network_address_generate_ipv6(vice_network_socket_address_t * socket_address, const char * address_string, unsigned short port)
{
#ifdef HAVE_IPV6
    int error = 1;

    do {
        struct hostent * host_entry = NULL;
#ifndef HAVE_GETHOSTBYNAME2
        int err6;
#endif

        /* preset the socket address */

        memset(&socket_address->address, 0, sizeof socket_address->address);
        socket_address->domain = PF_INET6;
        socket_address->protocol = IPPROTO_TCP;
        socket_address->len = sizeof socket_address->address.ipv6;
        socket_address->address.ipv6.sin6_family = AF_INET6;
        socket_address->address.ipv6.sin6_port = htons(port);
        socket_address->address.ipv6.sin6_addr = in6addr_any;

        if (!address_string || address_string[0] == 0) {
            /* there was no address give, do not try to process it. */
            error = 0;
            break;
        }
#ifdef HAVE_GETHOSTBYNAME2
        host_entry = gethostbyname2(address_string, AF_INET6);
#else
        host_entry = getipnodebyname(address_string, AF_INET6, AI_DEFAULT, &err6);
#endif
        if (host_entry == NULL) {
            break;
        }

        memcpy(&socket_address->address.ipv6.sin6_addr, host_entry->h_addr, host_entry->h_length);

#ifndef HAVE_GETHOSTBYNAME2
        freehostent(host_entry);
#endif
        error = 0;
    } while (0);

    return error;
#else /* #ifdef HAVE_IPV6 */
    log_message(LOG_DEFAULT, "IPv6 is not supported in this installation of VICE!\n");
    return -1;
#endif /* #ifdef HAVE_IPV6 */
}

/*! \internal \brief Generate a unix domain socket address

  Initialises a socket address with a unix domain socket address

  \param socket_address
     Pointer to an empty socket address that will be initialized

  \param address_string
     A string describing the address to set.

  \return
     0 on success,
     else an error occurred.

  \remark
     address_string must be the path of a special file
     which represents the Unix domain socket.

  \remark
     On platforms which do not support unix domain sockets, this function
     returns -1 as error.
*/
static int vice_network_address_generate_local(vice_network_socket_address_t * socket_address, const char * address_string)
{
#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    int error = 1;

    do {
        if (address_string[0] == 0) {
            break;
        }

        /* initialise the socket address */

        memset(&socket_address->address, 0, sizeof socket_address->address);
        socket_address->domain = PF_UNIX;
        socket_address->protocol = 0;
        socket_address->len = sizeof socket_address->address.local;
        socket_address->address.local.sun_family = AF_UNIX;

        if (strlen(address_string) >= sizeof socket_address->address.local.sun_path) {
            log_message(LOG_DEFAULT,
                        "Unix domain socket name of '%s' is too long; only %u chars are allowed.",
                        address_string, sizeof socket_address->address.local.sun_path);
            break;
        }
        strcpy(socket_address->address.local.sun_path, address_string);

        error = 0;
    } while (0);

    return error;

#else /* #ifdef HAVE_UNIX_DOMAIN_SOCKETS */
    log_message(LOG_DEFAULT, "Unix domain sockets are not supported in this installation of VICE!\n");
    return -1;
#endif /* #ifdef HAVE_UNIX_DOMAIN_SOCKETS */
}

/*! \brief Generate a socket address

  Initialises a socket address with the value
  specified as input parameter.

  \param address_string
     A string describing the address to set.

  \param port
     A default port number. If address_string does not
     specify a port number, the number here will be used.
     If address_string specifies a port number, that one
     will be used instead.

  \return socket_address
     Pointer to a socket address that has been initialised
     to the specification in address_string (and port, if
     supported).
     NULL in case of an error.

  \remark
     If address_string starts with a pipe ('|'), then the
     address_string is treated as a unix domain socket.
     Otherwise, address_string can be prepended with ip6://
     or ip4://, in which case address_string is treated
     exactly as an IPv6 or IPv4 address, respectively.

     If there is no specification, this routine tests for
     an IPv6 address. If this fails, it tries an IPv4 address.

*/
vice_network_socket_address_t * vice_network_address_generate(const char * address_string, unsigned short port)
{
    vice_network_socket_address_t * socket_address = NULL;
    int error = 1;

    do {
        socket_address = vice_network_alloc_new_socket_address();
        if (socket_address == NULL) {
            break;
        }

        if (address_string && address_string[0] == '|') {
            if (vice_network_address_generate_local(socket_address, &address_string[1])) {
                break;
            }
        } else if (address_string && strncmp("ip6://", address_string, sizeof "ip6://" - 1) == 0) {
            if (vice_network_address_generate_ipv6(socket_address, &address_string[sizeof "ip6://" - 1], port)) {
                break;
            }
        } else if (address_string && strncmp("ip4://", address_string, sizeof "ip4://" - 1) == 0) {
            if (vice_network_address_generate_ipv4(socket_address, &address_string[sizeof "ip4://" - 1], port)) {
                break;
            }
        } else {
            /* the user did not specify the type of the address, try to guess it by trying IPv6, then IPv4 */
#ifdef HAVE_IPV6
            if (vice_network_address_generate_ipv6(socket_address, address_string, port))
#endif /* #ifdef HAVE_IPV6 */
            {
                if (vice_network_address_generate_ipv4(socket_address, address_string, port)) {
                    break;
                }
            }
        }

        error = 0;
    } while (0);

    if (error && socket_address) {
        vice_network_address_close(socket_address);
        socket_address = NULL;
    }

    return socket_address;
}


/*! \brief Return memory of a socket address

  This function returns a socket address to the memory pool
  of addresses.

  \param address
     A socket address that has been allocated by
     vice_network_alloc_new_socket_address()
     or by vice_network_address_generate().
*/
void vice_network_address_close(vice_network_socket_address_t * address)
{
    if (address) {
        assert(address->used == 1);
        assert(((address_pool_usage & (1u << (address - address_pool))) != 0));

        address->used = 0;
        address_pool_usage &= ~(1u << (address - address_pool));
    }
}

/*! \brief Accept a connection on a listening socket

  This function accepts a connection on a socket that
  has been opened with vice_network_server().

  \param sockfd
     The socket that has been oped with vice_network_server()

  \return
     A new socket that can be used for transmission on this this connection.
*/
vice_network_socket_t * vice_network_accept(vice_network_socket_t * sockfd)
{
    SOCKET newsocket = INVALID_SOCKET;

    initialize_socket_address( &sockfd->address );

    newsocket = accept(sockfd->sockfd, &sockfd->address.address.generic, &sockfd->address.len);

    return SOCKET_IS_INVALID(newsocket) ? NULL : vice_network_alloc_new_socket(newsocket);
}

/*! \brief Close a socket

  This function closes the socket that has been
  opened either by vice_network_server(), vice_network_client()
  or vice_network_accept().

  \param sockfd
     The socket to be closed

  \return
     0 on success, else an error occurred.
*/
int vice_network_socket_close(vice_network_socket_t * sockfd)
{
    SOCKET localsockfd = INVALID_SOCKET;
    int error = -1;

    if (sockfd) {
        localsockfd = sockfd->sockfd;

        assert(sockfd->used == 1);
        assert(((socket_pool_usage & (1u << (sockfd - socket_pool))) != 0));

        sockfd->used = 0;
        socket_pool_usage &= ~(1u << (sockfd - socket_pool));

        error = closesocket(localsockfd);
    }

    return error;
}

/*! \brief Send data on a connected socket

  This function sends outgoing data to a connected socket.
  opened either by vice_network_client()
  or vice_network_accept().

  \param sockfd
     The connected socket to send to

  \param buffer
     Pointer to the buffer which holds the data to send

  \param buffer_length
     The length of the buffer pointed to by buffer.

  \param flags
     Flags for the socket. These flags are architecture dependent.

  \return
     the number of bytes send. For non-blocking sockets,
     this can be less than len. For blocking sockets (default),
     any return value different than len must be treated as an error.
*/
int vice_network_send(vice_network_socket_t * sockfd, const void * buffer, size_t buffer_length, int flags)
{
    int ret;
    signals_pipe_set();
    ret = send(sockfd->sockfd, buffer, buffer_length, flags);
    signals_pipe_unset();
    return ret;
}

/*! \brief Receive data from a connected socket

  This function receives incoming data from a connected socket.
  opened either by vice_network_client()
  or vice_network_accept().

  \param sockfd
     The connected socket to receive from

  \param buffer
     Pointer to the buffer which will hold the received data

  \param buffer_length
     The length of the buffer pointed to by buffer. This
     indicates the maximum number of bytes to receive.

  \param flags
     Flags for the socket. These flags are architecture dependent.

  \return
     the number of bytes received. This can be less than
     buffer_length.

     When the return value is 0, either the other site has
     gracefully closed the socket (and all data has been
     received), or no data was received in the case
     of a non-blocking socket.

     In case of an error, -1 is returned.
*/
int vice_network_receive(vice_network_socket_t * sockfd, void * buffer, size_t buffer_length, int flags)
{
    int ret;
    signals_pipe_set();
    ret = recv(sockfd->sockfd, buffer, buffer_length, flags);
    signals_pipe_unset();
    return ret;
}

/*! \brief Check if a socket has incoming data to receive

  This function is called in order to determine if there is
  data to be received on a socket. For a blocking socket, this
  is the only way to receive data without actually blocking.

  \param readsockfd
     The connected socket to test for data

  \return
     1 if the specified socket has data; 0 if it does not contain
     any data, and -1 in case of an error.
*/
int vice_network_select_poll_one(vice_network_socket_t * readsockfd)
{
    TIMEVAL timeout = { 0 };

    fd_set fdsockset;

    FD_ZERO(&fdsockset);
    FD_SET(readsockfd->sockfd, &fdsockset);

    return select( readsockfd->sockfd + 1, &fdsockset, NULL, NULL, &timeout);
}

/*! \brief Get the error of the last socket operation

  This function determines the error code for the last
  socket operation.

  \return
     the error code

  \remark
      It does not distinguish between
      different sockets, thus, make sure to call it directly
      after an erroneous socket operation. Furthermore,
      reading the error status does not reset the error code.
      Thus, do not expect correct results if the previous
      operation did not produce an error.

   \todo
       Generate a mapping between WIN32 and Unix style
       error numbers
*/
int vice_network_get_errorcode(void)
{
    return ARCHDEP_SOCKET_ERROR;
}
#endif
