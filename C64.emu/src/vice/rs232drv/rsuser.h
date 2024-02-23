/*
 * rsuser.h - Daniel Dallmann's 9600 baud RS232 userport interface
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_RSUSER_H_
#define VICE_RSUSER_H_

#include "types.h"

#define TXD_OUT         0x04    /* PA2 */
#define RXD_IN          0x01    /* PB0 (also connected to !FLAG2) */

#define RTS_OUT         0x02    /* PB1 */
#define DTR_OUT         0x04    /* PB2 */
#define OH_OUT          0x20    /* PB5 ON_HOOK output to some Commodore modems and clones */
                                /* Used to 'pick'/'hang' the phone and pulse dial             */

#define RI_IN           0x08    /* PB3 */
#define DCD_IN          0x10    /* PB4 */
#define CTS_IN          0x40    /* PB6 */
#define DSR_IN          0x80    /* PB7 */

#define RS_USER_DEVICE_1   0
#define RS_USER_DEVICE_2   1
#define RS_USER_DEVICE_3   2
#define RS_USER_DEVICE_4   3

void rsuser_init(long cycles_per_sec, void (*start_bit_trigger)(void), void (*byte_rx_func)(uint8_t));
void rsuser_change_timing(long cycles_per_sec);
int rsuser_resources_init(void);
int rsuser_cmdline_options_init(void);

#endif
