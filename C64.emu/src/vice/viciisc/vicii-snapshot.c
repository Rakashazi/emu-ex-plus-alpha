/*
 * vicii-snapshot.c - Snapshot functionality for the MOS 6569 (VIC-II)
 * emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
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

#include "interrupt.h"
#include "log.h"
#include "mem.h"
#include "snapshot.h"
#include "types.h"
#include "vicii-draw-cycle.h"
#include "vicii-resources.h"
#include "vicii-snapshot.h"
#include "vicii.h"
#include "viciitypes.h"

/* Dummy function called by c64-snapshot.c */
void vicii_snapshot_prepare(void)
{
}


/*

   This is the format of the VIC-II snapshot module.

   Name               Type   Size   Description

   (FIXME)

*/

static char snap_module_name[] = "VIC-II";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int vicii_snapshot_write_module(snapshot_t *s)
{
    int i;
    snapshot_module_t *m;
    BYTE color_ram[0x400];

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    mem_color_ram_to_snapshot(color_ram);

    if (0
        /* VICII model (for sanity checks) */
        || SMW_B(m, (BYTE)vicii_resources.model) < 0
        /* state from vicii_s, in the same order */
        || SMW_BA(m, vicii.regs, 0x40) < 0
        || SMW_DW(m, (DWORD)vicii.raster_cycle) < 0
        || SMW_DW(m, (DWORD)vicii.cycle_flags) < 0
        || SMW_DW(m, (DWORD)vicii.raster_line) < 0
        || SMW_B(m, (BYTE)vicii.start_of_frame) < 0
        || SMW_B(m, (BYTE)vicii.irq_status) < 0
        || SMW_DW(m, (DWORD)vicii.raster_irq_line) < 0
        || SMW_B(m, (BYTE)vicii.raster_irq_triggered) < 0
        /* ram_base_phi[12] and vaddr_* updated from elsewhere */
        || SMW_BA(m, vicii.vbuf, VICII_SCREEN_TEXTCOLS) < 0
        || SMW_BA(m, vicii.cbuf, VICII_SCREEN_TEXTCOLS) < 0
        || SMW_B(m, vicii.gbuf) < 0
        || SMW_DW(m, (DWORD)vicii.dbuf_offset) < 0
        || SMW_BA(m, vicii.dbuf, VICII_DRAW_BUFFER_SIZE) < 0
        || SMW_DW(m, (DWORD)vicii.ysmooth) < 0
        || SMW_B(m, (BYTE)vicii.allow_bad_lines) < 0
        || SMW_B(m, vicii.sprite_sprite_collisions) < 0
        || SMW_B(m, vicii.sprite_background_collisions) < 0
        || SMW_B(m, vicii.clear_collisions) < 0
        || SMW_DW(m, (DWORD)vicii.idle_state) < 0
        || SMW_DW(m, (DWORD)vicii.vcbase) < 0
        || SMW_DW(m, (DWORD)vicii.vc) < 0
        || SMW_DW(m, (DWORD)vicii.rc) < 0
        || SMW_DW(m, (DWORD)vicii.vmli) < 0
        || SMW_DW(m, (DWORD)vicii.bad_line) < 0
        || SMW_B(m, (BYTE)vicii.light_pen.state) < 0
        || SMW_B(m, (BYTE)vicii.light_pen.triggered) < 0
        || SMW_DW(m, (DWORD)vicii.light_pen.x) < 0
        || SMW_DW(m, (DWORD)vicii.light_pen.y) < 0
        || SMW_DW(m, (DWORD)vicii.light_pen.x_extra_bits) < 0
        || SMW_DW(m, (DWORD)vicii.light_pen.trigger_cycle) < 0
        /* vbank_phi[12] updated from elsewhere */
        /* log is initialized at startup */
        || SMW_B(m, vicii.reg11_delay) < 0
        || SMW_DW(m, (DWORD)vicii.prefetch_cycles) < 0
        || SMW_DW(m, (DWORD)vicii.sprite_display_bits) < 0
        || SMW_B(m, vicii.sprite_dma) < 0
        /* sprite[] handled below */
        /* geometry, parameters and cycle_table set from elsewhere */
        || SMW_B(m, vicii.last_color_reg) < 0
        || SMW_B(m, vicii.last_color_value) < 0
        || SMW_B(m, vicii.last_read_phi1) < 0
        || SMW_B(m, vicii.last_bus_phi2) < 0
        || SMW_B(m, (BYTE)vicii.vborder) < 0
        || SMW_B(m, (BYTE)vicii.set_vborder) < 0
        || SMW_B(m, (BYTE)vicii.main_border) < 0
        || SMW_B(m, vicii.refresh_counter) < 0
        /* ColorRam */
        || SMW_BA(m, color_ram, 0x400) < 0) {
        goto fail;
    }

    for (i = 0; i < VICII_NUM_SPRITES; i++) {
        if (0
            || SMW_DW(m, vicii.sprite[i].data) < 0
            || SMW_B(m, vicii.sprite[i].mc) < 0
            || SMW_B(m, vicii.sprite[i].mcbase) < 0
            || SMW_B(m, vicii.sprite[i].pointer) < 0
            || SMW_B(m, (BYTE)vicii.sprite[i].exp_flop) < 0
            || SMW_DW(m, (DWORD)vicii.sprite[i].x) < 0) {
            goto fail;
        }
    }

    if (vicii_draw_cycle_snapshot_write(m) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int vicii_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    int i;
    snapshot_module_t *m;
    BYTE color_ram[0x400];

    m = snapshot_module_open(s, snap_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(vicii.log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    /* VICII model */
    if (SMR_B_INT(m, &i) < 0) {
        goto fail;
    }

    if (i != vicii_resources.model) {
        /* FIXME */
        log_error(vicii.log,
                  "Snapshot was made with model %i while the current model is %i.",
                  i, vicii_resources.model);
        goto fail;
    }

    if (0
        /* state from vicii_s, in the same order */
        || SMR_BA(m, vicii.regs, 0x40) < 0
        || SMR_DW_UINT(m, &vicii.raster_cycle) < 0
        || SMR_DW_UINT(m, &vicii.cycle_flags) < 0
        || SMR_DW_UINT(m, &vicii.raster_line) < 0
        || SMR_B_INT(m, &vicii.start_of_frame) < 0
        || SMR_B_INT(m, &vicii.irq_status) < 0
        || SMR_DW_UINT(m, &vicii.raster_irq_line) < 0
        || SMR_B_INT(m, &vicii.raster_irq_triggered) < 0
        /* ram_base_phi[12] and vaddr_* updated from elsewhere */
        || SMR_BA(m, vicii.vbuf, VICII_SCREEN_TEXTCOLS) < 0
        || SMR_BA(m, vicii.cbuf, VICII_SCREEN_TEXTCOLS) < 0
        || SMR_B(m, &vicii.gbuf) < 0
        || SMR_DW_INT(m, &vicii.dbuf_offset) < 0
        || SMR_BA(m, vicii.dbuf, VICII_DRAW_BUFFER_SIZE) < 0
        || SMR_DW_UINT(m, &vicii.ysmooth) < 0
        || SMR_B_INT(m, &vicii.allow_bad_lines) < 0
        || SMR_B(m, &vicii.sprite_sprite_collisions) < 0
        || SMR_B(m, &vicii.sprite_background_collisions) < 0
        || SMR_B(m, &vicii.clear_collisions) < 0
        || SMR_DW_INT(m, &vicii.idle_state) < 0
        || SMR_DW_INT(m, &vicii.vcbase) < 0
        || SMR_DW_INT(m, &vicii.vc) < 0
        || SMR_DW_INT(m, &vicii.rc) < 0
        || SMR_DW_INT(m, &vicii.vmli) < 0
        || SMR_DW_INT(m, &vicii.bad_line) < 0
        || SMR_B_INT(m, &vicii.light_pen.state) < 0
        || SMR_B_INT(m, &vicii.light_pen.triggered) < 0
        || SMR_DW_INT(m, &vicii.light_pen.x) < 0
        || SMR_DW_INT(m, &vicii.light_pen.y) < 0
        || SMR_DW_INT(m, &vicii.light_pen.x_extra_bits) < 0
        || SMR_DW(m, &vicii.light_pen.trigger_cycle) < 0
        /* vbank_phi[12] updated from elsewhere */
        /* log is initialized at startup */
        || SMR_B(m, &vicii.reg11_delay) < 0
        || SMR_DW_INT(m, &vicii.prefetch_cycles) < 0
        || SMR_DW_UINT(m, &vicii.sprite_display_bits) < 0
        || SMR_B(m, &vicii.sprite_dma) < 0
        /* sprite[] handled below */
        /* geometry, parameters and cycle_table set from elsewhere */
        || SMR_B(m, &vicii.last_color_reg) < 0
        || SMR_B(m, &vicii.last_color_value) < 0
        || SMR_B(m, &vicii.last_read_phi1) < 0
        || SMR_B(m, &vicii.last_bus_phi2) < 0
        || SMR_B_INT(m, &vicii.vborder) < 0
        || SMR_B_INT(m, &vicii.set_vborder) < 0
        || SMR_B_INT(m, &vicii.main_border) < 0
        || SMR_B(m, &vicii.refresh_counter) < 0
        /* ColorRam */
        || SMR_BA(m, color_ram, 0x400) < 0) {
        goto fail;
    }

    mem_color_ram_from_snapshot(color_ram);

    for (i = 0; i < VICII_NUM_SPRITES; i++) {
        if (0
            || SMR_DW(m, &vicii.sprite[i].data) < 0
            || SMR_B(m, &vicii.sprite[i].mc) < 0
            || SMR_B(m, &vicii.sprite[i].mcbase) < 0
            || SMR_B(m, &vicii.sprite[i].pointer) < 0
            || SMR_B_INT(m, &vicii.sprite[i].exp_flop) < 0
            || SMR_DW_INT(m, &vicii.sprite[i].x) < 0) {
            goto fail;
        }
    }

    if (vicii_draw_cycle_snapshot_read(m) < 0) {
        goto fail;
    }
    {
        unsigned int l = vicii.raster_line;

        if (l >= (vicii.screen_height - 1)) {
            l = 0;
        }

        vicii.raster.current_line = l;
    }
    if (vicii.irq_status & 0x80) {
        interrupt_restore_irq(maincpu_int_status, vicii.int_num, 1);
    }

    raster_force_repaint(&vicii.raster);
    snapshot_module_close(m);
    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }

    return -1;
}
