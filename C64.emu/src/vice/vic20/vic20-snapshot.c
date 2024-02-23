/*
 * vic20-snapshot.c - VIC20 snapshot handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Multiple memory configuration support originally by
 *  Alexander Lehmann <alex@mathematik.th-darmstadt.de>
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

#include "archdep.h"
#include "drive-snapshot.h"
#include "joyport.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "serial.h"
#include "snapshot.h"
#include "sound.h"
#include "tapeport.h"
#include "types.h"
#include "userport.h"
#include "via.h"
#include "vic.h"
#include "vic20.h"
#include "vic20memsnapshot.h"
#include "vice-event.h"

#include "vic20-snapshot.h"


#define SNAP_MAJOR          3
#define SNAP_MINOR          0


int vic20_snapshot_write(const char *name, int save_roms, int save_disks,
                         int event_mode)
{
    snapshot_t *s;
    int ieee488;

    s = snapshot_create(name, ((uint8_t)(SNAP_MAJOR)), ((uint8_t)(SNAP_MINOR)),
                        machine_name);
    if (s == NULL) {
        return -1;
    }

    sound_snapshot_prepare();

    /* FIXME: Missing sound.  */
    if (maincpu_snapshot_write_module(s) < 0
        || vic20_snapshot_write_module(s, save_roms) < 0
        || vic_snapshot_write_module(s) < 0
        || viacore_snapshot_write_module(machine_context.via1, s) < 0
        || viacore_snapshot_write_module(machine_context.via2, s) < 0
        || drive_snapshot_write_module(s, save_disks, save_roms) < 0
        || fsdrive_snapshot_write_module(s) < 0
        || event_snapshot_write_module(s, event_mode) < 0
        || tapeport_snapshot_write_module(s, save_disks) < 0
        || keyboard_snapshot_write_module(s) < 0
        || joyport_snapshot_write_module(s, JOYPORT_1) < 0
        || userport_snapshot_write_module(s) < 0) {
        snapshot_close(s);
        archdep_remove(name);
        return -1;
    }

    resources_get_int("IEEE488", &ieee488);
    if (ieee488) {
        if (viacore_snapshot_write_module(machine_context.ieeevia1, s) < 0
            || viacore_snapshot_write_module(machine_context.ieeevia2, s) < 0) {
            snapshot_close(s);
            archdep_remove(name);
            return 1;
        }
    }

    snapshot_close(s);
    return 0;
}

int vic20_snapshot_read(const char *name, int event_mode)
{
    snapshot_t *s;
    uint8_t minor, major;

    s = snapshot_open(name, &major, &minor, machine_name);
    if (s == NULL) {
        return -1;
    }

    if (!snapshot_version_is_equal(major, minor, SNAP_MAJOR, SNAP_MINOR)) {
        log_error(LOG_DEFAULT, "Snapshot version (%d.%d) not valid: expecting %d.%d.", major, minor, SNAP_MAJOR, SNAP_MINOR);
        snapshot_set_error(SNAPSHOT_MODULE_INCOMPATIBLE);
        goto fail;
    }

    joyport_clear_devices();

    /* FIXME: Missing sound.  */
    if (maincpu_snapshot_read_module(s) < 0
        || vic20_snapshot_read_module(s) < 0
        || vic_snapshot_read_module(s) < 0
        || viacore_snapshot_read_module(machine_context.via1, s) < 0
        || viacore_snapshot_read_module(machine_context.via2, s) < 0
        || drive_snapshot_read_module(s) < 0
        || fsdrive_snapshot_read_module(s) < 0
        || event_snapshot_read_module(s, event_mode) < 0
        || tapeport_snapshot_read_module(s) < 0
        || keyboard_snapshot_read_module(s) < 0
        || joyport_snapshot_read_module(s, JOYPORT_1) < 0
        || userport_snapshot_read_module(s) < 0) {
        goto fail;
    }

    if (viacore_snapshot_read_module(machine_context.ieeevia1, s) < 0
        || viacore_snapshot_read_module(machine_context.ieeevia2, s) < 0) {
        /* IEEE488 module not undumped */
        resources_set_int("IEEE488", 0);
    } else {
        resources_set_int("IEEE488", 1);
    }

    snapshot_close(s);

    sound_snapshot_finish();

    return 0;

fail:
    if (s != NULL) {
        snapshot_close(s);
    }

    machine_trigger_reset(MACHINE_RESET_MODE_RESET_CPU);

    return -1;
}
