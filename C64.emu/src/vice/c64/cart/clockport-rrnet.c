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

#ifdef HAVE_RAWNET

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clockport.h"
#include "cs8900io.h"
#include "lib.h"
#include "uiapi.h"

#include "clockport-rrnet.h"

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/* Flag: Do we have the RRNET enabled?  */
static int clockport_rrnet_enabled = 0;
/* Flag: Type of device ?  */
static int clockport_rrnet_deviceid = 0;

static char *clockport_rrnet_owner = NULL;

/* ------------------------------------------------------------------------- */

static void clockport_rrnet_store(uint16_t address, uint8_t val, void *context)
{
    if (address < 0x02) {
        return;
    }
    address ^= 0x08;

    cs8900io_store(address, val);
}

static uint8_t clockport_rrnet_read(uint16_t address, int *valid, void *context)
{
    if (address < 0x02) {
        return 0;
    }

    /* on the MK3, the MAC and a checksum can be read from the last 4 bytes, see
       http://wiki.icomp.de/wiki/RR-Net#Detecting_MK3 */
    if (clockport_rrnet_deviceid == CLOCKPORT_DEVICE_RRNETMK3) {
        if ((address >= 0x0c) && (address <= 0x0f)) {
            unsigned char mk3mac[4];
            /* use the recommended default for the time being */
            mk3mac[0] = 0xfb;   /* MAC hi */
            mk3mac[1] = 0xff;   /* MAC lo */
            mk3mac[2] = mk3mac[0] ^ mk3mac[1] ^ 0x55; /* checksum 0 */
            mk3mac[3] = (mk3mac[0] + mk3mac[1] + mk3mac[2]) ^ 0xaa; /* checksum 1 */
            *valid = 1;
            return (mk3mac[address - 0x0c]);
        }
    }

    address ^= 0x08;
    *valid = 1;
    return cs8900io_read(address);
}

static uint8_t clockport_rrnet_peek(uint16_t address, void *context)
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
        clockport_rrnet_deviceid = 0;
    }
}

clockport_device_t *clockport_rrnet_open_device(const char *owner, int deviceid)
{
    clockport_device_t *retval = NULL;
    if (clockport_rrnet_enabled) {
        ui_error("ClockPort RRNET already in use by %s.", clockport_rrnet_owner);
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
    clockport_rrnet_deviceid = deviceid;

    return retval;
}

#endif /* #ifdef HAVE_RAWNET */
