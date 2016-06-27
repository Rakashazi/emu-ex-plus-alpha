/*
 * crtc-snapshot.c - A line-based CRTC emulation (under construction).
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * 16/24bpp support added by
 *  Steven Tieu <stieu@physics.ubc.ca>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

#include "crtc-mem.h"
#include "crtc.h"
#include "crtctypes.h"
#include "log.h"
#include "maincpu.h"
#include "snapshot.h"
#include "types.h"


/* Snapshot.  */

static char snap_module_name[] = "CRTC";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int crtc_snapshot_write_module(snapshot_t * s)
{
    int i, ef = 0;
    int current_char;
    int screen_rel;
    snapshot_module_t *m;

    /* derive some values */
    current_char = maincpu_clk - crtc.rl_start;
    screen_rel = crtc.screen_rel;
    if ((crtc.raster.ycounter == crtc.regs[9])
        && (current_char > crtc.rl_visible)
        && crtc.henable) {
        screen_rel += crtc.rl_visible * crtc.hw_cols;
    }

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* hardware-options */
    if (ef
        /* valid video address bits */
        || SMW_W(m, (WORD)crtc.vaddr_mask) < 0
        /* which bit selects different charset .. */
        || SMW_W(m, (WORD)crtc.vaddr_charswitch) < 0
        /* ...with offset in charset .. */
        || SMW_W(m, (WORD)crtc.vaddr_charoffset) < 0
        /* which bit reverses the screen */
        /* XXX: this implementation "forgets" the sign of vaddr_revswitch,
           that indicate whether the bit has to be set (<0) or cleared (>0)
           for reverse mode. V1.0 modules are broken here. V1.1 adds the
           sign at the end of the module */
        || SMW_W(m, (WORD)crtc.vaddr_revswitch) < 0

        /* size of character generator in byte - 1 */
        || SMW_W(m, (WORD)crtc.chargen_mask) < 0
        /* offset given by external circuitry */
        || SMW_W(m, (WORD)crtc.chargen_offset) < 0

        /* hardware cursor enabled? */
        || SMW_B(m, (BYTE)(crtc.hw_cursor ? 1 : 0)) < 0
        /* hardware column per character clock cycle */
        || SMW_B(m, (BYTE)crtc.hw_cols) < 0
        /* (external) hardware blanked */
        || SMW_B(m, (BYTE)crtc.hw_blank) < 0
        ) {
        ef = -1;
    }

    /* save the registers */
    for (i = 0; (!ef) && (i < 20); i++) {
        ef = SMW_B(m, crtc.regs[i]);
    }

    /* save the internal state of the CRTC counters */
    if (ef
        /* index in CRTC register file */
        || SMW_B(m, (BYTE)crtc.regno) < 0
        /* clock in the rasterline */
        || SMW_B(m, (BYTE)current_char) < 0
        /* current character line */
        || SMW_B(m, (BYTE)crtc.current_charline) < 0
        /* rasterline in character */
        || SMW_B(m, (BYTE)crtc.raster.ycounter) < 0

        /* cursor state & counter */
        || SMW_B(m, (BYTE)crtc.crsrcnt) < 0
        || SMW_B(m, (BYTE)crtc.crsrstate) < 0
        || SMW_B(m, (BYTE)crtc.cursor_lines) < 0

        /* memory pointer */
        || SMW_W(m, (WORD)crtc.chargen_rel) < 0
        || SMW_W(m, (WORD)screen_rel) < 0

        /* vsync */
        || SMW_W(m, (WORD)crtc.vsync) < 0
        /* venable */
        || SMW_B(m, (BYTE)crtc.venable) < 0
        ) {
        ef = -1;
    }

    /* VICE-dependent runtime variables */
    if (ef
        /* screen size */
        || SMW_W(m, (WORD)crtc.screen_width) < 0
        || SMW_W(m, (WORD)crtc.screen_height) < 0

        /* horizontal centering */
        || SMW_W(m, (WORD)crtc.screen_xoffset) < 0
        /* horizontal jitter */
        || SMW_W(m, (WORD)crtc.hjitter) < 0

        /* vertical centering */
        || SMW_W(m, (WORD)crtc.screen_yoffset) < 0

        /* expected number of rasterlines for the frame */
        || SMW_W(m, (WORD)crtc.framelines) < 0
        /* current frameline */
        || SMW_W(m, (WORD)crtc.current_line) < 0
        ) {
        ef = -1;
    }

    /* This value has been added in V1.1. V1.0 module readers ignore the
       additional value. This is only relevant for CBM-II as the PET only
       use positive values in rev_switch. Only bit 0 is defined as of today. */
    if (!ef) {
        BYTE rev_fl = (crtc.vaddr_revswitch < 0) ? 1 : 0;
        if (SMW_B(m, (BYTE)rev_fl) < 0) {
            ef = -1;
        }
    }

    if (ef) {
        snapshot_module_close(m);
    } else {
        ef = snapshot_module_close(m);
    }

    return ef;
}


int crtc_snapshot_read_module(snapshot_t * s)
{
    int i, ef = 0;
    snapshot_module_t *m;
    WORD w;
    BYTE b;
    BYTE major, minor;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);
    if (m == NULL) {
        return -1;
    }

    if (major != SNAP_MAJOR) {
        log_error(crtc.log,
                  "Major snapshot number (%d) invalid; %d expected.",
                  major, SNAP_MAJOR);
        snapshot_module_close (m);
        return -1;
    }

    /* hardware-options */
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.vaddr_mask = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.vaddr_charswitch = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.vaddr_charoffset = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.vaddr_revswitch = w;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.chargen_mask = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.chargen_offset = w;
    }

    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.hw_cursor = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.hw_cols = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.hw_blank = b;
    }

    crtc.rl_start = maincpu_clk;        /* just to be sure */

    /* read the registers */
    for (i = 0; (!ef) && (i < 20); i++) {
        if (!(ef = SMR_B(m, &b))) {
            crtc_store(0, (BYTE)i);
            crtc_store(1, b);
        }
    }

    /* save the internal state of the CRTC counters */
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.regno = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.rl_start = maincpu_clk - b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.current_charline = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.raster.ycounter = b;
    }

    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.crsrcnt = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.crsrstate = b;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.cursor_lines = b;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.chargen_rel = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.screen_rel = w;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.vsync = w;
    }
    if ((!ef) && !(ef = SMR_B(m, &b))) {
        crtc.venable = b;
    }

    /* VICE-dependent runtime variables */
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.screen_width = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.screen_height = w;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.screen_xoffset = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.hjitter = w;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.screen_yoffset = w;
    }

    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.framelines = w;
    }
    if ((!ef) && !(ef = SMR_W(m, &w))) {
        crtc.current_line = w;
    }

    /* this has been added in V1.1 module version. Values are not available
       when reading V1.0 modules */
    if (minor >= 1) {
        if ((!ef) && !(ef = SMR_B(m, &b))) {
            /* invert vaddr_revswitch, i.e. bit must be set for reverse mode */
            if (b & 1) {
                crtc.vaddr_revswitch = -crtc.vaddr_revswitch;
            }
        }
    }

    crtc.raster.current_line = crtc.current_line + crtc.screen_yoffset;

/* FIXME: compatibility mode for old snapshots */
#if 0
    if (SMR_B(m, &b) < 0) {
        goto fail;
    }
    /* for the moment simply ignore this value */

    if (SMR_W(m, &w) < 0) {
        goto fail;
    }
    /* for the moment simply ignore this value */

    if (0
        || SMR_W(m, &vmask) < 0
        || SMR_B(m, &hwflags)) {
        goto fail;
    }

    crtc_set_screen_mode(NULL, vmask, memptr_inc, hwflags);
    crtc_update_memory_ptrs();

    for (i = 0; i < 20; i++) {
        if (SMR_B(m, &b) < 0) {
            goto fail;
        }

        /* XXX: This assumes that there are no side effects.
           Well, there are, but the cursor state is restored later */
        store_crtc (i, b);
    }
    if (SMR_B(m, &b) < 0) {
        goto fail;
    }

    crsrcnt = b & 0x3f;
    crsrstate = (b & 0x80) ? 1 : 0;

    alarm_set(&raster_draw_alarm, clk + CYCLES_PER_LINE /* - RASTER_CYCLE */);

    SIGNAL_VERT_BLANK_OFF

    force_repaint();
#endif

    crtc_update_window();

    if (ef) {
        log_error(crtc.log, "Failed to load snapshot module %s",
                  snap_module_name);
        snapshot_module_close(m);
    } else {
        ef = snapshot_module_close(m);
    }

    return ef;
}
