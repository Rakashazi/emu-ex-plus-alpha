/*
 * iec.c - IEC drive specific routines.
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

#include "cia.h"
#include "ciad.h"
#include "drive.h"
#include "drivetypes.h"
#include "iec-cmdline-options.h"
#include "iec-resources.h"
#include "iec.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "iecrom.h"
#include "lib.h"
#include "memiec.h"
#include "resources.h"
#include "types.h"
#include "via.h"
#include "via1d1541.h"
#include "wd1770.h"
#include "via4000.h"
#include "pc8477.h"


/* Pointer to the IEC bus structure.  */
static iecbus_t *drive_iecbus;


int iec_drive_resources_init(void)
{
    return iec_resources_init();
}

void iec_drive_resources_shutdown(void)
{
    iec_resources_shutdown();
}

int iec_drive_cmdline_options_init(void)
{
    return iec_cmdline_options_init();
}

void iec_drive_init(struct drive_context_s *drv)
{
    iecrom_init();
    via1d1541_init(drv);
    cia1571_init(drv);
    cia1581_init(drv);
    via4000_init(drv);
    wd1770d_init(drv);
    pc8477d_init(drv);
}

void iec_drive_reset(struct drive_context_s *drv)
{
    if (drv->drive->type == DRIVE_TYPE_1540
        || drv->drive->type == DRIVE_TYPE_1541
        || drv->drive->type == DRIVE_TYPE_1541II
        || drv->drive->type == DRIVE_TYPE_1570
        || drv->drive->type == DRIVE_TYPE_1571
        || drv->drive->type == DRIVE_TYPE_1571CR) {
        viacore_reset(drv->via1d1541);
    } else {
        viacore_disable(drv->via1d1541);
    }

    if (drv->drive->type == DRIVE_TYPE_1570
        || drv->drive->type == DRIVE_TYPE_1571
        || drv->drive->type == DRIVE_TYPE_1571CR) {
        ciacore_reset(drv->cia1571);
    } else {
        ciacore_disable(drv->cia1571);
    }

    if (drv->drive->type == DRIVE_TYPE_1581) {
        ciacore_reset(drv->cia1581);
        wd1770_reset(drv->wd1770);
    } else {
        ciacore_disable(drv->cia1581);
    }

    if (drv->drive->type == DRIVE_TYPE_2000
        || drv->drive->type == DRIVE_TYPE_4000) {
        viacore_reset(drv->via4000);
        pc8477_reset(drv->pc8477, drv->drive->type == DRIVE_TYPE_4000);
    } else {
        viacore_disable(drv->via4000);
    }
}

void iec_drive_mem_init(struct drive_context_s *drv, unsigned int type)
{
    memiec_init(drv, type);
}

void iec_drive_setup_context(struct drive_context_s *drv)
{
    via1d1541_setup_context(drv);
    cia1571_setup_context(drv);
    cia1581_setup_context(drv);
    via4000_setup_context(drv);
    pc8477_setup_context(drv);
}

void iec_drive_shutdown(struct drive_context_s *drv)
{
    viacore_shutdown(drv->via1d1541);
    ciacore_shutdown(drv->cia1571);
    ciacore_shutdown(drv->cia1581);
    viacore_shutdown(drv->via4000);
    wd1770_shutdown(drv->wd1770);
    pc8477_shutdown(drv->pc8477);
}

void iec_drive_idling_method(unsigned int dnr)
{
    char *tmp;

    tmp = lib_msprintf("Drive%iIdleMethod", dnr + 8);

    resources_touch(tmp);

    lib_free(tmp);
}

void iec_drive_rom_load(void)
{
    iecrom_load_1540();
    iecrom_load_1541();
    iecrom_load_1541ii();
    iecrom_load_1570();
    iecrom_load_1571();
    iecrom_load_1581();
    iecrom_load_2000();
    iecrom_load_4000();
}

void iec_drive_rom_setup_image(unsigned int dnr)
{
    iecrom_setup_image(drive_context[dnr]->drive);
}

int iec_drive_rom_read(unsigned int type, WORD addr, BYTE *data)
{
    return iecrom_read(type, addr, data);
}

int iec_drive_rom_check_loaded(unsigned int type)
{
    return iecrom_check_loaded(type);
}

void iec_drive_rom_do_checksum(unsigned int dnr)
{
    iecrom_do_checksum(drive_context[dnr]->drive);
}

int iec_drive_snapshot_read(struct drive_context_s *ctxptr,
                            struct snapshot_s *s)
{
    switch (ctxptr->drive->type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        if (viacore_snapshot_read_module(ctxptr->via1d1541, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        if (viacore_snapshot_read_module(ctxptr->via1d1541, s) < 0) {
            return -1;
        }
        if (ciacore_snapshot_read_module(ctxptr->cia1571, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_1581:
        if (ciacore_snapshot_read_module(ctxptr->cia1581, s) < 0) {
            return -1;
        }
        if (wd1770_snapshot_read_module(ctxptr->wd1770, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
        if (viacore_snapshot_read_module(ctxptr->via4000, s) < 0) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

int iec_drive_snapshot_write(struct drive_context_s *ctxptr,
                             struct snapshot_s *s)
{
    switch (ctxptr->drive->type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        if (viacore_snapshot_write_module(ctxptr->via1d1541, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        if (viacore_snapshot_write_module(ctxptr->via1d1541, s) < 0) {
            return -1;
        }
        if (ciacore_snapshot_write_module(ctxptr->cia1571, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_1581:
        if (ciacore_snapshot_write_module(ctxptr->cia1581, s) < 0) {
            return -1;
        }
        if (wd1770_snapshot_write_module(ctxptr->wd1770, s) < 0) {
            return -1;
        }
        break;
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
        if (viacore_snapshot_write_module(ctxptr->via4000, s) < 0) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

int iec_drive_image_attach(struct disk_image_s *image, unsigned int unit)
{
    return wd1770_attach_image(image, unit) & pc8477_attach_image(image, unit);
}

int iec_drive_image_detach(struct disk_image_s *image, unsigned int unit)
{
    return wd1770_detach_image(image, unit) & pc8477_detach_image(image, unit);
}

void iec_drive_port_default(struct drive_context_s *drv)
{
    drive_iecbus = iecbus_drive_port();

    if (drive_iecbus != NULL) {
        drive_iecbus->drv_bus[drv->mynumber + 8] = 0xff;
        drive_iecbus->drv_data[drv->mynumber + 8] = 0xff;
    }
}
