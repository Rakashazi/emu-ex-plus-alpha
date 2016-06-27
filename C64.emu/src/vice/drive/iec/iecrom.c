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
#include "iecrom.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"


#define DRIVE_ROM1541_CHECKSUM      1991711


/* Logging goes here.  */
static log_t iecrom_log;

static BYTE drive_rom1540[DRIVE_ROM1540_SIZE_EXPANDED];
static BYTE drive_rom1541[DRIVE_ROM1541_SIZE_EXPANDED];
static BYTE drive_rom1541ii[DRIVE_ROM1541II_SIZE_EXPANDED];

#ifdef USE_EMBEDDED
#include "drivedos1570.h"
#include "drivedos1571.h"
#include "drivedos1581.h"
#else
static BYTE drive_rom1570[DRIVE_ROM1570_SIZE];
static BYTE drive_rom1571[DRIVE_ROM1571_SIZE];
static BYTE drive_rom1581[DRIVE_ROM1581_SIZE];
#endif

static BYTE drive_rom2000[DRIVE_ROM2000_SIZE];
static BYTE drive_rom4000[DRIVE_ROM4000_SIZE];

/* If nonzero, the ROM image has been loaded.  */
static unsigned int rom1540_loaded = 0;
static unsigned int rom1541_loaded = 0;
static unsigned int rom1541ii_loaded = 0;
static unsigned int rom1570_loaded = 0;
static unsigned int rom1571_loaded = 0;
static unsigned int rom1581_loaded = 0;
static unsigned int rom2000_loaded = 0;
static unsigned int rom4000_loaded = 0;

static unsigned int drive_rom1540_size;
static unsigned int drive_rom1541_size;
static unsigned int drive_rom1541ii_size;


static int iecrom_do_1541_checksum(void)
{
    int i;
    unsigned long s;

    /* Calculate ROM checksum.  */
    for (i = DRIVE_ROM1541_SIZE_EXPANDED - drive_rom1541_size, s = 0;
         i < DRIVE_ROM1541_SIZE_EXPANDED; i++) {
        s += drive_rom1541[i];
    }

    if (s != DRIVE_ROM1541_CHECKSUM) {
        log_warning(iecrom_log, "Unknown 1541 ROM image.  Sum: %lu.", s);
    }

    return 0;
}

int iecrom_load_1540(void)
{
    return driverom_load("DosName1540", drive_rom1540, &rom1540_loaded,
            DRIVE_ROM1540_SIZE, DRIVE_ROM1540_SIZE_EXPANDED, "1540",
            DRIVE_TYPE_1540, &drive_rom1540_size);
}

int iecrom_load_1541(void)
{
    return driverom_load("DosName1541", drive_rom1541, &rom1541_loaded,
            DRIVE_ROM1541_SIZE, DRIVE_ROM1541_SIZE_EXPANDED, "1541",
            DRIVE_TYPE_1541, &drive_rom1541_size);
}

int iecrom_load_1541ii(void)
{
    return driverom_load("DosName1541ii", drive_rom1541ii,
            &rom1541ii_loaded, DRIVE_ROM1541II_SIZE,
            DRIVE_ROM1541II_SIZE_EXPANDED, "1541-II", DRIVE_TYPE_1541II,
            &drive_rom1541ii_size);
}

int iecrom_load_1570(void)
{
    return driverom_load("DosName1570", drive_rom1570, &rom1570_loaded,
            DRIVE_ROM1570_SIZE, DRIVE_ROM1570_SIZE, "1570", DRIVE_TYPE_1570, NULL);
}

int iecrom_load_1571(void)
{
    return driverom_load("DosName1571", drive_rom1571, &rom1571_loaded,
            DRIVE_ROM1571_SIZE, DRIVE_ROM1571_SIZE, "1571", DRIVE_TYPE_1571, NULL);
}

int iecrom_load_1581(void)
{
    return driverom_load("DosName1581", drive_rom1581, &rom1581_loaded,
            DRIVE_ROM1581_SIZE, DRIVE_ROM1581_SIZE, "1581", DRIVE_TYPE_1581, NULL);
}

int iecrom_load_2000(void)
{
    return driverom_load("DosName2000", drive_rom2000, &rom2000_loaded,
            DRIVE_ROM2000_SIZE, DRIVE_ROM2000_SIZE, "2000", DRIVE_TYPE_2000, NULL);
}

int iecrom_load_4000(void)
{
    return driverom_load("DosName4000", drive_rom4000, &rom4000_loaded,
            DRIVE_ROM4000_SIZE, DRIVE_ROM4000_SIZE, "4000", DRIVE_TYPE_4000, NULL);
}

void iecrom_setup_image(drive_t *drive)
{
    if (rom_loaded) {
        switch (drive->type) {
            case DRIVE_TYPE_1540:
                if (drive_rom1540_size <= DRIVE_ROM1540_SIZE) {
                    memcpy(drive->rom, &drive_rom1540[DRIVE_ROM1540_SIZE],
                           DRIVE_ROM1540_SIZE);
                    memcpy(&(drive->rom[DRIVE_ROM1540_SIZE]),
                           &drive_rom1540[DRIVE_ROM1540_SIZE],
                           DRIVE_ROM1540_SIZE);
                } else {
                    memcpy(drive->rom, drive_rom1540,
                           DRIVE_ROM1540_SIZE_EXPANDED);
                }
                break;
            case DRIVE_TYPE_1541:
                if (drive_rom1541_size <= DRIVE_ROM1541_SIZE) {
                    memcpy(drive->rom, &drive_rom1541[DRIVE_ROM1541_SIZE],
                           DRIVE_ROM1541_SIZE);
                    memcpy(&(drive->rom[DRIVE_ROM1541_SIZE]),
                           &drive_rom1541[DRIVE_ROM1541_SIZE],
                           DRIVE_ROM1541_SIZE);
                } else {
                    memcpy(drive->rom, drive_rom1541,
                           DRIVE_ROM1541_SIZE_EXPANDED);
                }
                break;
            case DRIVE_TYPE_1541II:
                if (drive_rom1541ii_size <= DRIVE_ROM1541II_SIZE) {
                    memcpy(drive->rom, &drive_rom1541ii[DRIVE_ROM1541II_SIZE],
                           DRIVE_ROM1541II_SIZE);
                    memcpy(&(drive->rom[DRIVE_ROM1541II_SIZE]),
                           &drive_rom1541ii[DRIVE_ROM1541II_SIZE],
                           DRIVE_ROM1541II_SIZE);
                } else {
                    memcpy(drive->rom, drive_rom1541ii,
                           DRIVE_ROM1541II_SIZE_EXPANDED);
                }
                break;
            case DRIVE_TYPE_1570:
                memcpy(drive->rom, drive_rom1570, DRIVE_ROM1570_SIZE);
                break;
            case DRIVE_TYPE_1571:
                memcpy(drive->rom, drive_rom1571, DRIVE_ROM1571_SIZE);
                break;
            case DRIVE_TYPE_1581:
                memcpy(drive->rom, drive_rom1581, DRIVE_ROM1581_SIZE);
                break;
            case DRIVE_TYPE_2000:
                memcpy(drive->rom, drive_rom2000, DRIVE_ROM2000_SIZE);
                break;
            case DRIVE_TYPE_4000:
                memcpy(drive->rom, drive_rom4000, DRIVE_ROM4000_SIZE);
                break;
        }
    }
}

int iecrom_check_loaded(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_NONE:
            return 0;
        case DRIVE_TYPE_1540:
            if (rom1540_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1541:
            if (rom1541_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1541II:
            if (rom1541ii_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1570:
            if (rom1570_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1571:
            if (rom1571_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_1581:
            if (rom1581_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_2000:
            if (rom2000_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_4000:
            if (rom4000_loaded < 1 && rom_loaded) {
                return -1;
            }
            break;
        case DRIVE_TYPE_ANY:
            if ((!rom1540_loaded && !rom1541_loaded && !rom1541ii_loaded && !rom1570_loaded
                 && !rom1571_loaded && !rom1581_loaded && !rom2000_loaded)
                && !rom4000_loaded && rom_loaded) {
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}

void iecrom_do_checksum(drive_t *drive)
{
    if (drive->type == DRIVE_TYPE_1541) {
        iecrom_do_1541_checksum();
    }
}

void iecrom_init(void)
{
    iecrom_log = log_open("IECDriveROM");
}
