/*
 * tape-snapshot.c - Tape snapshot module.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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
#include <stdlib.h>

#include "archdep.h"
#include "datasette.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "snapshot.h"
#include "t64.h"
#include "tap.h"
#include "tape-snapshot.h"
#include "tape.h"
#include "types.h"


/* Logging.  */
static log_t tape_snapshot_log = LOG_ERR;


#define T64IMAGE_SNAP_MAJOR 1
#define T64IMAGE_SNAP_MINOR 0

static int tape_snapshot_write_t64image_module(snapshot_t *s)
{
    log_error(tape_snapshot_log, "T64 snapshot support is not implemented");
    return 0; /* should be -1, but that would make snapshots with default settings fail */
}


static int tape_snapshot_read_t64image_module(snapshot_t *s)
{
    log_error(tape_snapshot_log, "T64 snapshot support is not implemented");
    return 0; /* should be -1, but that would make snapshots with default settings fail */
}


#define TAPIMAGE_SNAP_MAJOR 1
#define TAPIMAGE_SNAP_MINOR 0

static int tape_snapshot_write_tapimage_module(int port, snapshot_t *s)
{
    snapshot_module_t *m;
    FILE *ftap;
    long pos, tap_size;
    uint8_t buffer[256];
    int i;

    m = snapshot_module_create(s, "TAPIMAGE", TAPIMAGE_SNAP_MAJOR,
                               TAPIMAGE_SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* get the file descriptor */
    ftap = ((tap_t*)tape_image_dev[port]->data)->fd;
    if (!ftap) {
        log_error(tape_snapshot_log, "Cannot open tapfile for reading");
        return -1;
    }

    /* remeber current position */
    pos = ftell(ftap);

    /* move to end and get size of file */
    if (fseek(ftap, 0, SEEK_END) != 0) {
        log_error(tape_snapshot_log, "Cannot move to end of tapfile");
        return -1;
    }

    tap_size = ftell(ftap);
    if (SMW_DW(m, (unsigned int)tap_size)) {
        fseek(ftap, pos, SEEK_SET);
        log_error(tape_snapshot_log, "Cannot write size of tap image");
    }

    /* move to beginning */
    if (fseek(ftap, 0, SEEK_SET) != 0) {
        log_error(tape_snapshot_log, "Cannot move to beginning of tapfile");
        return -1;
    }

    /* read every BYTE and write to snapshot module */
    while (tap_size > 0) {
        i = (int)fread(buffer, 1, 256, ftap);
        if (SMW_BA(m, buffer, i) < 0) {
            log_error(tape_snapshot_log, "Cannot write tap image");
            fseek(ftap, pos, SEEK_SET);
            return -1;
        }
        tap_size -= i;
    }

    /* restore position */
    fseek(ftap, pos, SEEK_SET);

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}


static int tape_snapshot_read_tapimage_module(int port, snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    char *filename = NULL;
    FILE *ftap;
    uint8_t *buffer;
    long tap_size = -1;

    m = snapshot_module_open(s, "TAPIMAGE",
                             &major_version, &minor_version);
    if (m == NULL) {
        return 0;
    }

    if (snapshot_version_is_bigger(major_version, minor_version, TAPIMAGE_SNAP_MAJOR, TAPIMAGE_SNAP_MINOR)) {
        log_error(tape_snapshot_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  TAPIMAGE_SNAP_MAJOR, TAPIMAGE_SNAP_MINOR);
    }

    /* create temporary file */
    /* FIXME: Were is this file deleted? */
    ftap = archdep_mkstemp_fd(&filename, MODE_WRITE);

    if (ftap == NULL) {
        log_error(tape_snapshot_log, "Could not create temporary file!");
        snapshot_module_close(m);
        goto fail;
    }

    SMR_DW_UL(m, (unsigned long *)&tap_size);

    buffer = lib_malloc(tap_size);

    SMR_BA(m, buffer, (unsigned int)tap_size);

    if (fwrite(buffer, tap_size, 1, ftap) != 1) {
        log_error(tape_snapshot_log, "Could not create temporary file");
        log_error(tape_snapshot_log, "filename=%s", filename);
        snapshot_module_close(m);
        fclose(ftap);
        goto fail;
    }

    lib_free(buffer);
    fclose(ftap);
    tape_image_attach(port + 1, filename);
    lib_free(filename);
    snapshot_module_close(m);
    return 0;

fail:
    lib_free(filename);
    return -1;
}


#define TAPE_SNAP_MAJOR 1
#define TAPE_SNAP_MINOR 0

static const char snap_module_name[] = "TAPE";

int tape_snapshot_write_module(int port, snapshot_t *s, int save_image)
{
    snapshot_module_t *m;
    tap_t *tap;

    if (tape_image_dev[port] == NULL || tape_image_dev[port]->name == NULL) {
        return 0;
    }

    if (save_image) {
        switch (tape_image_dev[port]->type) {
            case TAPE_TYPE_T64:
                if (tape_snapshot_write_t64image_module(s) < 0) {
                    return -1;
                }
                break;
            case TAPE_TYPE_TAP:
                if (tape_snapshot_write_tapimage_module(port, s) < 0) {
                    return -1;
                }
                break;
            default:
                break;
        }
    }

    m = snapshot_module_create(s, snap_module_name, TAPE_SNAP_MAJOR, TAPE_SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)tape_image_dev[port]->read_only) < 0
        || SMW_B(m, (uint8_t)tape_image_dev[port]->type) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    switch (tape_image_dev[port]->type) {
        case TAPE_TYPE_T64:
            break;
        case TAPE_TYPE_TAP:
            tap = (tap_t*)tape_image_dev[port]->data;
            if (tap == NULL
                || SMW_DW(m, tap->size) < 0
                || SMW_B(m, tap->version) < 0
                || SMW_B(m, tap->system) < 0
                || SMW_DW(m, tap->current_file_seek_position) < 0
                || SMW_DW(m, tap->offset) < 0
                || SMW_DW(m, tap->cycle_counter) < 0
                || SMW_DW(m, tap->cycle_counter_total) < 0
                || SMW_DW(m, tap->counter) < 0
                || SMW_DW(m, tap->mode) < 0
                || SMW_DW(m, tap->read_only) < 0
                || SMW_DW(m, tap->has_changed) < 0) {
                snapshot_module_close(m);
                return -1;
            }

            break;
        default:
            break;
    }

    return snapshot_module_close(m);
}


int tape_snapshot_read_module(int port, snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    unsigned int snap_type;
    tap_t *tap;

    if (tape_snapshot_read_tapimage_module(port, s) < 0
        || tape_snapshot_read_t64image_module(s) < 0) {
        return -1;
    }


    m = snapshot_module_open(s, snap_module_name,
                             &major_version, &minor_version);

    if (m == NULL) {
        /* no tape attached */
        tape_image_detach_internal(port + 1);
        return 0;
    }

    if (0
        || SMR_B_INT(m, (int *)&tape_image_dev[port]->read_only) < 0
        || SMR_B_INT(m, (int *)&snap_type) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snap_type != tape_image_dev[port]->type) {
        /* attached image type is not correct */
        log_error(tape_snapshot_log,
                  "No tape image attached or type not correct.");
        snapshot_module_close(m);
        return -1;
    }

    switch (tape_image_dev[port]->type) {
        case TAPE_TYPE_T64:
            break;
        case TAPE_TYPE_TAP:
            tap = (tap_t*)tape_image_dev[port]->data;
            if (tap == NULL
                || SMR_DW(m, (uint32_t *)&tap->size) < 0
                || SMR_B(m, &tap->version) < 0
                || SMR_B(m, &tap->system) < 0
                || SMR_DW(m, (uint32_t *)&tap->current_file_seek_position) < 0
                || SMR_DW(m, (uint32_t *)&tap->offset) < 0
                || SMR_DW(m, (uint32_t *)&tap->cycle_counter) < 0
                || SMR_DW(m, (uint32_t *)&tap->cycle_counter_total) < 0
                || SMR_DW(m, (uint32_t *)&tap->counter) < 0
                || SMR_DW(m, (uint32_t *)&tap->mode) < 0
                || SMR_DW(m, (uint32_t *)&tap->read_only) < 0
                || SMR_DW(m, (uint32_t *)&tap->has_changed) < 0) {
                snapshot_module_close(m);
                return -1;
            }

            break;
        default:
            break;
    }

    return snapshot_module_close(m);
}
