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

#include <string.h>


/* CRTC interface functions.
   - bit 0 of the addr is wired to register-select of the chip
   FIXME: Several registers are not implemented.  */

void crtc_store(uint16_t addr, uint8_t value)
{
    CLOCK current_cycle;

    /*
     * The current raster line starts at the left edge of the text area.
     * That is also when the raster draw alarm is called.
     */
    current_cycle = maincpu_clk - CRTC_STORE_OFFSET - crtc.rl_start;

    addr &= 1;

    if (!addr) {
        /* low on register-select permits writes to the address register */
        crtc.regno = value & 0x1f;
        return;
    }

#if 0
    /* debug display, just not the cursor (for CBM-II) */
    if (crtc.regno < 14 && crtc.regno != 10) {
        printf("store_crtc(reg=%d, %d) - cline=%d, ycount=%d, char=%lld\n",
               crtc.regno, value, crtc.current_charline, crtc.raster.ycounter,
               current_cycle);
    }
#endif

    crtc.regs[crtc.regno] = value;

    switch (crtc.regno) {
        case CRTC_REG_HTOTAL:       /* R00  Horizontal total (characters + 1) */
            if (current_cycle > value) {
                value = 255;
            }
            crtc.rl_len = value;
            if (crtc.initialized) {
                alarm_set(crtc.raster_draw_alarm, crtc.rl_start + value);
            }
            break;

        case CRTC_REG_HDISP:        /* R01  Horizontal characters displayed */
            if (!(current_cycle < crtc.rl_visible)) {
                break;
            }

            /* the compare is not yet done */
            if (crtc.regs[CRTC_REG_HDISP] > current_cycle) {
                /* only if we write a higher value than the counter,
                 we can update disp_cycles here */
                crtc.rl_visible = crtc.regs[CRTC_REG_HDISP];
                crtc.henable = 1;
            } else {
                /* we write a value lower than the counter -> never reached,
                 open border */
                crtc.rl_visible = crtc.rl_len + 1;
                crtc.henable = 0;
            }
#if CRTC_BEAM_RACING
            if (crtc.venable) {
                /* Just cache the new visible length of the line */
                crtc_fetch_prefetch();
            }
#endif
            break;

        case CRTC_REG_HSYNC:        /* R02  Horizontal Sync Position */
            if (current_cycle < crtc.rl_sync) {
                /* FIXME: middle of pulse, adjust from reg. 3 */
                crtc.rl_sync = value;
            }
            break;

        case CRTC_REG_SYNCWIDTH:    /* R03  Horizontal/Vertical Sync widths */
            break;

        case CRTC_REG_VTOTAL:       /* R04  Vertical total (character) rows */
            crtc.regs[CRTC_REG_VTOTAL] &= 0x7f;
            break;

        case CRTC_REG_VTOTALADJ:    /* R05  Vertical total line adjust */
            crtc.regs[CRTC_REG_VTOTALADJ] &= 0x1f;
            break;

        case CRTC_REG_VDISP:        /* R06  Number of display lines on screen */
            crtc.regs[CRTC_REG_VDISP] &= 0x7f;
            break;

        case CRTC_REG_VSYNC:        /* R07  Vertical sync position */
            crtc.regs[CRTC_REG_VSYNC] &= 0x7f;
            break;

        case CRTC_REG_MODECTRL:     /* R08  unused: Interlace and Skew */
            break;

        case CRTC_REG_SCANLINE:     /* R09  number of lines per character line, including spacing */
            crtc.regs[CRTC_REG_SCANLINE] &= 0x1f;
            break;

        case CRTC_REG_CURSORSTART:  /* R10  Cursor start (not implemented on the PET) */
            crtc.regs[CRTC_REG_CURSORSTART] &= 0x7f;
            /* FIXME: set start line */
            value = ((value >> 5) & 0x03) ^ 0x01; /* cursor mode */
            if (!(crtc.hw_cursor && (crtc.crsrmode != value))) {
                break;
            }

            crtc.crsrmode = value;
            crtc.crsrstate = 1;
            crtc.crsrcnt = 16;
            break;

        case CRTC_REG_CURSOREND:    /* R11  Cursor end (not implemented on the PET) */
            crtc.regs[CRTC_REG_CURSOREND] &= 0x7f;
            /* FIXME: set end line */
            break;

        case CRTC_REG_DISPSTARTH:   /* R12  Control register, MA8-13 */
            /* This is actually the upper 6 video RAM address bits.
             * But CBM decided that the two uppermost bits should be used
             * for control.
             * The usage here is from the 8032 schematics on funet.
             *
             * Bit 0(8): 1=add 256 to screen start address ( 512 for 80-columns)
             * Bit 1(9): 1=add 512 to screen start address (1024 for 80-columns)
             * Bit 2(10): no connection (8296: connected)
             * Bit 3(11): no connection (8296: connected)
             * Bit 4(12): invert video signal
             *            8296: HRE HiRes mode, or connected via JU8/9(?)
             * Bit 5(13): use top half of 4K character generator
             * Bit 6: (no pin on the CRTC, video address is 14 bit only)
             * Bit 7: (no pin on the CRTC, video address is 14 bit only)
             */
            /* FIXME: check if the above also applies to CBM2 and all the other PET models */
            /* The CRTC loads its internal counter when it starts a new
             * frame. At this point the address/mode changes are evaluated now.
             */
            crtc.regs[CRTC_REG_DISPSTARTH] &= 0x3f;
            break;

        case CRTC_REG_DISPSTARTL:   /* R13  Address of first character: MA0-7 */
            break;

        case CRTC_REG_CURSORPOSH:   /* R14  Cursor location  HI -- unused */
            crtc.regs[CRTC_REG_CURSORPOSH] &= 0x3f;
#if 0
            crsr_set_dirty();
            crsrpos = ((crsrpos & 0x00ff) | ((value << 8) & 0x3f00)) & addr_mask;
            crsrrel = crsrpos - scrpos;
            crsr_set_dirty();
#endif
            break;

        case CRTC_REG_CURSORPOSL:   /* R15  Cursor location  LO -- unused */
#if 0
            crsr_set_dirty();
            crsrpos = ((crsrpos & 0x3f00) | (value & 0xff)) & addr_mask;
            crsrrel = crsrpos - scrpos;
            crsr_set_dirty();
#endif
            break;

        case CRTC_REG_LPENH:        /* R16 Light Pen HI -- read only */
        case CRTC_REG_LPENL:        /* R17 Light Pen LO -- read only */
            break;

        case 18:
        case 19:                    /* R18-9 Update address HI/LO (only 6545)  */
            break;

        default:
            break;
    }
}

uint8_t crtc_read(uint16_t addr)
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
        /* low on register-select permits reads from the status register */
        /* FIXME: implement bit 7 and bit 6 */
        return crtc_offscreen() ? 32 : 0;
    }

    /* internal registers */
    switch (crtc.regno) {
        case CRTC_REG_CURSORPOSH:
        case CRTC_REG_CURSORPOSL:
            return crtc.regs[crtc.regno];

        case CRTC_REG_LPENH:
        case CRTC_REG_LPENL:
            /* FIXME: Light Pen X,Y */
            return 0xff;

        default:
            return 0;       /* All the rest are write-only registers */
    }

    return 0;
}

uint8_t crtc_peek(uint16_t addr)
{
    return crtc_read(addr);
}

/* directly read a register (for monitor) */
uint8_t crtc_peek_register(uint8_t regno)
{
    return crtc.regs[regno];
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
