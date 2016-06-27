/*
 * tcbmrom.c
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
#include "log.h"
#include "resources.h"
#include "sysfile.h"
#include "tcbmrom.h"


/* Logging goes here.  */
static log_t tcbmrom_log;

#ifdef USE_EMBEDDED
#include "drivedos1551.h"
#else
static BYTE drive_rom1551[DRIVE_ROM1551_SIZE];
#endif

/* If nonzero, the ROM image has been loaded.  */
static unsigned int rom1551_loaded = 0;


int tcbmrom_load_1551(void)
{
    return driverom_load("DosName1551", drive_rom1551, &rom1551_loaded,
            DRIVE_ROM1551_SIZE, DRIVE_ROM1551_SIZE, "1551",
            DRIVE_TYPE_1551, NULL);
}

void tcbmrom_setup_image(drive_t *drive)
{
    if (rom_loaded) {
        switch (drive->type) {
            case DRIVE_TYPE_1551:
                memcpy(&(drive->rom[0x4000]), drive_rom1551,
                       DRIVE_ROM1551_SIZE);
                break;
        }
    }
}

int tcbmrom_check_loaded(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_NONE:
            return 0;
        case DRIVE_TYPE_1551:
            if (rom1551_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_ANY:
            if ((!rom1551_loaded) && rom_loaded) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}

void tcbmrom_init(void)
{
    tcbmrom_log = log_open("TCBMDriveROM");
}
