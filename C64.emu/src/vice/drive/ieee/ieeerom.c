/*
 * ieeerom.c
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
#include "ieeerom.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"


/* Logging goes here.  */
static log_t ieeerom_log;

#ifdef USE_EMBEDDED
#include "drivedos2031.h"
#include "drivedos1001.h"
#include "drivedos2040.h"
#include "drivedos3040.h"
#include "drivedos4040.h"
#else
static BYTE drive_rom2031[DRIVE_ROM2031_SIZE];
static BYTE drive_rom1001[DRIVE_ROM1001_SIZE];
static BYTE drive_rom2040[DRIVE_ROM2040_SIZE];
static BYTE drive_rom3040[DRIVE_ROM3040_SIZE];
static BYTE drive_rom4040[DRIVE_ROM4040_SIZE];
#endif

/* If nonzero, the ROM image has been loaded.  */
static unsigned int rom2031_loaded = 0;
static unsigned int rom2040_loaded = 0;
static unsigned int rom3040_loaded = 0;
static unsigned int rom4040_loaded = 0;
static unsigned int rom1001_loaded = 0;


int ieeerom_load_2031(void)
{
    return driverom_load("DosName2031", drive_rom2031, &rom2031_loaded,
            DRIVE_ROM2031_SIZE, DRIVE_ROM2031_SIZE, "2031",
            DRIVE_TYPE_2031, NULL);
}

int ieeerom_load_2040(void)
{
    return driverom_load("DosName2040", drive_rom2040, &rom2040_loaded,
            DRIVE_ROM2040_SIZE, DRIVE_ROM2040_SIZE, "2040",
            DRIVE_TYPE_2040, NULL);
}

int ieeerom_load_3040(void)
{
    return driverom_load("DosName3040", drive_rom3040, &rom3040_loaded,
            DRIVE_ROM3040_SIZE, DRIVE_ROM3040_SIZE, "3040",
            DRIVE_TYPE_3040, NULL);
}

int ieeerom_load_4040(void)
{
    return driverom_load("DosName4040", drive_rom4040, &rom4040_loaded,
            DRIVE_ROM4040_SIZE, DRIVE_ROM4040_SIZE, "4040",
            DRIVE_TYPE_4040, NULL);
}

int ieeerom_load_1001(void)
{
    return driverom_load("DosName1001", drive_rom1001, &rom1001_loaded,
            DRIVE_ROM1001_SIZE, DRIVE_ROM1001_SIZE, "1001/8050/8250",
            DRIVE_TYPE_1001, NULL);
}

void ieeerom_setup_image(drive_t *drive)
{
    if (rom_loaded) {
        switch (drive->type) {
            case DRIVE_TYPE_2031:
                memcpy(&(drive->rom[0x4000]), drive_rom2031,
                       DRIVE_ROM2031_SIZE);
                break;
            case DRIVE_TYPE_2040:
                memcpy(&(drive->rom[DRIVE_ROM_SIZE - DRIVE_ROM2040_SIZE]),
                       drive_rom2040, DRIVE_ROM2040_SIZE);
                break;
            case DRIVE_TYPE_3040:
                memcpy(&(drive->rom[DRIVE_ROM_SIZE - DRIVE_ROM3040_SIZE]),
                       drive_rom3040, DRIVE_ROM3040_SIZE);
                break;
            case DRIVE_TYPE_4040:
                memcpy(&(drive->rom[DRIVE_ROM_SIZE - DRIVE_ROM4040_SIZE]),
                       drive_rom4040, DRIVE_ROM4040_SIZE);
                break;
            case DRIVE_TYPE_1001:
            case DRIVE_TYPE_8050:
            case DRIVE_TYPE_8250:
                memcpy(&(drive->rom[0x4000]), drive_rom1001,
                       DRIVE_ROM1001_SIZE);
                break;
        }
    }
}

int ieeerom_read(unsigned int type, WORD addr, BYTE *data)
{
    switch (type) {
        case DRIVE_TYPE_2031:
            *data = drive_rom2031[addr & (DRIVE_ROM2031_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_2040:
            *data = drive_rom2040[addr & (DRIVE_ROM2040_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_3040:
            *data = drive_rom3040[addr & (DRIVE_ROM3040_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_4040:
            *data = drive_rom4040[addr & (DRIVE_ROM4040_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            *data = drive_rom1001[addr & (DRIVE_ROM1001_SIZE - 1)];
            return 0;
    }

    return -1;
}

int ieeerom_check_loaded(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_NONE:
            return 0;
        case DRIVE_TYPE_2031:
            if (rom2031_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_2040:
            if (rom2040_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_3040:
            if (rom3040_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_4040:
            if (rom4040_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            if (rom1001_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_ANY:
            if ((!rom2031_loaded && !rom2040_loaded && !rom3040_loaded
                 && !rom4040_loaded && !rom1001_loaded) && rom_loaded) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}

void ieeerom_init(void)
{
    ieeerom_log = log_open("IEEEDriveROM");
}
