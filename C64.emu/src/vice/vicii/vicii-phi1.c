/*
 * vicii-phi1.c - Memory interface for the MOS6569 (VIC-II) emulation,
 *                PHI1 support.
 *
 * Written by
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

#include "maincpu.h"
#include "types.h"
#include "vicii-phi1.h"
#include "viciitypes.h"


inline static BYTE gfx_data_illegal_bitmap(unsigned int num)
{
    if (vicii.idle_state) {
        return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x39ff];
    } else {
        unsigned int j;

        j = ((vicii.memptr << 3) + vicii.raster.ycounter + num * 8);

        if (j & 0x1000) {
            return vicii.bitmap_high_ptr[j & 0x9ff];
        } else {
            return vicii.bitmap_low_ptr[j & 0x9ff];
        }
    }
}

inline static BYTE gfx_data_hires_bitmap(unsigned int num)
{
    if (vicii.idle_state) {
        return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3fff];
    } else {
        unsigned int j;

        j = ((vicii.memptr << 3) + vicii.raster.ycounter + num * 8);

        if (j & 0x1000) {
            return vicii.bitmap_high_ptr[j & 0xfff];
        } else {
            return vicii.bitmap_low_ptr[j & 0xfff];
        }
    }
}

inline static BYTE gfx_data_extended_text(unsigned int num)
{
    if (vicii.idle_state) {
        return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x39ff];
    } else {
        return vicii.chargen_ptr[(vicii.vbuf[num] & 0x3f) * 8
                                 + vicii.raster.ycounter];
    }
}

inline static BYTE gfx_data_normal_text(unsigned int num)
{
    if (vicii.idle_state) {
        return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3fff];
    } else {
        return vicii.chargen_ptr[vicii.vbuf[num] * 8
                                 + vicii.raster.ycounter];
    }
}

static BYTE gfx_data(unsigned int num)
{
    BYTE value = 0;

    switch (vicii.raster.video_mode) {
        case VICII_NORMAL_TEXT_MODE:
        case VICII_MULTICOLOR_TEXT_MODE:
            value = gfx_data_normal_text(num);
            break;
        case VICII_HIRES_BITMAP_MODE:
        case VICII_MULTICOLOR_BITMAP_MODE:
            value = gfx_data_hires_bitmap(num);
            break;
        case VICII_EXTENDED_TEXT_MODE:
        case VICII_ILLEGAL_TEXT_MODE:
            value = gfx_data_extended_text(num);
            break;
        case VICII_ILLEGAL_BITMAP_MODE_1:
        case VICII_ILLEGAL_BITMAP_MODE_2:
            value = gfx_data_illegal_bitmap(num);
            break;
        default:
            value = vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3fff];
    }

    return value;
}

static BYTE idle_gap(void)
{
    return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3fff];
}

static BYTE sprite_data(unsigned int num)
{
    return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3fff];
}

static BYTE sprite_pointer(unsigned int num)
{
    WORD offset;

    offset = ((vicii.regs[0x18] & 0xf0) << 6) + 0x3f8 + num;

    return vicii.ram_base_phi1[vicii.vbank_phi1 + offset];
}

static BYTE refresh_counter(unsigned int num)
{
    BYTE offset;

    offset = 0xff - (VICII_RASTER_Y(maincpu_clk) * 5 + num);

    return vicii.ram_base_phi1[vicii.vbank_phi1 + 0x3f00 + offset];
}

inline static BYTE phi1_pal(unsigned int cycle)
{
    switch (cycle) {
        case 0:
            return sprite_pointer(3);
        case 1:
            return sprite_data(3);
        case 2:
            return sprite_pointer(4);
        case 3:
            return sprite_data(4);
        case 4:
            return sprite_pointer(5);
        case 5:
            return sprite_data(5);
        case 6:
            return sprite_pointer(6);
        case 7:
            return sprite_data(6);
        case 8:
            return sprite_pointer(7);
        case 9:
            return sprite_data(7);

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            return refresh_counter(cycle - 10);

        default: /* 15 .. 54 */
            return gfx_data(cycle - 15);

        case 55:
        case 56:
            return idle_gap();

        case 57:
            return sprite_pointer(0);
        case 58:
            return sprite_data(0);
        case 59:
            return sprite_pointer(1);
        case 60:
            return sprite_data(1);
        case 61:
            return sprite_pointer(2);
        case 62:
            return sprite_data(2);
    }
}

inline static BYTE phi1_ntsc_old(unsigned int cycle)
{
    switch (cycle) {
        case 0:
            return sprite_pointer(3);
        case 1:
            return sprite_data(3);
        case 2:
            return sprite_pointer(4);
        case 3:
            return sprite_data(4);
        case 4:
            return sprite_pointer(5);
        case 5:
            return sprite_data(5);
        case 6:
            return sprite_pointer(6);
        case 7:
            return sprite_data(6);
        case 8:
            return sprite_pointer(7);
        case 9:
            return sprite_data(7);

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            return refresh_counter(cycle - 10);

        default: /* 15 .. 54 */
            return gfx_data(cycle - 15);

        case 55:
        case 56:
        case 57:
            return idle_gap();

        case 58:
            return sprite_pointer(0);
        case 59:
            return sprite_data(0);
        case 60:
            return sprite_pointer(1);
        case 61:
            return sprite_data(1);
        case 62:
            return sprite_pointer(2);
        case 63:
            return sprite_data(2);
    }
}

inline static BYTE phi1_ntsc(unsigned int cycle)
{
    switch (cycle) {
        case 0:
            return sprite_data(3);
        case 1:
            return sprite_pointer(4);
        case 2:
            return sprite_data(4);
        case 3:
            return sprite_pointer(5);
        case 4:
            return sprite_data(5);
        case 5:
            return sprite_pointer(6);
        case 6:
            return sprite_data(6);
        case 7:
            return sprite_pointer(7);
        case 8:
            return sprite_data(7);

        case 9:
            return idle_gap();

        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
            return refresh_counter(cycle - 10);

        default: /* 15 .. 54 */
            return gfx_data(cycle - 15);

        case 55:
        case 56:
        case 57:
            return idle_gap();

        case 58:
            return sprite_pointer(0);
        case 59:
            return sprite_data(0);
        case 60:
            return sprite_pointer(1);
        case 61:
            return sprite_data(1);
        case 62:
            return sprite_pointer(2);
        case 63:
            return sprite_data(2);
        case 64:
            return sprite_pointer(3);
    }
}

BYTE vicii_read_phi1_lowlevel(void)
{
    unsigned int cycle = VICII_RASTER_CYCLE(maincpu_clk);

    switch (vicii.cycles_per_line) {
        case 63:
        default:
            return phi1_pal(cycle);
        case 64:
            return phi1_ntsc_old(cycle);
        case 65:
            return phi1_ntsc(cycle);
    }
}

BYTE vicii_read_phi1(void)
{
    vicii_handle_pending_alarms(0);

    return vicii_read_phi1_lowlevel();
}
