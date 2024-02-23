/*
 * rs232.h - RS232 emulation.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

/*
 * This is the header for the RS232 emulation.
 *
 * The RS232 emulation captures the bytes sent to the RS232 interfaces
 * available (currently only ACIA 6551, later UART 16550A, std C64,
 * and Daniel Dallmanns fast RS232 with 9600 Baud).
 * The characters captured are displayed in a special terminal window.
 * Characters typed in the terminal window are sent back to the
 * chip emulations.
 */

#ifndef VICE_RS232_H
#define VICE_RS232_H

#include "types.h"

#include "rs232drv.h"

/* Initializes all RS232 stuff */
void rs232_init(void);

/* Reset for RS232 interfaces */
void rs232_reset(void);

/* Opens a rs232 window, returns handle to give to functions below. */
int rs232_open(int device);

/* Closes the rs232 window again */
void rs232_close(int fd);

/* Sends a byte to the RS232 line */
int rs232_putc(int fd, uint8_t b);

/* Gets a byte to the RS232 line, returns !=1 if byte received, byte in *b. */
int rs232_getc(int fd, uint8_t *b);

/* write the output handshake lines */
int rs232_set_status(int fd, enum rs232handshake_out status);

/* write the output handshake lines */
enum rs232handshake_in rs232_get_status(int fd);

/* set the bps rate of the physical device */
void rs232_set_bps(int fd, unsigned int bps);

int rs232_resources_init(void);
void rs232_resources_shutdown(void);
int rs232_cmdline_options_init(void);


#define RS232_NUM_DEVICES 4

extern char *rs232_devfile[RS232_NUM_DEVICES];
extern int rs232_useip232[RS232_NUM_DEVICES];

/* the "ip232" protocol used by tcpser

tcpser->vice

0xff nn ->
 nn     bit 0   0: DCD = false      1: DCD = true
        bit 1   0: RI = false       1: RI = true
 nn = 255    literal 0xff
other   ->   unchanged

vice->tcpser

0xff nn ->
 nn     bit 0   0: DTR = false      1: DTR = true
 nn = 255    literal 0xff
other   ->   unchanged

*/

#define IP232MAGIC  0xff
/* sending */
#define IP232DTRLO  0   /* original patch only sends this once on rs232 reset */
#define IP232DTRHI  1   /* original patch only sends this once on 0->1 transition */
/* reading */
#define IP232DCDLO  0
#define IP232DCDHI  1
#define IP232DCDMASK    1

#define IP232RILO   0
#define IP232RIHI   2
#define IP232RIMASK     2

#endif
