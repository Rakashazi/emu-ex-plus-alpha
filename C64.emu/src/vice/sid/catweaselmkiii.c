/*
 * catweaselmkiii.c - Generic cw3 abstraction layer for arch specific cw3 drivers.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Dirk Jagdmann <doj@cubic.org>
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

#ifdef HAVE_CATWEASELMKIII

#include <string.h>

#include "catweaselmkiii.h"
#include "sid-snapshot.h"
#include "types.h"


/* buffer containing current register state of SIDs */
static BYTE sidbuf[CW_MAXCARDS * 0x20];

/* 0 = pal, !0 = ntsc */
static BYTE sid_ntsc = 0;

static long sid_cycles;

/* cw3 device is open */
static int cw3_open = -1;

int catweaselmkiii_open(void)
{
    if (cw3_open == -1) {
        cw3_open = catweaselmkiii_drv_open();
        memset(sidbuf, 0, sizeof(sidbuf));
    }
    return cw3_open;
}

int catweaselmkiii_close(void)
{
    int retval = 0;

    if (cw3_open != -1) {
        retval = catweaselmkiii_drv_close();
        cw3_open = -1;
    }
    return retval;
}

int catweaselmkiii_read(WORD addr, int chipno)
{
    if (cw3_open != -1 && chipno < CW_MAXCARDS) {
        /* use sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            return sidbuf[(chipno * 0x20) + addr];
        }
        return catweaselmkiii_drv_read(addr, chipno);
    }
    return 0;
}

void catweaselmkiii_store(WORD addr, BYTE val, int chipno)
{
    if (cw3_open != -1 && chipno < CW_MAXCARDS) {
        /* write to sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            sidbuf[(chipno * 0x20) + addr] = val;
        }
        catweaselmkiii_drv_store(addr, val, chipno);
    }
}

/* set current main clock frequency, which gives us the possibilty to
   choose between pal and ntsc frequencies */
void catweaselmkiii_set_machine_parameter(long cycles_per_sec)
{
    sid_cycles = cycles_per_sec;
    sid_ntsc = (BYTE)((cycles_per_sec <= 1000000) ? 0 : 1);
    catweaselmkiii_drv_set_machine_parameter(cycles_per_sec);
}

int catweaselmkiii_available(void)
{
    int i = catweaselmkiii_open();

    if (!i) {
        return catweaselmkiii_drv_available();
    }

    return 0;
}

int catweaselmkiii_get_ntsc(void)
{
    return sid_ntsc;
}

/* ---------------------------------------------------------------------*/

void catweaselmkiii_state_read(int chipno, struct sid_cw3_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < CW_MAXCARDS) {
        sid_state->ntsc = sid_ntsc;

        sid_state->cycles_per_second = (DWORD)sid_cycles;

        for (i = 0; i < 32; ++i) {
            sid_state->regs[i] = sidbuf[i + (chipno * 0x20)];
        }
    }
}

void catweaselmkiii_state_write(int chipno, struct sid_cw3_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < CW_MAXCARDS) {
        catweaselmkiii_set_machine_parameter((long)sid_state->cycles_per_second);

        for (i = 0; i < 32; ++i) {
            sidbuf[i + (chipno * 0x20)] = sid_state->regs[i];
            catweaselmkiii_drv_store((WORD)i, sid_state->regs[i], chipno);
        }
    }
}
#endif
