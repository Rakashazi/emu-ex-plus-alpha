/*
 * vicii-snapshot.c - Snapshot functionality for the MOS 6569 (VIC-II)
 * emulation.
 *
 * Written by
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

#include "alarm.h"
#include "interrupt.h"
#include "log.h"
#include "mem.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "snapshot.h"
#include "types.h"
#include "vicii-irq.h"
#include "vicii-snapshot.h"
#include "vicii-sprites.h"
#include "vicii.h"
#include "viciitypes.h"


/* Make sure all the VIC-II alarms are removed.  This just makes it easier to
   write functions for loading snapshot modules in other video chips without
   caring that the VIC-II alarms are dispatched when they really shouldn't
   be.  */
void vicii_snapshot_prepare(void)
{
    vicii.fetch_clk = CLOCK_MAX;
    alarm_unset(vicii.raster_fetch_alarm);
    vicii.draw_clk = CLOCK_MAX;
    alarm_unset(vicii.raster_draw_alarm);
    vicii.raster_irq_clk = CLOCK_MAX;
    alarm_unset(vicii.raster_irq_alarm);
}


/*

   This is the format of the VIC-II snapshot module.

   Name               Type   Size   Description

   AllowBadLines      BYTE   1      flag: if true, bad lines can happen
   BadLine            BYTE   1      flag: this is a bad line
   Blank              BYTE   1      flag: draw lines in border color
   ColorBuf           BYTE   40     character memory buffer (loaded at bad line)
   ColorRam           BYTE   1024   contents of color RAM
   IdleState          BYTE   1      flag: idle state enabled
   LPTrigger          BYTE   1      flag: light pen has been triggered
   LPX                BYTE   1      light pen X
   LPY                BYTE   1      light pen Y
   MatrixBuf          BYTE   40     video matrix buffer (loaded at bad line)
   NewSpriteDmaMask   BYTE   1      value for SpriteDmaMask after drawing
                                    sprites
   RamBase            DWORD  1      pointer to the start of RAM seen by the VIC
   RasterCycle        BYTE   1      current vicii.raster cycle
   RasterLine         WORD   1      current vicii.raster line
   Registers          BYTE   64     VIC-II registers
   SbCollMask         BYTE   1      sprite-background collisions so far
   SpriteDmaMask      BYTE   1      sprites having DMA turned on
   SsCollMask         BYTE   1      sprite-sprite collisions so far
   VBank              BYTE   1      location of memory bank
   Vc                 WORD   1      internal VIC-II counter
   VcAdd              BYTE   1      value to add to Vc at the end of this line
                                    (vicii.mem_counter_inc)
   VcBase             WORD   1      internal VIC-II memory pointer
   VideoInt           BYTE   1      status of VIC-II IRQ (vicii.irq_status)

   [Sprite section: (repeat 8 times)]

   SpriteXMemPtr      BYTE   1      sprite memory pointer
   SpriteXMemPtrInc   BYTE   1      value to add to the MemPtr after fetch
   SpriteXExpFlipFlop BYTE   1      sprite expansion flip-flop

   [Alarm section]
   FetchEventTick     DWORD  1      ticks for the next "fetch" (DMA) event
   FetchEventType     BYTE   1      type of event (0: matrix, 1: sprite check, 2: sprite fetch)

 */

static char snap_module_name[] = "VIC-II";
#define SNAP_MAJOR 1
#define SNAP_MINOR 1

int vicii_snapshot_write_module(snapshot_t *s)
{
    int i;
    snapshot_module_t *m;
    BYTE color_ram[0x400];

    /* FIXME: Dispatch all events?  */

    m = snapshot_module_create (s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    mem_color_ram_to_snapshot(color_ram);

    if (0
        /* AllowBadLines */
        || SMW_B(m, (BYTE)vicii.allow_bad_lines) < 0
        /* BadLine */
        || SMW_B(m, (BYTE)vicii.bad_line) < 0
        /* Blank */
        || SMW_B(m, (BYTE)vicii.raster.blank_enabled) < 0
        /* ColorBuf */
        || SMW_BA(m, vicii.cbuf, 40) < 0
        /* ColorRam */
        || SMW_BA(m, color_ram, 1024) < 0
        /* IdleState */
        || SMW_B(m, (BYTE)vicii.idle_state) < 0
        /* LPTrigger */
        || SMW_B(m, (BYTE)vicii.light_pen.triggered) < 0
        /* LPX */
        || SMW_B(m, (BYTE)vicii.light_pen.x) < 0
        /* LPY */
        || SMW_B(m, (BYTE)vicii.light_pen.y) < 0
        /* MatrixBuf */
        || SMW_BA(m, vicii.vbuf, 40) < 0
        /* NewSpriteDmaMask */
        || SMW_B(m, vicii.raster.sprite_status->new_dma_msk) < 0
        /* RamBase */
        || SMW_DW(m, (DWORD)(vicii.ram_base_phi1 - mem_ram)) < 0
        /* RasterCycle */
        || SMW_B(m, (BYTE)(VICII_RASTER_CYCLE(maincpu_clk))) < 0
        /* RasterLine */
        || SMW_W(m, (WORD)(VICII_RASTER_Y(maincpu_clk))) < 0) {
        goto fail;
    }

    for (i = 0; i < 0x40; i++) {
        /* Registers */
        if (SMW_B(m, vicii.regs[i]) < 0) {
            goto fail;
        }
    }

    if (0
        /* SbCollMask */
        || SMW_B(m, (BYTE)vicii.sprite_background_collisions) < 0
        /* SpriteDmaMask */
        || SMW_B(m, (BYTE)vicii.raster.sprite_status->dma_msk) < 0
        /* SsCollMask */
        || SMW_B(m, (BYTE)vicii.sprite_sprite_collisions) < 0
        /* VBank */
        || SMW_W(m, (WORD)vicii.vbank_phi1) < 0
        /* Vc */
        || SMW_W(m, (WORD)vicii.mem_counter) < 0
        /* VcInc */
        || SMW_B(m, (BYTE)vicii.mem_counter_inc) < 0
        /* VcBase */
        || SMW_W(m, (WORD)vicii.memptr) < 0
        /* VideoInt */
        || SMW_B(m, (BYTE)vicii.irq_status) < 0) {
        goto fail;
    }

    for (i = 0; i < 8; i++) {
        if (0
            /* SpriteXMemPtr */
            || SMW_B(m, (BYTE)vicii.raster.sprite_status->sprites[i].memptr) < 0
            /* SpriteXMemPtrInc */
            || SMW_B(m, (BYTE)vicii.raster.sprite_status->sprites[i].memptr_inc) < 0
            /* SpriteXExpFlipFlop */
            || SMW_B(m, (BYTE)vicii.raster.sprite_status->sprites[i].exp_flag) < 0) {
            goto fail;
        }
    }

    if (0
        /* FetchEventTick */
        || SMW_DW(m, vicii.fetch_clk - maincpu_clk) < 0
        /* FetchEventType */
        || SMW_B(m, (BYTE)vicii.fetch_idx) < 0) {
        goto fail;
    }

    /* Added in version 1.1 of the snapshot module */
    /* using "ram_base-ram" is F***ing bullshit - what when external memory
       is not mapped anywhere in ram[]? We should rather use some more generic
       configuration info. But as we use it above in V1.0... :-(
       AF 16jan2001 */
    if (0
        /* RamBase */
        || SMW_DW(m, (DWORD)(vicii.ram_base_phi2 - mem_ram)) < 0
        /* VBank */
        || SMW_W(m, (WORD)vicii.vbank_phi2) < 0) {
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

    /* FIXME: initialize changes?  */

    if (0
        /* AllowBadLines */
        || SMR_B_INT(m, &vicii.allow_bad_lines) < 0
        /* BadLine */
        || SMR_B_INT(m, &vicii.bad_line) < 0
        /* Blank */
        || SMR_B_INT(m, &vicii.raster.blank_enabled) < 0
        /* ColorBuf */
        || SMR_BA(m, vicii.cbuf, 40) < 0
        /* ColorRam */
        || SMR_BA(m, color_ram, 1024) < 0
        /* IdleState */
        || SMR_B_INT(m, &vicii.idle_state) < 0
        /* LPTrigger */
        || SMR_B_INT(m, &vicii.light_pen.triggered) < 0
        /* LPX */
        || SMR_B_INT(m, &vicii.light_pen.x) < 0
        /* LPY */
        || SMR_B_INT(m, &vicii.light_pen.y) < 0
        /* MatrixBuf */
        || SMR_BA(m, vicii.vbuf, 40) < 0
        /* NewSpriteDmaMask */
        || SMR_B(m, &vicii.raster.sprite_status->new_dma_msk) < 0) {
        goto fail;
    }

    mem_color_ram_from_snapshot(color_ram);

    {
        DWORD RamBase;

        if (SMR_DW(m, &RamBase) < 0) {
            goto fail;
        }
        vicii.ram_base_phi1 = mem_ram + RamBase;
    }

    /* Read the current raster line and the current raster cycle.  As they
       are a function of `clk', this is just a sanity check.  */
    {
        WORD RasterLine;
        BYTE RasterCycle;

        if (SMR_B(m, &RasterCycle) < 0
            || SMR_W(m, &RasterLine) < 0) {
            goto fail;
        }

        if (RasterCycle != (BYTE)VICII_RASTER_CYCLE(maincpu_clk)) {
            log_error(vicii.log,
                      "Not matching raster cycle (%d) in snapshot; should be %d.",
                      RasterCycle, VICII_RASTER_CYCLE(maincpu_clk));
            goto fail;
        }

        if (RasterLine != (WORD)VICII_RASTER_Y(maincpu_clk)) {
            log_error(vicii.log,
                      "VIC-II: Not matching raster line (%d) in snapshot; should be %d.",
                      RasterLine, VICII_RASTER_Y(maincpu_clk));
            goto fail;
        }
    }

    for (i = 0; i < 0x40; i++) {
        if (SMR_B(m, &vicii.regs[i]) < 0 /* Registers */) {
            goto fail;
        }
    }

    if (0
        /* SbCollMask */
        || SMR_B(m, &vicii.sprite_background_collisions) < 0
        /* SpriteDmaMask */
        || SMR_B(m, &vicii.raster.sprite_status->dma_msk) < 0
        /* SsCollMask */
        || SMR_B(m, &vicii.sprite_sprite_collisions) < 0
        /* VBank */
        || SMR_W_INT(m, &vicii.vbank_phi1) < 0
        /* Vc */
        || SMR_W_INT(m, &vicii.mem_counter) < 0
        /* VcInc */
        || SMR_B_INT(m, &vicii.mem_counter_inc) < 0
        /* VcBase */
        || SMR_W_INT(m, &vicii.memptr) < 0
        /* VideoInt */
        || SMR_B_INT(m, &vicii.irq_status) < 0) {
        goto fail;
    }

    for (i = 0; i < 8; i++) {
        if (0
            /* SpriteXMemPtr */
            || SMR_B_INT(m, &vicii.raster.sprite_status->sprites[i].memptr) < 0
            /* SpriteXMemPtrInc */
            || SMR_B_INT(m, &vicii.raster.sprite_status->sprites[i].memptr_inc) < 0
            /* SpriteXExpFlipFlop */
            || SMR_B_INT(m, &vicii.raster.sprite_status->sprites[i].exp_flag) < 0
            ) {
            goto fail;
        }
    }

    /* FIXME: Recalculate alarms and derived values.  */
#if 1
    {
        /*
            We cannot use vicii_irq_set_raster_line as this would delay
            an alarm on line 0 for one frame
        */
        unsigned int line = vicii.regs[0x12] | ((vicii.regs[0x11] & 0x80) << 1);

        if (line < (unsigned int)vicii.screen_height) {
            vicii.raster_irq_clk = (VICII_LINE_START_CLK(maincpu_clk)
                                    + VICII_RASTER_IRQ_DELAY - INTERRUPT_DELAY
                                    + (vicii.cycles_per_line * line));

            /* Raster interrupts on line 0 are delayed by 1 cycle.  */
            if (line == 0) {
                vicii.raster_irq_clk++;
            }

            alarm_set(vicii.raster_irq_alarm, vicii.raster_irq_clk);
        } else {
            vicii.raster_irq_clk = CLOCK_MAX;
            alarm_unset(vicii.raster_irq_alarm);
        }
        vicii.raster_irq_line = line;
    }

#else
    vicii_irq_set_raster_line(vicii.regs[0x12]
                              | ((vicii.regs[0x11] & 0x80) << 1));
#endif

    /* compatibility with older versions */
    vicii.ram_base_phi2 = vicii.ram_base_phi1;
    vicii.vbank_phi2 = vicii.vbank_phi1;

    vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));

    /* Update sprite parameters.  We had better do this manually, or the
       VIC-II emulation could be quite upset.  */
    {
        BYTE msk;

        for (i = 0, msk = 0x1; i < 8; i++, msk <<= 1) {
            raster_sprite_t *sprite;
            int tmp;

            sprite = vicii.raster.sprite_status->sprites + i;

            /* X/Y coordinates.  */
            tmp = vicii.regs[i * 2] + ((vicii.regs[0x10] & msk) ? 0x100 : 0);

            /* (-0xffff makes sure it's updated NOW.) */
            vicii_sprites_set_x_position(i, tmp, -0xffff);

            sprite->y = (int)vicii.regs[i * 2 + 1];
            sprite->x_expanded = (int)(vicii.regs[0x1d] & msk);
            sprite->y_expanded = (int)(vicii.regs[0x17] & msk);
            sprite->multicolor = (int)(vicii.regs[0x1c] & msk);
            sprite->in_background = (int)(vicii.regs[0x1b] & msk);
            sprite->color = (int) vicii.regs[0x27 + i] & 0xf;
            sprite->dma_flag = (int)(vicii.raster.sprite_status->new_dma_msk & msk);
        }
    }

    vicii.sprite_fetch_msk = vicii.raster.sprite_status->new_dma_msk;
    vicii.sprite_fetch_clk = VICII_LINE_START_CLK(maincpu_clk)
                             + vicii.sprite_fetch_cycle
                             - vicii.cycles_per_line;

    /* calculate the sprite_fetch_idx */
    {
        const vicii_sprites_fetch_t *sf;

        sf = vicii_sprites_fetch_table[vicii.sprite_fetch_msk];
        i = 0;
        while (sf[i].cycle >= 0 && sf[i].cycle + vicii.sprite_fetch_cycle <= vicii.cycles_per_line) {
            i++;
        }
        vicii.sprite_fetch_idx = i;
    }

    vicii.raster.xsmooth = vicii.regs[0x16] & 0x7;
    vicii.raster.sprite_xsmooth = vicii.regs[0x16] & 0x7;
    vicii.raster.ysmooth = vicii.regs[0x11] & 0x7;
    vicii.raster.current_line = VICII_RASTER_Y(maincpu_clk); /* FIXME? */

    vicii.raster.sprite_status->visible_msk = vicii.regs[0x15];

    /* Update colors.  */
    vicii.raster.border_color = vicii.regs[0x20] & 0xf;
    vicii.raster.background_color = vicii.regs[0x21] & 0xf;
    vicii.ext_background_color[0] = vicii.regs[0x22] & 0xf;
    vicii.ext_background_color[1] = vicii.regs[0x23] & 0xf;
    vicii.ext_background_color[2] = vicii.regs[0x24] & 0xf;
    vicii.raster.sprite_status->mc_sprite_color_1 = vicii.regs[0x25] & 0xf;
    vicii.raster.sprite_status->mc_sprite_color_2 = vicii.regs[0x26] & 0xf;

    vicii.raster.blank = !(vicii.regs[0x11] & 0x10);

    if (VICII_IS_ILLEGAL_MODE(vicii.raster.video_mode)) {
        vicii.raster.idle_background_color = 0;
        vicii.force_black_overscan_background_color = 1;
    } else {
        vicii.raster.idle_background_color
            = vicii.raster.background_color;
        vicii.force_black_overscan_background_color = 0;
    }

    if (vicii.regs[0x11] & 0x8) {
        vicii.raster.display_ystart = vicii.row_25_start_line;
        vicii.raster.display_ystop = vicii.row_25_stop_line;
    } else {
        vicii.raster.display_ystart = vicii.row_24_start_line;
        vicii.raster.display_ystop = vicii.row_24_stop_line;
    }

    if (vicii.regs[0x16] & 0x8) {
        vicii.raster.display_xstart = VICII_40COL_START_PIXEL;
        vicii.raster.display_xstop = VICII_40COL_STOP_PIXEL;
    } else {
        vicii.raster.display_xstart = VICII_38COL_START_PIXEL;
        vicii.raster.display_xstop = VICII_38COL_STOP_PIXEL;
    }

    /* `vicii.raster.draw_idle_state', `vicii.raster.open_right_border' and
       `vicii.raster.open_left_border' should be needed, but they would only
       affect the current vicii.raster line, and would not cause any
       difference in timing.  So who cares.  */

    /* FIXME: `vicii.ycounter_reset_checked'?  */
    /* FIXME: `vicii.force_display_state'?  */

    vicii.memory_fetch_done = 0; /* FIXME? */

    vicii_update_video_mode(VICII_RASTER_CYCLE(maincpu_clk));

    vicii.draw_clk = maincpu_clk + (vicii.draw_cycle - VICII_RASTER_CYCLE(maincpu_clk));
    vicii.last_emulate_line_clk = vicii.draw_clk - vicii.cycles_per_line;
    alarm_set(vicii.raster_draw_alarm, vicii.draw_clk);

    {
        DWORD dw;
        BYTE b;

        if (0
            || SMR_DW(m, &dw) < 0  /* FetchEventTick */
            || SMR_B(m, &b) < 0    /* FetchEventType */
            ) {
            goto fail;
        }

        vicii.fetch_clk = maincpu_clk + dw;
        vicii.fetch_idx = b;

        alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
    }

    if (vicii.irq_status & 0x80) {
        interrupt_restore_irq(maincpu_int_status, vicii.int_num, 1);
    }

    /* added in version 1.1 of snapshot format */
    if (minor_version > 0) {
        DWORD RamBase;

        if (0
            || SMR_DW(m, &RamBase) < 0
            || SMR_W_INT(m, &vicii.vbank_phi2) < 0 /* VBank */
            ) {
            goto fail;
        }
        vicii.ram_base_phi2 = mem_ram + RamBase;

        vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
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
