/*
 * vicii-draw-cycle.c - Cycle based rendering for the VIC-II emulation.
 *
 * Written by
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

#include "vice.h"

#include <string.h>

#include "types.h"
#include "snapshot.h"
#include "vicii-chip-model.h"
#include "vicii-draw-cycle.h"
#include "viciitypes.h"

/* disable for debugging */
#define DRAW_INLINE inline

/* colors */
#define COL_NONE     0x10
#define COL_VBUF_L   0x11
#define COL_VBUF_H   0x12
#define COL_CBUF     0x13
#define COL_CBUF_MC  0x14
#define COL_D02X_EXT 0x15
#define COL_D020     0x20
#define COL_D021     0x21
#define COL_D022     0x22
#define COL_D023     0x23
#define COL_D024     0x24
#define COL_D025     0x25
#define COL_D026     0x26
#define COL_D027     0x27
#define COL_D028     0x28
#define COL_D029     0x29
#define COL_D02A     0x2a
#define COL_D02B     0x2b
#define COL_D02C     0x2c
#define COL_D02D     0x2d
#define COL_D02E     0x2e

/* foreground/background graphics */

static BYTE gbuf_pipe0_reg = 0;
static BYTE cbuf_pipe0_reg = 0;
static BYTE vbuf_pipe0_reg = 0;
static BYTE gbuf_pipe1_reg = 0;
static BYTE cbuf_pipe1_reg = 0;
static BYTE vbuf_pipe1_reg = 0;

static BYTE xscroll_pipe = 0;
static BYTE vmode11_pipe = 0;
static BYTE vmode16_pipe = 0;
static BYTE vmode16_pipe2 = 0;

/* gbuf shift register */
static BYTE gbuf_reg = 0;
static BYTE gbuf_mc_flop = 0;
static BYTE gbuf_pixel_reg = 0;

/* cbuf and vbuf registers */
static BYTE cbuf_reg = 0;
static BYTE vbuf_reg = 0;

static BYTE dmli = 0;


/* sprites */
static int sprite_x_pipe[8];
static BYTE sprite_pri_bits = 0;
static BYTE sprite_mc_bits = 0;
static BYTE sprite_expx_bits = 0;

static BYTE sprite_pending_bits = 0;
static BYTE sprite_active_bits = 0;
static BYTE sprite_halt_bits = 0;

/* sbuf shift registers */
static DWORD sbuf_reg[8];
static BYTE sbuf_pixel_reg[8];
static BYTE sbuf_expx_flops;
static BYTE sbuf_mc_flops;

/* border */
static int border_state = 0;

/* pixel buffer */
static BYTE render_buffer[8];
static BYTE pri_buffer[8];

static BYTE pixel_buffer[8];

/* color resolution registers */
static BYTE cregs[0x2f];
static BYTE last_color_reg;
static BYTE last_color_value;

static unsigned int cycle_flags_pipe;


/**************************************************************************
 *
 * SECTION  draw_graphics()
 *
 ******/

static const BYTE colors[] = {
    COL_D021, COL_D021, COL_CBUF, COL_CBUF,         /* ECM=0 BMM=0 MCM=0 */
    COL_D021, COL_D022, COL_D023, COL_CBUF_MC,      /* ECM=0 BMM=0 MCM=1 */
    COL_VBUF_L, COL_VBUF_L, COL_VBUF_H, COL_VBUF_H, /* ECM=0 BMM=1 MCM=0 */
    COL_D021, COL_VBUF_H, COL_VBUF_L, COL_CBUF,     /* ECM=0 BMM=1 MCM=1 */
    COL_D02X_EXT, COL_D02X_EXT, COL_CBUF, COL_CBUF, /* ECM=1 BMM=0 MCM=0 */
    COL_NONE, COL_NONE, COL_NONE, COL_NONE,         /* ECM=1 BMM=0 MCM=1 */
    COL_NONE, COL_NONE, COL_NONE, COL_NONE,         /* ECM=1 BMM=1 MCM=0 */
    COL_NONE, COL_NONE, COL_NONE, COL_NONE          /* ECM=1 BMM=1 MCM=1 */
};

static DRAW_INLINE void draw_graphics(int i)
{
    BYTE px;
    BYTE cc;
    BYTE pixel_pri;
    BYTE vmode;

    /* Load new gbuf/vbuf/cbuf values at offset == xscroll */
    if (i == xscroll_pipe) {
        /* latch values at time xs */
        vbuf_reg = vbuf_pipe1_reg;
        cbuf_reg = cbuf_pipe1_reg;
        gbuf_reg = gbuf_pipe1_reg;
        gbuf_mc_flop = 1;
    }

    /*
     * read pixels depending on video mode
     * mc pixels if MCM=1 and BMM=1, or MCM=1 and cbuf bit 3 = 1
     */
    if (vmode16_pipe2) {
        if ((vmode11_pipe & 0x08) || (cbuf_reg & 0x08)) {
            /* mc pixels */
            if (gbuf_mc_flop) {
                gbuf_pixel_reg = gbuf_reg >> 6;
            }
        } else {
            /* hires pixels */
            gbuf_pixel_reg = (gbuf_reg & 0x80) ? 3 : 0;
        }
    } else {
        /*
         * some kludge magic to fix $d023 glitch at MCM=0 -> 1 during
         * MC and non-MC chars.
         * This is rather ugly. There must be a simpler solution.
         */
        if ((vmode11_pipe & 0x08) || (cbuf_reg & 0x08)) {
            /* hires pixels */
            gbuf_pixel_reg = (gbuf_reg & 0x80) ? 2 : 0;
        } else {
            /* hires pixels */
            gbuf_pixel_reg = (gbuf_reg & 0x80) ? 3 : 0;
        }
    }
    px = gbuf_pixel_reg;

    /* shift the graphics buffer */
    gbuf_reg <<= 1;
    gbuf_mc_flop ^= 1;

    /* Determine pixel color and priority */
    vmode = vmode11_pipe | vmode16_pipe;
    pixel_pri = (px & 0x2);
    cc = colors[vmode | px];

    /* lookup colors and render pixel */
    switch (cc) {
        case COL_NONE:
            cc = 0;
            break;
        case COL_VBUF_L:
            cc = vbuf_reg & 0x0f;
            break;
        case COL_VBUF_H:
            cc = vbuf_reg >> 4;
            break;
        case COL_CBUF:
            cc = cbuf_reg;
            break;
        case COL_CBUF_MC:
            cc = cbuf_reg & 0x07;
            break;
        case COL_D02X_EXT:
            cc = COL_D021 + (vbuf_reg >> 6);
            break;
        default:
            break;
    }

    render_buffer[i] = cc;
    pri_buffer[i] = pixel_pri;
}

static DRAW_INLINE void draw_graphics8(unsigned int cycle_flags)
{
    int vis_en;

    vis_en = cycle_is_visible(cycle_flags);

    /* render pixels */
    /* pixel 0 */
    draw_graphics(0);
    /* pixel 1 */
    draw_graphics(1);
    /* pixel 2 */
    draw_graphics(2);
    /* pixel 3 */
    draw_graphics(3);
    /* pixel 4 */
    vmode16_pipe = ( vicii.regs[0x16] & 0x10 ) >> 2;
    if (vicii.color_latency) {
        /* handle rising edge of internal signal */
        vmode11_pipe |= ( vicii.regs[0x11] & 0x60 ) >> 2;
    }
    draw_graphics(4);
    /* pixel 5 */
    draw_graphics(5);
    /* pixel 6 */
    if (vicii.color_latency) {
        /* handle falling edge of internal signal */
        vmode11_pipe &= ( vicii.regs[0x11] & 0x60 ) >> 2;
    }
    draw_graphics(6);
    /* pixel 7 */
    if (vmode16_pipe && !vmode16_pipe2) {
        gbuf_mc_flop = 0;
    }
    vmode16_pipe2 = vmode16_pipe;
    draw_graphics(7);

    if (!vicii.color_latency) {
        vmode11_pipe = ( vicii.regs[0x11] & 0x60 ) >> 2;
    }

    /* shift and put the next data into the pipe. */
    vbuf_pipe1_reg = vbuf_pipe0_reg;
    cbuf_pipe1_reg = cbuf_pipe0_reg;
    gbuf_pipe1_reg = gbuf_pipe0_reg;

    /* this makes sure gbuf is 0 outside the visible area
       It should probably be done somewhere around the fetch instead */
    if (vis_en && vicii.vborder == 0) {
        gbuf_pipe0_reg = vicii.gbuf;
        xscroll_pipe = vicii.regs[0x16] & 0x07;
    } else {
        gbuf_pipe0_reg = 0;
    }

    /* Only update vbuf and cbuf registers in the display state. */
    if (vis_en && vicii.vborder == 0) {
        if (!vicii.idle_state) {
            vbuf_pipe0_reg = vicii.vbuf[dmli];
            cbuf_pipe0_reg = vicii.cbuf[dmli];
        } else {
            vbuf_pipe0_reg = 0;
            cbuf_pipe0_reg = 0;
        }
    }

    /* update display index in the visible region */
    if (vis_en) {
        dmli++;
    } else {
        dmli = 0;
    }
}



/**************************************************************************
 *
 * SECTION  draw_sprites()
 *
 ******/
static DRAW_INLINE BYTE get_trigger_candidates(int xpos)
{
    int s;
    BYTE candidate_bits = 0;

    /* check for partial xpos match */
    for (s = 0; s < 8; s++) {
        if ((xpos & 0x1f8) == (sprite_x_pipe[s] & 0x1f8)) {
            candidate_bits |= 1 << s;
        }
    }
    return candidate_bits;
}

static DRAW_INLINE void trigger_sprites(int xpos, BYTE candidate_bits)
{
    int s;

    /* do nothing if no sprites are candidates or pending */
    if (!candidate_bits || !sprite_pending_bits) {
        return;
    }

    /* check for pending */
    for (s = 0; s < 8; s++) {
        BYTE m = 1 << s;

        /* start rendering on position match */
        if ((candidate_bits & m) && (sprite_pending_bits & m) && !(sprite_active_bits & m) && !(sprite_halt_bits & m)) {
            if (xpos == sprite_x_pipe[s]) {
                sbuf_expx_flops |= m;
                sbuf_mc_flops |= m;
                sprite_active_bits |= m;
            }
        }
    }
}

static DRAW_INLINE void draw_sprites(int i)
{
    int s;
    int active_sprite;
    BYTE collision_mask;

    /* do nothing if all sprites are inactive */
    if (!sprite_active_bits) {
        return;
    }

    /* check for active sprites */
    active_sprite = -1;
    collision_mask = 0;
    for (s = 7; s >= 0; --s) {
        BYTE m = 1 << s;

        if (sprite_active_bits & m) {
            /* render pixels if shift register or pixel reg still contains data */
            if (sbuf_reg[s] || sbuf_pixel_reg[s]) {
                if (!(sprite_halt_bits & m)) {
                    if (sbuf_expx_flops & m) {
                        if (sprite_mc_bits & m) {
                            if (sbuf_mc_flops & m) {
                                /* fetch 2 bits */
                                sbuf_pixel_reg[s] = (BYTE)((sbuf_reg[s] >> 22) & 0x03);
                            }
                            sbuf_mc_flops ^= m;
                        } else {
                            /* fetch 1 bit and make it 0 or 2 */
                            sbuf_pixel_reg[s] = (BYTE)(((sbuf_reg[s] >> 23) & 0x01 ) << 1);
                        }
                    }

                    /* shift the sprite buffer and handle expansion flags */
                    if (sbuf_expx_flops & m) {
                        sbuf_reg[s] <<= 1;
                    }
                    if (sprite_expx_bits & m) {
                        sbuf_expx_flops ^= m;
                    } else {
                        sbuf_expx_flops |= m;
                    }
                }

                /*
                 * set collision mask bits and determine the highest
                 * priority sprite number that has a pixel.
                 */
                if (sbuf_pixel_reg[s]) {
                    active_sprite = s;
                    collision_mask |= m;
                }
            } else {
                sprite_active_bits &= ~m;
            }
        }
    }

    if (collision_mask) {
        BYTE pixel_pri = pri_buffer[i];
        int s = active_sprite;
        BYTE spri = sprite_pri_bits & (1 << s);
        if (!(pixel_pri && spri)) {
            switch (sbuf_pixel_reg[s]) {
                case 1:
                    render_buffer[i] = COL_D025;
                    break;
                case 2:
                    render_buffer[i] = COL_D027 + s;
                    break;
                case 3:
                    render_buffer[i] = COL_D026;
                    break;
                default:
                    break;
            }
        }
        /* if there was a foreground pixel, trigger collision */
        if (pixel_pri) {
            vicii.sprite_background_collisions |= collision_mask;
        }
    }

    /* if 2 or more bits are set, trigger collisions */
    if (collision_mask & (collision_mask - 1)) {
        vicii.sprite_sprite_collisions |= collision_mask;
    }
}


static DRAW_INLINE void update_sprite_mc_bits_6569(void)
{
    BYTE next_mc_bits = vicii.regs[0x1c];
    BYTE toggled = next_mc_bits ^ sprite_mc_bits;

    sbuf_mc_flops &= ~toggled;
    sprite_mc_bits = next_mc_bits;
}

static DRAW_INLINE void update_sprite_mc_bits_8565(void)
{
    BYTE next_mc_bits = vicii.regs[0x1c];
    BYTE toggled = next_mc_bits ^ sprite_mc_bits;

    sbuf_mc_flops ^= toggled & (~sbuf_expx_flops);
    sprite_mc_bits = next_mc_bits;
}

static DRAW_INLINE void update_sprite_data(unsigned int cycle_flags)
{
    if (cycle_is_sprite_dma1_dma2(cycle_flags)) {
        int s = cycle_get_sprite_num(cycle_flags);
        sbuf_reg[s] = vicii.sprite[s].data;
    }
}

static DRAW_INLINE void update_sprite_xpos(void)
{
    int s;
    for (s = 0; s < 8; s++) {
        sprite_x_pipe[s] = vicii.sprite[s].x;
    }
}



static DRAW_INLINE void draw_sprites8(unsigned int cycle_flags)
{
    BYTE candidate_bits;
    BYTE dma_cycle_0 = 0;
    BYTE dma_cycle_2 = 0;
    int xpos;
    int spr_en;

    xpos = cycle_get_xpos(cycle_flags);

    spr_en = cycle_is_check_spr_disp(cycle_flags);

    if (cycle_is_sprite_ptr_dma0(cycle_flags)) {
        dma_cycle_0 = 1 << cycle_get_sprite_num(cycle_flags);
    }
    if (cycle_is_sprite_dma1_dma2(cycle_flags)) {
        dma_cycle_2 = 1 << cycle_get_sprite_num(cycle_flags);
    }
    candidate_bits = get_trigger_candidates(xpos);

    /* process and render sprites */
    /* pixel 0 */
    trigger_sprites(xpos + 0, candidate_bits);
    draw_sprites(0);
    /* pixel 1 */
    trigger_sprites(xpos + 1, candidate_bits);
    draw_sprites(1);
    /* pixel 2 */
    sprite_active_bits &= ~dma_cycle_2;
    trigger_sprites(xpos + 2, candidate_bits);
    draw_sprites(2);
    /* pixel 3 */
    sprite_halt_bits |= dma_cycle_0;
    trigger_sprites(xpos + 3, candidate_bits);
    draw_sprites(3);
    /* pixel 4 */
    if (spr_en) {
        sprite_pending_bits = vicii.sprite_display_bits;
    }
    update_sprite_data(cycle_flags);
    trigger_sprites(xpos + 4, candidate_bits);
    draw_sprites(4);
    /* pixel 5 */
    trigger_sprites(xpos + 5, candidate_bits);
    draw_sprites(5);
    /* pixel 6 */
    if (!vicii.color_latency) {
        update_sprite_mc_bits_8565();
    }
    sprite_pri_bits = vicii.regs[0x1b];
    sprite_expx_bits = vicii.regs[0x1d];
    trigger_sprites(xpos + 6, candidate_bits);
    draw_sprites(6);
    /* pixel 7 */
    if (vicii.color_latency) {
        update_sprite_mc_bits_6569();
    }
    sprite_halt_bits &= ~dma_cycle_2;
    trigger_sprites(xpos + 7, candidate_bits);
    draw_sprites(7);

    /* pipe xpos */
    update_sprite_xpos();
}


/**************************************************************************
 *
 * SECTION  draw_border()
 *
 ******/

static DRAW_INLINE void draw_border8(void)
{
    BYTE csel = vicii.regs[0x16] & 0x8;

#if 1
    /* early exit for the no border case */
    if (!(border_state || vicii.main_border)) {
        return;
    }
    /* early exit for the continuous border case */
    if (border_state && vicii.main_border) {
        memset(render_buffer, COL_D020, 8);
        return;
    }
#endif

    /*
     * normal border handling in case there was a transition
     * (the code below can handle all border logic)
     */
    if (csel) {
        if (border_state) {
            memset(render_buffer, COL_D020, 8);
        }
        border_state = vicii.main_border;
    } else {
        if (border_state) {
            memset(render_buffer, COL_D020, 7);
        }
        border_state = vicii.main_border;
        if (border_state) {
            render_buffer[7] = COL_D020;
        }
    }
}


/**************************************************************************
 *
 * SECTION  draw_colors()
 *
 ******/

static DRAW_INLINE void update_cregs(void)
{
    last_color_reg = vicii.last_color_reg;
    last_color_value = vicii.last_color_value;
    vicii.last_color_reg = 0xff;
}

static DRAW_INLINE void draw_colors_6569(int offs, int i)
{
    int lookup_index;

    /* resolve any unresolved colors */
    lookup_index = (i + 1) & 0x07;
    pixel_buffer[lookup_index] = cregs[pixel_buffer[lookup_index]];

    /* draw pixel to buffer */
    vicii.dbuf[offs + i] = pixel_buffer[i];

    pixel_buffer[i] = render_buffer[i];
}

static DRAW_INLINE void draw_colors_8565(int offs, int i)
{
    int lookup_index;

    lookup_index = i;
    /* resolve any unresolved colors */

    /* special case for grey dot handling */
    if (i == 0 && pixel_buffer[lookup_index] == last_color_reg) {
        pixel_buffer[lookup_index] = 0x0f;
    } else {
        pixel_buffer[lookup_index] = cregs[pixel_buffer[lookup_index]];
    }

    /* draw pixel to buffer */
    vicii.dbuf[offs + i] = pixel_buffer[i];

    pixel_buffer[i] = render_buffer[i];
}

static DRAW_INLINE void draw_colors8(void)
{
    int offs = vicii.dbuf_offset;

    /* guard (could possibly be removed) */
    if (offs > VICII_DRAW_BUFFER_SIZE - 8) {
        return;
    }

    /* update color register (if written) */
    if (last_color_reg != 0xff) {
        cregs[last_color_reg] = last_color_value;
    }

    /* render pixels */
    if (vicii.color_latency) {
        draw_colors_6569(offs, 0);
        draw_colors_6569(offs, 1);
        draw_colors_6569(offs, 2);
        draw_colors_6569(offs, 3);
        draw_colors_6569(offs, 4);
        draw_colors_6569(offs, 5);
        draw_colors_6569(offs, 6);
        draw_colors_6569(offs, 7);
    } else {
        draw_colors_8565(offs, 0);
        draw_colors_8565(offs, 1);
        draw_colors_8565(offs, 2);
        draw_colors_8565(offs, 3);
        draw_colors_8565(offs, 4);
        draw_colors_8565(offs, 5);
        draw_colors_8565(offs, 6);
        draw_colors_8565(offs, 7);
    }
    vicii.dbuf_offset += 8;

    update_cregs();
}


/**************************************************************************
 *
 * SECTION  vicii_draw_cycle()
 *
 ******/

void vicii_draw_cycle(void)
{
    /* reset rendering on raster cycle 1 */
    if (vicii.raster_cycle == 1) {
        vicii.dbuf_offset = 0;
    }

    draw_graphics8(cycle_flags_pipe);

    draw_sprites8(cycle_flags_pipe);

    draw_border8();

    draw_colors8();

    cycle_flags_pipe = vicii.cycle_flags;
}


void vicii_draw_cycle_init(void)
{
    int i;

    /* initialize the draw buffer */
    memset(vicii.dbuf, 0, VICII_DRAW_BUFFER_SIZE);
    vicii.dbuf_offset = 0;

    /* initialize the pixel ring buffer. */
    memset(pixel_buffer, 0, sizeof(pixel_buffer));

    /* clear cregs and fill 0x00-0x0f with 1:1 mapping */
    memset(cregs, 0, sizeof(cregs));
    for (i = 0; i < 0x10; i++) {
        cregs[i] = i;
    }
    vicii.last_color_reg = 0xff;
    last_color_reg = 0xff;

    cycle_flags_pipe = 0;
}


/**************************************************************************
 *
 * SECTION  snapshot
 *
 ******/

/* FIXME this is likely way more state than needed */

int vicii_draw_cycle_snapshot_write(snapshot_module_t *m)
{
    int i;

    if (0
        || SMW_B(m, gbuf_pipe0_reg) < 0
        || SMW_B(m, cbuf_pipe0_reg) < 0
        || SMW_B(m, vbuf_pipe0_reg) < 0
        || SMW_B(m, gbuf_pipe1_reg) < 0
        || SMW_B(m, cbuf_pipe1_reg) < 0
        || SMW_B(m, vbuf_pipe1_reg) < 0
        || SMW_B(m, xscroll_pipe) < 0
        || SMW_B(m, vmode11_pipe) < 0
        || SMW_B(m, vmode16_pipe) < 0
        || SMW_B(m, vmode16_pipe2) < 0
        || SMW_B(m, gbuf_reg) < 0
        || SMW_B(m, gbuf_mc_flop) < 0
        || SMW_B(m, gbuf_pixel_reg) < 0
        || SMW_B(m, cbuf_reg) < 0
        || SMW_B(m, vbuf_reg) < 0
        || SMW_B(m, dmli) < 0) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (SMW_DW(m, (DWORD)sprite_x_pipe[i]) < 0) {
            return -1;
        }
    }

    if (0
        || SMW_B(m, sprite_pri_bits) < 0
        || SMW_B(m, sprite_mc_bits) < 0
        || SMW_B(m, sprite_expx_bits) < 0
        || SMW_B(m, sprite_pending_bits) < 0
        || SMW_B(m, sprite_active_bits) < 0
        || SMW_B(m, sprite_halt_bits) < 0) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (SMW_DW(m, sbuf_reg[i]) < 0) {
            return -1;
        }
    }

    if (0
        || SMW_BA(m, sbuf_pixel_reg, 8) < 0
        || SMW_B(m, sbuf_expx_flops) < 0
        || SMW_B(m, sbuf_mc_flops) < 0
        || SMW_B(m, (BYTE)border_state) < 0
        || SMW_BA(m, render_buffer, 8) < 0
        || SMW_BA(m, pri_buffer, 8) < 0
        || SMW_BA(m, pixel_buffer, 8) < 0
        || SMW_BA(m, cregs, 0x2f) < 0
        || SMW_B(m, last_color_reg) < 0
        || SMW_B(m, last_color_value) < 0
        || SMW_DW(m, (DWORD)cycle_flags_pipe) < 0) {
        return -1;
    }

    return 0;
}

int vicii_draw_cycle_snapshot_read(snapshot_module_t *m)
{
    int i;

    if (0
        || SMR_B(m, &gbuf_pipe0_reg) < 0
        || SMR_B(m, &cbuf_pipe0_reg) < 0
        || SMR_B(m, &vbuf_pipe0_reg) < 0
        || SMR_B(m, &gbuf_pipe1_reg) < 0
        || SMR_B(m, &cbuf_pipe1_reg) < 0
        || SMR_B(m, &vbuf_pipe1_reg) < 0
        || SMR_B(m, &xscroll_pipe) < 0
        || SMR_B(m, &vmode11_pipe) < 0
        || SMR_B(m, &vmode16_pipe) < 0
        || SMR_B(m, &vmode16_pipe2) < 0
        || SMR_B(m, &gbuf_reg) < 0
        || SMR_B(m, &gbuf_mc_flop) < 0
        || SMR_B(m, &gbuf_pixel_reg) < 0
        || SMR_B(m, &cbuf_reg) < 0
        || SMR_B(m, &vbuf_reg) < 0
        || SMR_B(m, &dmli) < 0) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (SMR_DW_INT(m, &sprite_x_pipe[i]) < 0) {
            return -1;
        }
    }

    if (0
        || SMR_B(m, &sprite_pri_bits) < 0
        || SMR_B(m, &sprite_mc_bits) < 0
        || SMR_B(m, &sprite_expx_bits) < 0
        || SMR_B(m, &sprite_pending_bits) < 0
        || SMR_B(m, &sprite_active_bits) < 0
        || SMR_B(m, &sprite_halt_bits) < 0) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        if (SMR_DW(m, &sbuf_reg[i]) < 0) {
            return -1;
        }
    }

    if (0
        || SMR_BA(m, sbuf_pixel_reg, 8) < 0
        || SMR_B(m, &sbuf_expx_flops) < 0
        || SMR_B(m, &sbuf_mc_flops) < 0
        || SMR_B_INT(m, &border_state) < 0
        || SMR_BA(m, render_buffer, 8) < 0
        || SMR_BA(m, pri_buffer, 8) < 0
        || SMR_BA(m, pixel_buffer, 8) < 0
        || SMR_BA(m, cregs, 0x2f) < 0
        || SMR_B(m, &last_color_reg) < 0
        || SMR_B(m, &last_color_value) < 0
        || SMR_DW_UINT(m, &cycle_flags_pipe) < 0) {
        return -1;
    }

    return 0;
}
