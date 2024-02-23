/*
 * i8255a.h - Intel 8255a
 *
 * Written by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#ifndef VICE_I8255A_H_
#define VICE_I8255A_H_

#include "types.h"

/* control register bit masks */

#define I8255A_G2_PC 0x01
#define I8255A_G2_PB 0x02
#define I8255A_G2_MS 0x04
#define I8255A_G1_PC 0x08
#define I8255A_G1_PA 0x10
#define I8255A_G1_MS 0x60
#define I8255A_MODE  0x80

#define I8255A_BS    0x01

typedef struct _i8255a_state {
    uint8_t ctrl;
    uint8_t data[3];

    /* hooks that set the i/o lines */
    void (*set_pa)(struct _i8255a_state*, uint8_t, int8_t);
    void (*set_pb)(struct _i8255a_state*, uint8_t, int8_t);
    void (*set_pc)(struct _i8255a_state*, uint8_t, int8_t);

    /* hooks that read the status of i/o lines */
    uint8_t (*get_pa)(struct _i8255a_state*, int8_t);
    uint8_t (*get_pb)(struct _i8255a_state*, int8_t);
    uint8_t (*get_pc)(struct _i8255a_state*, int8_t);

    /* parent context that may be used by the hooks */
    void *p;
} i8255a_state;

struct snapshot_module_s;

void i8255a_reset(i8255a_state *ctx);
uint8_t i8255a_read(i8255a_state *ctx, int8_t reg);
uint8_t i8255a_peek(i8255a_state *ctx, int8_t reg);
void i8255a_store(i8255a_state *ctx, int8_t reg, uint8_t data);
int i8255a_dump(i8255a_state *ctx);

int i8255a_snapshot_write_data(i8255a_state *ctx, struct snapshot_module_s *m);
int i8255a_snapshot_read_data(i8255a_state *ctx, struct snapshot_module_s *m);

#endif
