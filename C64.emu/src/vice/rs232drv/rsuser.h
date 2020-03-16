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

#define RI_IN           0x08    /* PB3 */
#define DCD_IN          0x10    /* PB4 */
#define CTS_IN          0x40    /* PB6 */
#define DSR_IN          0x80    /* PB7 */

extern int rsuser_enabled;

extern void rsuser_init(long cycles_per_sec, void (*start_bit_trigger)(void),
                        void (*byte_rx_func)(uint8_t));
extern int rsuser_resources_init(void);
extern int rsuser_cmdline_options_init(void);

extern void rsuser_tx_byte(uint8_t b);
extern void rsuser_write_ctrl(uint8_t b);
extern uint8_t rsuser_read_ctrl(uint8_t b);

extern void rsuser_reset(void);

extern uint8_t rsuser_get_rx_bit(void);
extern void rsuser_set_tx_bit(int b);

#endif
