/*
 * crtc-mem.c - A line-based CRTC emulation (under construction).
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

#include "alarm.h"
#include "crtc-mem.h"
#include "crtc.h"
#include "crtctypes.h"
#include "maincpu.h"
#include "types.h"


/* CRTC interface functions.
   FIXME: Several registers are not implemented.  */

void crtc_store(WORD addr, BYTE value)
{
    int current_cycle;

    current_cycle = maincpu_clk - crtc.rl_start;

    addr &= 1;

    if (!addr) {
        crtc.regno = value;
        return;
    }

#if 0
    /* debug display, just not the cursor (for CBM-II) */
    if (crtc.regno < 14 && crtc.regno != 10) {
        printf("store_crtc(reg=%d, %d) - cline=%d, ycount=%d, char=%d\n",
               crtc.regno, value, crtc.current_charline, crtc.raster.ycounter,
               current_cycle);
    }
#endif

    crtc.regs[crtc.regno] = value;

    switch (crtc.regno) {
        case 0:                 /* R00  Horizontal total (characters + 1) */
            if (current_cycle > value) {
                value = 255;
            }
            crtc.rl_len = value;
            if (crtc.initialized) {
                alarm_set(crtc.raster_draw_alarm, crtc.rl_start + value);
            }
            break;

        case 1:                 /* R01  Horizontal characters displayed */
            if (!(current_cycle < crtc.rl_visible)) {
                break;
            }

            /* the compare is not yet done */
            if ((crtc.regs[1]) > current_cycle) {
                /* only if we write a higher value than the counter,
                 we can update disp_cycles here */
                crtc.rl_visible = crtc.regs[1];
                crtc.henable = 1;
            } else {
                /* we write a value lower than the counter -> never reached,
                 open border */
                crtc.rl_visible = crtc.rl_len + 1;
                crtc.henable = 0;
            }
            break;

        case 2:                 /* R02  Horizontal Sync Position */
            if (current_cycle < crtc.rl_sync) {
                /* FIXME: middle of pulse, adjust from reg. 3 */
                crtc.rl_sync = value;
            }
            break;
        case 3:                 /* R03  Horizontal/Vertical Sync widths */
            break;

        case 7:                 /* R07  Vertical sync position */
            break;

        case 8:                 /* R08  unused: Interlace and Skew */
            break;

        case 6:                 /* R06  Number of display lines on screen */
            crtc.regs[6] &= 0x7f;
            break;

        case 9:                 /* R09  Rasters between two display lines */
            crtc.regs[9] &= 0x1f;
            break;

        case 4:                 /* R04  Vertical total (character) rows */
            break;

        case 5:                 /* R05  Vertical total line adjust */
            break;

        case 10:                /* R10  Cursor (not implemented on the PET) */
            value = ((value >> 5) & 0x03) ^ 0x01;
            if (!(crtc.hw_cursor && (crtc.crsrmode != value))) {
                break;
            }

            crtc.crsrmode = value;
            crtc.crsrstate = 1;
            crtc.crsrcnt = 16;
            break;

        case 11:                /* R11  Cursor (not implemented on the PET) */
            break;

        case 12:                /* R12  Control register */
            /* This is actually the upper 6 video RAM address bits.
             * But CBM decided that the two uppermost bits should be used
             * for control.
             * The usage here is from the 8032 schematics on funet.
             *
             * Bit 0: 1=add 256 to screen start address ( 512 for 80-columns)
             * Bit 1: 1=add 512 to screen start address (1024 for 80-columns)
             * Bit 2: no connection
             * Bit 3: no connection
             * Bit 4: invert video signal
             * Bit 5: use top half of 4K character generator
             * Bit 6: (no pin on the CRTC, video address is 14 bit only)
             * Bit 7: (no pin on the CRTC, video address is 14 bit only)
             */
            /* The CRTC loads its internal counter when it starts a new
             * frame. At this point the address/mode changes are evaluated now.
             */
            break;

        case 13:                /* R13  Address of first character */
            break;
#if 0
        case 14:
            crsr_set_dirty();
            crsrpos = ((crsrpos & 0x00ff) | ((value << 8) & 0x3f00)) & addr_mask;
            crsrrel = crsrpos - scrpos;
            crsr_set_dirty();
            break;

        case 15:                /* R14-5 Cursor location HI/LO -- unused */
            crsr_set_dirty();
            crsrpos = ((crsrpos & 0x3f00) | (value & 0xff)) & addr_mask;
            crsrrel = crsrpos - scrpos;
            crsr_set_dirty();
            break;
#endif
        case 16:
        case 17:                /* R16-7 Light Pen HI/LO -- read only */
            break;

        case 18:
        case 19:                /* R18-9 Update address HI/LO (only 6545)  */
            break;

        default:
            break;
    }
}

BYTE crtc_read(WORD addr)
{
    /* Status register:
     *  bit 7: 0 = register 31 (update reg.) has been read/written by CPU
     *         1 = update strobe received
     *      6: 0 = register 16 or 17 has been read by CPU
     *         1 = light pen strobe has been received
     *      5: 0 = scan is not in vertical retrace
     *         1 = scan is in vertical retrace (ends 5 char clock times
     *             before end of retrace, for possibly critical RAM refresh
     *             timings...)
     */
    if (!(addr & 1)) {
        return crtc_offscreen() ? 32 : 0;
    }

    /* internal registers */
    switch (crtc.regno) {
        case 14:
        case 15:                      /* Cursor location HI/LO */
            if (addr >= 64) {
                /*log_debug("crtc_read: ERROR");*/
                return 0;
            }
            return crtc.regs[addr];

        case 16:
        case 17:                      /* Light Pen X,Y */
            return 0xff;

        default:
            return 0;       /* All the rest are write-only registers */
    }

    return 0;
}

BYTE crtc_peek(WORD addr)
{
    return crtc_read(addr);
}

/* FIXME: to be moved to `crtc.c'.  */

#if 0
void crtc_set_char(int crom)
{
    chargen_rel = (chargen_rel & ~0x800) | (crom ? 0x800 : 0);
    /* chargen_rel is computed for 8bytes/char, but charom is 16bytes/char */
    chargen_ptr = chargen_rom + (chargen_rel << 1);
}

static void crtc_update_memory_ptrs(void)
{
    scraddr = crtc[13] + ((crtc[12] & 0x3f) << 8);

    /* depends on machine */
    do_update_memory_ptrs();

    /* chargen_rel is computed for 8bytes/char, but charom is 16bytes/char */
    chargen_ptr = chargen_rom + (chargen_rel << 1);

    scraddr &= addr_mask;
}

#endif
