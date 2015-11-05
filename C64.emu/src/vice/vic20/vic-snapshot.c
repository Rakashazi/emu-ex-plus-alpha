/*
 * vic-snapshot.c - Snapshot support for the VIC-I emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "snapshot.h"
#include "sound.h"
#include "types.h"
#include "vic.h"
#include "victypes.h"
#include "vic-mem.h"
#include "vic-snapshot.h"


static char snap_module_name[] = "VIC-I";
#define SNAP_MAJOR 0
#define SNAP_MINOR 2


int vic_snapshot_write_module(snapshot_t *s)
{
    int i;
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (BYTE)VIC_RASTER_CYCLE(maincpu_clk)) < 0
        || SMW_W(m, (WORD)VIC_RASTER_Y(maincpu_clk)) < 0) {
        goto fail;
    }

    if (0
        || (SMW_W(m, (WORD)vic.area) < 0)
        || (SMW_W(m, (WORD)vic.fetch_state) < 0)
        || (SMW_DW(m, (DWORD)vic.raster_line) < 0)
        || (SMW_DW(m, (DWORD)vic.text_cols) < 0)
        || (SMW_DW(m, (DWORD)vic.text_lines) < 0)
        || (SMW_DW(m, (DWORD)vic.pending_text_cols) < 0)
        || (SMW_DW(m, (DWORD)vic.line_was_blank) < 0)
        || (SMW_DW(m, (DWORD)vic.memptr) < 0)
        || (SMW_DW(m, (DWORD)vic.memptr_inc) < 0)
        || (SMW_DW(m, (DWORD)vic.row_counter) < 0)
        || (SMW_DW(m, (DWORD)vic.buf_offset) < 0)
        || SMW_B(m, (BYTE)vic.light_pen.state) < 0
        || SMW_B(m, (BYTE)vic.light_pen.triggered) < 0
        || SMW_DW(m, (DWORD)vic.light_pen.x) < 0
        || SMW_DW(m, (DWORD)vic.light_pen.y) < 0
        || SMW_DW(m, (DWORD)vic.light_pen.x_extra_bits) < 0
        || SMW_DW(m, (DWORD)vic.light_pen.trigger_cycle) < 0
        || (SMW_B(m, vic.vbuf) < 0)) {
        goto fail;
    }

    /* Color RAM.  */
    if (SMW_BA(m, mem_ram + 0x9400, 0x400) < 0) {
        goto fail;
    }

    for (i = 0; i < 0x10; i++) {
        if (SMW_B(m, (BYTE)vic.regs[i]) < 0) {
            goto fail;
        }
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}


int vic_snapshot_read_module(snapshot_t *s)
{
    WORD i;
    snapshot_module_t *m;
    BYTE major_version, minor_version;
    WORD w;
    BYTE b;

    sound_close();

    m = snapshot_module_open(s, snap_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(vic.log, "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_B(m, &b) < 0) {
        goto fail;
    }
    if (b != VIC_RASTER_CYCLE(maincpu_clk)) {
        log_error(vic.log, "Cycle value (%d) incorrect; should be %d.",
                  (int)b, VIC_RASTER_CYCLE(maincpu_clk));
        goto fail;
    }
    vic.raster_cycle = (unsigned int)b;

    if (SMR_W(m, &w) < 0) {
        goto fail;
    }
    if (w != VIC_RASTER_Y(maincpu_clk)) {
        log_error(vic.log, "Raster line value (%d) incorrect; should be %d.",
                  (int)w, VIC_RASTER_Y(maincpu_clk));
        goto fail;
    }

    if (SMR_W(m, &w) < 0) {
        goto fail;
    }
    vic.area = (vic_area_state_t)w;

    if (SMR_W(m, &w) < 0) {
        goto fail;
    }
    vic.fetch_state = (vic_fetch_state_t)w;

    if (0
        || (SMR_DW_UINT(m, &vic.raster_line) < 0)
        || (SMR_DW_UINT(m, &vic.text_cols) < 0)
        || (SMR_DW_UINT(m, &vic.text_lines) < 0)
        || (SMR_DW_UINT(m, &vic.pending_text_cols) < 0)
        || (SMR_DW_UINT(m, &vic.line_was_blank) < 0)
        || (SMR_DW_UINT(m, &vic.memptr) < 0)
        || (SMR_DW_UINT(m, &vic.memptr_inc) < 0)
        || (SMR_DW_UINT(m, &vic.row_counter) < 0)
        || (SMR_DW_UINT(m, &vic.buf_offset) < 0)
        || SMR_B_INT(m, &vic.light_pen.state) < 0
        || SMR_B_INT(m, &vic.light_pen.triggered) < 0
        || SMR_DW_INT(m, &vic.light_pen.x) < 0
        || SMR_DW_INT(m, &vic.light_pen.y) < 0
        || SMR_DW_INT(m, &vic.light_pen.x_extra_bits) < 0
        || SMR_DW(m, &vic.light_pen.trigger_cycle) < 0
        || (SMR_B(m, &vic.vbuf) < 0)) {
        goto fail;
    }

    /* Color RAM.  */
    if (SMR_BA(m, mem_ram + 0x9400, 0x400) < 0) {
        goto fail;
    }

    for (i = 0; i < 0x10; i++) {
        if (SMR_B(m, &b) < 0) {
            goto fail;
        }

        /* XXX: This assumes that there are no side effects.  */
        vic_store(i, b);
    }

    raster_force_repaint(&vic.raster);
    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
