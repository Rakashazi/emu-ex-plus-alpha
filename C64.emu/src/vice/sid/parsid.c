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

#include <string.h>

#include "vice.h"

#ifdef HAVE_PARSID

#include "parsid.h"
#include "types.h"

#define MAX_PAR_SID 3

static BYTE sidbuf[0x20 * MAX_PAR_SID];

static BYTE parsid_ctrport[MAX_PAR_SID];
static int parsid_open_status = -1;

/* chip control pin assignments */
static void parsid_chip_select(int chipno)
{
    parsid_ctrport[chipno] |= parsid_STROBE;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_chip_deselect(int chipno)
{
    parsid_ctrport[chipno] &= ~parsid_STROBE;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_reset_start(int chipno)
{
    parsid_ctrport[chipno] |= parsid_SELECTIN;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_reset_end(int chipno)
{
    parsid_ctrport[chipno] &= ~parsid_SELECTIN;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_latch_open(int chipno)
{
    parsid_ctrport[chipno] &= ~parsid_AUTOFEED;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_latch_lock(int chipno)
{
    parsid_ctrport[chipno] |= parsid_AUTOFEED;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_RW_write(int chipno)
{
    parsid_ctrport[chipno] &= ~parsid_nINIT;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_RW_read(int chipno)
{
    parsid_ctrport[chipno] |= parsid_nINIT;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

/* parallel port direction control */
static void parsid_port_write(int chipno)
{
    parsid_ctrport[chipno] &= ~parsid_PCD;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

static void parsid_port_read(int chipno)
{
    parsid_ctrport[chipno] |= parsid_PCD;
    parsid_drv_out_ctr(parsid_ctrport[chipno], chipno);
}

void parsid_reset(void)
{
    int i;

    if (!parsid_open_status) {
        for (i = 0; i < MAX_PAR_SID; ++i) {
            parsid_RW_write(i);
            parsid_port_write(i);
            parsid_chip_select(i);
            parsid_latch_open(i);
            parsid_drv_out_data(0, i);
            parsid_reset_start(i);
            parsid_drv_sleep(1);
            parsid_reset_end(i);
            parsid_latch_lock(i);
            parsid_chip_deselect(i);
        }
    }
}

int parsid_open(void)
{
    if (parsid_open_status) {
        parsid_open_status = parsid_drv_open();
        if (!parsid_open_status) {
            parsid_reset();
            parsid_ctrport[0] = parsid_drv_in_ctr(0);
            parsid_ctrport[1] = parsid_drv_in_ctr(1);
            parsid_ctrport[2] = parsid_drv_in_ctr(2);
            memset(sidbuf, 0, sizeof(sidbuf));
        }
    }
    return parsid_open_status;
}

int parsid_close(void)
{
    if (!parsid_open_status) {
        parsid_reset();
        parsid_open_status = -1;
        return parsid_drv_close();
    }
    return 0;
}

int parsid_read(WORD addr, int chipno)
{
    BYTE value = 0;

    if (!parsid_open_status && chipno < MAX_PAR_SID) {
        /* use sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            return sidbuf[addr + (chipno * 0x20)];
        }
        parsid_drv_out_data((BYTE)(addr & 0x1f), chipno);
        parsid_latch_open(chipno);
        parsid_latch_lock(chipno);
        parsid_port_read(chipno);
        parsid_RW_read(chipno);
        parsid_chip_select(chipno);
        value = parsid_drv_in_data(chipno);
        parsid_chip_deselect(chipno);
        parsid_port_write(chipno);
        parsid_RW_write(chipno);
    }
    return (int)value;
}

void parsid_store(WORD addr, BYTE outval, int chipno)
{
    if (!parsid_open_status && chipno < MAX_PAR_SID) {
        /* write to sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            sidbuf[addr + (chipno * 0x20)] = outval;
        }
        parsid_drv_out_data((BYTE)(addr & 0x1f), chipno);
        parsid_latch_open(chipno);
        parsid_latch_lock(chipno);
        parsid_drv_out_data(outval, chipno);
        parsid_chip_select(chipno);
        parsid_chip_deselect(chipno);
    }
}

int parsid_available(void)
{
    parsid_open();

    if (!parsid_open_status) {
        return parsid_drv_available();
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

void parsid_state_read(int chipno, struct sid_parsid_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < MAX_PAR_SID) {
        for (i = 0; i < 32; ++i) {
            sid_state->regs[i] = sidbuf[i + (chipno * 0x20)];
        }
        sid_state->parsid_ctrport = parsid_ctrport[chipno];
    }
}

void parsid_state_write(int chipno, struct sid_parsid_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < MAX_PAR_SID) {
        for (i = 0; i < 32; ++i) {
            sidbuf[i + (chipno * 0x20)] = sid_state->regs[i];
            parsid_store((WORD)i, sid_state->regs[i], chipno);
        }
        parsid_ctrport[chipno] = sid_state->parsid_ctrport;
    }
}
#endif
