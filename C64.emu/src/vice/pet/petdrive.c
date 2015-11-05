/*
 * petdrive.c
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
#include "iecieee.h"
#include "ieee.h"
#include "machine-drive.h"
#include "types.h"


int machine_drive_resources_init(void)
{
    /* FIXME: drive depends on machine (sub) type */
    return drive_resources_type_init(DRIVE_TYPE_2031) | ieee_drive_resources_init();
}

void machine_drive_resources_shutdown(void)
{
    ieee_drive_resources_shutdown();
}

int machine_drive_cmdline_options_init(void)
{
    return ieee_drive_cmdline_options_init();
}

void machine_drive_init(struct drive_context_s *drv)
{
    iecieee_drive_init(drv);
    ieee_drive_init(drv);
}

void machine_drive_shutdown(struct drive_context_s *drv)
{
    iecieee_drive_shutdown(drv);
    ieee_drive_shutdown(drv);
}

void machine_drive_reset(struct drive_context_s *drv)
{
    iecieee_drive_reset(drv);
    ieee_drive_reset(drv);
}

void machine_drive_mem_init(struct drive_context_s *drv, unsigned int type)
{
    ieee_drive_mem_init(drv, type);
}

void machine_drive_setup_context(struct drive_context_s *drv)
{
    iecieee_drive_setup_context(drv);
    ieee_drive_setup_context(drv);
}

void machine_drive_idling_method(unsigned int dnr)
{
}

void machine_drive_vsync_hook(void)
{
}

void machine_drive_rom_load(void)
{
    ieee_drive_rom_load();
}

void machine_drive_rom_setup_image(unsigned int dnr)
{
    ieee_drive_rom_setup_image(dnr);
}

int machine_drive_rom_read(unsigned int type, WORD addr, BYTE *data)
{
    if (ieee_drive_rom_read(type, addr, data) == 0) {
        return 0;
    }

    return -1;
}

int machine_drive_rom_check_loaded(unsigned int type)
{
    if (ieee_drive_rom_check_loaded(type) == 0) {
        return 0;
    }

    return -1;
}

void machine_drive_rom_do_checksum(unsigned int dnr)
{
    ieee_drive_rom_do_checksum(dnr);
}

int machine_drive_snapshot_read(struct drive_context_s *ctxptr,
                                struct snapshot_s *s)
{
    if (iecieee_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }
    if (ieee_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_snapshot_write(struct drive_context_s *ctxptr,
                                 struct snapshot_s *s)
{
    if (iecieee_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }
    if (ieee_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_image_attach(struct disk_image_s *image, unsigned int unit)
{
    return ieee_drive_image_attach(image, unit);
}

int machine_drive_image_detach(struct disk_image_s *image, unsigned int unit)
{
    return ieee_drive_image_detach(image, unit);
}

void machine_drive_port_default(struct drive_context_s *drv)
{
}

void machine_drive_flush(void)
{
    drive_gcr_data_writeback_all();
}

void machine_drive_stub(void)
{
}
