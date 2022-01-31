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
#include "diskimage.h"
#include "drive.h"
#include "log.h"
#include "resources.h"
#include "snapshot.h"
#include "vdrive-snapshot.h"
#include "vdrive.h"

/** \brief  Size of a snapshot module name
 *
 * len('VDRIVEIMAGE') + 11 (for %i) + 1 = 24
 * Prepare for dual drives (ie '8:0') -> 32 to be safe
 */
#define SNAP_MODNAME_SIZE   32


static log_t vdrive_snapshot_log = LOG_ERR;

void vdrive_snapshot_init(void)
{
    vdrive_snapshot_log = log_open("VDriveSnapshot");
}

#define SNAP_MAJOR 2
#define SNAP_MINOR 0

int vdrive_snapshot_module_write(snapshot_t *s)
{
    int i, j, tde;
    char snap_module_name[SNAP_MODNAME_SIZE];
    snapshot_module_t *m;
    vdrive_t *floppy;
    disk_image_t *image;

    for (i = DRIVE_UNIT_MIN; i <= DRIVE_UNIT_MAX; i++) {
        resources_get_int_sprintf("Drive%iTrueEmulation", &tde, i);
        if (tde == 0) {
            floppy = file_system_get_vdrive(i);
            for (j = DRIVE_NUMBER_MIN; j <= DRIVE_NUMBER_MAX; j++) {
                image = vdrive_get_image(floppy, j);
                if (image != NULL) {
                    snprintf(snap_module_name, SNAP_MODNAME_SIZE, "VDRIVEIMAGE%i", i);
                    m = snapshot_module_create(s, snap_module_name, ((uint8_t)SNAP_MAJOR),
                                            ((uint8_t)SNAP_MINOR));
                    if (m == NULL) {
                        return -1;
                    }
                    snapshot_module_close(m);
                }
            }
        }
    }
    return 0;
}

int vdrive_snapshot_module_read(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    int i, j, tde;
    snapshot_module_t *m;
    char snap_module_name[SNAP_MODNAME_SIZE];

    for (i = DRIVE_UNIT_MIN; i <= DRIVE_UNIT_MAX; i++) {
        resources_get_int_sprintf("Drive%iTrueEmulation", &tde, i);
        if (tde == 0) {
            for (j = DRIVE_NUMBER_MIN; j <= DRIVE_NUMBER_MAX; j++) {
                snprintf(snap_module_name, SNAP_MODNAME_SIZE, "VDRIVEIMAGE%i", i);
                m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
                if (m == NULL) {
                    return 0;
                }

                /* FIXME: this gives a linker error? */
                /* if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) { */
                if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
                    log_message(vdrive_snapshot_log,
                                "Snapshot module version (%d.%d) newer than %d.%d.",
                                major_version, minor_version, SNAP_MAJOR, SNAP_MINOR);
                }
                snapshot_module_close(m);
            }
        }
    }
    return 0;
}
