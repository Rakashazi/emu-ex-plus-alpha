/*
 * vdrive-snapshot.h - Virtual disk-drive implementation. Snapshot handling.
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

#include "attach.h"
#include "log.h"
#include "snapshot.h"
#include "vdrive-snapshot.h"
#include "vdrive.h"

static log_t vdrive_snapshot_log = LOG_ERR;

void vdrive_snapshot_init(void)
{
    vdrive_snapshot_log = log_open("VDriveSnapshot");
}

#define SNAP_MAJOR 1
#define SNAP_MINOR 0

int vdrive_snapshot_module_write(snapshot_t *s, int start)
{
    int i;
    char snap_module_name[14];
    snapshot_module_t *m;
    vdrive_t *floppy;

    for (i = start; i <= 11; i++) {
        floppy = file_system_get_vdrive(i);
        if (floppy->image != NULL) {
            sprintf(snap_module_name, "VDRIVEIMAGE%i", i);
            m = snapshot_module_create(s, snap_module_name, ((BYTE)SNAP_MAJOR),
                                       ((BYTE)SNAP_MINOR));
            if (m == NULL) {
                return -1;
            }
            snapshot_module_close(m);
        }
    }
    return 0;
}

int vdrive_snapshot_module_read(snapshot_t *s, int start)
{
    BYTE major_version, minor_version;
    int i;
    snapshot_module_t *m;
    char snap_module_name[14];

    for (i = start; i <= 11; i++) {
        sprintf(snap_module_name, "VDRIVEIMAGE%i", i);
        m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
        if (m == NULL) {
            return 0;
        }

        if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
            log_message(vdrive_snapshot_log,
                        "Snapshot module version (%d.%d) newer than %d.%d.",
                        major_version, minor_version, SNAP_MAJOR, SNAP_MINOR);
        }
        snapshot_module_close(m);
    }
    return 0;
}
