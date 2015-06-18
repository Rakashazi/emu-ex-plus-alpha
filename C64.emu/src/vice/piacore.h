/*
 * piacore.h -- PIA chip emulation.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_PIACORE_H
#define VICE_PIACORE_H

#include "log.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

#define IS_CA2_HANDSHAKE()      ((mypia.ctrl_a & 0x30) == 0x20)
#define IS_CA2_PULSE_MODE()     ((mypia.ctrl_a & 0x38) == 0x28)
#define IS_CA2_TOGGLE_MODE()    ((mypia.ctrl_a & 0x38) == 0x20)

#define IS_CB2_HANDSHAKE()      ((mypia.ctrl_b & 0x30) == 0x20)
#define IS_CB2_PULSE_MODE()     ((mypia.ctrl_b & 0x38) == 0x28)
#define IS_CB2_TOGGLE_MODE()    ((mypia.ctrl_b & 0x38) == 0x20)

#define P_PORT_A        0
#define P_CTRL_A        1
#define P_PORT_B        2
#define P_CTRL_B        3

typedef struct {
    BYTE port_a;        /* output register, i.e. what has been written by
                           the CPU. input is assembled at read time */
    BYTE ddr_a;         /* PIA Port A DDR */
    BYTE ctrl_a;

    BYTE port_b;
    BYTE ddr_b;         /* PIA Port B DDR */
    BYTE ctrl_b;

    int ca_state;
    int cb_state;
} piareg;

/* ------------------------------------------------------------------------- */

static int is_peek_access = 0;

static log_t mypia_log = LOG_ERR;

#endif
