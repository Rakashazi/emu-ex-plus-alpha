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

static void drive_sync_cpu_set_factor(drive_context_t *drv,
                                      unsigned int sync_factor)
{
    drv->cpud->sync_factor = sync_factor;
}

void drivesync_factor(struct drive_context_s *drv)
{
    drive_sync_cpu_set_factor(drv, drv->drive->clock_frequency
                              * sync_factor);
}

void drive_set_machine_parameter(long cycles_per_sec)
{
    unsigned int dnr;

    sync_factor = (unsigned int)floor(65536.0 * (1000000.0 / ((double)cycles_per_sec)));

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drivesync_factor(drive_context[dnr]);
    }
}

void drivesync_set_1571(int new_sync, struct drive_context_s *drv)
{
    unsigned int dnr;

    dnr = drv->mynumber;

    if (rom_loaded) {
        rotation_rotate_disk(drv->drive);
        rotation_init(new_sync ? 1 : 0, dnr);
        drv->drive->clock_frequency = (new_sync) ? 2 : 1;
        drivesync_factor(drv);
    }
}

void drivesync_set_4000(struct drive_context_s *drv, int new_sync)
{
    if (rom_loaded && drv->drive->type == DRIVE_TYPE_4000) {
        drv->drive->clock_frequency = (new_sync) ? 4 : 2;
        drivesync_factor(drv);
    }
}

void drivesync_clock_frequency(unsigned int type, drive_t *drive)
{
    switch (type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
            drive->clock_frequency = 1;
            break;
        case DRIVE_TYPE_1551:
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            drive->clock_frequency = 2;
            break;
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            drive->clock_frequency = 1;
            break;
        default:
            drive->clock_frequency = 1;
    }
}
