/*
 * machine-bus.c - Generic interface to IO bus functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdio.h>

#include "lib.h"
#include "log.h"
#include "machine-bus.h"
#include "serial.h"
#include "types.h"

/* #define DEBUG_BUS */

#ifdef DEBUG_BUS
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define SERIAL_DEVICE_NOT_PRESENT       (0x80)

/* Call this if device is not attached: -128 == device not present.  */

/*
 * A whole bunch of function stubs with proper prototypes to fix warnings
 * about incompatible function pointers
 */

static int fn_getf(struct vdrive_s *foo, uint8_t *bar, unsigned int bas)
{
    return SERIAL_DEVICE_NOT_PRESENT;
}

static int fn_putf(struct vdrive_s *foo, uint8_t bar, unsigned int bas)
{
    return SERIAL_DEVICE_NOT_PRESENT;
}


static int fn_openf(struct vdrive_s *foo, const uint8_t *bar, unsigned int bas,
        unsigned int meloen, struct cbmdos_cmd_parse_s *appel)
{
    return SERIAL_DEVICE_NOT_PRESENT;
}


static int fn_closef(struct vdrive_s *foo, unsigned int bar)
{
    return SERIAL_DEVICE_NOT_PRESENT;
}

static void fn_flushf(struct vdrive_s *foo, unsigned int bar)
{
}

static void fn_listenf(struct vdrive_s *foo, unsigned int bar)
{
}

void machine_bus_init(void)
{
    unsigned int i;

    for (i = 0; i < SERIAL_MAXDEVICES; i++) {
        serial_t *p;

        p = serial_device_get(i);

        p->inuse = 0;
        p->getf = fn_getf;
        p->putf = fn_putf;
        p->openf = fn_openf;
        p->closef = fn_closef;
        p->flushf = fn_flushf;
        p->listenf = fn_listenf;
    }

    machine_bus_init_machine();
}

int machine_bus_device_attach(unsigned int unit, const char *name,
                              int (*getf)(struct vdrive_s *, uint8_t *, unsigned int),
                              int (*putf)(struct vdrive_s *, uint8_t, unsigned int),
                              int (*openf)(struct vdrive_s *, const uint8_t *,
                                           unsigned int, unsigned int,
                                           struct cbmdos_cmd_parse_s *),
                              int (*closef)(struct vdrive_s *, unsigned int),
                              void (*flushf)(struct vdrive_s *, unsigned int),
                              void (*listenf)(struct vdrive_s *, unsigned int))
{
    serial_t *p;
    int i;

    if (unit >= SERIAL_MAXDEVICES) {
        return 1;
    }

    p = serial_device_get(unit);

    DBG(("machine_bus_device_attach unit %d devtype:%d inuse:%d\n", unit, p->device, p->inuse));

    if (p->inuse != 0) {
        machine_bus_device_detach(unit);
    }

    if (p->device != SERIAL_DEVICE_NONE) {
        p->getf = getf;
        p->putf = putf;
        p->openf = openf;
        p->closef = closef;
        p->flushf = flushf;
        p->listenf = listenf;
        p->inuse = 1;
        if (p->name) {
            lib_free(p->name);
        }
        p->name = lib_strdup(name);
    }

    for (i = 0; i < 16; i++) {
        p->nextok[i] = 0;
        p->isopen[i] = 0;
    }

    return 0;
}

/* Detach and kill serial devices.  */
int machine_bus_device_detach(unsigned int unit)
{
    serial_t *p;

    DBG(("machine_bus_device_detach unit %d\n", unit));

    if (unit >= SERIAL_MAXDEVICES) {
        log_error(LOG_DEFAULT, "Illegal device number %u.", unit);
        return -1;
    }

    p = serial_device_get(unit);

    if (p != NULL && p->inuse != 0) {
        p->inuse = 0;
        if (p->name) {
            lib_free(p->name);
        }
        p->name = NULL;

        p->getf = fn_getf;
        p->putf = fn_putf;
        p->openf = fn_openf;
        p->closef = fn_closef;
        p->flushf = fn_flushf;
        p->listenf = fn_listenf;
    }

    return 0;
}
