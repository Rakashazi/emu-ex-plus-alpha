/*
 * rs232.c - RS232 emulation.
 *
 * Written by
 *   Marco van den Heuvel <blackystardust68@yahoo.com>
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

/*
 * The RS232 emulation captures the bytes sent to the RS232 interfaces
 * available (currently ACIA 6551, std C64 and Daniel Dallmanns fast RS232
 * with 9600 Baud).
 *
 * I/O is done to a socket.  If the socket isnt connected, no data
 * is read and written data is discarded.
 */

#undef        DEBUG
/* #define DEBUG */

#include "vice.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)

#include <assert.h>
#include <errno.h>
#include <string.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "log.h"
#include "rs232.h"
#include "rs232dev.h"
#include "rs232net.h"
#include "types.h"
#include "util.h"
#include "vicesocket.h"

#ifdef DEBUG
# define DEBUG_LOG_MESSAGE(_xxx) log_message _xxx
#else
# define DEBUG_LOG_MESSAGE(_xxx)
#endif

/* ------------------------------------------------------------------------- */

enum {
    RS232_IS_PHYSICAL_DEVICE = 0x4000
};

/* ------------------------------------------------------------------------- */

int rs232_resources_init(void)
{
#ifdef HAVE_RS232DEV
    rs232dev_resources_init();
#endif

#ifdef HAVE_RS232NET
    rs232net_resources_init();
#endif

    return 0;
}

void rs232_resources_shutdown(void)
{
#ifdef HAVE_RS232DEV
    rs232dev_resources_shutdown();
#endif

#ifdef HAVE_RS232NET
    rs232net_resources_shutdown();
#endif
}

int rs232_cmdline_options_init(void)
{
#ifdef HAVE_RS232DEV
    rs232dev_cmdline_options_init();
#endif

#ifdef HAVE_RS232NET
    rs232net_cmdline_options_init();
#endif

    return 0;
}

/* ------------------------------------------------------------------------- */

/* initializes all RS232 stuff */
void rs232_init(void)
{
#ifdef HAVE_RS232DEV
    rs232dev_init();
#endif

#ifdef HAVE_RS232NET
    rs232net_init();
#endif
}

/* reset RS232 stuff */
void rs232_reset(void)
{
#ifdef HAVE_RS232DEV
    rs232dev_reset();
#endif

#ifdef HAVE_RS232NET
    rs232net_reset();
#endif
}

/*! \internal find out if the rs232 channel is for a physical device (COMx:) or for networking.
 *
 * An RS232 channel is for a physical device if its name is NOT a network acceptable address
 *
 */
static int rs232_is_physical_device(int device)
{
#ifdef HAVE_RS232NET
    vice_network_socket_address_t *ad = NULL;

    ad = vice_network_address_generate(rs232_devfile[device], 0);

    if (ad == NULL) {
        return 1;
    } else {
        return 0;
    }
#else
    return 1;
#endif
}

/* opens a rs232 window, returns handle to give to functions below. */
int rs232_open(int device)
{
    int ret;

    assert(device < RS232_NUM_DEVICES);

    if (rs232_is_physical_device(device)) {
#ifdef HAVE_RS232DEV
        ret = rs232dev_open(device);
        if (ret >= 0) {
            ret |= RS232_IS_PHYSICAL_DEVICE;
        }
#endif
    } else {
#ifdef HAVE_RS232NET
        ret = rs232net_open(device);
#else
        ret = -1;
#endif
    }
    return ret;
}

/* closes the rs232 window again */
void rs232_close(int fd)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        rs232dev_close(fd & ~RS232_IS_PHYSICAL_DEVICE);
#endif
    }
#ifdef HAVE_RS232NET
    else {
        rs232net_close(fd);
    }
#endif
}

/* sends a byte to the RS232 line */
int rs232_putc(int fd, BYTE b)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        return rs232dev_putc(fd & ~RS232_IS_PHYSICAL_DEVICE, b);
#endif
    }
#ifdef HAVE_RS232NET
    else {
        return rs232net_putc(fd, b);
    }
#endif
}

/* gets a byte to the RS232 line, returns !=0 if byte received, byte in *b. */
int rs232_getc(int fd, BYTE * b)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        return rs232dev_getc(fd & ~RS232_IS_PHYSICAL_DEVICE, b);
#endif
    }
#ifdef HAVE_RS232NET
    else {
        return rs232net_getc(fd, b);
    }
#endif
}

/* set the status lines of the RS232 device */
int rs232_set_status(int fd, enum rs232handshake_out status)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        return rs232dev_set_status(fd & ~RS232_IS_PHYSICAL_DEVICE, status);
#endif
    }
#ifdef HAVE_RS232NET
    else {
        return rs232net_set_status(fd, status);
    }
#endif
}

/* get the status lines of the RS232 device */
enum rs232handshake_in rs232_get_status(int fd)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        return rs232dev_get_status(fd & ~RS232_IS_PHYSICAL_DEVICE);
#endif
    }
#ifdef HAVE_RS232NET
    else {
        return rs232net_get_status(fd);
    }
#endif
}

/* set the bps rate of the physical device */
void rs232_set_bps(int fd, unsigned int bps)
{
    if (fd & RS232_IS_PHYSICAL_DEVICE) {
#ifdef HAVE_RS232DEV
        rs232dev_set_bps(fd & ~RS232_IS_PHYSICAL_DEVICE, bps);
#endif
    }
}
#endif
