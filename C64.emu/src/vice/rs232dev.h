/*
 * rs232dev.h - RS232 emulation.
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

#ifndef VICE_RS232DEV_H
#define VICE_RS232DEV_H

#include "types.h"

/* Initializes all RS232 stuff */
extern void rs232dev_init(void);

/* Reset for RS232 interfaces */
extern void rs232dev_reset(void);

/* Opens a rs232 window, returns handle to give to functions below. */
extern int rs232dev_open(int device);

/* Closes the rs232 window again */
extern void rs232dev_close(int fd);

/* Sends a byte to the RS232 line */
extern int rs232dev_putc(int fd, BYTE b);

/* Gets a byte to the RS232 line, returns !=1 if byte received, byte in *b. */
extern int rs232dev_getc(int fd, BYTE *b);

/* write the output handshake lines */
extern int rs232dev_set_status(int fd, enum rs232handshake_out status);

/* write the output handshake lines */
extern enum rs232handshake_in rs232dev_get_status(int fd);

extern void rs232dev_set_bps(int fd, unsigned int bps);

extern int rs232dev_resources_init(void);
extern void rs232dev_resources_shutdown(void);
extern int rs232dev_cmdline_options_init(void);

#endif
