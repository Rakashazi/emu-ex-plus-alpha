/*
 * rs232drv.h - Common RS232 driver handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_RS232DRV_H
#define VICE_RS232DRV_H

#include "types.h"

extern int rs232drv_resources_init(void);
extern void rs232drv_resources_shutdown(void);
extern int rs232drv_cmdline_options_init(void);

extern void rs232drv_init(void);
extern void rs232drv_reset(void);
extern int rs232drv_open(int device);
extern void rs232drv_close(int fd);
extern int rs232drv_putc(int fd, BYTE b);
extern int rs232drv_getc(int fd, BYTE *b);

enum rs232handshake_out {
    RS232_HSO_RTS = 0x01,
    RS232_HSO_DTR = 0x02
};

enum rs232handshake_in {
    RS232_HSI_CTS = 0x01, /* we assume DCD = CTS */
    RS232_HSI_DSR = 0x02
};

/* write the output handshake lines */
extern int rs232drv_set_status(int fd, enum rs232handshake_out status);

/* write the output handshake lines */
extern enum rs232handshake_in rs232drv_get_status(int fd);

extern void rs232drv_set_bps(int fd, unsigned int bps);

#endif
