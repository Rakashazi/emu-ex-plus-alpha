/*
 * mc6821core.h - Motorola MC6821 Emulation
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_MC6821CORE_H_
#define VICE_MC6821CORE_H_

#include "types.h"

/* control register bit masks */

#define MC6821_CTRL_IRQ1        0x80
#define MC6821_CTRL_IRQ2        0x40
#define MC6821_CTRL_C2DDR       0x20
#define MC6821_CTRL_C2MODE      0x18
#define MC6821_CTRL_REG         0x04
#define MC6821_CTRL_C1MODE      0x03

/* control register bits */

#define MC6821_CTRL_C2_IN       0x00
#define MC6821_CTRL_C2_OUT      0x20
/* when C(A/B)2 is input */
#define MC6821_CTRL_C2_IRQDIS   0x00
#define MC6821_CTRL_C2_IRQEN    0x08
#define MC6821_CTRL_C2_IRQHILO  0x00
#define MC6821_CTRL_C2_IRQLOHI  0x10
/* when C(A/B)2 is output */
#define MC6821_CTRL_C2_STROBE_C 0x00
#define MC6821_CTRL_C2_STROBE_E 0x08
#define MC6821_CTRL_C2_RESET_C2 0x10
#define MC6821_CTRL_C2_SET_C2   0x18

#define MC6821_CTRL_REG_DDR     0x00
#define MC6821_CTRL_REG_DATA    0x04

#define MC6821_CTRL_C1_IRQDIS   0x00
#define MC6821_CTRL_C1_IRQEN    0x01
#define MC6821_CTRL_C1_IRQHILO  0x00
#define MC6821_CTRL_C1_IRQLOHI  0x02

typedef struct _mc6821_state {
    BYTE ctrlA;
    BYTE dataA;
    BYTE ddrA;

    BYTE ctrlB;
    BYTE dataB;
    BYTE ddrB;

    int CA2;
    int CA2state;
    int CB2;
    int CB2state;

    /* hooks that set the i/o lines */
    void (*set_pa)(struct _mc6821_state*);
    void (*set_pb)(struct _mc6821_state*);

/* TODO
    void (*set_irqa)(struct _mc6821_state*);
    void (*set_irqb)(struct _mc6821_state*);
*/
    void (*set_ca2)(struct _mc6821_state*);
    void (*set_cb2)(struct _mc6821_state*);

    /* hooks that read the status of i/o lines */
    BYTE (*get_pa)(struct _mc6821_state*);
    BYTE (*get_pb)(struct _mc6821_state*);

    void *p;    /* parent context that may be used by the hooks */
} mc6821_state;

void mc6821core_reset(mc6821_state *ctx);
BYTE mc6821core_read(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */);
BYTE mc6821core_peek(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */);
void mc6821core_store(mc6821_state *ctx, int port /* rs1 */, int reg /* rs0 */, BYTE data);

/* Signal values (for signaling edges on the control lines)  */
#define MC6821_SIGNAL_CA1 0
#define MC6821_SIGNAL_CA2 1
#define MC6821_SIGNAL_CB1 2
#define MC6821_SIGNAL_CB2 3
void mc6821core_set_signal(mc6821_state *ctx, int line);

struct snapshot_module_s;

int mc6821core_snapshot_write_data(mc6821_state *ctx, struct snapshot_module_s *m);
int mc6821core_snapshot_read_data(mc6821_state *ctx, struct snapshot_module_s *m);

#endif
