/*
 * dolphindos3.c - Parallel cable emulation for drives with DD3.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

/* #define DD3_DEBUG */

#ifdef DD3_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "dolphindos3.h"
#include "drive.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "iecdrive.h"
#include "log.h"
#include "mc6821core.h"

static mc6821_state my6821[DRIVE_NUM];

/*-----------------------------------------------------------------------*/
/* MC6821 hooks */

static void dd3_set_pa(mc6821_state *ctx)
{
    unsigned int dnr = (unsigned int)(((drive_context_t *)(ctx->p))->mynumber);
    parallel_cable_drive_write(DRIVE_PC_DD3, ctx->dataA, PARALLEL_WRITE, dnr);
    /* DBG(("DD3 (%d) 6821 PA WR %02x\n", dnr, ctx->dataA)); */
}

static BYTE dd3_get_pa(mc6821_state *ctx)
{
    unsigned int dnr = (unsigned int)(((drive_context_t *)(ctx->p))->mynumber);
    BYTE data;
    int hs = 0;

    /* output all pins that are in input mode as 1 first */
    parallel_cable_drive_write(DRIVE_PC_DD3, (BYTE)((~ctx->ddrA) | ctx->dataA), PARALLEL_WRITE, dnr);

    /* FIXME: this is an ugly hack */
    hs = 0;
    if ((ctx->ctrlA & 0x28) == 0x28) {
        hs = 1;
    }

    data = parallel_cable_drive_read(DRIVE_PC_DD3, hs);

    DBG(("DD3 6821 PA RD %02x CTRLA %02x CA2 %02x\n", data, ctx->ctrlA, ctx->CA2));
    return data;
}

static void dd3_set_ca2(mc6821_state *ctx)
{
    /* used for handshaking */
    DBG(("DD3 6821 CA2 WR %02x\n", ctx->CA2));
}

static void dd3_set_pb(mc6821_state *ctx)
{
    /* nothing here ? */
    DBG(("DD3 6821 PB WR %02x\n", ctx->dataB));
}

static BYTE dd3_get_pb(mc6821_state *ctx)
{
    BYTE data = 0xff; /* unconnected pins return 1 */
    DBG(("DD3 6821 PB RD %02x\n", data));
    return data;
}

static void dd3_set_cb2(mc6821_state *ctx)
{
    /* nothing here ? */
    DBG(("DD3 6821 CB2 WR %02x\n", ctx->CB2));
}

/*-----------------------------------------------------------------------*/

static void mc6821_store(drive_context_t *drv, WORD addr, BYTE byte)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */
    mc6821core_store(&my6821[drv->mynumber], port /* rs1 */, reg /* rs0 */, byte);
}

static BYTE mc6821_read(drive_context_t *drv, WORD addr)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */
    return mc6821core_read(&my6821[drv->mynumber], port /* rs1 */, reg /* rs0 */);
}

static BYTE mc6821_peek(drive_context_t *drv, WORD addr)
{
    int port, reg;

    port = (addr >> 1) & 1; /* rs1 */
    reg = (addr >> 0) & 1;  /* rs0 */
    return mc6821core_peek(&my6821[drv->mynumber], port /* rs1 */, reg /* rs0 */);
}

/*-----------------------------------------------------------------------*/

void dd3_set_signal(drive_context_t *drv)
{
/*    DBG(("DD3 (%d) 6821 SIGNAL\n", dnr)); */
    mc6821core_set_signal(&my6821[drv->mynumber], MC6821_SIGNAL_CA1);
}

void dd3_init(drive_context_t *drv)
{
    my6821[drv->mynumber].p = (void*)drv;
    my6821[drv->mynumber].set_pa = dd3_set_pa;
    my6821[drv->mynumber].set_pb = dd3_set_pb;
    my6821[drv->mynumber].get_pa = dd3_get_pa;
    my6821[drv->mynumber].get_pb = dd3_get_pb;
    my6821[drv->mynumber].set_ca2 = dd3_set_ca2;
    my6821[drv->mynumber].set_cb2 = dd3_set_cb2;
}

void dd3_reset(drive_context_t *drv)
{
    mc6821core_reset(&my6821[drv->mynumber]);
}

void dd3_mem_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    if (drv->drive->parallel_cable != DRIVE_PC_DD3) {
        return;
    }

    /* Setup parallel cable */
    switch (type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        drivemem_set_func(cpud, 0x50, 0x60, mc6821_read, mc6821_store, mc6821_peek, NULL, 0);
        break;
    default:
        break;
    }
}
