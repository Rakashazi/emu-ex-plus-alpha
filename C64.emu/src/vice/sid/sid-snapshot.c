/*
 * sid-snapshot.c - SID snapshot.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <string.h>

#include "log.h"
#include "resources.h"
#include "screenshot.h"
#include "sid-snapshot.h"
#include "sid.h"
#include "sound.h"
#include "snapshot.h"
#include "types.h"


static const char snap_module_name_simple[] = "SID";
#define SNAP_MAJOR_SIMPLE 1
#define SNAP_MINOR_SIMPLE 1

static int sid_snapshot_write_module_simple(snapshot_t *s)
{
    int sound, sid_engine;
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name_simple, SNAP_MAJOR_SIMPLE,
                               SNAP_MINOR_SIMPLE);
    if (m == NULL) {
        return -1;
    }

    resources_get_int("Sound", &sound);
    if (SMW_B(m, (BYTE)sound) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (sound) {
        resources_get_int("SidEngine", &sid_engine);
        if (SMW_B(m, (BYTE)sid_engine) < 0) {
            snapshot_module_close(m);
            return -1;
        }

        /* FIXME: Only data for first SID stored. */
        if (SMW_BA(m, sid_get_siddata(0), 32) < 0) {
            snapshot_module_close(m);
            return -1;
        }
    }

    snapshot_module_close(m);
    return 0;
}

static int sid_snapshot_read_module_simple(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    BYTE tmp[34];

    m = snapshot_module_open(s, snap_module_name_simple,
                             &major_version, &minor_version);
    if (m == NULL) {
        goto fail;
    }

    if (major_version > SNAP_MAJOR_SIMPLE
        || minor_version > SNAP_MINOR_SIMPLE) {
        log_error(LOG_DEFAULT,
                  "SID: Snapshot module version (%d.%d) newer than %d.%d.\n",
                  major_version, minor_version,
                  SNAP_MAJOR_SIMPLE, SNAP_MINOR_SIMPLE);
        snapshot_module_close(m);
        goto fail;
    }

    /* If more than 32 bytes are present then the resource "Sound" and
       "SidEngine" come first! If there is only one byte present, then
       sound is disabled. */
    if (SMR_BA(m, tmp, 34) < 0) {
        if (SMR_BA(m, tmp, 32) < 0) {
            if (SMR_BA(m, tmp, 1) < 0) {
                snapshot_module_close(m);
                goto fail;
            } else {
                sound_close();
            }
        } else {
            memcpy(sid_get_siddata(0), &tmp[0], 32);
        }
    } else {
        int res_sound = (int)(tmp[0]);
        int res_engine = (int)(tmp[1]);

        screenshot_prepare_reopen();
        sound_close();
        screenshot_try_reopen();
        resources_set_int("Sound", res_sound);
        if (res_sound) {
            resources_set_int("SidEngine", res_engine);
            /* FIXME: Only data for first SID read. */
            memcpy(sid_get_siddata(0), &tmp[2], 32);
            sound_open();
        }
    }

    return snapshot_module_close(m);

fail:
    log_error(LOG_DEFAULT, "Failed reading SID snapshot");
    return -1;
}

static const char snap_module_name_extended[] = "SIDEXTENDED";
#define SNAP_MAJOR_EXTENDED 1
#define SNAP_MINOR_EXTENDED 1

static int sid_snapshot_write_module_extended(snapshot_t *s)
{
    snapshot_module_t *m;
    sid_snapshot_state_t sid_state;
    int sound, sid_engine;

    resources_get_int("Sound", &sound);

    if (sound == 0) {
        return 0;
    }

    resources_get_int("SidEngine", &sid_engine);

    if (sid_engine != SID_ENGINE_FASTSID
#ifdef HAVE_RESID
        && sid_engine != SID_ENGINE_RESID
#endif
        ) {
        return 0;
    }

    sid_state_read(0, &sid_state);

    m = snapshot_module_create(s, snap_module_name_extended,
                               SNAP_MAJOR_EXTENDED, SNAP_MINOR_EXTENDED);
    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, sid_state.sid_register, 32) < 0
        || SMW_B(m, sid_state.bus_value) < 0
        || SMW_DW(m, sid_state.bus_value_ttl) < 0
        || SMW_DWA(m, sid_state.accumulator, 3) < 0
        || SMW_DWA(m, sid_state.shift_register, 3) < 0
        || SMW_WA(m, sid_state.rate_counter, 3) < 0
        || SMW_WA(m, sid_state.exponential_counter, 3) < 0
        || SMW_BA(m, sid_state.envelope_counter, 3) < 0
        || SMW_BA(m, sid_state.envelope_state, 3) < 0
        || SMW_BA(m, sid_state.hold_zero, 3) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMW_WA(m, sid_state.rate_counter_period, 3) < 0
        || SMW_WA(m, sid_state.exponential_counter_period, 3) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (SMW_BA(m, sid_state.envelope_pipeline, 3) < 0
        || SMW_BA(m, sid_state.shift_pipeline, 3) < 0
        || SMW_DWA(m, sid_state.shift_register_reset, 3) < 0
        || SMW_DWA(m, sid_state.floating_output_ttl, 3) < 0
        || SMW_WA(m, sid_state.pulse_output, 3) < 0
        || SMW_B(m, sid_state.write_pipeline) < 0
        || SMW_B(m, sid_state.write_address) < 0
        || SMW_B(m, sid_state.voice_mask) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

static int sid_snapshot_read_module_extended(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    sid_snapshot_state_t sid_state;
    int sound, sid_engine;

    memset(&sid_state, 0, sizeof(sid_state));

    resources_get_int("Sound", &sound);

    if (sound == 0) {
        return 0;
    }

    resources_get_int("SidEngine", &sid_engine);

    if (sid_engine != SID_ENGINE_FASTSID
#ifdef HAVE_RESID
        && sid_engine != SID_ENGINE_RESID
#endif
        ) {
        return 0;
    }

    m = snapshot_module_open(s, snap_module_name_extended,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR_EXTENDED
        || minor_version > SNAP_MINOR_EXTENDED) {
        log_error(LOG_DEFAULT,
                  "SID: Snapshot module version (%d.%d) newer than %d.%d.\n",
                  major_version, minor_version,
                  SNAP_MAJOR_EXTENDED, SNAP_MINOR_EXTENDED);
        return snapshot_module_close(m);
    }

    if (SMR_BA(m, sid_state.sid_register, 32) < 0
        || SMR_B(m, &(sid_state.bus_value)) < 0
        || SMR_DW(m, &(sid_state.bus_value_ttl)) < 0
        || SMR_DWA(m, sid_state.accumulator, 3) < 0
        || SMR_DWA(m, sid_state.shift_register, 3) < 0
        || SMR_WA(m, sid_state.rate_counter, 3) < 0
        || SMR_WA(m, sid_state.exponential_counter, 3) < 0
        || SMR_BA(m, sid_state.envelope_counter, 3) < 0
        || SMR_BA(m, sid_state.envelope_state, 3) < 0
        || SMR_BA(m, sid_state.hold_zero, 3) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_WA(m, sid_state.rate_counter_period, 3);
    SMR_WA(m, sid_state.exponential_counter_period, 3);

    SMR_BA(m, sid_state.envelope_pipeline, 3);
    SMR_BA(m, sid_state.shift_pipeline, 3);
    SMR_DWA(m, sid_state.shift_register_reset, 3);
    SMR_DWA(m, sid_state.floating_output_ttl, 3);
    SMR_WA(m, sid_state.pulse_output, 3);
    SMR_B(m, &(sid_state.write_pipeline));
    SMR_B(m, &(sid_state.write_address));
    SMR_B(m, &(sid_state.voice_mask));

    sid_state_write(0, &sid_state);

    return snapshot_module_close(m);
}

int sid_snapshot_write_module(snapshot_t *s)
{
    if (sid_snapshot_write_module_simple(s) < 0) {
        return -1;
    }

    if (sid_snapshot_write_module_extended(s) < 0) {
        return -1;
    }

    return 0;
}

int sid_snapshot_read_module(snapshot_t *s)
{
    if (sid_snapshot_read_module_simple(s) < 0) {
        return -1;
    }

    sid_snapshot_read_module_extended(s);

    return 0;
}
