/*
 * vicii-fetch.c - Phi1/Phi2 data fetch for the VIC-II emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
 *
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "c64cart.h"
#include "c64cartmem.h"
#include "debug.h"
#include "maincpu.h"
#include "mem.h"
#include "types.h"
#include "vicii-chip-model.h"
#include "vicii-fetch.h"
#include "viciitypes.h"

#ifdef DEBUG
#include "log.h"
#endif

/*-----------------------------------------------------------------------*/

inline static BYTE fetch_phi1(int addr)
{
    BYTE *p;

    addr = ((addr + vicii.vbank_phi1) & vicii.vaddr_mask_phi1) | vicii.vaddr_offset_phi1;

    if (export.ultimax_phi1) {
        BYTE value;
        if (ultimax_romh_phi1_read((WORD)(0x1000 + (addr & 0xfff)), &value)) {
            if ((addr & 0x3fff) >= 0x3000) {
                return value;
            } else {
                p = vicii.ram_base_phi1 + addr;
                return *p;
            }
        }
    }

    if ((addr & vicii.vaddr_chargen_mask_phi1) == vicii.vaddr_chargen_value_phi1) {
        p = mem_chargen_rom_ptr + (addr & 0xfff);
    } else {
        p = vicii.ram_base_phi1 + addr;
    }
    return *p;
}

inline static BYTE fetch_phi2(int addr)
{
    BYTE *p;

    addr = ((addr + vicii.vbank_phi2) & vicii.vaddr_mask_phi2) | vicii.vaddr_offset_phi2;

    if (export.ultimax_phi2) {
        BYTE value;
        if (ultimax_romh_phi2_read((WORD)(0x1000 + (addr & 0xfff)), &value)) {
            if ((addr & 0x3fff) >= 0x3000) {
                return value;
            } else {
                p = vicii.ram_base_phi2 + addr;
                return *p;
            }
        }
    }

    if ((addr & vicii.vaddr_chargen_mask_phi2) == vicii.vaddr_chargen_value_phi2) {
        p = mem_chargen_rom_ptr + (addr & 0xfff);
    } else {
        p = vicii.ram_base_phi2 + addr;
    }

    return *p;
}

/*-----------------------------------------------------------------------*/

inline static int check_sprite_dma(int i)
{
    return vicii.sprite_dma & (1 << i);
}

inline static void sprite_dma_cycle_0(int i)
{
    BYTE sprdata = vicii.last_bus_phi2;

    if (check_sprite_dma(i)) {
        if (!vicii.prefetch_cycles) {
            sprdata = fetch_phi2((vicii.sprite[i].pointer << 6) + vicii.sprite[i].mc);
        }

        vicii.sprite[i].mc++;
        vicii.sprite[i].mc &= 0x3f;

#ifdef DEBUG
        if (debug.maincpu_traceflg && (vicii.sprite_dma & (1 << i))) {
            log_debug("SDMA0 in cycle %i   %d", vicii.raster_cycle, maincpu_clk);
        }
#endif
    }

    vicii.sprite[i].data &= 0x00ffff;
    vicii.sprite[i].data |= sprdata << 16;
}

inline static void sprite_dma_cycle_2(int i)
{
    BYTE sprdata = vicii.last_bus_phi2;

    if (check_sprite_dma(i)) {
        if (!vicii.prefetch_cycles) {
            sprdata = fetch_phi2((vicii.sprite[i].pointer << 6) + vicii.sprite[i].mc);
        }

        vicii.sprite[i].mc++;
        vicii.sprite[i].mc &= 0x3f;

#ifdef DEBUG
        if (debug.maincpu_traceflg && (vicii.sprite_dma & (1 << i))) {
            log_debug("SDMA2 in cycle %i   %d", vicii.raster_cycle, maincpu_clk);
        }
#endif
    }

    vicii.sprite[i].data &= 0xffff00;
    vicii.sprite[i].data |= sprdata;
}

/*-----------------------------------------------------------------------*/

inline static int v_fetch_addr(int offset)
{
    return ((vicii.regs[0x18] & 0xf0) << 6) + offset;
}

inline static WORD g_fetch_addr(BYTE mode)
{
    WORD a;

    /* BMM */
    if (mode & 0x20) {
        a = (vicii.vc << 3) | vicii.rc;
        a |= (vicii.regs[0x18] & 0x8) << 10;
    } else {
        a = (vicii.vbuf[vicii.vmli] << 3) | vicii.rc;
        a |= (vicii.regs[0x18] & 0xe) << 10;
    }

    /* ECM */
    if (mode & 0x40) {
        a &= 0x39ff;
    }

    return a;
}

inline static int is_char_rom(WORD addr)
{
    addr = ((addr + vicii.vbank_phi1) & vicii.vaddr_mask_phi1) | vicii.vaddr_offset_phi1;
    return (addr & vicii.vaddr_chargen_mask_phi1) == vicii.vaddr_chargen_value_phi1;
}

/*-----------------------------------------------------------------------*/

void vicii_fetch_matrix(void)
{
    if (vicii.prefetch_cycles) {
        vicii.vbuf[vicii.vmli] = 0xff;
        vicii.cbuf[vicii.vmli] = vicii.ram_base_phi2[reg_pc] & 0xf;
    } else {
        vicii.vbuf[vicii.vmli] = fetch_phi2(v_fetch_addr(vicii.vc));
        vicii.cbuf[vicii.vmli] = mem_color_ram_vicii[vicii.vc];
    }
}

BYTE vicii_fetch_refresh(void)
{
    return fetch_phi1(0x3f00 + vicii.refresh_counter--);
}

BYTE vicii_fetch_idle(void)
{
    return fetch_phi1(0x3fff);
}

BYTE vicii_fetch_idle_gfx(void)
{
    BYTE data;
    BYTE reg11;

    if (vicii.color_latency) {
        reg11 = vicii.regs[0x11];
    } else {
        reg11 = vicii.reg11_delay;
    }

    if (reg11 & 0x40) {
        data = fetch_phi1(0x39ff);
    } else {
        data = fetch_phi1(0x3fff);
    }
    vicii.gbuf = data;

    return data;
}

BYTE vicii_fetch_graphics(void)
{
    BYTE data;
    WORD addr;

    if (vicii.color_latency) {
        addr = g_fetch_addr((BYTE)(vicii.regs[0x11] | (vicii.reg11_delay & 0x20)));

        if ((vicii.regs[0x11] ^ vicii.reg11_delay) & 0x20) {
            /* 6569 fetch magic! (FIXME: proper explanation)
               When changing from RAM to (char)ROM fetches, the LSB of the
               fetch address is (apparently) latched using the mode from
               the previous cycle, and the upper bits come from the current
               mode, due to ...

               TODO: test with $d018 splits and fix above test if needed.
            */
            WORD addr_from, addr_to;

            addr_from = g_fetch_addr(vicii.reg11_delay);
            addr_to = g_fetch_addr(vicii.regs[0x11]);

            if (!is_char_rom(addr_from) && is_char_rom(addr_to)) {
                addr = (addr_from & 0xff) | (addr_to & 0x3f00);
            }
        }
    } else {
        addr = g_fetch_addr(vicii.reg11_delay);
    }

    data = fetch_phi1(addr);
    vicii.gbuf = data;

    vicii.vmli++;

    vicii.vc++;
    vicii.vc &= 0x3ff;

    return data;
}

BYTE vicii_fetch_sprite_pointer(int i)
{
    vicii.sprite[i].pointer = fetch_phi1(v_fetch_addr(0x3f8 + i));

    return vicii.sprite[i].pointer;
}

BYTE vicii_fetch_sprite_dma_1(int i)
{
    BYTE sprdata;

    if (check_sprite_dma(i)) {
        sprdata = fetch_phi1((vicii.sprite[i].pointer << 6) + vicii.sprite[i].mc);

        vicii.sprite[i].mc++;
        vicii.sprite[i].mc &= 0x3f;
    } else {
        sprdata = vicii_fetch_idle();
    }

    vicii.sprite[i].data &= 0xff00ff;
    vicii.sprite[i].data |= sprdata << 8;

    return sprdata;
}

int vicii_check_sprite_ba(unsigned int cycle_flags)
{
    if (vicii.sprite_dma & cycle_get_sprite_ba_mask(cycle_flags)) {
        return 1;
    }
    return 0;
}

void vicii_fetch_sprites(unsigned int cycle_flags)
{
    int s;

    if (cycle_is_sprite_ptr_dma0(cycle_flags)) {
        s = cycle_get_sprite_num(cycle_flags);
        sprite_dma_cycle_0(s);
    }

    if (cycle_is_sprite_dma1_dma2(cycle_flags)) {
        s = cycle_get_sprite_num(cycle_flags);
        sprite_dma_cycle_2(s);
    }
}
