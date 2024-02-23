/*
 * vic20drive.c
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
#include "iec.h"
#include "iecieee.h"
#include "ieee.h"
#include "machine-drive.h"
#include "types.h"


int machine_drive_resources_init(void)
{
    /* init drive type resource last, so the ROMs are loaded when it initializes */
    return iec_drive_resources_init() |
           ieee_drive_resources_init() |
           drive_resources_type_init(DRIVE_TYPE_1540);
}

void machine_drive_resources_shutdown(void)
{
    iec_drive_resources_shutdown();
    ieee_drive_resources_shutdown();
}

int machine_drive_cmdline_options_init(void)
{
    return iec_drive_cmdline_options_init() | ieee_drive_cmdline_options_init();
}

void machine_drive_init(struct diskunit_context_s *drv)
{
    iec_drive_init(drv);
    iecieee_drive_init(drv);
    ieee_drive_init(drv);
}

void machine_drive_shutdown(struct diskunit_context_s *drv)
{
    iec_drive_shutdown(drv);
    iecieee_drive_shutdown(drv);
    ieee_drive_shutdown(drv);
}

void machine_drive_reset(struct diskunit_context_s *drv)
{
    iec_drive_reset(drv);
    iecieee_drive_reset(drv);
    ieee_drive_reset(drv);
}

void machine_drive_mem_init(struct diskunit_context_s *drv, unsigned int type)
{
    iec_drive_mem_init(drv, type);
    ieee_drive_mem_init(drv, type);
}

void machine_drive_setup_context(struct diskunit_context_s *drv)
{
    iec_drive_setup_context(drv);
    iecieee_drive_setup_context(drv);
    ieee_drive_setup_context(drv);
}

void machine_drive_idling_method(unsigned int dnr)
{
    iec_drive_idling_method(dnr);
}

void machine_drive_rom_load(void)
{
    iec_drive_rom_load();
    ieee_drive_rom_load();
}

void machine_drive_rom_setup_image(unsigned int dnr)
{
    iec_drive_rom_setup_image(dnr);
    ieee_drive_rom_setup_image(dnr);
}

int machine_drive_rom_check_loaded(unsigned int type)
{
    if (iec_drive_rom_check_loaded(type) == 0) {
        return 0;
    }
    if (ieee_drive_rom_check_loaded(type) == 0) {
        return 0;
    }

    return -1;
}

void machine_drive_rom_do_checksum(unsigned int dnr)
{
    iec_drive_rom_do_checksum(dnr);
    ieee_drive_rom_do_checksum(dnr);
}

int machine_drive_snapshot_read(struct diskunit_context_s *ctxptr,
                                struct snapshot_s *s)
{
    if (iec_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }
    if (iecieee_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }
    if (ieee_drive_snapshot_read(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_snapshot_write(struct diskunit_context_s *ctxptr,
                                 struct snapshot_s *s)
{
    if (iec_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }
    if (iecieee_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }
    if (ieee_drive_snapshot_write(ctxptr, s) < 0) {
        return -1;
    }

    return 0;
}

int machine_drive_image_attach(struct disk_image_s *image, unsigned int unit, unsigned int drive)
{
    return iec_drive_image_attach(image, unit, drive)
           & ieee_drive_image_attach(image, unit, drive);
}

int machine_drive_image_detach(struct disk_image_s *image, unsigned int unit, unsigned int drive)
{
    return iec_drive_image_detach(image, unit, drive)
           & ieee_drive_image_detach(image, unit, drive);
}

void machine_drive_port_default(struct diskunit_context_s *drv)
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

/** \brief  List of drive type names and ID's supported by VIC20
 *
 * Convenience function for UI's. This list should be updated whenever drive
 * types are added or removed.
 */
static drive_type_info_t drive_type_info_list[] = {
    { DRIVE_NAME_NONE, DRIVE_TYPE_NONE },
    { DRIVE_NAME_1540, DRIVE_TYPE_1540 },
    { DRIVE_NAME_1541, DRIVE_TYPE_1541 },
    { DRIVE_NAME_1541II, DRIVE_TYPE_1541II },
    { DRIVE_NAME_1570, DRIVE_TYPE_1570 },
    { DRIVE_NAME_1571, DRIVE_TYPE_1571 },
    { DRIVE_NAME_1581, DRIVE_TYPE_1581 },
    { DRIVE_NAME_2000, DRIVE_TYPE_2000 },
    { DRIVE_NAME_4000, DRIVE_TYPE_4000 },
    { DRIVE_NAME_CMDHD, DRIVE_TYPE_CMDHD },
    { DRIVE_NAME_2031, DRIVE_TYPE_2031 },
    { DRIVE_NAME_2040, DRIVE_TYPE_2040 },
    { DRIVE_NAME_3040, DRIVE_TYPE_3040 },
    { DRIVE_NAME_4040, DRIVE_TYPE_4040 },
    { DRIVE_NAME_1001, DRIVE_TYPE_1001 },
    { DRIVE_NAME_8050, DRIVE_TYPE_8050 },
    { DRIVE_NAME_8250, DRIVE_TYPE_8250 },
    { DRIVE_NAME_9000, DRIVE_TYPE_9000 },
    { NULL, -1 }
};

/** \brief  Get a list of (name, id) tuples for the drives handles by VIC20
 *
 * Usefull for UI's, get a list of currently supported drive types with a name
 * to display and and ID to use in callbacks.
 *
 * \return  list of drive types, NULL terminated
 *
 * \note    'supported' in this context means the drives VIC20 can support, not
 *          what actually is supported due to ROMs and other settings
 */
drive_type_info_t *machine_drive_get_type_info_list(void)
{
    return drive_type_info_list;
}
