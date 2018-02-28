/*
 * clockport-rrnet.c - ClockPort RRNET emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifdef HAVE_PCAP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clockport.h"
#include "cs8900io.h"
#include "lib.h"
#include "translate.h"
#include "uiapi.h"

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/* Flag: Do we have the RRNET enabled?  */
static int clockport_rrnet_enabled = 0;

static char *clockport_rrnet_owner = NULL;

/* ------------------------------------------------------------------------- */

static void clockport_rrnet_store(WORD address, BYTE val, void *context)
{
    if (address < 0x02) {
        return;
    }
    address ^= 0x08;

    cs8900io_store(address, val);
}

static BYTE clockport_rrnet_read(WORD address, int *valid, void *context)
{
    if (address < 0x02) {
        return 0;
    }
    address ^= 0x08;

    *valid = 1;
    return cs8900io_read(address);
}

static BYTE clockport_rrnet_peek(WORD address, void *context)
{
    if (address < 0x02) {
        return 0;
    }
    address ^= 0x08;

    return cs8900io_read(address);
}

static void clockport_rrnet_reset(void *context)
{
    cs8900io_reset();
}

static int clockport_rrnet_dump(void *context)
{
    return cs8900io_dump();
}

static void clockport_rrnet_close(struct clockport_device_s *device)
{
    if (clockport_rrnet_enabled) {
        cs8900io_disable();
        clockport_rrnet_enabled = 0;
        clockport_rrnet_owner = NULL;
        lib_free(device);
    }
}

/* ------------------------------------------------------------------------- */

int clockport_rrnet_init(void)
{
    cs8900io_init();

    return 0;
}

void clockport_rrnet_shutdown(void)
{
    if (clockport_rrnet_enabled) {
        cs8900io_disable();
    }
}

clockport_device_t *clockport_rrnet_open_device(char *owner)
{
    clockport_device_t *retval = NULL;
    if (clockport_rrnet_enabled) {
        ui_error(translate_text(IDGS_CLOCKPORT_RRNET_IN_USE_BY_S), clockport_rrnet_owner);
        return NULL;
    }
    if (cs8900io_enable(owner) < 0) {
        return NULL;
    }
    retval = lib_malloc(sizeof(clockport_device_t));
    retval->owner = owner;
    retval->devicenr = 0;
    retval->store = clockport_rrnet_store;
    retval->read = clockport_rrnet_read;
    retval->peek = clockport_rrnet_peek;
    retval->reset = clockport_rrnet_reset;
    retval->dump = clockport_rrnet_dump;
    retval->close = clockport_rrnet_close;
    retval->device_context = NULL;

    clockport_rrnet_enabled = 1;

    return retval;
}

#endif /* #ifdef HAVE_PCAP */
