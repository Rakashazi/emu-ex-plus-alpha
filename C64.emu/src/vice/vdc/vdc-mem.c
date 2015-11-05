/*
 * vdc-mem.c - Memory interface for the MOS 8563 (VDC) emulation.
 *
 * Written by
 *  Markus Brenner   <markus@brenner.de>
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

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "types.h"
#include "vdc-mem.h"
#include "vdc.h"
#include "vdctypes.h"


#include "vdc-draw.h"

/*#define REG_DEBUG*/

/* bitmask to set the unused bits in returned register values */
static const BYTE regmask[38] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00,
    0xFC, 0xE0, 0x80, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F
};

static void vdc_write_data(void)
{
    int ptr;

    /* Update address.  */
    ptr = (vdc.regs[18] << 8) + vdc.regs[19];

    /* Write data byte to update address. */
    vdc.ram[ptr & vdc.vdc_address_mask] = vdc.regs[31];
#ifdef REG_DEBUG
    log_message(vdc.log, "STORE %04x %02x", ptr & vdc.vdc_address_mask,
                vdc.regs[31]);
#endif
    ptr += 1;
    vdc.regs[18] = (ptr >> 8) & 0xff;
    vdc.regs[19] = ptr & 0xff;
}


static void vdc_perform_fillcopy(void)
{
    int ptr, ptr2;
    int i;
    int blklen;

    /* Word count, # of bytes to copy */
    blklen = vdc.regs[30] ? vdc.regs[30] : 256;

    /* Update address.  */
    ptr = (vdc.regs[18] << 8) + vdc.regs[19];

    if (vdc.regs[24] & 0x80) { /* COPY flag */
        /* Block start address.  */
        ptr2 = (vdc.regs[32] << 8) + vdc.regs[33];
        for (i = 0; i < blklen; i++) {
            vdc.ram[(ptr + i) & vdc.vdc_address_mask]
                = vdc.ram[(ptr2 + i) & vdc.vdc_address_mask];
        }
        ptr2 += blklen;
        vdc.regs[31] = vdc.ram[(ptr2 - 1) & vdc.vdc_address_mask];
        vdc.regs[32] = (ptr2 >> 8) & 0xff;
        vdc.regs[33] = ptr2 & 0xff;
    } else { /* FILL */
#ifdef REG_DEBUG
        log_message(vdc.log, "Fill mem %04x, len %03x, data %02x",
                    ptr, blklen, vdc.regs[31]);
#endif
        for (i = 0; i < blklen; i++) {
            vdc.ram[(ptr + i) & vdc.vdc_address_mask] = vdc.regs[31];
        }
    }

    ptr = ptr + blklen;
    vdc.regs[18] = (ptr >> 8) & 0xff;
    vdc.regs[19] = ptr & 0xff;
}


/* VDC interface functions. */

/* Store a value in a VDC register. */
void vdc_store(WORD addr, BYTE value)
{
    BYTE oldval;

    /* WARNING: assumes `maincpu_rmw_flag' is 0 or 1.  */
    machine_handle_pending_alarms(maincpu_rmw_flag + 1);

    /* $d600 sets the internal vdc address pointer */
    if ((addr & 1) == 0) {  /* writing to $d600   */
#ifdef REG_DEBUG
        /*log_message(vdc.log, "STORE $D600 %02x", value);*/
#endif
        vdc.update_reg = value & 0x3f;
        /* VDC ignores values of top 2 bits */
        return;
    }

    /* otherwise we are writing to $d601
    save the old register value in case we need it for reference */
    oldval = vdc.regs[vdc.update_reg];

    /* $d601 sets the vdc register indexed by the update register pointer */
    vdc.regs[vdc.update_reg] = value;

#ifdef REG_DEBUG
    switch (vdc.update_reg) {
        case 10:
        case 11:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 36:
        case 37:
            break;
        default:
            log_message(vdc.log, "REG %02i VAL %02x CRL:%03i BH:%03i 0:%02X 1:%02X 2:%02X 3:%02X 4:%02X 5:%02X 6:%02X 7:%02X 8:%01X 9:%02X 12:%02X 13:%02X 20:%02X 21:%02X 22:%02X 23:%02X 24:%02X 25:%02X 26:%02X 27:%02X 34:%02X 35:%02X",
                        vdc.update_reg, value,
                        vdc.raster.current_line, vdc.border_height,
                        vdc.regs[0], vdc.regs[1], vdc.regs[2], vdc.regs[3],
                        vdc.regs[4], (vdc.regs[5] & 0x1f), vdc.regs[6], vdc.regs[7],
                        vdc.regs[8] & 0x03, vdc.regs[9] & 0x1f, /* vdc.regs[10] & 0x7f, vdc.regs[11] & 0x1f,  */
                        vdc.regs[12], vdc.regs[13], /* vdc.regs[14], vdc.regs[15], */
                        /* 16, 17, 18, 19 */
                        vdc.regs[20], vdc.regs[21], vdc.regs[22], vdc.regs[23] & 0x1f,
                        vdc.regs[24], vdc.regs[25], vdc.regs[26], vdc.regs[27],
                        vdc.regs[34], vdc.regs[35]
                        );
            break;
    }

    log_message(vdc.log, "REG %02i VAL %02x", vdc.update_reg, value);
#endif

    switch (vdc.update_reg) {
        case 0:                 /* R00  Horizontal total (characters + 1) */
            if (vdc.regs[0] != oldval) {
                if (vdc.regs[0] >= 120 && vdc.regs[0] <= 127) {
                    vdc.xchars_total = vdc.regs[0] + 1;
                    vdc_calculate_xsync();
                }
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 0 Horizontal Total:%02x", vdc.xchars_total);
#endif
            break;
        case 1:                 /* R01  Horizontal characters displayed */
            if (vdc.regs[1] != oldval) {
                if (vdc.regs[1] >= 8 && vdc.regs[1] <= VDC_SCREEN_MAX_TEXTCOLS) {
                    if (vdc.screen_text_cols != vdc.regs[1]) {
                        vdc.update_geometry = 1;
                    }
                }
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 1 Horizontal Displayed:%02x", vdc.regs[1]);
#endif
            break;

        case 2:                 /* R02  Horizontal Sync Position */
            if (vdc.regs[2] != oldval) {
                vdc.update_geometry = 1;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 2 Horizontal Sync Pos:%02x", vdc.regs[2]);
#endif
            break;

        case 3:                 /* R03  Horizontal/Vertical Sync widths */
            if ((vdc.regs[3] & 0xF0u) != (oldval & 0xF0u)) {
                vdc.update_geometry = 1;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 3 Hor/Ver Sync Width:%02x", vdc.regs[3]);
#endif
            break;

        case 4:                 /* R04  Vertical total (character) rows */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 4 Vertical Total :%02x", vdc.regs[4]);
#endif
            break;

        case 5:                 /* R05  Vertical total line adjust */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 5 Vertical Total Fine Adjust :%02x", vdc.regs[5]);
#endif
            break;

        case 6:                 /* R06  Number of display lines on screen */
            if (vdc.regs[6] != oldval) {
                vdc.update_geometry = 1;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 6 Vertical Displayed :%02x", vdc.regs[6]);
#endif
            break;

        case 7:                 /* R07  Vertical sync position */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 7 Vertical Sync Position :%02x", vdc.regs[7]);
#endif
            break;

        case 8:                 /* R08  unused: Interlace and Skew */
            vdc.update_geometry = 1;
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 8 Interlace:%02x");
#endif
            break;

        case 9:                 /* R09  Rasters between two display lines */
            if ((vdc.regs[9] & 0x1fu) != (oldval & 0x1fu)) {
                vdc.raster_ycounter_max = vdc.regs[9] & 0x1f;
                if (vdc.raster_ycounter_max < 16) {
                    vdc.bytes_per_char = 16;
                } else {
                    vdc.bytes_per_char = 32;
                }
            }
            /* set the attribute offset to 3 if reg[9] = 0 to correctly (?)
               emulate the 8x1 colour cell VDC trick (RFO FLI picture) */
            if (vdc.regs[9] & 0x1f) {
                vdc.attribute_offset = 0;
            } else {
                vdc.attribute_offset = 3;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "Character Total Vertical %i", vdc.regs[9]);
#endif
            break;

        case 10:                /* R10  Cursor Mode, Start Scan */
            break;

        case 11:                /* R11  Cursor End Scan */
            break;

        case 12:                /* R12  Display Start Address hi */
        case 13:                /* R13  Display Start Address lo */
            /* Screen address will be taken after last displayed line.  */
#ifdef REG_DEBUG
            log_message(vdc.log, "Update screen_adr: %x.", vdc.screen_adr);
#endif
            break;

        case 14:
        case 15:                /* R14-5 Cursor location HI/LO */
            vdc.crsrpos = ((vdc.regs[14] << 8) | vdc.regs[15]) & vdc.vdc_address_mask;
            break;

        case 16:                /* R16/17 Light Pen hi/lo */
        case 17:
            /* lightpen registers are read-only, so put the old value back */
            vdc.regs[vdc.update_reg] = oldval;
            break;

        case 18:                /* R18/19 Update Address hi/lo */
        case 19:
            vdc.update_adr = ((vdc.regs[18] << 8) | vdc.regs[19]) & vdc.vdc_address_mask;
            break;

        case 20:                /* R20/21 Attribute Start Address hi/lo */
        case 21:
            /* Attribute address will be taken after last displayed line.  */
#ifdef REG_DEBUG
            log_message(vdc.log, "Update attribute_adr: %x.", vdc.attribute_adr);
#endif
            break;

        case 22:                /* R22 Character Horizontal Size Control */
            /* TODO - changes to this register are real time, so need raster_changes() type call, but why bother... */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 22 only partially supported!");
#endif
            break;

        case 23:                /* R23 Vert. Character Pxl Spc */
#ifdef REG_DEBUG
            log_message(vdc.log, "Vert. Character Pxl Spc %i.",
                        vdc.regs[23] & 0x1f);
#endif
            break;

        case 24:
            if (vdc.regs[24] & 0x20) {
                vdc.attribute_blink = vdc.frame_counter & 16;
            } else {
                vdc.attribute_blink = vdc.frame_counter & 8;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "Vertical Smooth Scroll %i.", vdc.regs[24] & 0x1f);
            log_message(vdc.log, "Blink frequency: %s.",
                        (vdc.regs[24] & 0x20) ? "1/32" : "1/16");
            log_message(vdc.log, "Screen mode: %s.",
                        (vdc.regs[24] & 0x40) ? "reverse" : "normal");
#endif
            break;

        case 25:
            if ((vdc.regs[25] & 0x0Fu) != (oldval & 0x0Fu)) {
                /* Horizontal smooth scroll */
#ifdef ALLOW_UNALIGNED_ACCESS
                /* Smooth scroll behaviour differs between VDC versions */
                if (vdc.revision == VDC_REVISION_0) {
                    /* v0 VDC, incrementing HSS moves screen to the left, so xsmooth should decrease */
                    vdc.xsmooth = ((vdc.regs[22] >> 4) - (vdc.regs[25] & 0x0F)) & 0x0F;
                } else {
                    /* v1/2 VDC, incrementing HSS moves screen to the right */
                    vdc.xsmooth = (vdc.regs[25] & 0x0F);
                }
                vdc.raster.xsmooth = 0;
                /* Hack to get the line redrawn because we are not actually using the xsmooth in raster
                (so the xsmooth color is irrelevant, but changing it still forces a repaint of the line) */
                vdc.raster.xsmooth_color++;
#else
                vdc.xsmooth = (vdc.regs[22] >> 4) - ((vdc.regs[25] & 0x10) ? 1 : 0);
                vdc.raster.xsmooth = 0;
#endif
            }
            if ((vdc.regs[25] & 0x10u) != (oldval & 0x10u)) {
                /* Double-Pixel Mode */
                vdc.update_geometry = 1;
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "Video mode: %s.",
                        (vdc.regs[25] & 0x80) ? "bitmap" : "text");
            log_message(vdc.log, "Color source: %s.",
                        (vdc.regs[25] & 0x40) ? "attribute space" : "register 26");
            if (vdc.regs[25] & 0x20) {
                log_message(vdc.log, "Semi-Graphics Mode");
            }
            if (vdc.regs[25] & 0x10) {
                log_message(vdc.log, "Double-Pixel Mode unsupported!");
            }
#endif
            break;

        case 26:
            /* TODO - figure out if this was really needed. Doesn't seem to make a difference.
                Original Comment: repaint if something changes and we are not in graphics attribute mode */
            /* if ((vdc.regs[26] != oldval) && ((vdc.regs[25] & 0xC0) != 0xC0)) {
                vdc.force_repaint = 1;
            }   */
            if ((vdc.regs[26] & 0x0Fu) != (oldval & 0x0Fu)) {
                /* Background colour changes */
                /* TODO - calculate a real current horizontal raster position for this call (2nd value) */
                /* based on blacky_stardust calculations, calculating current_x_pixel should be like:
                current_x_pixel = pixels_per_line / (vdc.xsync_increment >> 16) * (current_cycle - vdc_line_start) */
                /* int current_x_pixel = 0; */
                /*        if (((vdc.xsync_increment * (maincpu_clk - vdc.vdc_line_start)) >> 16) != 0) {
                            current_x_pixel = (long long) (vdc.regs[0] + 1) * ((vdc.regs[22] >> 4) + 1) / ((vdc.xsync_increment * (maincpu_clk - vdc.vdc_line_start)) >> 16);
                        }
            */
                /* TODO get rid of this when it works properly */
                /*     fprintf(stderr, "current_x_pixel=%1i\n", current_x_pixel);
                     raster_changes_border_add_int(&vdc.raster,
                     current_x_pixel,
                     (int*)&vdc.raster.border_color,
                     (vdc.regs[26] & 0x0F));
                     vdc.raster.xsmooth_color = vdc.regs[26] & 0x0F; */
                /* Set the xsmooth area too for the 0-7pixel gap between border & foreground */

                vdc.raster.border_color = (vdc.regs[26] & 0x0F);
            }
#ifdef REG_DEBUG
            log_message(vdc.log, "Color register %x.", vdc.regs[26]);
#endif
            break;

        case 27:                /* R27  Row/Adrs. Increment */
            /* We need to redraw the current line if this changes,
            as cache will be wrong. Uses xsmooth_color hack (see reg 25) */
            vdc.raster.xsmooth_color++;
#ifdef REG_DEBUG
            log_message(vdc.log, "Row/Adrs. Increment %i.", vdc.regs[27]);
#endif
            break;

        case 28:
            vdc.chargen_adr = ((vdc.regs[28] << 8) & 0xe000) & vdc.vdc_address_mask;
#ifdef REG_DEBUG
            log_message(vdc.log, "Update chargen_adr: %x.", vdc.chargen_adr);
#endif
            break;

        case 30:                /* Word Count */
            vdc_perform_fillcopy();
            break;

        case 31:                /* Data */
            vdc_write_data();
            break;

        case 32:                /* R32/33 Block Start Address hi/lo */
        case 33:
            break;

        case 34:                /* R34  Display Enable Begin */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 34 unsupported!");
#endif
            break;

        case 35:                /* R35  Display Enable End */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 35 unsupported!");
#endif
            break;

        case 36:                /* R36  DRAM Refresh Rate */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 36 unsupported!");
#endif
            break;

        case 37:                /* R37  Vertical/Horizontal Sync Polarity on 8568 (128DCR) only */
#ifdef REG_DEBUG
            log_message(vdc.log, "REG 37 unsupported!");
#endif
            break;
    }
}


BYTE vdc_read(WORD addr)
{
    BYTE retval;
    int ptr;

    machine_handle_pending_alarms(0);

    if (addr & 1) { /* read $d601 (and mirrors $d603/5/7....$d6ff)  */
        /*log_message(vdc.log, "read: addr = %x", addr);*/

        if (vdc.update_reg == 31) {
            retval = vdc.ram[((vdc.regs[18] << 8) + vdc.regs[19]) & vdc.vdc_address_mask];
            ptr = (1 + vdc.regs[19] + (vdc.regs[18] << 8))
                  & vdc.vdc_address_mask;
            vdc.regs[18] = (ptr >> 8) & 0xff;
            vdc.regs[19] = ptr & 0xff;
            return retval;
        }

        /* reg28 bit 4 is how much ram is installed. Technically this is only half-right as this bit is not set for 16k setups upgraded to 64k */
        if (vdc.update_reg == 28) {
            if (vdc.vdc_address_mask == 0xffff) {
                return vdc.regs[28] | 0x1f;
            } else {
                return vdc.regs[28] | 0x0f;
            }
        }

        /* reset light pen flag if either light pen position register is read */
        if (vdc.update_reg == 16 || vdc.update_reg == 17) {
            vdc.light_pen.triggered = 0;
        }

        if (vdc.update_reg < 38) {
            return (vdc.regs[vdc.update_reg] | regmask[vdc.update_reg]);
        }

        return 0xff; /* return 0xFF for invalid register numbers */
    } else { /* read $d600 (and mirrors $d602/4/6....$d6fe) */
        /* NOTE - Status ($80) is currently unsupported and always returns 1 (ready) */
        retval = 0x80 | vdc.revision;

        /* Emulate lightpen flag. */
        if (vdc.light_pen.triggered) {
            retval |= 0x40;
        }

        /* Emulate vblank bit.  */
        if ((vdc.raster.current_line <= vdc.border_height) || (vdc.raster.current_line > (vdc.border_height + vdc.screen_ypix))) {
            retval |= 0x20;
        }

        return retval;
    }
}


BYTE vdc_peek(WORD addr)    /* No sidefx read of external VDC registers */
{
    if (addr & 1) { /* read $d601 (and mirrors $d603/5/7....$d6ff)  */

        /* Read VDCs RAM without incrementing the pointer */
        if (vdc.update_reg == 31) {
            return vdc.ram[((vdc.regs[18] << 8) + vdc.regs[19]) & vdc.vdc_address_mask];
        }

        /* Make sure light pen flag is not altered if either light pen position register is read */
        if (vdc.update_reg == 16 || vdc.update_reg == 17) {
            return (vdc.regs[vdc.update_reg]);
        }

        return vdc_read(addr);  /* Use existing read function for rest */
    } else {    /* read $d600 (and mirrors $d602/4/6....$d6fe) */
        /* read of $d600 is non destructive, so just use the value calculated in vdc_read() */
        return vdc_read(addr);
    }
}


BYTE vdc_ram_read(WORD addr)
{
    return vdc.ram[addr & vdc.vdc_address_mask];
}

void vdc_ram_store(WORD addr, BYTE value)
{
    vdc.ram[addr & vdc.vdc_address_mask] = value;
}


int vdc_dump(void)
{
    unsigned int r, c, regnum=0;

    mon_out("VDC Revision: %d\n", vdc.revision);
    mon_out("Vertical Blanking Period: ");
    mon_out(((vdc.raster.current_line <= vdc.border_height) || (vdc.raster.current_line > (vdc.border_height + vdc.screen_ypix))) ? "Yes" : "No");
    mon_out("\nLight Pen Triggered: ");
    mon_out(vdc.light_pen.triggered ? "Yes" : "No");
    mon_out("\nActive Register: %d\n", vdc.update_reg);
  
    /* Dump the internal VDC registers */
    mon_out("\nVDC Internal Registers:\n");
    for (r = 0; r < 5; r++) {
        mon_out("%02x: ", regnum);
        for (c = 0; c < 8; c++) {
            if (regnum <= 37)
            {
                mon_out("%02x ", vdc.regs[regnum] | regmask[regnum]);
            }
            regnum++;
            if (c == 3)
            {
                mon_out(" ");
            }
        }
        mon_out("\n");
    }

    /*
      TODO:
      Add STATUS when it's actually supported
      Expand on VDC internal register settings
      Current memory address etc.
    */
    return 0;
}
