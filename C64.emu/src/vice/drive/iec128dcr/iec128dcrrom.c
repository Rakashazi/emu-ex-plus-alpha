/*
 * iecrom.c
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
#include <string.h>

#include "drive.h"
#include "driverom.h"
#include "drivetypes.h"
#include "iec128dcrrom.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"


/* Logging goes here.  */
static log_t iec128dcrrom_log;

#ifdef USE_EMBEDDED
#include "drived1571cr.h"
#else
static BYTE drive_rom1571cr[DRIVE_ROM1571CR_SIZE];
#endif

/* If nonzero, the ROM image has been loaded.  */
static unsigned int rom1571cr_loaded = 0;


int iec128dcrrom_load_1571cr(void)
{
    return driverom_load("DosName1571cr", drive_rom1571cr, &rom1571cr_loaded,
            DRIVE_ROM1571CR_SIZE, DRIVE_ROM1571CR_SIZE, "1571CR",
            DRIVE_TYPE_1571CR, NULL);
}

void iec128dcrrom_setup_image(drive_t *drive)
{
    if (rom_loaded) {
        switch (drive->type) {
            case DRIVE_TYPE_1571CR:
                memcpy(drive->rom, drive_rom1571cr, DRIVE_ROM1571CR_SIZE);
                break;
        }
    }
}

int iec128dcrrom_check_loaded(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_NONE:
            return 0;
        case DRIVE_TYPE_1571CR:
            if (rom1571cr_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_ANY:
            if ((!rom1571cr_loaded) && rom_loaded) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}

void iec128dcrrom_init(void)
{
    iec128dcrrom_log = log_open("IEC128DCRDriveROM");
}
