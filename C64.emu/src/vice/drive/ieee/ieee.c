/*
 * ieee.c
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

#include "drive-check.h"
#include "drive.h"
#include "drivetypes.h"
#include "fdc.h"
#include "ieee-cmdline-options.h"
#include "ieee-resources.h"
#include "ieee.h"
#include "ieeerom.h"
#include "memieee.h"
#include "parallel.h"
#include "riot.h"
#include "riotd.h"
#include "types.h"
#include "via.h"
#include "via1d2031.h"


int ieee_drive_resources_init(void)
{
    return ieee_resources_init();
}

void ieee_drive_resources_shutdown(void)
{
    ieee_resources_shutdown();
}

int ieee_drive_cmdline_options_init(void)
{
    return ieee_cmdline_options_init();
}

void ieee_drive_init(struct drive_context_s *drv)
{
    ieeerom_init();
    via1d2031_init(drv);
    fdc_init(drv);
    riot1_init(drv);
    riot2_init(drv);
}

void ieee_drive_shutdown(struct drive_context_s *drv)
{
    viacore_shutdown(drv->via1d2031);
    riotcore_shutdown(drv->riot1);
    riotcore_shutdown(drv->riot2);
}

void ieee_drive_reset(struct drive_context_s *drv)
{
    if (drv->drive->type == DRIVE_TYPE_2031) {
        viacore_reset(drv->via1d2031);
    } else {
        viacore_disable(drv->via1d2031);
    }

    if (drive_check_old(drv->drive->type)) {
        fdc_reset(drv->mynumber, drv->drive->type);
        riotcore_reset(drv->riot1);
        riotcore_reset(drv->riot2);
    } else {
        /* alarm is unset by fdc_reset */
        fdc_reset(drv->mynumber, drv->drive->type);
        riotcore_disable(drv->riot1);
        riotcore_disable(drv->riot2);
    }
}

void ieee_drive_mem_init(struct drive_context_s *drv, unsigned int type)
{
    memieee_init(drv, type);
}

void ieee_drive_setup_context(struct drive_context_s *drv)
{
    if (drv->mynumber < DRIVE_NUM) {
        *drv->func = drive_funcs[drv->mynumber];
    }

    via1d2031_setup_context(drv);
    riot1_setup_context(drv);
    riot2_setup_context(drv);
}

void ieee_drive_rom_load(void)
{
    ieeerom_load_2031();
    ieeerom_load_2040();
    ieeerom_load_3040();
    ieeerom_load_4040();
    ieeerom_load_1001();
}

void ieee_drive_rom_setup_image(unsigned int dnr)
{
    ieeerom_setup_image(drive_context[dnr]->drive);
}

int ieee_drive_rom_check_loaded(unsigned int type)
{
    return ieeerom_check_loaded(type);
}

void ieee_drive_rom_do_checksum(unsigned int dnr)
{
}

int ieee_drive_snapshot_read(struct drive_context_s *ctxptr,
                             struct snapshot_s *s)
{
    if (ctxptr->drive->type == DRIVE_TYPE_2031) {
        if (viacore_snapshot_read_module(ctxptr->via1d2031, s) < 0) {
            return -1;
        }
    }

    if (drive_check_old(ctxptr->drive->type)) {
        if (riotcore_snapshot_read_module(ctxptr->riot1, s) < 0
            || riotcore_snapshot_read_module(ctxptr->riot2, s) < 0
            || fdc_snapshot_read_module(s, ctxptr->mynumber) < 0) {
            return -1;
        }
    }

    return 0;
}

int ieee_drive_snapshot_write(struct drive_context_s *ctxptr,
                              struct snapshot_s *s)
{
    if (ctxptr->drive->type == DRIVE_TYPE_2031) {
        if (viacore_snapshot_write_module(ctxptr->via1d2031, s) < 0) {
            return -1;
        }
    }

    if (drive_check_old(ctxptr->drive->type)) {
        if (riotcore_snapshot_write_module(ctxptr->riot1, s) < 0
            || riotcore_snapshot_write_module(ctxptr->riot2, s) < 0
            || fdc_snapshot_write_module(s, ctxptr->mynumber) < 0) {
            return -1;
        }
    }

    return 0;
}

int ieee_drive_image_attach(struct disk_image_s *image, unsigned int unit)
{
    return fdc_attach_image(image, unit);
}

int ieee_drive_image_detach(struct disk_image_s *image, unsigned int unit)
{
    return fdc_detach_image(image, unit);
}

void ieee_drive_parallel_set_atn(int state, drive_context_t *drv)
{
    via1d2031_set_atn(drv->via1d2031, state);
    riot2_set_atn(drv->riot2, state);
}
