/*
 * tcbm.c
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

#include "drivetypes.h"
#include "mem1551.h"
#include "glue1551.h"
#include "tcbm-cmdline-options.h"
#include "tcbm-resources.h"
#include "tcbm.h"
#include "tcbmrom.h"
#include "tpi.h"
#include "tpid.h"
#include "types.h"
#include "drive-sound.h"

int tcbm_drive_resources_init(void)
{
    return tcbm_resources_init();
}

void tcbm_drive_resources_shutdown(void)
{
    tcbm_resources_shutdown();
}

int tcbm_drive_cmdline_options_init(void)
{
    return tcbm_cmdline_options_init();
}

void tcbm_drive_init(struct drive_context_s *drv)
{
    tcbmrom_init();
    tpid_init(drv);
    glue1551_init(drv);
}

void tcbm_drive_shutdown(struct drive_context_s *drv)
{
    tpicore_shutdown(drv->tpid);
}

void tcbm_drive_reset(struct drive_context_s *drv)
{
    tpicore_reset(drv->tpid);
    glue1551_reset(drv);
    drive_sound_update(DRIVE_SOUND_MOTOR_ON, drv->mynumber);
}

void tcbm_drive_mem_init(struct drive_context_s *drv, unsigned int type)
{
    mem1551_init(drv, type);
}

void tcbm_drive_setup_context(struct drive_context_s *drv)
{
    tpid_setup_context(drv);
}

void tcbm_drive_rom_load(void)
{
    tcbmrom_load_1551();
}

void tcbm_drive_rom_setup_image(unsigned int dnr)
{
    tcbmrom_setup_image(drive_context[dnr]->drive);
}

int tcbm_drive_rom_check_loaded(unsigned int type)
{
    return tcbmrom_check_loaded(type);
}

void tcbm_drive_rom_do_checksum(unsigned int dnr)
{
}

int tcbm_drive_snapshot_read(struct drive_context_s *ctxptr,
                             struct snapshot_s *s)
{
    return 0;
}

int tcbm_drive_snapshot_write(struct drive_context_s *ctxptr,
                              struct snapshot_s *s)
{
    return 0;
}
