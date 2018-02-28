/*
 * ssi2001.c - Generic SSI2001 (ISA SID card) abstraction layer for arch specific SSI2001 drivers.
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

#ifdef HAVE_SSI2001

#include "sid-snapshot.h"
#include "ssi2001.h"
#include "types.h"

#define MAX_SSI2001_SID 1

static BYTE sidbuf[0x20];

static int ssi2001_open_status = -1;

int ssi2001_open(void)
{
    if (ssi2001_open_status) {
        ssi2001_open_status = ssi2001_drv_open();
        if (!ssi2001_open_status) {
            memset(sidbuf, 0, sizeof(sidbuf));
        }
    }
    return ssi2001_open_status;
}

int ssi2001_close(void)
{
    if (!ssi2001_open_status) {
        ssi2001_drv_close();
        ssi2001_open_status = -1;
    }
    return 0;
}

int ssi2001_read(WORD addr, int chipno)
{
    if (!ssi2001_open_status && chipno < MAX_SSI2001_SID) {
        /* use sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            return sidbuf[addr];
        }
        return ssi2001_drv_read(addr, chipno);
    }
    return 0;
}

void ssi2001_store(WORD addr, BYTE val, int chipno)
{
    if (!ssi2001_open_status && chipno < MAX_SSI2001_SID) {
        /* write to sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            sidbuf[addr] = val;
        }
        ssi2001_drv_store(addr, val, chipno);
    }
}

void ssi2001_set_machine_parameter(long cycles_per_sec)
{
}

int ssi2001_available(void)
{
    ssi2001_open();

    if (!ssi2001_open_status) {
        return ssi2001_drv_available();
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

void ssi2001_state_read(int chipno, struct sid_ssi2001_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < MAX_SSI2001_SID) {
        for (i = 0; i < 32; ++i) {
            sid_state->regs[i] = sidbuf[i];
        }
    }
}

void ssi2001_state_write(int chipno, struct sid_ssi2001_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < MAX_SSI2001_SID) {
        for (i = 0; i < 32; ++i) {
            sidbuf[i] = sid_state->regs[i];
            ssi2001_drv_store((WORD)i, sid_state->regs[i], chipno);
        }
    }
}
#endif
