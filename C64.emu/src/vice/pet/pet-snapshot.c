/*
 * pet-snapshot.c - PET snapshot handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "6809.h"
#include "crtc.h"
#include "drive-snapshot.h"
#include "ioutil.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "pet-snapshot.h"
#include "pet.h"
#include "petacia.h"
#include "petdww.h"
#include "petmemsnapshot.h"
#include "petpia.h"
#include "pets.h"
#include "snapshot.h"
#include "sound.h"
#include "tape-snapshot.h"
#include "types.h"
#include "via.h"
#include "vice-event.h"


#define SNAP_MAJOR 0
#define SNAP_MINOR 0


int pet_snapshot_write(const char *name, int save_roms, int save_disks,
                       int event_mode)
{
    snapshot_t *s;
    int ef = 0;

    s = snapshot_create(name, SNAP_MAJOR, SNAP_MINOR, machine_name);

    if (s == NULL) {
        return -1;
    }

    sound_snapshot_prepare();

    if (maincpu_snapshot_write_module(s) < 0
        || cpu6809_snapshot_write_module(s) < 0
        || pet_snapshot_write_module(s, save_roms) < 0
        || crtc_snapshot_write_module(s) < 0
        || pia1_snapshot_write_module(s) < 0
        || pia2_snapshot_write_module(s) < 0
        || petdww_snapshot_write_module(s) < 0
        || viacore_snapshot_write_module(machine_context.via, s) < 0
        || drive_snapshot_write_module(s, save_disks, save_roms) < 0
        || event_snapshot_write_module(s, event_mode) < 0
        || tape_snapshot_write_module(s, save_disks) < 0
        || keyboard_snapshot_write_module(s)
        || joystick_snapshot_write_module(s)) {
        ef = -1;
    }

    if ((!ef) && petres.superpet) {
        ef = acia1_snapshot_write_module(s);
    }

    snapshot_close(s);

    if (ef) {
        ioutil_remove(name);
    }

    return ef;
}

int pet_snapshot_read(const char *name, int event_mode)
{
    snapshot_t *s;
    BYTE minor, major;
    int ef = 0;

    s = snapshot_open(name, &major, &minor, machine_name);

    if (s == NULL) {
        return -1;
    }

    if (major != SNAP_MAJOR || minor != SNAP_MINOR) {
        log_error(LOG_DEFAULT,
                  "Snapshot version (%d.%d) not valid: expecting %d.%d.",
                  major, minor, SNAP_MAJOR, SNAP_MINOR);
        ef = -1;
    }

    if (ef
        || maincpu_snapshot_read_module(s) < 0
        || cpu6809_snapshot_read_module(s) < 0
        || pet_snapshot_read_module(s) < 0
        || crtc_snapshot_read_module(s) < 0
        || pia1_snapshot_read_module(s) < 0
        || pia2_snapshot_read_module(s) < 0
        || petdww_snapshot_read_module(s) < 0
        || viacore_snapshot_read_module(machine_context.via, s) < 0
        || drive_snapshot_read_module(s) < 0
        || event_snapshot_read_module(s, event_mode) < 0
        || tape_snapshot_read_module(s) < 0
        || keyboard_snapshot_read_module(s) < 0
        || joystick_snapshot_read_module(s) < 0) {
        ef = -1;
    }

    if (!ef) {
        acia1_snapshot_read_module(s);  /* optional, so no error check */
    }

    snapshot_close(s);

    if (ef) {
        machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
    }

    sound_snapshot_finish();

    return ef;
}
