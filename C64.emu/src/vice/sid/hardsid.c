/*
 * hardsid.c - Generic hardsid abstraction layer.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  HardSID Support <support@hardsid.com>
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

/* #define DEBUG_HARDSID */
/* define to trace hardsid stuff without having a hardsid */
/* #define DEBUG_HARDSID_DUMMY */

#include "vice.h"

#include "hardsid.h"

#ifdef HAVE_HARDSID

#include <stdio.h>
#include <string.h>

#include "sid-snapshot.h"
#include "types.h"
#include "log.h"

#if defined(DEBUG_HARDSID_DUMMY)

#define hardsid_drv_available() 1
#define hardsid_drv_reset() printf("hardsid_drv_reset\n")
#define hardsid_drv_open() (printf("hardsid_drv_open\n"), 0)
#define hardsid_drv_close() printf("hardsid_drv_close\n")
#define hardsid_drv_read(addr, chipno)  (printf("hardsid_drv_read addr:%02x chip:%d\n", addr, chipno), 1)
#define hardsid_drv_store(addr, val, chipno) printf("hardsid_drv_store addr:%02x val:%02x chip:%d\n", addr, val, chipno)
#define hardsid_drv_set_device(chipno, device) printf("hardsid_drv_set_device chip:%02x device:%u\n", chipno, device)
#define hardsid_drv_state_read(chipno, sid_state) printf("hardsid_drv_state_read chip:%d sid_state:%p\n", chipno, sid_state)
#define hardsid_drv_state_write(chipno, sid_state) printf("hardsid_drv_state_write chip:%d sid_state:%p\n", chipno, sid_state)

#endif

#if defined(DEBUG_HARDSID_DUMMY) || defined(DEBUG_HARDSID)
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

static int hardsid_is_open = -1;

/* buffer containing current register state of SIDs */
static uint8_t sidbuf[0x20 * HS_MAXSID];

void hardsid_reset(void)
{
    if (!hardsid_is_open) {
        hardsid_drv_reset();
    }
}

int hardsid_open(void)
{
    if (hardsid_is_open) {
        hardsid_is_open = hardsid_drv_open();
        memset(sidbuf, 0, sizeof(sidbuf));
    }
    DBG(("hardsid_open hardsid_is_open=%d\n", hardsid_is_open));
    return hardsid_is_open;
}

int hardsid_close(void)
{
    if (!hardsid_is_open) {
        hardsid_drv_close();
        hardsid_is_open = -1;
    }
    DBG(("hardsid_close hardsid_is_open=%d\n", hardsid_is_open));
    return 0;
}

int hardsid_read(uint16_t addr, int chipno)
{
    if (!hardsid_is_open && chipno < HS_MAXSID) {
        /* use sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            return sidbuf[(chipno * 0x20) + addr];
        }
        return hardsid_drv_read(addr, chipno);
    }

    return 0;
}

void hardsid_store(uint16_t addr, uint8_t val, int chipno)
{
    if (!hardsid_is_open && chipno < HS_MAXSID) {
        /* write to sidbuf[] for write-only registers */
        if (addr <= 0x18) {
            sidbuf[(chipno * 0x20) + addr] = val;
        }
        hardsid_drv_store(addr, val, chipno);
    }
}

void hardsid_set_machine_parameter(long cycles_per_sec)
{
}

int hardsid_available(void)
{
    if (hardsid_is_open) {
        hardsid_open();
    }

    if (!hardsid_is_open) {
        return hardsid_drv_available();
    }
    return 0;
}

void hardsid_set_device(unsigned int chipno, unsigned int device)
{
    if (!hardsid_is_open) {
        hardsid_drv_set_device(chipno, device);
    }
}

/* ---------------------------------------------------------------------*/

void hardsid_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < HS_MAXSID) {
        for (i = 0; i < 32; ++i) {
            sid_state->regs[i] = sidbuf[i + (chipno * 0x20)];
        }
        hardsid_drv_state_read(chipno, sid_state);
    }
}

void hardsid_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state)
{
    int i;

    if (chipno < HS_MAXSID) {
        for (i = 0; i < 32; ++i) {
            sidbuf[i + (chipno * 0x20)] = sid_state->regs[i];
        }
        hardsid_drv_state_write(chipno, sid_state);
    }
}
#else
int hardsid_available(void)
{
    return 0;
}
#endif
