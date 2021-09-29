/*
 * drivesync.c
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

#if !defined(__BEOS__) || !defined(WORDS_BIGENDIAN)
#include <math.h>
#endif

#include "drive.h"
#include "drivesync.h"
#include "drivetypes.h"
#include "rotation.h"


static unsigned int sync_factor;

static void drive_sync_cpu_set_factor(diskunit_context_t *drv,
                                      unsigned int sf)
{
    drv->cpud->sync_factor = sf;
}

void drivesync_factor(struct diskunit_context_s *drv)
{
    drive_sync_cpu_set_factor(drv, drv->clock_frequency
                              * sync_factor);
}

void drive_set_machine_parameter(long cycles_per_sec)
{
    unsigned int dnr;

    sync_factor = (unsigned int)floor(65536.0 * (1000000.0 / ((double)cycles_per_sec)));

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        drivesync_factor(diskunit_context[dnr]);
    }
}

void drivesync_set_1571(struct diskunit_context_s *drv, int new_sync) 
{
    unsigned int dnr;

    dnr = drv->mynumber;

    if (rom_loaded) {
        rotation_rotate_disk(drv->drives[0]);
        rotation_init(new_sync ? 1 : 0, dnr);
        drv->clock_frequency = (new_sync) ? 2 : 1;
        drivesync_factor(drv);
    }
}

void drivesync_set_4000(struct diskunit_context_s *drv, int new_sync)
{
    if (rom_loaded && drv->type == DRIVE_TYPE_4000) {
        drv->clock_frequency = (new_sync) ? 4 : 2;
        drivesync_factor(drv);
    }
}

void drivesync_clock_frequency(diskunit_context_t *unit, unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
            unit->clock_frequency = 1;
            break;
        case DRIVE_TYPE_1551:
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
        case DRIVE_TYPE_CMDHD:
            unit->clock_frequency = 2;
            break;
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
        case DRIVE_TYPE_9000:
            unit->clock_frequency = 1;
            break;
        default:
            unit->clock_frequency = 1;
    }
}
