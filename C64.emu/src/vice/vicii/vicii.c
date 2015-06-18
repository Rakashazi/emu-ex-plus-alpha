/*
 * vicii.c - A cycle-exact event-driven MOS6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * DTV sections written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
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

/* TODO: - speed optimizations;
   - faster sprites and registers.  */

/*
   Current (most important) known limitations:

   - sprite colors (and other attributes) cannot change in the middle of the
   raster line;

   Probably something else which I have not figured out yet...

 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alarm.h"
#include "c64.h"
#include "cartridge.h"
#include "c64cart.h"
#include "c64cartmem.h"
#include "c64dtvblitter.h"
#include "c64dtvdma.h"
#include "clkguard.h"
#include "dma.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "raster-changes.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "raster-sprite-status.h"
#include "raster-sprite.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"
#include "vicii-cmdline-options.h"
#include "vicii-color.h"
#include "vicii-draw.h"
#include "vicii-fetch.h"
#include "vicii-irq.h"
#include "vicii-mem.h"
#include "vicii-sprites.h"
#include "vicii-resources.h"
#include "vicii-timing.h"
#include "vicii.h"
#include "viciitypes.h"
#include "vsync.h"
#include "video.h"
#include "videoarch.h"
#include "viewport.h"


void vicii_set_phi1_addr_options(WORD mask, WORD offset)
{
    vicii.vaddr_mask_phi1 = mask;
    vicii.vaddr_offset_phi1 = offset;

    VICII_DEBUG_REGISTER(("Set phi1 video addr mask=%04x, offset=%04x", mask, offset));
    vicii_update_memory_ptrs_external();
}

void vicii_set_phi2_addr_options(WORD mask, WORD offset)
{
    vicii.vaddr_mask_phi2 = mask;
    vicii.vaddr_offset_phi2 = offset;

    VICII_DEBUG_REGISTER(("Set phi2 video addr mask=%04x, offset=%04x", mask, offset));
    vicii_update_memory_ptrs_external();
}

void vicii_set_phi1_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi1 = mask;
    vicii.vaddr_chargen_value_phi1 = value;

    VICII_DEBUG_REGISTER(("Set phi1 chargen addr mask=%04x, value=%04x", mask, value));
    vicii_update_memory_ptrs_external();
}

void vicii_set_phi2_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi2 = mask;
    vicii.vaddr_chargen_value_phi2 = value;

    VICII_DEBUG_REGISTER(("Set phi2 chargen addr mask=%04x, value=%04x", mask, value));
    vicii_update_memory_ptrs_external();
}

void vicii_set_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi1 = mask;
    vicii.vaddr_chargen_value_phi1 = value;
    vicii.vaddr_chargen_mask_phi2 = mask;
    vicii.vaddr_chargen_value_phi2 = value;

    VICII_DEBUG_REGISTER(("Set chargen addr mask=%04x, value=%04x", mask, value));
    vicii_update_memory_ptrs_external();
}

/* ---------------------------------------------------------------------*/

vicii_t vicii;

static void vicii_set_geometry(void);

static void clk_overflow_callback(CLOCK sub, void *unused_data)
{
    vicii.raster_irq_clk -= sub;
    vicii.last_emulate_line_clk -= sub;
    vicii.fetch_clk -= sub;
    vicii.draw_clk -= sub;
    vicii.sprite_fetch_clk -= sub;
}

void vicii_change_timing(machine_timing_t *machine_timing, int border_mode)
{
    vicii_timing_set(machine_timing, border_mode);

    if (vicii.initialized) {
        vicii_set_geometry();
        raster_mode_change();
    }
}

static CLOCK old_maincpu_clk = 0;

void vicii_delay_oldclk(CLOCK num)
{
    old_maincpu_clk += num;
}

inline void vicii_delay_clk(void)
{
#if 0
    CLOCK diff;

    /*log_debug("MCLK %d OMCLK %d", maincpu_clk, old_maincpu_clk);*/

    if (vicii.fastmode == 0) {
        diff = maincpu_clk - old_maincpu_clk;

        if (!vicii.badline_disable) {
            dma_maincpu_steal_cycles(maincpu_clk, diff, 0);
        }
    }

    old_maincpu_clk = maincpu_clk;

    return;
#endif
}

inline void vicii_handle_pending_alarms(int num_write_cycles)
{
    if (vicii.viciie != 0) {
        vicii_delay_clk();
    }

    if (num_write_cycles != 0) {
        int f;

        /* Cycles can be stolen only during the read accesses, so we serve
           only the events that happened during them.  The last read access
           happened at `clk - maincpu_write_cycles()' as all the opcodes
           except BRK and JSR do all the write accesses at the very end.  BRK
           cannot take us here and we would not be able to handle JSR
           correctly anyway, so we don't care about them...  */

        /* Go back to the time when the read accesses happened and serve VIC
         events.  */
        maincpu_clk -= num_write_cycles;

        do {
            f = 0;
            if (maincpu_clk > vicii.fetch_clk) {
                vicii_fetch_alarm_handler(0, NULL);
                f = 1;
                if (vicii.viciie != 0) {
                    vicii_delay_clk();
                }
            }
            if (maincpu_clk >= vicii.draw_clk) {
                vicii_raster_draw_alarm_handler((CLOCK)(maincpu_clk - vicii.draw_clk), NULL);
                f = 1;
                if (vicii.viciie != 0) {
                    vicii_delay_clk();
                }
            }
        }
        while (f);

        /* Go forward to the time when the last write access happens (that's
           the one we care about, as the only instructions that do two write
           accesses - except BRK and JSR - are the RMW ones, which store the
           old value in the first write access, and then store the new one in
           the second write access).  */
        maincpu_clk += num_write_cycles;
    } else {
        int f;

        do {
            f = 0;
            if (maincpu_clk >= vicii.fetch_clk) {
                vicii_fetch_alarm_handler(0, NULL);
                f = 1;
                if (vicii.viciie != 0) {
                    vicii_delay_clk();
                }
            }
            if (maincpu_clk >= vicii.draw_clk) {
                vicii_raster_draw_alarm_handler(0, NULL);
                f = 1;
                if (vicii.viciie != 0) {
                    vicii_delay_clk();
                }
            }
        }
        while (f);
    }
}

void vicii_handle_pending_alarms_external(int num_write_cycles)
{
    if (vicii.initialized) {
        vicii_handle_pending_alarms(num_write_cycles);
    }
}

void vicii_handle_pending_alarms_external_write(void)
{
    /* WARNING: assumes `maincpu_rmw_flag' is 0 or 1.  */
    if (vicii.initialized) {
        vicii_handle_pending_alarms(maincpu_rmw_flag + 1);
    }
}

/* return pixel aspect ratio for current video mode
 * based on http://codebase64.com/doku.php?id=base:pixel_aspect_ratio
 */
static float vicii_get_pixel_aspect(void)
{
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
            return 0.93650794f;
        case MACHINE_SYNC_PALN:
            return 0.90769231f;
        case MACHINE_SYNC_NTSC:
            return 0.75000000f;
        case MACHINE_SYNC_NTSCOLD:
            return 0.76171875f;
        default:
            return 1.0f;
    }
}

/* return type of monitor used for current video mode */
static int vicii_get_crt_type(void)
{
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            return 1; /* PAL */
        default:
            return 0; /* NTSC */
    }
}

static void vicii_set_geometry(void)
{
    unsigned int width, height;

    width = vicii.screen_leftborderwidth + VICII_SCREEN_XPIX + vicii.screen_rightborderwidth;
    height = vicii.last_displayed_line - vicii.first_displayed_line + 1;

    raster_set_geometry(&vicii.raster,
                        width, height, /* canvas dimensions */
                        width, vicii.screen_height, /* screen dimensions */
                        VICII_SCREEN_XPIX, VICII_SCREEN_YPIX, /* gfx dimensions */
                        VICII_SCREEN_TEXTCOLS, VICII_SCREEN_TEXTLINES, /* text dimensions */
                        vicii.screen_leftborderwidth, vicii.row_25_start_line, /* gfx position */
                        vicii.viciidtv ? 1 : 0, /* gfx area doesn't move except on DTV*/
                        vicii.first_displayed_line,
                        vicii.last_displayed_line,
                        -VICII_RASTER_X(0),  /* extra offscreen border left */
                        vicii.sprite_wrap_x - VICII_SCREEN_XPIX -
                        vicii.screen_leftborderwidth - vicii.screen_rightborderwidth + VICII_RASTER_X(0)) /* extra offscreen border right */;
#ifdef __MSDOS__
    video_ack_vga_mode();
#endif

    vicii.raster.geometry->pixel_aspect_ratio = vicii_get_pixel_aspect();
    vicii.raster.viewport->crt_type = vicii_get_crt_type();
}

static int init_raster(void)
{
    raster_t *raster;

    raster = &vicii.raster;

    raster_sprite_status_new(raster, VICII_NUM_SPRITES, vicii_sprite_offset());
    raster_line_changes_sprite_init(raster);

    /* FIXME: VICII_NUM_VMODES is only valid for DTV.
       A smaller number could be used for !DTV, but
       VICII_IDLE_MODE needs to be < VICII_NUM_VMODES. */
    if (raster_init(raster, VICII_NUM_VMODES) < 0) {
        return -1;
    }
    raster_modes_set_idle_mode(raster->modes, VICII_IDLE_MODE);
    resources_touch("VICIIVideoCache");

    vicii_set_geometry();

    if (vicii_color_update_palette(raster->canvas) < 0) {
        log_error(vicii.log, "Cannot load palette.");
        return -1;
    }

    raster_set_title(raster, machine_name);

    if (raster_realize(raster) < 0) {
        return -1;
    }
    raster->display_ystart = vicii.row_25_start_line;
    raster->display_ystop = vicii.row_25_stop_line;
    raster->display_xstart = VICII_40COL_START_PIXEL;
    raster->display_xstop = VICII_40COL_STOP_PIXEL;

    if (vicii.viciidtv) {
        raster->can_disable_border = 1;
    }
    return 0;
}

/* Initialize the VIC-II emulation.  */
raster_t *vicii_init(unsigned int flag)
{
    vicii.fastmode = 0;
    vicii.half_cycles = 0;

    switch (flag) {
        case VICII_EXTENDED:
            vicii.viciie = 1;
            vicii.viciidtv = 0;
            vicii.log = log_open("VIC-IIe");
            break;
        case VICII_DTV:
            vicii.viciie = 0;
            vicii.viciidtv = 1;
            vicii.log = log_open("VIC-II DTV");
            break;
        default:
        case VICII_STANDARD:
            vicii.viciie = 0;
            vicii.viciidtv = 0;
            vicii.log = log_open("VIC-II");
            break;
    }

    vicii_irq_init();

    vicii_fetch_init();

    vicii.raster_draw_alarm = alarm_new(maincpu_alarm_context,
                                        "VicIIRasterDraw",
                                        vicii_raster_draw_alarm_handler, NULL);
    if (init_raster() < 0) {
        return NULL;
    }

    vicii_powerup();

    vicii.video_mode = -1;
    vicii_update_video_mode(0);
    vicii_update_memory_ptrs(0);

    vicii_draw_init();
    vicii_sprites_init();

    vicii.num_idle_3fff = 0;
    vicii.num_idle_3fff_old = 0;
    vicii.idle_3fff = lib_malloc(sizeof(idle_3fff_t) * 64);
    vicii.idle_3fff_old = lib_malloc(sizeof(idle_3fff_t) * 64);

    vicii.buf_offset = 0;

    vicii.initialized = 1;

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    return &vicii.raster;
}

struct video_canvas_s *vicii_get_canvas(void)
{
    return vicii.raster.canvas;
}

/* Reset the VIC-II chip.  */
void vicii_reset(void)
{
    int i;

    raster_reset(&vicii.raster);

    vicii.last_emulate_line_clk = 0;

    vicii.draw_clk = vicii.draw_cycle;
    alarm_set(vicii.raster_draw_alarm, vicii.draw_clk);

    vicii.fetch_clk = VICII_FETCH_CYCLE;
    alarm_set(vicii.raster_fetch_alarm, vicii.fetch_clk);
    vicii.fetch_idx = VICII_FETCH_MATRIX;
    vicii.sprite_fetch_idx = 0;
    vicii.sprite_fetch_msk = 0;
    vicii.sprite_fetch_clk = CLOCK_MAX;

    /* FIXME: I am not sure this is exact emulation.  */
    vicii.raster_irq_line = 0;
    vicii.raster_irq_clk = 0;
    vicii.regs[0x11] = 0;
    vicii.regs[0x12] = 0;

    /* Setup the raster IRQ alarm.  The value is `1' instead of `0' because we
       are at the first line, which has a +1 clock cycle delay in IRQs.  */
    alarm_set(vicii.raster_irq_alarm, 1);

    vicii.force_display_state = 0;

    vicii.light_pen.state = 0;
    vicii.light_pen.triggered = 0;
    vicii.light_pen.x = vicii.light_pen.y = vicii.light_pen.x_extra_bits = 0;

    /* Remove all the IRQ sources.  */
    vicii.regs[0x1a] = 0;

    vicii.raster.display_ystart = vicii.row_25_start_line;
    vicii.raster.display_ystop = vicii.row_25_stop_line;

    vicii.store_clk = CLOCK_MAX;

    vicii.counta = 0;
    vicii.counta_mod = 0;
    vicii.counta_step = 0;
    vicii.countb = 0;
    vicii.countb_mod = 0;
    vicii.countb_step = 0;
    for (i = 0; i < 256; i++) {
        vicii.dtvpalette[i] = i;
    }

    vicii.dtvpalette[0] = 0;
    vicii.dtvpalette[1] = 0x0f;
    vicii.dtvpalette[2] = 0x36;
    vicii.dtvpalette[3] = 0xbe;
    vicii.dtvpalette[4] = 0x58;
    vicii.dtvpalette[5] = 0xdb;
    vicii.dtvpalette[6] = 0x86;
    vicii.dtvpalette[7] = 0xff;
    vicii.dtvpalette[8] = 0x29;
    vicii.dtvpalette[9] = 0x26;
    vicii.dtvpalette[10] = 0x3b;
    vicii.dtvpalette[11] = 0x05;
    vicii.dtvpalette[12] = 0x07;
    vicii.dtvpalette[13] = 0xdf;
    vicii.dtvpalette[14] = 0x9a;
    vicii.dtvpalette[15] = 0x0a;

    /* clear the high nibble from colors so that standard colors can be written without
       having extended_enable=1 */
    vicii.regs[0x20] &= 0xf;
    vicii.regs[0x21] &= 0xf;
    vicii.regs[0x22] &= 0xf;
    vicii.regs[0x23] &= 0xf;
    vicii.regs[0x24] &= 0xf;

    vicii.regs[0x3c] = 0;

    vicii.regs[0x36] = 0x76;
    vicii.regs[0x37] = 0;

    /* clear count[ab] & other regs,
       fixes problem with DTVBIOS, gfxmodes & soft reset */
    vicii.regs[0x38] = 0;
    vicii.regs[0x39] = 0;
    vicii.regs[0x3a] = 0;
    vicii.regs[0x3b] = 0;
    vicii.regs[0x3d] = 0;
    vicii.regs[0x44] = 64;
    vicii.regs[0x45] = 0;
    vicii.regs[0x46] = 0;
    vicii.regs[0x47] = 0;
    vicii.regs[0x48] = 0;
    vicii.regs[0x49] = 0;
    vicii.regs[0x4a] = 0;
    vicii.regs[0x4b] = 0;
    vicii.regs[0x4c] = 0;
    vicii.regs[0x4d] = 0;

    vicii.extended_enable = 0;
    vicii.badline_disable = 0;
    vicii.colorfetch_disable = 0;
    vicii.border_off = 0;
    vicii.overscan = 0;
    vicii.color_ram_ptr = &mem_ram[0x01d800];

    vicii.raster_irq_offset = 0;
    vicii.raster_irq_prevent = 0;

    /* Disable DTV features on non-DTV VIC-II */
    if (!vicii.viciidtv) {
        vicii.extended_lockout = 1;
    } else {
        vicii.extended_lockout = 0;
    }
}

void vicii_reset_registers(void)
{
    WORD i;

    if (!vicii.initialized) {
        return;
    }

    if (!vicii.viciidtv) {
        for (i = 0; i <= 0x3f; i++) {
            vicii_store(i, 0);
        }
    } else {
        vicii.extended_enable = 1;
        vicii.extended_lockout = 0;
        for (i = 0; i <= 0x3e; i++) {
            vicii_store(i, 0);
        }
        vicii_store(0x36, 0x76);
        for (i = 0x40; i <= 0x4f; i++) {
            vicii_store(i, 0);
        }
        vicii_store(0x3f, 0);
    }
    raster_sprite_status_reset(vicii.raster.sprite_status, vicii_sprite_offset());
}

/* This /should/ put the VIC-II in the same state as after a powerup, if
   `reset_vicii()' is called afterwards.  But FIXME, as we are not really
   emulating everything correctly here; just $D011.  */
void vicii_powerup(void)
{
    memset(vicii.regs, 0, sizeof(vicii.regs));

    vicii.irq_status = 0;
    vicii.raster_irq_line = 0;
    vicii.raster_irq_clk = 1;
    vicii.ram_base_phi1 = mem_ram;
    vicii.ram_base_phi2 = mem_ram;

    vicii.vaddr_mask_phi1 = 0xffff;
    vicii.vaddr_mask_phi2 = 0xffff;
    vicii.vaddr_offset_phi1 = 0;
    vicii.vaddr_offset_phi2 = 0;

    vicii.allow_bad_lines = 0;
    vicii.sprite_sprite_collisions = vicii.sprite_background_collisions = 0;
    vicii.fetch_idx = VICII_FETCH_MATRIX;
    vicii.idle_state = 0;
    vicii.force_display_state = 0;
    vicii.memory_fetch_done = 0;
    vicii.memptr = 0;
    vicii.mem_counter = 0;
    vicii.mem_counter_inc = 0;
    vicii.bad_line = 0;
    vicii.ycounter_reset_checked = 0;
    vicii.force_black_overscan_background_color = 0;
    vicii.light_pen.state = 0;
    vicii.light_pen.x = vicii.light_pen.y = vicii.light_pen.x_extra_bits = vicii.light_pen.triggered = 0;
    vicii.vbank_phi1 = 0;
    vicii.vbank_phi2 = 0;
    /* vicii.vbank_ptr = ram; */
    vicii.idle_data = 0;
    vicii.idle_data_location = IDLE_NONE;
    vicii.last_emulate_line_clk = 0;

    vicii_reset();

    vicii.raster.blank = 1;
    vicii.raster.display_ystart = vicii.row_24_start_line;
    vicii.raster.display_ystop = vicii.row_24_stop_line;

    vicii.raster.ysmooth = 0;
}

/* ---------------------------------------------------------------------*/

/* This hook is called whenever video bank must be changed.  */
static inline void vicii_set_vbanks(int vbank_p1, int vbank_p2)
{
    /* Warning: assumes it's called within a memory write access.
       FIXME: Change name?  */
    /* Also, we assume the bank has *really* changed, and do not do any
       special optimizations for the not-really-changed case.  */
    vicii_handle_pending_alarms(maincpu_rmw_flag + 1);
    if (maincpu_clk >= vicii.draw_clk) {
        vicii_raster_draw_alarm_handler(maincpu_clk - vicii.draw_clk, NULL);
    }

    vicii.vbank_phi1 = vbank_p1;
    vicii.vbank_phi2 = vbank_p2;
    vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
}

/* Phi1 and Phi2 accesses */
void vicii_set_vbank(int num_vbank)
{
    int tmp = num_vbank << 14;
    vicii_set_vbanks(tmp, tmp);
}

/* Phi1 accesses */
void vicii_set_phi1_vbank(int num_vbank)
{
    vicii_set_vbanks(num_vbank << 14, vicii.vbank_phi2);
}

/* Phi2 accesses */
void vicii_set_phi2_vbank(int num_vbank)
{
    vicii_set_vbanks(vicii.vbank_phi1, num_vbank << 14);
}

/* ---------------------------------------------------------------------*/

/* Set light pen input state.  */
void vicii_set_light_pen(CLOCK mclk, int state)
{
    if (state) {
        vicii_trigger_light_pen(mclk);
    }
    vicii.light_pen.state = state;
}

/* Trigger the light pen.  */
void vicii_trigger_light_pen(CLOCK mclk)
{
    if (!vicii.light_pen.triggered) {
        vicii.light_pen.triggered = 1;
        vicii.light_pen.x = VICII_RASTER_X(mclk % vicii.cycles_per_line) - vicii.screen_leftborderwidth + 0x20;

        if (vicii.light_pen.x < 0) {
            vicii.light_pen.x = vicii.sprite_wrap_x + vicii.light_pen.x;
        }

        /* FIXME: why `+2'? */
        vicii.light_pen.x = vicii.light_pen.x / 2 + 2 + vicii.light_pen.x_extra_bits;
        vicii.light_pen.x_extra_bits = 0;
        vicii.light_pen.y = VICII_RASTER_Y(mclk);

        vicii_irq_lightpen_set(mclk);
    }
}

/* Calculate lightpen pulse time based on x/y */
CLOCK vicii_lightpen_timing(int x, int y)
{
    CLOCK pulse_time = maincpu_clk;

    x += 0x80 - vicii.screen_leftborderwidth;
    y += vicii.first_displayed_line;

    /* Check if x would wrap to previous line */
    if (x < 104) {
        /* lightpen is off screen */
        pulse_time = 0;
    } else {
        pulse_time += (x / 8) + (y * vicii.cycles_per_line);
        /* Remove frame alarm jitter */
        pulse_time -= maincpu_clk - VICII_LINE_START_CLK(maincpu_clk);

        /* Store x extra bits for sub CLK precision */
        vicii.light_pen.x_extra_bits = (x >> 1) & 0x3;
    }

    return pulse_time;
}

/* Change the base of RAM seen by the VIC-II.  */
static inline void vicii_set_ram_bases(BYTE *base_p1, BYTE *base_p2)
{
    /* WARNING: assumes `maincpu_rmw_flag' is 0 or 1.  */
    vicii_handle_pending_alarms(maincpu_rmw_flag + 1);

    vicii.ram_base_phi1 = base_p1;
    vicii.ram_base_phi2 = base_p2;
    vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
}

void vicii_set_ram_base(BYTE *base)
{
    vicii_set_ram_bases(base, base);
}

void vicii_set_phi1_ram_base(BYTE *base)
{
    vicii_set_ram_bases(base, vicii.ram_base_phi2);
}

void vicii_set_phi2_ram_base(BYTE *base)
{
    vicii_set_ram_bases(vicii.ram_base_phi1, base);
}


void vicii_update_memory_ptrs_external(void)
{
    if (vicii.initialized > 0) {
        vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
    }
}

/* Set the memory pointers according to the values in the registers.  */
void vicii_update_memory_ptrs(unsigned int cycle)
{
    /* FIXME: This is *horrible*!  */
    static BYTE *old_screen_ptr, *old_bitmap_low_ptr, *old_bitmap_high_ptr;
    static BYTE *old_chargen_ptr;
    static int old_vbank_p1 = -1;
    static int old_vbank_p2 = -1;
    WORD screen_addr;             /* Screen start address.  */
    BYTE *char_base;              /* Pointer to character memory.  */
    BYTE *bitmap_low_base;        /* Pointer to bitmap memory (low part).  */
    BYTE *bitmap_high_base;       /* Pointer to bitmap memory (high part).  */
    int tmp, bitmap_bank;

    if (vicii.viciidtv) {
        viciidtv_update_colorram();
    }

    screen_addr = vicii.vbank_phi2 + ((vicii.regs[0x18] & 0xf0) << 6);

    screen_addr = (screen_addr & vicii.vaddr_mask_phi2)
                  | vicii.vaddr_offset_phi2;

    VICII_DEBUG_REGISTER(("Screen memory at $%04X", screen_addr));

    tmp = (vicii.regs[0x18] & 0xe) << 10;
    tmp = (tmp + vicii.vbank_phi1);
    tmp &= vicii.vaddr_mask_phi1;
    tmp |= vicii.vaddr_offset_phi1;

    bitmap_bank = tmp & 0xe000;
    bitmap_low_base = vicii.ram_base_phi1 + bitmap_bank;

    VICII_DEBUG_REGISTER(("Bitmap memory at $%04X", tmp & 0xe000));

    if (export.ultimax_phi2 != 0) {
        /* phi2 fetch from expansion port in ultimax mode */
#if 0
        if ((screen_addr & 0x3fff) >= 0x3000) {
            /* vicii.screen_base_phi2 = romh_banks + (romh_bank << 13)
                                     + (screen_addr & 0xfff) + 0x1000; */
            vicii.screen_base_phi2 = ultimax_romh_phi2_ptr((WORD)(0x1000 + (screen_addr & 0xfff)));
        } else {
            vicii.screen_base_phi2 = vicii.ram_base_phi2 + screen_addr;
        }
#endif
        BYTE *ptr;
        if ((ptr = ultimax_romh_phi2_ptr((WORD)(0x1000 + (screen_addr & 0xfff))))) {
            if ((screen_addr & 0x3fff) >= 0x3000) {
                vicii.screen_base_phi2 = ptr;
            } else {
                vicii.screen_base_phi2 = vicii.ram_base_phi2 + screen_addr;
            }
        } else {
            goto phi2noultimax;
        }
    } else {
phi2noultimax:
        if ((screen_addr & vicii.vaddr_chargen_mask_phi2)
            != vicii.vaddr_chargen_value_phi2) {
            vicii.screen_base_phi2 = vicii.ram_base_phi2 + screen_addr;
        } else {
            vicii.screen_base_phi2 = mem_chargen_rom_ptr
                                     + (screen_addr & 0xc00);
        }
    }

    if (export.ultimax_phi1 != 0) {
        /* phi1 fetch from expansion port in ultimax mode */
#if 0
        if ((screen_addr & 0x3fff) >= 0x3000) {
            /* vicii.screen_base_phi1 = romh_banks + (romh_bank << 13)
                                     + (screen_addr & 0xfff) + 0x1000; */
            vicii.screen_base_phi1 = ultimax_romh_phi1_ptr((WORD)(0x1000 + (screen_addr & 0xfff)));
        } else {
            vicii.screen_base_phi1 = vicii.ram_base_phi1 + screen_addr;
        }
#endif
        BYTE *ptr;
        if ((ptr = ultimax_romh_phi1_ptr((WORD)(0x1000 + (screen_addr & 0xfff))))) {
            if ((screen_addr & 0x3fff) >= 0x3000) {
                vicii.screen_base_phi1 = ptr;
            } else {
                vicii.screen_base_phi1 = vicii.ram_base_phi1 + screen_addr;
            }
        } else {
            goto phi1noultimax;
        }
        if ((tmp & 0x3fff) >= 0x3000) {
            /* char_base = romh_banks + (romh_bank << 13) + (tmp & 0xfff) + 0x1000; */
            char_base = ultimax_romh_phi1_ptr((WORD)(0x1000 + (tmp & 0xfff)));
        } else {
            char_base = vicii.ram_base_phi1 + tmp;
        }

        if (((bitmap_bank + 0x1000) & 0x3fff) >= 0x3000) {
            /* bitmap_high_base = romh_banks + (romh_bank << 13) + 0x1000; */
            bitmap_high_base = ultimax_romh_phi1_ptr(0x1000);
        } else {
            bitmap_high_base = bitmap_low_base + 0x1000;
        }
    } else {
phi1noultimax:
        if ((screen_addr & vicii.vaddr_chargen_mask_phi1) != vicii.vaddr_chargen_value_phi1) {
            vicii.screen_base_phi1 = vicii.ram_base_phi1 + screen_addr;
        } else {
            vicii.screen_base_phi1 = mem_chargen_rom_ptr + (screen_addr & 0xc00);
        }

        if ((tmp & vicii.vaddr_chargen_mask_phi1) != vicii.vaddr_chargen_value_phi1) {
            char_base = vicii.ram_base_phi1 + tmp;
        } else {
            char_base = mem_chargen_rom_ptr + (tmp & 0x0800);
        }

        if (((bitmap_bank + 0x1000) & vicii.vaddr_chargen_mask_phi1) != vicii.vaddr_chargen_value_phi1) {
            bitmap_high_base = bitmap_low_base + 0x1000;
        } else {
            bitmap_high_base = mem_chargen_rom_ptr;
        }
    }

    if (vicii.viciidtv) {
        switch (vicii.video_mode) {
            /* TODO other modes */
            case VICII_8BPP_PIXEL_CELL_MODE:
            case VICII_ILLEGAL_LINEAR_MODE:
                vicii.screen_base_phi2 = vicii.ram_base_phi2 + (vicii.regs[0x45] << 16) + (vicii.regs[0x3b] << 8) + vicii.regs[0x3a];
                break;
            default:
                vicii.screen_base_phi2 += (vicii.regs[0x45] << 16);
                char_base += (vicii.regs[0x3d] << 16);
                bitmap_low_base += (vicii.regs[0x3d] << 16);
                bitmap_high_base += (vicii.regs[0x3d] << 16);
                break;
        }
    }

    tmp = VICII_RASTER_CHAR(cycle);

    if (vicii.idle_data_location != IDLE_NONE &&
        old_vbank_p2 != vicii.vbank_phi2) {
        if (vicii.idle_data_location == IDLE_39FF) {
            raster_changes_foreground_add_int(&vicii.raster,
                                              VICII_RASTER_CHAR(cycle),
                                              &vicii.idle_data,
                                              vicii.ram_base_phi2[vicii.vbank_phi2
                                                                  + 0x39ff]);
        } else {
            raster_changes_foreground_add_int(&vicii.raster,
                                              VICII_RASTER_CHAR(cycle),
                                              &vicii.idle_data,
                                              vicii.ram_base_phi2[vicii.vbank_phi2
                                                                  + 0x3fff]);
        }
    }

    if (tmp <= 0 && maincpu_clk < vicii.draw_clk) {
        old_screen_ptr = vicii.screen_ptr = vicii.screen_base_phi2;
        old_bitmap_low_ptr = vicii.bitmap_low_ptr = bitmap_low_base;
        old_bitmap_high_ptr = vicii.bitmap_high_ptr = bitmap_high_base;
        old_chargen_ptr = vicii.chargen_ptr = char_base;
        old_vbank_p1 = vicii.vbank_phi1;
        old_vbank_p2 = vicii.vbank_phi2;
        /* vicii.vbank_ptr = vicii.ram_base + vicii.vbank; */
    } else if (tmp < VICII_SCREEN_TEXTCOLS) {
        if (vicii.screen_base_phi2 != old_screen_ptr) {
            raster_changes_foreground_add_ptr(&vicii.raster, tmp,
                                              (void *)&vicii.screen_ptr,
                                              (void *)vicii.screen_base_phi2);
            old_screen_ptr = vicii.screen_base_phi2;
        }

        if (bitmap_low_base != old_bitmap_low_ptr) {
            raster_changes_foreground_add_ptr(&vicii.raster,
                                              tmp,
                                              (void *)&vicii.bitmap_low_ptr,
                                              (void *)(bitmap_low_base));
            old_bitmap_low_ptr = bitmap_low_base;
        }

        if (bitmap_high_base != old_bitmap_high_ptr) {
            raster_changes_foreground_add_ptr(&vicii.raster,
                                              tmp,
                                              (void *)&vicii.bitmap_high_ptr,
                                              (void *)(bitmap_high_base));
            old_bitmap_high_ptr = bitmap_high_base;
        }

        if (char_base != old_chargen_ptr) {
            raster_changes_foreground_add_ptr(&vicii.raster,
                                              tmp,
                                              (void *)&vicii.chargen_ptr,
                                              (void *)char_base);
            old_chargen_ptr = char_base;
        }

        if (vicii.vbank_phi1 != old_vbank_p1) {
            old_vbank_p1 = vicii.vbank_phi1;
        }

        if (vicii.vbank_phi2 != old_vbank_p2) {
            old_vbank_p2 = vicii.vbank_phi2;
        }
    } else {
        if (vicii.screen_base_phi2 != old_screen_ptr) {
            raster_changes_next_line_add_ptr(&vicii.raster,
                                             (void *)&vicii.screen_ptr,
                                             (void *)vicii.screen_base_phi2);
            old_screen_ptr = vicii.screen_base_phi2;
        }

        if (bitmap_low_base != old_bitmap_low_ptr) {
            raster_changes_next_line_add_ptr(&vicii.raster,
                                             (void *)&vicii.bitmap_low_ptr,
                                             (void *)(bitmap_low_base));
            old_bitmap_low_ptr = bitmap_low_base;
        }

        if (bitmap_high_base != old_bitmap_high_ptr) {
            raster_changes_next_line_add_ptr(&vicii.raster,
                                             (void *)&vicii.bitmap_high_ptr,
                                             (void *)(bitmap_high_base));
            old_bitmap_high_ptr = bitmap_high_base;
        }

        if (char_base != old_chargen_ptr) {
            raster_changes_next_line_add_ptr(&vicii.raster,
                                             (void *)&vicii.chargen_ptr,
                                             (void *)char_base);
            old_chargen_ptr = char_base;
        }

        if (vicii.vbank_phi1 != old_vbank_p1) {
            old_vbank_p1 = vicii.vbank_phi1;
        }

        if (vicii.vbank_phi2 != old_vbank_p2) {
            old_vbank_p2 = vicii.vbank_phi2;
        }
    }
}

/* Set the video mode according to the values in registers $D011 and $D016 of
   the VIC-II chip.  */
void vicii_update_video_mode(unsigned int cycle)
{
    int new_video_mode;

    new_video_mode = ((vicii.regs[0x11] & 0x60) | (vicii.regs[0x16] & 0x10)) >> 4;

    if (vicii.viciidtv) {
        new_video_mode |= (((vicii.regs[0x3c] & 0x04) << 1) | ((vicii.regs[0x3c] & 0x01) << 3));

        if (((new_video_mode) == VICII_8BPP_FRED_MODE) && ((vicii.regs[0x3c] & 0x04) == 0)) {
            new_video_mode = VICII_8BPP_FRED2_MODE;
        }

        if (((new_video_mode) == VICII_8BPP_CHUNKY_MODE) && ((vicii.regs[0x3c] & 0x10) == 0)) {
            if (vicii.regs[0x3c] & 0x04) {
                new_video_mode = VICII_8BPP_PIXEL_CELL_MODE;
            } else {
                new_video_mode = VICII_ILLEGAL_LINEAR_MODE;
            }
        }

        /* HACK to make vcache display gfx in chunky & the rest */
        if ((new_video_mode >= VICII_8BPP_CHUNKY_MODE) && (new_video_mode <= VICII_8BPP_PIXEL_CELL_MODE)) {
            vicii.raster.dont_cache = 1;
        }

        viciidtv_update_colorram();
    }

    if (new_video_mode != vicii.video_mode) {
        switch (new_video_mode) {
            case VICII_ILLEGAL_TEXT_MODE:
            case VICII_ILLEGAL_BITMAP_MODE_1:
            case VICII_ILLEGAL_BITMAP_MODE_2:
            case VICII_ILLEGAL_LINEAR_MODE:
                /* Force the overscan color to black.  */
                raster_changes_background_add_int
                    (&vicii.raster, VICII_RASTER_X(cycle),
                    &vicii.raster.idle_background_color, 0);
                raster_changes_background_add_int
                    (&vicii.raster,
                    VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                    &vicii.raster.xsmooth_color, 0);
                vicii.get_background_from_vbuf = 0;
                vicii.force_black_overscan_background_color = 1;
                break;
            case VICII_HIRES_BITMAP_MODE:
                raster_changes_background_add_int
                    (&vicii.raster, VICII_RASTER_X(cycle),
                    &vicii.raster.idle_background_color, 0);
                raster_changes_background_add_int
                    (&vicii.raster,
                    VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                    &vicii.raster.xsmooth_color,
                    vicii.background_color_source & 0x0f);
                vicii.get_background_from_vbuf = VICII_HIRES_BITMAP_MODE;
                vicii.force_black_overscan_background_color = 1;
                break;
            case VICII_EXTENDED_TEXT_MODE:
                raster_changes_background_add_int
                    (&vicii.raster, VICII_RASTER_X(cycle),
                    &vicii.raster.idle_background_color,
                    vicii.viciidtv ? vicii.dtvpalette[vicii.regs[0x21]] : vicii.regs[0x21]);
                raster_changes_background_add_int
                    (&vicii.raster,
                    VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                    &vicii.raster.xsmooth_color,
                    vicii.viciidtv ? vicii.dtvpalette[vicii.regs[0x21 + (vicii.background_color_source >> 6)]] : vicii.regs[0x21 + (vicii.background_color_source >> 6)]);
                vicii.get_background_from_vbuf = VICII_EXTENDED_TEXT_MODE;
                vicii.force_black_overscan_background_color = 0;
                break;
            default:
                /* The overscan background color is given by the background
                   color register.  */
                raster_changes_background_add_int
                    (&vicii.raster, VICII_RASTER_X(cycle),
                    &vicii.raster.idle_background_color,
                    vicii.viciidtv ? vicii.dtvpalette[vicii.regs[0x21]] : vicii.regs[0x21]);
                raster_changes_background_add_int
                    (&vicii.raster,
                    VICII_RASTER_X(VICII_RASTER_CYCLE(maincpu_clk)),
                    &vicii.raster.xsmooth_color,
                    vicii.viciidtv ? vicii.dtvpalette[vicii.regs[0x21]] : vicii.regs[0x21]);
                vicii.get_background_from_vbuf = 0;
                vicii.force_black_overscan_background_color = 0;
                break;
        }

        {
            int pos;

            pos = VICII_RASTER_CHAR(cycle) - 1;

            raster_changes_background_add_int(&vicii.raster,
                                              VICII_RASTER_X(cycle),
                                              &vicii.raster.video_mode,
                                              new_video_mode);

            raster_changes_foreground_add_int(&vicii.raster, pos,
                                              &vicii.raster.last_video_mode,
                                              vicii.video_mode);

            raster_changes_foreground_add_int(&vicii.raster, pos,
                                              &vicii.raster.video_mode,
                                              new_video_mode);

            if (vicii.idle_data_location != IDLE_NONE) {
                if (vicii.regs[0x11] & 0x40) {
                    raster_changes_foreground_add_int
                        (&vicii.raster, pos + 1, (void *)&vicii.idle_data,
                        vicii.ram_base_phi2[vicii.vbank_phi2 + 0x39ff]);
                } else {
                    raster_changes_foreground_add_int
                        (&vicii.raster, pos + 1, (void *)&vicii.idle_data,
                        vicii.ram_base_phi2[vicii.vbank_phi2 + 0x3fff]);
                }
            }

            raster_changes_foreground_add_int(&vicii.raster, pos + 2,
                                              &vicii.raster.last_video_mode,
                                              -1);
        }

        vicii.video_mode = new_video_mode;
    }

#ifdef VICII_VMODE_DEBUG
    switch (new_video_mode) {
        case VICII_NORMAL_TEXT_MODE:
            VICII_DEBUG_VMODE(("Standard Text"));
            break;
        case VICII_MULTICOLOR_TEXT_MODE:
            VICII_DEBUG_VMODE(("Multicolor Text"));
            break;
        case VICII_HIRES_BITMAP_MODE:
            VICII_DEBUG_VMODE(("Hires Bitmap"));
            break;
        case VICII_MULTICOLOR_BITMAP_MODE:
            VICII_DEBUG_VMODE(("Multicolor Bitmap"));
            break;
        case VICII_EXTENDED_TEXT_MODE:
            VICII_DEBUG_VMODE(("Extended Text"));
            break;
        case VICII_ILLEGAL_TEXT_MODE:
            VICII_DEBUG_VMODE(("Illegal Text"));
            break;
        case VICII_ILLEGAL_BITMAP_MODE_1:
            VICII_DEBUG_VMODE(("Invalid Bitmap"));
            break;
        case VICII_ILLEGAL_BITMAP_MODE_2:
            VICII_DEBUG_VMODE(("Invalid Bitmap"));
            break;
        case VICII_8BPP_NORMAL_TEXT_MODE:
            VICII_DEBUG_VMODE(("8BPP Standard Text"));
            break;
        case VICII_8BPP_MULTICOLOR_TEXT_MODE:
            VICII_DEBUG_VMODE(("8BPP Multicolor Text"));
            break;
        case VICII_8BPP_HIRES_BITMAP_MODE:
            VICII_DEBUG_VMODE(("8BPP Hires Bitmap (?)"));
            break;
        case VICII_8BPP_MULTICOLOR_BITMAP_MODE:
            VICII_DEBUG_VMODE(("8BPP Multicolor Bitmap"));
            break;
        case VICII_8BPP_EXTENDED_TEXT_MODE:
            VICII_DEBUG_VMODE(("8BPP Extended Text"));
            break;
        case VICII_8BPP_CHUNKY_MODE:
            VICII_DEBUG_VMODE(("Chunky mode"));
            break;
        case VICII_8BPP_TWO_PLANE_BITMAP_MODE:
            VICII_DEBUG_VMODE(("Two plane bitmap"));
            break;
        case VICII_8BPP_FRED_MODE:
            VICII_DEBUG_VMODE(("FRED"));
            break;
        case VICII_8BPP_FRED2_MODE:
            VICII_DEBUG_VMODE(("FRED2"));
            break;
        case VICII_8BPP_PIXEL_CELL_MODE:
            VICII_DEBUG_VMODE(("8BPP Pixel Cell"));
            break;
        case VICII_ILLEGAL_LINEAR_MODE:
            VICII_DEBUG_VMODE(("Illegal Linear"));
            break;
        default:                  /* cannot happen */
            VICII_DEBUG_VMODE(("???"));
    }

    VICII_DEBUG_VMODE(("Mode enabled at line $%04X, cycle %d.",
                       VICII_RASTER_Y(maincpu_clk), cycle));
#endif
}

/* Redraw the current raster line.  This happens at cycle VICII_DRAW_CYCLE
   of each line.  */
void vicii_raster_draw_alarm_handler(CLOCK offset, void *data)
{
    BYTE prev_sprite_sprite_collisions;
    BYTE prev_sprite_background_collisions;
    int in_visible_area;

    prev_sprite_sprite_collisions = vicii.sprite_sprite_collisions;
    prev_sprite_background_collisions = vicii.sprite_background_collisions;

    /* if the current line is between first and last displayed line, it is visible */
    /* additionally we must make sure not to skip lines within the range of active
       DMA, or certain effects will break in "no border" mode (see bug #3601657) */
    in_visible_area = (vicii.raster.current_line
                       >= ((vicii.first_dma_line < (unsigned int)vicii.first_displayed_line) ? vicii.first_dma_line : vicii.first_displayed_line))
                       && (vicii.raster.current_line
                       <= (((vicii.last_dma_line + 7) > (unsigned int)vicii.last_displayed_line) ? (vicii.last_dma_line + 7) : vicii.last_displayed_line));

    /* handle wrap if the first few lines are displayed in the visible lower border */
    if ((unsigned int)vicii.last_displayed_line >= vicii.screen_height) {
        in_visible_area |= vicii.raster.current_line
                           <= ((unsigned int)vicii.last_displayed_line - vicii.screen_height);
    }

    vicii.raster.xsmooth_shift_left = 0;

    vicii_sprites_reset_xshift();

    raster_line_emulate(&vicii.raster);

#if 0
    if (vicii.raster.current_line >= 60 && vicii.raster.current_line <= 60) {
        char buf[1000];
        int j, i;
        for (i = 0; i < 8; i++) {
            memset(buf, 0, sizeof(buf));
            for (j = 0; j < 40; j++) {
                sprintf(&buf[strlen(buf)], "%02x",
                        vicii.raster.draw_buffer_ptr[vicii.raster.xsmooth
                                                     + vicii.raster.geometry->gfx_position.x + i * 40 + j]);
            }
            log_debug(buf);
        }
    }
#endif

    if (vicii.raster.current_line == 0) {
        /* no vsync here for NTSC  */
        if ((unsigned int)vicii.last_displayed_line < vicii.screen_height) {
            raster_skip_frame(&vicii.raster,
                              vsync_do_vsync(vicii.raster.canvas,
                                             vicii.raster.skip_frame));
        }
        vicii.memptr = 0;
        vicii.mem_counter = 0;
        vicii.light_pen.triggered = 0;

        if (vicii.light_pen.state) {
            vicii_trigger_light_pen(maincpu_clk);
        }

        vicii.raster.blank_off = 0;

        if (vicii.viciidtv) {
            memset(vicii.cbuf, 0, sizeof(vicii.cbuf));

            /* Scheduled Blitter */
            if (blitter_on_irq & 0x40) {
                c64dtvblitter_trigger_blitter();
            }
            /* Scheduled DMA */
            if (dma_on_irq & 0x40) {
                c64dtvdma_trigger_dma();
            }

            /* HACK to make vcache display gfx in chunky & the rest */
            if ((vicii.video_mode >= VICII_8BPP_CHUNKY_MODE) &&
                (vicii.video_mode <= VICII_8BPP_PIXEL_CELL_MODE)) {
                vicii.raster.dont_cache = 1;
            }

            /* HACK to fix greetings in 2008 */
            if (vicii.video_mode == VICII_8BPP_PIXEL_CELL_MODE) {
                vicii_update_memory_ptrs(VICII_RASTER_CYCLE(maincpu_clk));
            }
        }

#ifdef __MSDOS__
        if ((unsigned int)vicii.last_displayed_line < vicii.screen_height) {
            if (vicii.raster.canvas->draw_buffer->canvas_width
                <= VICII_SCREEN_XPIX
                && vicii.raster.canvas->draw_buffer->canvas_height
                <= VICII_SCREEN_YPIX
                && vicii.raster.canvas->viewport->update_canvas) {
                canvas_set_border_color(vicii.raster.canvas,
                                        vicii.raster.border_color);
            }
        }
#endif
    }

    /* vsync for NTSC */
    if ((unsigned int)vicii.last_displayed_line >= vicii.screen_height
        && vicii.raster.current_line == vicii.last_displayed_line - vicii.screen_height + 1) {
        raster_skip_frame(&vicii.raster,
                          vsync_do_vsync(vicii.raster.canvas,
                                         vicii.raster.skip_frame));
#ifdef __MSDOS__
        if (vicii.raster.canvas->draw_buffer->canvas_width
            <= VICII_SCREEN_XPIX
            && vicii.raster.canvas->draw_buffer->canvas_height
            <= VICII_SCREEN_YPIX
            && vicii.raster.canvas->viewport->update_canvas) {
            canvas_set_border_color(vicii.raster.canvas,
                                    vicii.raster.border_color);
        }
#endif
    }

    if (vicii.viciidtv) {
        if ((!vicii.overscan && vicii.raster.current_line == 48) || (vicii.overscan && vicii.raster.current_line == 10)) {
            vicii.counta = vicii.regs[0x3a] + (vicii.regs[0x3b] << 8) + (vicii.regs[0x45] << 16);

            vicii.countb = vicii.regs[0x49] + (vicii.regs[0x4a] << 8) + (vicii.regs[0x4b] << 16);
        }
    }

    if (in_visible_area) {
        if (!vicii.idle_state) {
            vicii.mem_counter = (vicii.mem_counter + vicii.mem_counter_inc) & 0x3ff;
        }
        vicii.mem_counter_inc = VICII_SCREEN_TEXTCOLS;

        if (vicii.viciidtv && !vicii.idle_state) {
            /* TODO should be done in cycle 57 */
            if (!(VICII_MODULO_BUG(vicii.video_mode) && (vicii.raster.ycounter == 7))) {
                vicii.counta += vicii.counta_mod;
                vicii.countb += vicii.countb_mod;
            }

            /* TODO hack */
            if (!vicii.overscan) {
                vicii.counta += vicii.counta_step * 40;
                vicii.countb += vicii.countb_step * 40;
            } else {
                /* faked overscan */
                vicii.counta += vicii.counta_step * 48;
                vicii.countb += vicii.countb_step * 48;
            }

            /* HACK to fix greetings in 2008 */
            if ((vicii.video_mode == VICII_8BPP_PIXEL_CELL_MODE) && (vicii.raster.ycounter == 7)) {
                vicii.screen_base_phi2 += vicii.counta_mod;
            }
        }

        /* `ycounter' makes the chip go to idle state when it reaches the
           maximum value.  */
        if (vicii.raster.ycounter == 7) {
            vicii.idle_state = 1;
            vicii.memptr = vicii.mem_counter;
        }
        if (!vicii.idle_state || vicii.bad_line) {
            vicii.raster.ycounter = (vicii.raster.ycounter + 1) & 0x7;
            vicii.idle_state = 0;
        }
        if (vicii.force_display_state) {
            vicii.idle_state = 0;
            vicii.force_display_state = 0;
        }
        vicii.raster.draw_idle_state = vicii.idle_state;
        vicii.bad_line = 0;
    }

    vicii.ycounter_reset_checked = 0;
    vicii.memory_fetch_done = 0;
    vicii.buf_offset = 0;

    if (vicii.raster.current_line == vicii.first_dma_line) {
        vicii.allow_bad_lines = !vicii.raster.blank;
    }

    /* As explained in Christian's article, only the first collision
       (i.e. the first time the collision register becomes non-zero) actually
       triggers an interrupt.  */
    if (vicii_resources.sprite_sprite_collisions_enabled
        && vicii.raster.sprite_status->sprite_sprite_collisions != 0
        && !prev_sprite_sprite_collisions) {
        vicii_irq_sscoll_set();
    }

    if (vicii_resources.sprite_background_collisions_enabled
        && vicii.raster.sprite_status->sprite_background_collisions
        && !prev_sprite_background_collisions) {
        vicii_irq_sbcoll_set();
    }

    if (vicii.idle_state) {
        if (vicii.regs[0x11] & 0x40) {
            vicii.idle_data_location = IDLE_39FF;
            vicii.idle_data = vicii.ram_base_phi2[vicii.vbank_phi2 + 0x39ff];
        } else {
            vicii.idle_data_location = IDLE_3FFF;
            vicii.idle_data = vicii.ram_base_phi2[vicii.vbank_phi2 + 0x3fff];
        }
    } else {
        vicii.idle_data_location = IDLE_NONE;
    }

    /* Set the next draw event.  */
    vicii.last_emulate_line_clk += vicii.cycles_per_line;
    vicii.draw_clk = vicii.last_emulate_line_clk + vicii.draw_cycle;
    alarm_set(vicii.raster_draw_alarm, vicii.draw_clk);
}

void vicii_set_canvas_refresh(int enable)
{
    raster_set_canvas_refresh(&vicii.raster, enable);
}

void vicii_shutdown(void)
{
    lib_free(vicii.idle_3fff);
    lib_free(vicii.idle_3fff_old);
    vicii_sprites_shutdown();
    raster_sprite_status_destroy(&vicii.raster);
    raster_shutdown(&vicii.raster);
}

void vicii_screenshot(screenshot_t *screenshot)
{
    raster_screenshot(&vicii.raster, screenshot);
    screenshot->chipid = "VICII";
    screenshot->video_regs = vicii.regs;
    screenshot->screen_ptr = vicii.screen_base_phi2;
    screenshot->chargen_ptr = vicii.chargen_ptr;
    screenshot->bitmap_ptr = NULL;
    screenshot->bitmap_low_ptr = vicii.bitmap_low_ptr;
    screenshot->bitmap_high_ptr = vicii.bitmap_high_ptr;
    screenshot->color_ram_ptr = mem_color_ram_vicii;
}

void vicii_async_refresh(struct canvas_refresh_s *refresh)
{
    raster_async_refresh(&vicii.raster, refresh);
}

int vicii_dump(void)
{
    int m_muco, m_disp, m_ext, v_bank, v_vram;
    int i, bits;

    m_ext = ((vicii.regs[0x11] & 0x40) >> 6); /* 0 standard, 1 extended */
    m_muco = ((vicii.regs[0x16] & 0x10) >> 4); /* 0 hires, 1 multi */
    m_disp = ((vicii.regs[0x11] & 0x20) >> 5); /* 0 text, 1 bitmap */

    v_bank = vicii.vbank_phi2;

    mon_out("Rasterline:   current: %d IRQ: %d\n", vicii.raster.current_line, vicii.raster_irq_line);
    mon_out("Display Mode:");
    mon_out(m_ext ? " Extended" : " Standard");
    mon_out(m_muco ? " Multi Color" : " Hires");
    mon_out(m_disp ? " Bitmap" : " Text");
    mon_out("\nColors:       Border: %2d Background: %2d\n", vicii.regs[0x20], vicii.regs[0x21]);
    if (m_ext) {
        mon_out("              BGCol1: %2d BGCol2: %2d BGCol3: %2d\n", vicii.regs[0x22], vicii.regs[0x23], vicii.regs[0x24]);
    } else if (m_muco && !m_disp) {
        mon_out("              MuCol1: %2d MuCol2: %2d\n", vicii.regs[0x22], vicii.regs[0x23]);
    }
    mon_out("Scroll X/Y:   %d/%d\n", vicii.regs[0x16] & 0x07, vicii.regs[0x11] & 0x07);
    mon_out("Screen Size:  %d x %d\n", 39 + ((vicii.regs[0x16] >> 3) & 1), 24 + ((vicii.regs[0x11] >> 3) & 1));

    mon_out("\nVIC Memory Bank:   $%04x - $%04x\n", v_bank, v_bank + 0x3fff);
    v_vram = ((vicii.regs[0x18] >> 4) * 0x0400) + v_bank;
    mon_out("\nVideo Memory:      $%04x\n", v_vram);
    if (m_disp) {
        mon_out("Bitmap Memory:     $%04x\n", (((vicii.regs[0x18] >> 3) & 1) * 0x2000) + v_bank);
    } else {
        i = (((vicii.regs[0x18] >> 1) & 0x7) * 0x800) + v_bank;
        /* FIXME: how does cbm510 work ? */
        if (machine_class == VICE_MACHINE_C64 ||
            machine_class == VICE_MACHINE_C128 ||
            machine_class == VICE_MACHINE_C64DTV ||
            machine_class == VICE_MACHINE_C64SC ||
            machine_class == VICE_MACHINE_SCPU64) {
            /* $1x00 and $9x00 mapped to $dx00 */
            if ((( i >> 12) == 1 ) || (( i >> 12) == 9 )) {
                i = 0xd000 | (i & 0x0f00);
            }
        }
        mon_out("Character Set:     $%04x\n", i);
    }

    mon_out("\nSprites:");
    mon_out("\n           Spr.0  Spr.1  Spr.2  Spr.3  Spr.4  Spr.5  Spr.6  Spr.7");
    mon_out("\nEnabled: ");
    bits = vicii.regs[0x15];
    for (i = 0; i < 8; i++) {
        mon_out("  %5s", (bits & 1) ? "yes" : "no");
        bits >>= 1;
    }
    mon_out("\nPointer: ");
    for (i = 0x3f8; i < 0x400; i++) {
        mon_out("    $%02x", vicii.screen_ptr[i]);
    }
    mon_out("\nAddress: ");
    for (i = 0x3f8; i < 0x400; i++) {
        mon_out("  $%04x", v_bank + (vicii.screen_ptr[i] * 0x40));
    }
    mon_out("\nX-Pos:   ");
    bits = vicii.regs[0x10]; /* sprite x msb */
    for (i = 0; i < 8; i++) {
        mon_out("  %5d", vicii.regs[i << 1] + (256 * (bits & 1)));
        bits >>= 1;
    }
    mon_out("\nY-Pos:   ");
    for (i = 0; i < 8; i++) {
        mon_out("  %5d", vicii.regs[1 + (i << 1)]);
    }
    mon_out("\nX-Expand:");
    bits = vicii.regs[0x1d];
    for (i = 0; i < 8; i++) {
        mon_out("  %5s", (bits & 1) ? "yes" : "no");
        bits >>= 1;
    }
    mon_out("\nY-Expand:");
    bits = vicii.regs[0x17];
    for (i = 0; i < 8; i++) {
        mon_out("  %5s", (bits & 1) ? "yes" : "no");
        bits >>= 1;
    }
    mon_out("\nPriority:");
    bits = vicii.regs[0x1b];
    for (i = 0; i < 8; i++) {
        mon_out("  %5s", (bits & 1) ? "bg" : "spr");
        bits >>= 1;
    }
    mon_out("\nMode:    ");
    bits = vicii.regs[0x1c];
    for (i = 0; i < 8; i++) {
        mon_out("  %5s", (bits & 1) ? "muco" : "std");
        bits >>= 1;
    }
    mon_out("\nColor:   ");
    for (i = 0; i < 8; i++) {
        mon_out("  %5d", vicii.regs[i + 0x27]);
    }
    if (vicii.regs[0x1c]) {
        mon_out("\nMulti Color 1: %d  Multi Color 2: %d", vicii.regs[0x25], vicii.regs[0x26]);
    }
    mon_out("\n");

/*
  TODO:

  Current Scanline: 11
  Raster IRQ Scanline: 311
  Enabled Interrupts:  None
  Pending Interrupts:  Raster
*/
    return 0;
}
