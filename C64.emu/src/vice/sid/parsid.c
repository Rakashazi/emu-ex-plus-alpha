/*
 * parsid.c - PARallel port SID abstraction layer.
 *
 * Written by
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

#include "vice.h"

#ifdef HAVE_PARSID

#include "parsid.h"
#include "types.h"

/* control register bits */
#define parsid_STROBE   0x01
#define parsid_AUTOFEED 0x02
#define parsid_nINIT    0x04
#define parsid_SELECTIN 0x08
#define parsid_PCD      0x20

static BYTE sidbuf[0x20];

static BYTE parsid_ctrport;
static int parsid_open_status = 0;

/* chip control pin assignments */
static void parsid_chip_select(void)
{
    parsid_ctrport |= parsid_STROBE;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_chip_deselect(void)
{
    parsid_ctrport &= ~parsid_STROBE;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_reset_start(void)
{
    parsid_ctrport |= parsid_SELECTIN;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_reset_end(void)
{
    parsid_ctrport &= ~parsid_SELECTIN;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_latch_open(void)
{
    parsid_ctrport &= ~parsid_AUTOFEED;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_latch_lock(void)
{
    parsid_ctrport |= parsid_AUTOFEED;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_RW_write(void)
{
    parsid_ctrport &= ~parsid_nINIT;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_RW_read(void)
{
    parsid_ctrport |= parsid_nINIT;
    parsid_drv_out_ctr(parsid_ctrport);
}

/* parallel port direction control */
static void parsid_port_write(void)
{
    parsid_ctrport &= ~parsid_PCD;
    parsid_drv_out_ctr(parsid_ctrport);
}

static void parsid_port_read(void)
{
    parsid_ctrport |= parsid_PCD;
    parsid_drv_out_ctr(parsid_ctrport);
}

int parsid_check_port(int port)
{
    int retval = parsid_drv_check_port(port);

    if (!retval) {
        parsid_ctrport = parsid_drv_in_ctr();
    }
    return retval;
}

void parsid_reset(void)
{
    if (parsid_open_status) {
        parsid_RW_write();
        parsid_port_write();
        parsid_chip_select();
        parsid_latch_open();
        parsid_drv_out_data(0);
        parsid_reset_start();
        parsid_drv_sleep(1);
        parsid_reset_end();
        parsid_latch_lock();
        parsid_chip_deselect();
    }
}

int parsid_open(int port)
{
    if (!parsid_open_status) {
        if (parsid_drv_init() < 0) {
            return -1;
        }
        if (parsid_check_port(port) < 0) {
            return -1;
        }
        parsid_reset();
        parsid_open_status = 1;
    }
    return 0;
}

int parsid_close(void)
{
    if (parsid_open_status) {
        parsid_reset();
        parsid_open_status = 0;
        return parsid_drv_close();
    }
    return 0;
}

int parsid_read(WORD addr, int chipno)
{
    BYTE value = 0;

    if (parsid_open_status) {
        /* use sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            return sidbuf[addr];
        }
        parsid_drv_out_data((BYTE)(addr & 0x1f));
        parsid_latch_open();
        parsid_latch_lock();
        parsid_port_read();
        parsid_RW_read();
        parsid_chip_select();
        value = parsid_drv_in_data();
        parsid_chip_deselect();
    }
    return (int)value;
}

void parsid_store(WORD addr, BYTE outval, int chipno)
{
    if (parsid_open_status) {
        /* write to sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            sidbuf[addr] = outval;
        }
        parsid_drv_out_data((BYTE)(addr & 0x1f));
        parsid_latch_open();
        parsid_latch_lock();
        parsid_drv_out_data(outval);
        parsid_chip_select();
        parsid_chip_deselect();
    }
}

/* ---------------------------------------------------------------------*/

void parsid_state_read(int chipno, struct sid_parsid_snapshot_state_s *sid_state)
{
    int i;

    for (i = 0; i < 32; ++i) {
        sid_state->regs[i] = sidbuf[i + (chipno * 0x20)];
    }
    sid_state->parsid_ctrport = parsid_ctrport;
}

void parsid_state_write(int chipno, struct sid_parsid_snapshot_state_s *sid_state)
{
    int i;

    for (i = 0; i < 32; ++i) {
        sidbuf[i + (chipno * 0x20)] = sid_state->regs[i];
        parsid_store((WORD)i, sid_state->regs[i], chipno);
    }
    parsid_ctrport = sid_state->parsid_ctrport;
}
#endif
