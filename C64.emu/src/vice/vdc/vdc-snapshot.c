/*
 * vdc-snapshot.c - Snapshot functionality for the MOS 8563 (VDC)
 * emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Markus Brenner <markus@brenner.de>
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

#include "log.h"
#include "snapshot.h"
#include "types.h"
#include "vdc-snapshot.h"
#include "vdc.h"
#include "vdctypes.h"

/*

   This is the format of the VDC snapshot module.

   Name               Type   Size   Description

   (nothing yet)

 */


static char snap_module_name[] = "VDC";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0


int vdc_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create (s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    return snapshot_module_close(m);

/*
fail:
  if (m != NULL)
      snapshot_module_close(m);
  return -1;
*/
}

/* Helper functions.  */

int vdc_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(vdc.log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
