/*
 * plusdrive.c
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

#include "drive.h"
#include "iec-plus4exp.h"
#include "iec.h"
#include "iecieee.h"
#include "machine-drive.h"
#include "tcbm.h"
#include "types.h"


int machine_drive_resources_init(void)
{
    return drive_resources_type_init(DRIVE_TYPE_1551)
           | iec_drive_resources_init() | iec_plus4exp_resources_init()
           | tcbm_drive_resources_init();
}

void machine_drive_resources_shutdown(void)
{
    iec_drive_resources_shutdown();
    iec_plus4exp_resources_shutdown();
    tcbm_drive_resources_shutdown();
}

int machine_drive_cmdline_options_init(void)
{
    return iec_drive_cmdline_options_init()
           | iec_plus4exp_cmdline_options_init()
           | tcbm_drive_cmdline_options_init();
}

void machine_drive_init(struct drive_context_s *drv)
{
    iec_drive_init(drv);
    iecieee_drive_init(drv);
    iec_plus4exp_init(drv);
    tcbm_drive_init(drv);
}

void machine_drive_shutdown(struct drive_context_s *drv)
{
    iec_drive_shutdown(drv);
    iecieee_drive_shutdown(drv);
    tcbm_drive_shutdown(drv);
}

void machine_drive_reset(struct drive_context_s *drv)
{
    iec_drive_reset(drv);
    iecieee_drive_reset(drv);
    iec_plus4exp_reset(drv);
    tcbm_drive_reset(drv);
}

void machine_drive_mem_init(struct drive_context_s *drv, unsigned int type)
{
    iec_drive_mem_init(drv, type);
    iec_plus4exp_mem_init(drv, type);
    tcbm_drive_mem_init(drv, type);
}

void machine_drive_setup_context(struct drive_context_s *drv)
{
    iec_drive_setup_context(drv);
    iecieee_drive_setup_context(drv);
    tcbm_drive_setup_context(drv);
}

void machine_drive_idling_method(unsigned int dnr)
{
    iec_drive_idling_method(dnr);
}

void machine_drive_rom_load(void)
{
    iec_drive_rom_load();
    tcbm_drive_rom_load();
}

void machine_drive_rom_setup_image(unsigned int dnr)
{
    iec_drive_rom_setup_image(dnr);
    tcbm_drive_rom_setup_image(dnr);
}

int machine_drive_rom_check_loaded(unsigned int type)
{
    if (iec_drive_rom_check_loaded(type) == 0) {
        return 0;
    }
    if (tcbm_drive_rom_check_loaded(type) == 0) {
        return 0;
    }

    return -1;
}

void machine_drive_rom_do_checksum(unsigned int dnr)
{
    iec_drive_rom_do_checksum(dnr);
    tcbm_drive_rom_do_checksum(dnr);
}

int machine_drive_snapshot_read(struct drive_context_s *ctxptr,
                                struct snapshot_s *s)
{
    if (iec_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }
    if (iecieee_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }
    if (tcbm_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_snapshot_write(struct drive_context_s *ctxptr,
                                 struct snapshot_s *s)
{
    if (iec_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }
    if (iecieee_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }
    if (tcbm_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_image_attach(struct disk_image_s *image, unsigned int unit)
{
    return iec_drive_image_attach(image, unit);
}

int machine_drive_image_detach(struct disk_image_s *image, unsigned int unit)
{
    return iec_drive_image_detach(image, unit);
}

void machine_drive_port_default(struct drive_context_s *drv)
{
    iec_drive_port_default(drv);
}

void machine_drive_flush(void)
{
    drive_gcr_data_writeback_all();
}

void machine_drive_stub(void)
{
}
