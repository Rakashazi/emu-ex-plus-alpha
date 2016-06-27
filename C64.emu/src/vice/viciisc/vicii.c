/*
 * vicii.c - A cycle-exact event-driven MOS6569 (VIC-II) emulation.
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
#include <stdlib.h>
#include <string.h>

#include "c64cart.h"
#include "c64cartmem.h"
#include "clkguard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"
#include "vicii-chip-model.h"
#include "vicii-cmdline-options.h"
#include "vicii-color.h"
#include "vicii-draw.h"
#include "vicii-draw-cycle.h"
#include "vicii-fetch.h"
#include "vicii-irq.h"
#include "vicii-mem.h"
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
}

void vicii_set_phi2_addr_options(WORD mask, WORD offset)
{
    vicii.vaddr_mask_phi2 = mask;
    vicii.vaddr_offset_phi2 = offset;

    VICII_DEBUG_REGISTER(("Set phi2 video addr mask=%04x, offset=%04x", mask, offset));
}

void vicii_set_phi1_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi1 = mask;
    vicii.vaddr_chargen_value_phi1 = value;

    VICII_DEBUG_REGISTER(("Set phi1 chargen addr mask=%04x, value=%04x", mask, value));
}

void vicii_set_phi2_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi2 = mask;
    vicii.vaddr_chargen_value_phi2 = value;

    VICII_DEBUG_REGISTER(("Set phi2 chargen addr mask=%04x, value=%04x", mask, value));
}

void vicii_set_chargen_addr_options(WORD mask, WORD value)
{
    vicii.vaddr_chargen_mask_phi1 = mask;
    vicii.vaddr_chargen_value_phi1 = value;
    vicii.vaddr_chargen_mask_phi2 = mask;
    vicii.vaddr_chargen_value_phi2 = value;

    VICII_DEBUG_REGISTER(("Set chargen addr mask=%04x, value=%04x", mask, value));
}

/* ---------------------------------------------------------------------*/

vicii_t vicii;

static void vicii_set_geometry(void);

static void clk_overflow_callback(CLOCK sub, void *unused_data)
{
    if (vicii.light_pen.trigger_cycle < CLOCK_MAX) {
        vicii.light_pen.trigger_cycle -= sub;
    }
}

void vicii_change_timing(machine_timing_t *machine_timing, int border_mode)
{
    vicii_timing_set(machine_timing, border_mode);

    if (vicii.initialized) {
        vicii_set_geometry();
        raster_mode_change();
    }
}

void vicii_handle_pending_alarms_external(int num_write_cycles)
{
    return;
}

void vicii_handle_pending_alarms_external_write(void)
{
    return;
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
                        vicii.screen_leftborderwidth, VICII_NO_BORDER_FIRST_DISPLAYED_LINE, /* gfx position */
                        0, /* gfx area doesn't move */
                        vicii.first_displayed_line,
                        vicii.last_displayed_line,
                        0, /* extra offscreen border left */
                        0) /* extra offscreen border right */;

#ifdef __MSDOS__
    video_ack_vga_mode();
#endif

    vicii.raster.display_ystart = 0;
    vicii.raster.display_ystop = vicii.screen_height;
    vicii.raster.display_xstart = 0;
    vicii.raster.display_xstop = width;
    vicii.raster.dont_cache_all = 1;

    vicii.raster.geometry->pixel_aspect_ratio = vicii_get_pixel_aspect();
    vicii.raster.viewport->crt_type = vicii_get_crt_type();
}

static int init_raster(void)
{
    raster_t *raster;

    raster = &vicii.raster;

    raster->sprite_status = NULL;
    raster_line_changes_init(raster);

    /* We only use the dummy mode for "drawing" to raster.
       Report only 1 video mode and set the idle mode to it. */
    if (raster_init(raster, 1) < 0) {
        return -1;
    }
    raster_modes_set_idle_mode(raster->modes, VICII_DUMMY_MODE);

    resources_touch("VICIIVideoCache");

    vicii_set_geometry();

    if (vicii_color_update_palette(raster->canvas) < 0) {
        log_error(vicii.log, "Cannot load palette.");
        return -1;
    }

#if 0
    raster_set_title(raster, machine_name);
#else
    /* hack */
    if (machine_class != VICE_MACHINE_C64SC) {
        raster_set_title(raster, machine_name);
    } else {
        raster_set_title(raster, "C64 (x64sc)");
    }
#endif

    if (raster_realize(raster) < 0) {
        return -1;
    }

    return 0;
}

static void vicii_new_sprites_init(void)
{
    int i;

    for (i = 0; i < VICII_NUM_SPRITES; i++) {
        vicii.sprite[i].data = 0;
        vicii.sprite[i].mc = 0;
        vicii.sprite[i].mcbase = 0;
        vicii.sprite[i].pointer = 0;
        vicii.sprite[i].exp_flop = 1;
        vicii.sprite[i].x = 0;
    }

    vicii.sprite_display_bits = 0;
    vicii.sprite_dma = 0;
}

/* Initialize the VIC-II emulation.  */
raster_t *vicii_init(unsigned int flag)
{
    if (flag != VICII_STANDARD) {
        return NULL;
    }

    vicii.log = log_open("VIC-II");

    vicii_chip_model_init();

    vicii_irq_init();

    if (init_raster() < 0) {
        return NULL;
    }

    vicii_powerup();

    vicii_draw_init();
    vicii_draw_cycle_init();
    vicii_new_sprites_init();

    vicii.vmli = 0;

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
    raster_reset(&vicii.raster);

    vicii.raster_line = 0;
    vicii.raster_cycle = 6;
    /* this should probably be updated through some function */
    vicii.cycle_flags = 0;
    vicii.start_of_frame = 0;
    vicii.raster_irq_triggered = 0;

    vicii.light_pen.state = 0;
    vicii.light_pen.triggered = 0;
    vicii.light_pen.x = vicii.light_pen.y = vicii.light_pen.x_extra_bits = 0;
    vicii.light_pen.trigger_cycle = CLOCK_MAX;

    /* Remove all the IRQ sources.  */
    vicii.regs[0x1a] = 0;

    vicii.vborder = 1;
    vicii.set_vborder = 1;
    vicii.main_border = 1;
}

void vicii_reset_registers(void)
{
    WORD i;

    if (!vicii.initialized) {
        return;
    }

    for (i = 0; i <= 0x3f; i++) {
        vicii_store(i, 0);
    }
}

/* This /should/ put the VIC-II in the same state as after a powerup, if
   `reset_vicii()' is called afterwards.  But FIXME, as we are not really
   emulating everything correctly here; just $D011.  */
void vicii_powerup(void)
{
    memset(vicii.regs, 0, sizeof(vicii.regs));

    vicii.irq_status = 0;
    vicii.raster_irq_line = 0;
    vicii.ram_base_phi1 = mem_ram;
    vicii.ram_base_phi2 = mem_ram;

    vicii.vaddr_mask_phi1 = 0xffff;
    vicii.vaddr_mask_phi2 = 0xffff;
    vicii.vaddr_offset_phi1 = 0;
    vicii.vaddr_offset_phi2 = 0;

    vicii.allow_bad_lines = 0;
    vicii.sprite_sprite_collisions = vicii.sprite_background_collisions = 0;
    vicii.clear_collisions = 0x00;
    vicii.idle_state = 0;
    vicii.vcbase = 0;
    vicii.vc = 0;
    vicii.bad_line = 0;
    vicii.light_pen.state = 0;
    vicii.light_pen.x = vicii.light_pen.y = vicii.light_pen.x_extra_bits = vicii.light_pen.triggered = 0;
    vicii.light_pen.trigger_cycle = CLOCK_MAX;
    vicii.vbank_phi1 = 0;
    vicii.vbank_phi2 = 0;

    vicii_reset();

    vicii.ysmooth = 0;
}

/* ---------------------------------------------------------------------*/

/* This hook is called whenever video bank must be changed.  */
static inline void vicii_set_vbanks(int vbank_p1, int vbank_p2)
{
    vicii.vbank_phi1 = vbank_p1;
    vicii.vbank_phi2 = vbank_p2;
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

/* Change the base of RAM seen by the VIC-II.  */
static inline void vicii_set_ram_bases(BYTE *base_p1, BYTE *base_p2)
{
    vicii.ram_base_phi1 = base_p1;
    vicii.ram_base_phi2 = base_p2;
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
}

/* Redraw the current raster line.  This happens after the last cycle
   of each line.  */
void vicii_raster_draw_handler(void)
{
    int in_visible_area;

    in_visible_area = (vicii.raster.current_line
                       >= (unsigned int)vicii.first_displayed_line
                       && vicii.raster.current_line
                       <= (unsigned int)vicii.last_displayed_line);

    /* handle wrap if the first few lines are displayed in the visible lower border */
    if ((unsigned int)vicii.last_displayed_line >= vicii.screen_height) {
        in_visible_area |= vicii.raster.current_line
                           <= ((unsigned int)vicii.last_displayed_line - vicii.screen_height);
    }

    raster_line_emulate(&vicii.raster);

    if (vicii.raster.current_line == 0) {
        /* no vsync here for NTSC  */
        if ((unsigned int)vicii.last_displayed_line < vicii.screen_height) {
            raster_skip_frame(&vicii.raster,
                              vsync_do_vsync(vicii.raster.canvas,
                                             vicii.raster.skip_frame));
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
}

void vicii_set_canvas_refresh(int enable)
{
    raster_set_canvas_refresh(&vicii.raster, enable);
}

void vicii_shutdown(void)
{
    raster_shutdown(&vicii.raster);
}

void vicii_screenshot(screenshot_t *screenshot)
{
    WORD screen_addr;             /* Screen start address.  */
    BYTE *screen_base_phi2;       /* Pointer to screen memory.  */
    BYTE *char_base;              /* Pointer to character memory.  */
    BYTE *bitmap_low_base;        /* Pointer to bitmap memory (low part).  */
    BYTE *bitmap_high_base;       /* Pointer to bitmap memory (high part).  */
    int tmp, bitmap_bank;

    screen_addr = vicii.vbank_phi2 + ((vicii.regs[0x18] & 0xf0) << 6);

    screen_addr = (screen_addr & vicii.vaddr_mask_phi2)
                  | vicii.vaddr_offset_phi2;

    tmp = (vicii.regs[0x18] & 0xe) << 10;
    tmp = (tmp + vicii.vbank_phi1);
    tmp &= vicii.vaddr_mask_phi1;
    tmp |= vicii.vaddr_offset_phi1;

    bitmap_bank = tmp & 0xe000;
    bitmap_low_base = vicii.ram_base_phi1 + bitmap_bank;

    if (export.ultimax_phi2) {
        if ((screen_addr & 0x3fff) >= 0x3000) {
            screen_base_phi2 = ultimax_romh_phi2_ptr((WORD)(0x1000 + (screen_addr & 0xfff)));
        } else {
            screen_base_phi2 = vicii.ram_base_phi2 + screen_addr;
        }
    } else {
        if ((screen_addr & vicii.vaddr_chargen_mask_phi2)
            != vicii.vaddr_chargen_value_phi2) {
            screen_base_phi2 = vicii.ram_base_phi2 + screen_addr;
        } else {
            screen_base_phi2 = mem_chargen_rom_ptr + (screen_addr & 0xc00);
        }
    }

    if (export.ultimax_phi1) {
        if ((tmp & 0x3fff) >= 0x3000) {
            char_base = ultimax_romh_phi1_ptr((WORD)(0x1000 + (tmp & 0xfff)));
        } else {
            char_base = vicii.ram_base_phi1 + tmp;
        }

        if (((bitmap_bank + 0x1000) & 0x3fff) >= 0x3000) {
            bitmap_high_base = ultimax_romh_phi1_ptr(0x1000);
        } else {
            bitmap_high_base = bitmap_low_base + 0x1000;
        }
    } else {
        if ((tmp & vicii.vaddr_chargen_mask_phi1)
            != vicii.vaddr_chargen_value_phi1) {
            char_base = vicii.ram_base_phi1 + tmp;
        } else {
            char_base = mem_chargen_rom_ptr + (tmp & 0x0800);
        }

        if (((bitmap_bank + 0x1000) & vicii.vaddr_chargen_mask_phi1)
            != vicii.vaddr_chargen_value_phi1) {
            bitmap_high_base = bitmap_low_base + 0x1000;
        } else {
            bitmap_high_base = mem_chargen_rom_ptr;
        }
    }

    raster_screenshot(&vicii.raster, screenshot);

    screenshot->chipid = "VICII";
    screenshot->video_regs = vicii.regs;
    screenshot->screen_ptr = screen_base_phi2;
    screenshot->chargen_ptr = char_base;
    screenshot->bitmap_ptr = NULL;
    screenshot->bitmap_low_ptr = bitmap_low_base;
    screenshot->bitmap_high_ptr = bitmap_high_base;
    screenshot->color_ram_ptr = mem_color_ram_vicii;
}

void vicii_async_refresh(struct canvas_refresh_s *refresh)
{
    raster_async_refresh(&vicii.raster, refresh);
}

/* ---------------------------------------------------------------------*/

static const char *fetch_phi1_type(int addr)
{
    addr = (addr & vicii.vaddr_mask_phi1) | vicii.vaddr_offset_phi1;

    if (export.ultimax_phi1) {
        if ((addr & 0x3fff) >= 0x3000) {
            return "Cart";
        } else {
            return "RAM";
        }
    } else {
        if ((addr & vicii.vaddr_chargen_mask_phi1) == vicii.vaddr_chargen_value_phi1) {
            return "CharROM";
        } else {
            return "RAM";
        }
    }
}


int vicii_dump(void)
{
    static const char *mode_name[] = {
        "Standard Text",
        "Multicolor Text",
        "Hires Bitmap",
        "Multicolor Bitmap",
        "Extended Text",
        "Illegal Text",
        "Invalid Bitmap 1",
        "Invalid Bitmap 2"
    };

    int video_mode, m_mcm, m_bmm, m_ecm, v_bank, v_vram;
    int i, bits, bits2;

    video_mode = ((vicii.regs[0x11] & 0x60) | (vicii.regs[0x16] & 0x10)) >> 4;

    m_ecm = (video_mode & 4) >> 2;  /* 0 standard, 1 extended */
    m_bmm = (video_mode & 2) >> 1;  /* 0 text, 1 bitmap */
    m_mcm = video_mode & 1;         /* 0 hires, 1 multi */

    v_bank = vicii.vbank_phi1;

    mon_out("Raster cycle/line: %d/%d IRQ: %d\n", vicii.raster_cycle, vicii.raster_line, vicii.raster_irq_line);
    mon_out("Mode: %s (ECM/BMM/MCM=%d/%d/%d)\n", mode_name[video_mode], m_ecm, m_bmm, m_mcm);
    mon_out("Colors: Border: %x BG: %x ", vicii.regs[0x20], vicii.regs[0x21]);
    if (m_ecm) {
        mon_out("BG1: %x BG2: %x BG3: %x\n", vicii.regs[0x22], vicii.regs[0x23], vicii.regs[0x24]);
    } else if (m_mcm && !m_bmm) {
        mon_out("MC1: %x MC2: %x\n", vicii.regs[0x22], vicii.regs[0x23]);
    } else {
        mon_out("\n");
    }

    mon_out("Scroll X/Y: %d/%d, RC %d, Idle: %d, ", vicii.regs[0x16] & 0x07, vicii.regs[0x11] & 0x07, vicii.rc, vicii.idle_state);
    mon_out("%dx%d\n", 39 + ((vicii.regs[0x16] >> 3) & 1), 24 + ((vicii.regs[0x11] >> 3) & 1));

    mon_out("VC $%03x, VCBASE $%03x, VMLI %2d, Phi1 $%02x\n", vicii.vc, vicii.vcbase, vicii.vmli, vicii.last_read_phi1);

    v_vram = ((vicii.regs[0x18] >> 4) * 0x0400) + vicii.vbank_phi2;
    mon_out("Video $%04x, ", v_vram);
    if (m_bmm) {
        i = ((vicii.regs[0x18] >> 3) & 1) * 0x2000 + v_bank;
        mon_out("Bitmap $%04x (%s)\n", i, fetch_phi1_type(i));
    } else {
        i = (((vicii.regs[0x18] >> 1) & 0x7) * 0x800) + v_bank;
        mon_out("Charset $%04x (%s)\n", i, fetch_phi1_type(i));
    }

    mon_out("\nSprites: S.0 S.1 S.2 S.3 S.4 S.5 S.6 S.7");
    mon_out("\nEnabled:");
    bits = vicii.regs[0x15];
    for (i = 0; i < 8; i++) {
        mon_out("%4s", (bits & 1) ? "yes" : "no");
        bits >>= 1;
    }

    mon_out("\nDMA/dis:");
    bits = vicii.sprite_dma;
    bits2 = vicii.sprite_display_bits;
    for (i = 0; i < 8; i++) {
        mon_out(" %c/%c", (bits & 1) ? 'D' : ' ', (bits2 & 1) ? 'd' : ' ');
        bits >>= 1;
        bits2 >>= 1;
    }

    mon_out("\nPointer:");
    for (i = 0; i < 8; i++) {
        mon_out(" $%02x", vicii.sprite[i].pointer);
    }

    mon_out("\nMC:     ");
    for (i = 0; i < 8; i++) {
        mon_out(" $%02x", vicii.sprite[i].mc);
    }

    mon_out("\nMCBASE: ");
    for (i = 0; i < 8; i++) {
        mon_out(" $%02x", vicii.sprite[i].mcbase);
    }

    mon_out("\nX-Pos:  ");
    for (i = 0; i < 8; i++) {
        mon_out("$%03x", vicii.sprite[i].x);
    }
    mon_out("\nY-Pos:  ");
    for (i = 0; i < 8; i++) {
        mon_out("%4d", vicii.regs[1 + (i << 1)]);
    }
    mon_out("\nX/Y-Exp:");
    bits = vicii.regs[0x1d];
    bits2 = vicii.regs[0x17];
    for (i = 0; i < 8; i++) {
        mon_out(" %c/%c", (bits & 1) ? 'X' : ' ', (bits2 & 1) ? (vicii.sprite[i].exp_flop ? 'Y' : 'y') : ' ');
        bits >>= 1;
        bits2 >>= 1;
    }
    mon_out("\nPri./MC:");
    bits = vicii.regs[0x1b];
    bits = vicii.regs[0x1c];
    for (i = 0; i < 8; i++) {
        mon_out(" %c/%c", (bits & 1) ? 'b' : 's', (bits2 & 1) ? '*' : ' ');
        bits >>= 1;
    }
    mon_out("\nColor:  ");
    for (i = 0; i < 8; i++) {
        mon_out("   %x", vicii.regs[i + 0x27]);
    }
    if (vicii.regs[0x1c]) {
        mon_out("\nMulti Color 1: %x  Multi Color 2: %x", vicii.regs[0x25], vicii.regs[0x26]);
    }
    mon_out("\n");

    return 0;
}
