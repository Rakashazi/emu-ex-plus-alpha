/*
 * iecieee.c
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

#include "drive.h"
#include "drivetypes.h"
#include "iecieee.h"
#include "types.h"
#include "via.h"
#include "viad.h"
#include "drive-sound.h"

void iecieee_drive_init(struct drive_context_s *drv)
{
    via2d_init(drv);
}

void iecieee_drive_shutdown(struct drive_context_s *drv)
{
    viacore_shutdown(drv->via2);
}

void iecieee_drive_reset(struct drive_context_s *drv)
{
    if (drv->drive->type == DRIVE_TYPE_1541
        || drv->drive->type == DRIVE_TYPE_1541II
        || drv->drive->type == DRIVE_TYPE_1570
        || drv->drive->type == DRIVE_TYPE_1571
        || drv->drive->type == DRIVE_TYPE_1571CR
        || drv->drive->type == DRIVE_TYPE_2031) {
        viacore_reset(drv->via2);
        drive_sound_update(DRIVE_SOUND_MOTOR_ON, drv->mynumber);
    } else {
        viacore_disable(drv->via2);
    }
}

void iecieee_drive_setup_context(struct drive_context_s *drv)
{
    via2d_setup_context(drv);
}

int iecieee_drive_snapshot_read(struct drive_context_s *ctxptr,
                                struct snapshot_s *s)
{
    if (ctxptr->drive->type == DRIVE_TYPE_1541
        || ctxptr->drive->type == DRIVE_TYPE_1541II
        || ctxptr->drive->type == DRIVE_TYPE_1570
        || ctxptr->drive->type == DRIVE_TYPE_1571
        || ctxptr->drive->type == DRIVE_TYPE_1571CR
        || ctxptr->drive->type == DRIVE_TYPE_2031) {
        if (viacore_snapshot_read_module(ctxptr->via2, s) < 0) {
            return -1;
        }
    }

    return 0;
}

int iecieee_drive_snapshot_write(struct drive_context_s *ctxptr,
                                 struct snapshot_s *s)
{
    if (ctxptr->drive->type == DRIVE_TYPE_1541
        || ctxptr->drive->type == DRIVE_TYPE_1541II
        || ctxptr->drive->type == DRIVE_TYPE_1570
        || ctxptr->drive->type == DRIVE_TYPE_1571
        || ctxptr->drive->type == DRIVE_TYPE_1571CR
        || ctxptr->drive->type == DRIVE_TYPE_2031) {
        if (viacore_snapshot_write_module(ctxptr->via2, s) < 0) {
            return -1;
        }
    }

    return 0;
}
