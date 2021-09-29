/*
 * i8255a.c - Intel 8255a
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

/*

Currently used by:

cmdhd

*/

#include "vice.h"

#include "log.h"
#include "types.h"
#include "snapshot.h"
#include "monitor.h"
#include "i8255a.h"

/* #define I8255ALOG */
/* #define I8255ADBG */

#define LOG LOG_DEFAULT
#define ERR LOG_ERR

#ifdef I8255ALOG
#define LOG1(_x_) log_message _x_
#else
#define LOG1(_x_)
#endif

#ifdef i8255ADBG
#define DBG(_x_) log_message _x_
#else
#define DBG(_x_)
#endif

#define CRIT(_x_) log_message _x_

void i8255a_reset(i8255a_state *ctx)
{
    ctx->ctrl = 0x1b;
    ctx->data[0] = 0;
    ctx->data[1] = 0;
    ctx->data[2] = 0;

    /* on reset relay inputs to outputs */
    if (ctx->get_pa && ctx->set_pa) {
        ctx->set_pa(ctx, ctx->get_pa(ctx, 0), 0);
    }
    if (ctx->get_pb && ctx->set_pb) {
        ctx->set_pb(ctx, ctx->get_pb(ctx, 1), 1);
    }
    if (ctx->get_pc && ctx->set_pc) {
        ctx->set_pc(ctx, ctx->get_pc(ctx, 2), 2);
    }
}

uint8_t i8255a_read(i8255a_state *ctx, int8_t reg)
{
    uint8_t data = 0xff;

    switch(reg & 3) {
    case 0:
        if (ctx->ctrl & I8255A_G1_PA) {
            if (ctx->get_pa) {
                data = ctx->get_pa(ctx, reg);
            }
        } else {
            data = ctx->data[0];
        }
        break;
    case 1:
        if (ctx->ctrl & I8255A_G2_PB) {
            if (ctx->get_pb) {
                data = ctx->get_pb(ctx, reg);
            }
        } else {
            data = ctx->data[1];
        }
        break;
    case 2:
        if (ctx->ctrl & (I8255A_G2_PC | I8255A_G1_PC)) {
            if (ctx->get_pc) {
                data = ctx->get_pc(ctx, reg);
            }
        }
        if (!(ctx->ctrl & I8255A_G2_PC)) {
            data = (data & 0xf0) | (ctx->data[2] & 0x0f);
        }
        if (!(ctx->ctrl & I8255A_G1_PC)) {
            data = (data & 0x0f) | (ctx->data[2] & 0xf0);
        }
        break;
    }
    return data;
}

uint8_t i8255a_peek(i8255a_state *ctx, int8_t reg)
{
    if ((reg & 3) == 3) {
        return ctx->ctrl;
    } else {
        return i8255a_read(ctx, reg | 4);
    }
}

void i8255a_store(i8255a_state *ctx, int8_t reg, uint8_t data)
{
    reg = reg & 3;
    switch (reg) {
    case 0:
        ctx->data[0] = data;
        if (!(ctx->ctrl & I8255A_G1_PA) && ctx->set_pa) {
            ctx->set_pa(ctx, ctx->data[0], 0);
        }
        break;
    case 1:
        ctx->data[1] = data;
        if (!(ctx->ctrl & I8255A_G2_PB) && ctx->set_pb) {
            ctx->set_pb(ctx, ctx->data[1], 1);
        }
        break;
    case 3:
        if (!(data & I8255A_MODE)) {
            break;
        }
        if ( (data & I8255A_G2_MS) || (data & I8255A_G1_MS) ) {
            CRIT((ERR,"I8255A: Unsupported mode set."));
        }
        ctx->ctrl = data;
        if (!(ctx->ctrl & I8255A_G1_PA) && ctx->set_pa) {
            ctx->set_pa(ctx, ctx->data[0], 3);
        } else if ((ctx->ctrl & I8255A_G1_PA) && ctx->set_pa) {
            ctx->set_pa(ctx, ctx->get_pa(ctx, 3), 3);
        }
        if (!(ctx->ctrl & I8255A_G2_PB) && ctx->set_pb) {
            ctx->set_pb(ctx, ctx->data[1], 3);
        } else if ((ctx->ctrl & I8255A_G2_PB) && ctx->set_pb) {
            ctx->set_pb(ctx, ctx->get_pb(ctx, 3), 3);
        }
        /* fall through */
    case 2:
        if (reg == 2) {
            ctx->data[2] = data;
        } else {
            data = ctx->data[2];
        }
        /* if both upper and lower port c are inputs, we are done */
        if ((ctx->ctrl & I8255A_G2_PC) && (ctx->ctrl & I8255A_G1_PC)) {
            if (reg == 3) {
                ctx->set_pc(ctx, ctx->get_pc(ctx, 3), 3);
            }
            break;
        }
        /* one or more is an output; grab from port c if one is an input */
        if (ctx->ctrl & (I8255A_G2_PC | I8255A_G1_PC)) {
            if (ctx->get_pc) {
                data = ctx->get_pc(ctx, reg);
            }
        }
        if (!(ctx->ctrl & I8255A_G2_PC)) {
            data = (data & 0xf0) | (ctx->data[2] & 0x0f);
        }
        if (!(ctx->ctrl & I8255A_G1_PC)) {
            data = (data & 0x0f) | (ctx->data[2] & 0xf0);
        }
        if (ctx->set_pc) {
            ctx->set_pc(ctx, data, reg);
        }
        break;
    }
}

int i8255a_snapshot_write_data(i8255a_state *ctx, snapshot_module_t *m)
{
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, ctx->ctrl) < 0
        || SMW_BA(m, ctx->data, 3) < 0) {
        return -1;
    }

    return 0;
}

int i8255a_snapshot_read_data(i8255a_state *ctx, snapshot_module_t *m)
{
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMR_B(m, &ctx->ctrl) < 0
        || SMR_BA(m, ctx->data, 3) < 0) {
        return -1;
    }

    return 0;
}

int i8255a_dump(i8255a_state *ctx)
{
    mon_out("Port A: %02x\n", i8255a_peek(ctx, 0));
    mon_out("Port B: %02x\n", i8255a_peek(ctx, 1));
    mon_out("Port C: %02x\n", i8255a_peek(ctx, 2));
    mon_out("CONTRL: %02x\n", i8255a_peek(ctx, 3));

    return 0;
}
