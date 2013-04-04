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

static BYTE drive_rom1541[DRIVE_ROM1541_SIZE_EXPANDED];
static BYTE drive_rom1541ii[DRIVE_ROM1541II_SIZE_EXPANDED];

#ifdef USE_EMBEDDED
#include "drivedos1570.h"
#include "drivedos1571.h"
#include "drivedos1581.h"
#include "drivedos2000.h"
#include "drivedos4000.h"
#else
static BYTE drive_rom1570[DRIVE_ROM1571_SIZE];
static BYTE drive_rom1571[DRIVE_ROM1571_SIZE];
static BYTE drive_rom1581[DRIVE_ROM1581_SIZE];
static BYTE drive_rom2000[DRIVE_ROM2000_SIZE];
static BYTE drive_rom4000[DRIVE_ROM4000_SIZE];
#endif

/* If nonzero, the ROM image has been loaded.  */
static unsigned int rom1541_loaded = 0;
static unsigned int rom1541ii_loaded = 0;
static unsigned int rom1570_loaded = 0;
static unsigned int rom1571_loaded = 0;
static unsigned int rom1581_loaded = 0;
static unsigned int rom2000_loaded = 0;
static unsigned int rom4000_loaded = 0;

static unsigned int drive_rom1541_size;
static unsigned int drive_rom1541ii_size;


static void iecrom_new_image_loaded(unsigned int dtype)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        if (drive->type == dtype) {
            iecrom_setup_image(drive);
        }
    }
}

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

int iecrom_load_1541(void)
{
    const char *rom_name = NULL;
    int filesize;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName1541", &rom_name);

    filesize = sysfile_load(rom_name, drive_rom1541, DRIVE_ROM1541_SIZE,
                            DRIVE_ROM1541_SIZE_EXPANDED);
    if (filesize < 0) {
        log_error(iecrom_log,
                  "1541 ROM image not found.  "
                  "Hardware-level 1541 emulation is not available.");
        drive_rom1541_size = 0;
    } else {
        rom1541_loaded = 1;
        drive_rom1541_size = (unsigned int)filesize;
        iecrom_do_1541_checksum();
        iecrom_new_image_loaded(DRIVE_TYPE_1541);
        return 0;
    }
    return -1;
}

int iecrom_load_1541ii(void)
{
    const char *rom_name = NULL;
    int filesize;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName1541ii", &rom_name);

    filesize = sysfile_load(rom_name, drive_rom1541ii, DRIVE_ROM1541II_SIZE,
                            DRIVE_ROM1541II_SIZE_EXPANDED);
    if (filesize < 0) {
        log_error(iecrom_log,
                  "1541-II ROM image not found.  "
                  "Hardware-level 1541-II emulation is not available.");
        drive_rom1541ii_size = 0;
    } else {
        rom1541ii_loaded = 1;
        drive_rom1541ii_size = (unsigned int)filesize;
        iecrom_new_image_loaded(DRIVE_TYPE_1541II);
        return 0;
    }
    return -1;
}

int iecrom_load_1570(void)
{
    const char *rom_name = NULL;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName1570", &rom_name);

    if (sysfile_load(rom_name, drive_rom1570, DRIVE_ROM1571_SIZE,
                     DRIVE_ROM1571_SIZE) < 0) {
        log_error(iecrom_log,
                  "1570 ROM image not found.  "
                  "Hardware-level 1570 emulation is not available.");
    } else {
        rom1570_loaded = 1;
        iecrom_new_image_loaded(DRIVE_TYPE_1570);
        return 0;
    }
    return -1;
}

int iecrom_load_1571(void)
{
    const char *rom_name = NULL;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName1571", &rom_name);

    if (sysfile_load(rom_name, drive_rom1571, DRIVE_ROM1571_SIZE,
                     DRIVE_ROM1571_SIZE) < 0) {
        log_error(iecrom_log,
                  "1571 ROM image not found.  "
                  "Hardware-level 1571 emulation is not available.");
    } else {
        rom1571_loaded = 1;
        iecrom_new_image_loaded(DRIVE_TYPE_1571);
        return 0;
    }
    return -1;
}

int iecrom_load_1581(void)
{
    const char *rom_name = NULL;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName1581", &rom_name);

    if (sysfile_load(rom_name, drive_rom1581, DRIVE_ROM1581_SIZE,
                     DRIVE_ROM1581_SIZE) < 0) {
        log_error(iecrom_log,
                  "1581 ROM image not found.  "
                  "Hardware-level 1581 emulation is not available.");
    } else {
        rom1581_loaded = 1;
        iecrom_new_image_loaded(DRIVE_TYPE_1581);
        return 0;
    }
    return -1;
}

int iecrom_load_2000(void)
{
    const char *rom_name = NULL;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName2000", &rom_name);

    if (sysfile_load(rom_name, drive_rom2000, DRIVE_ROM2000_SIZE,
                     DRIVE_ROM2000_SIZE) < 0) {
        log_error(iecrom_log,
                  "2000 ROM image not found.  "
                  "Hardware-level 2000 emulation is not available.");
    } else {
        rom2000_loaded = 1;
        iecrom_new_image_loaded(DRIVE_TYPE_2000);
        return 0;
    }
    return -1;
}

int iecrom_load_4000(void)
{
    const char *rom_name = NULL;

    if (!drive_rom_load_ok) {
        return 0;
    }

    resources_get_string("DosName4000", &rom_name);

    if (sysfile_load(rom_name, drive_rom4000, DRIVE_ROM4000_SIZE,
                     DRIVE_ROM4000_SIZE) < 0) {
        log_error(iecrom_log,
                  "4000 ROM image not found.  "
                  "Hardware-level 4000 emulation is not available.");
    } else {
        rom4000_loaded = 1;
        iecrom_new_image_loaded(DRIVE_TYPE_4000);
        return 0;
    }
    return -1;
}

void iecrom_setup_image(drive_t *drive)
{
    if (rom_loaded) {
        switch (drive->type) {
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
                memcpy(drive->rom, drive_rom1570, DRIVE_ROM1571_SIZE);
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

int iecrom_read(unsigned int type, WORD addr, BYTE *data)
{
    switch (type) {
        case DRIVE_TYPE_1541:
            *data = drive_rom1541[addr & (DRIVE_ROM1541_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_1541II:
            *data = drive_rom1541ii[addr & (DRIVE_ROM1541II_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_1570:
            *data = drive_rom1570[addr & (DRIVE_ROM1571_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_1571:
            *data = drive_rom1571[addr & (DRIVE_ROM1571_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_1581:
            *data = drive_rom1581[addr & (DRIVE_ROM1581_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_2000:
            *data = drive_rom2000[addr & (DRIVE_ROM2000_SIZE - 1)];
            return 0;
        case DRIVE_TYPE_4000:
            *data = drive_rom4000[addr & (DRIVE_ROM4000_SIZE - 1)];
            return 0;
    }

    return -1;
}

int iecrom_check_loaded(unsigned int type)
{
    switch (type) {
        case DRIVE_TYPE_NONE:
            return 0;
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
            if ((!rom1541_loaded && !rom1541ii_loaded && !rom1570_loaded
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
