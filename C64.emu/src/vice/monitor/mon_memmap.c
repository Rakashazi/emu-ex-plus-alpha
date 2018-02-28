/*
 * mon_memmap.c - The VICE built-in monitor, memmap/cpuhistory functions.
 *
 * Written by
 *  Hannu Nuotio <nojoopa@users.sourceforge.net>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "lib.h"
#include "machine.h"
#include "mon_disassemble.h"
#include "mon_memmap.h"
#include "monitor.h"
#include "montypes.h"
#include "screenshot.h"
#include "types.h"


/* Globals */

BYTE memmap_state = 0;

#ifdef FEATURE_CPUMEMHISTORY

/* Defines */

#define CPUHISTORY_SIZE 4096

#define MEMMAP_SIZE 0x10000
#define MEMMAP_PICX 0x100
#define MEMMAP_PICY 0x100

#define OP_JSR 0x20
#define OP_RTI 0x40
#define OP_RTS 0x60

#define BAD_ADDR (new_addr(e_invalid_space, 0))

/* Types */

#define MEMMAP_ELEM WORD

struct cpuhistory_s {
   WORD addr;
   BYTE op;
   BYTE p1;
   BYTE p2;
   BYTE reg_a;
   BYTE reg_x;
   BYTE reg_y;
   BYTE reg_sp;
   WORD reg_st;
};
typedef struct cpuhistory_s cpuhistory_t;

/* CPU history variables */
static cpuhistory_t cpuhistory[CPUHISTORY_SIZE];
static int cpuhistory_i = 0;

void monitor_cpuhistory_store(unsigned int addr, unsigned int op,
                              unsigned int p1, unsigned int p2,
                              BYTE reg_a,
                              BYTE reg_x,
                              BYTE reg_y,
                              BYTE reg_sp,
                              unsigned int reg_st)
{
    ++cpuhistory_i;
    cpuhistory_i &= (CPUHISTORY_SIZE - 1);
    cpuhistory[cpuhistory_i].addr = addr;
    cpuhistory[cpuhistory_i].op = op;
    cpuhistory[cpuhistory_i].p1 = p1;
    cpuhistory[cpuhistory_i].p2 = p2;
    cpuhistory[cpuhistory_i].reg_a = reg_a;
    cpuhistory[cpuhistory_i].reg_x = reg_x;
    cpuhistory[cpuhistory_i].reg_y = reg_y;
    cpuhistory[cpuhistory_i].reg_sp = reg_sp;
    cpuhistory[cpuhistory_i].reg_st = reg_st;
}

void monitor_cpuhistory_fix_p2(unsigned int p2)
{
    cpuhistory[cpuhistory_i].p2 = p2;
}

void mon_cpuhistory(int count)
{
    BYTE op, p1, p2, p3 = 0;
    MEMSPACE mem;
    WORD loc, addr;
    int hex_mode = 1;
    const char *dis_inst;
    unsigned opc_size;
    int i, pos;

    if ((count < 1) || (count > CPUHISTORY_SIZE)) {
        count = CPUHISTORY_SIZE;
    }

    pos = (cpuhistory_i + 1 - count) & (CPUHISTORY_SIZE - 1);

    for (i = 0; i < count; ++i) {
        addr = cpuhistory[pos].addr;
        op = cpuhistory[pos].op;
        p1 = cpuhistory[pos].p1;
        p2 = cpuhistory[pos].p2;

        mem = addr_memspace(addr);
        loc = addr_location(addr);

        dis_inst = mon_disassemble_to_string_ex(mem, loc, op, p1, p2, p3, hex_mode,
                                                &opc_size);

        /* Print the disassembled instruction */
        mon_out("%04x  %-30s - A:%02x X:%02x Y:%02x SP:%02x %c%c-%c%c%c%c%c\n",
            loc, dis_inst,
            cpuhistory[pos].reg_a, cpuhistory[pos].reg_x, cpuhistory[pos].reg_y, cpuhistory[pos].reg_sp,
            ((cpuhistory[pos].reg_st & (1 << 7)) != 0) ? 'N' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 6)) != 0) ? 'V' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 4)) != 0) ? 'B' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 3)) != 0) ? 'D' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 2)) != 0) ? 'I' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 1)) != 0) ? 'Z' : ' ',
            ((cpuhistory[pos].reg_st & (1 << 0)) != 0) ? 'C' : ' '
            );

        pos = (pos + 1) & (CPUHISTORY_SIZE - 1);
    }
}


/* memmap variables */
static MEMMAP_ELEM *mon_memmap = NULL;
static int mon_memmap_size;
static int mon_memmap_picx;
static int mon_memmap_picy;
static unsigned int mon_memmap_mask;


void mon_memmap_zap(void)
{
    memset(mon_memmap, 0, mon_memmap_size * sizeof(MEMMAP_ELEM));
}

void mon_memmap_show(int mask, MON_ADDR start_addr, MON_ADDR end_addr)
{
    unsigned int addr;
    MEMMAP_ELEM b;
    const char *line_fmt = NULL;

    if (start_addr == BAD_ADDR) {
        start_addr = 0;
    }

    if (end_addr == BAD_ADDR) {
        end_addr = mon_memmap_mask;
    }

    if (start_addr > end_addr) {
        start_addr = end_addr;
    }

    if (machine_class == VICE_MACHINE_C64DTV) {
        mon_out("  addr: IO  ROM RAM\n");
        line_fmt = "%06x: %c%c%c %c%c%c %c%c%c\n";
    } else {
        mon_out("addr: IO  ROM RAM\n");
        line_fmt = "%04x: %c%c%c %c%c%c %c%c%c\n";
    }

    for (addr = start_addr; addr <= end_addr; ++addr) {
        b = mon_memmap[addr];

        if ((b & mask) == 0) {
            continue;
        }

        mon_out(line_fmt, addr,
                (b & MEMMAP_I_O_R) ? 'r' : '-',
                (b & MEMMAP_I_O_W) ? 'w' : '-',
                (b & MEMMAP_I_O_X) ? 'x' : '-',
                (b & MEMMAP_ROM_R) ? 'r' : '-',
                (b & MEMMAP_ROM_W) ? 'w' : '-',
                (b & MEMMAP_ROM_X) ? 'x' : '-',
                (b & MEMMAP_RAM_R) ? 'r' : '-',
                (b & MEMMAP_RAM_W) ? 'w' : '-',
                (b & MEMMAP_RAM_X) ? 'x' : '-');
    }
}

void monitor_memmap_store(unsigned int addr, unsigned int type)
{
    BYTE op = cpuhistory[cpuhistory_i].op;

    if (memmap_state & MEMMAP_STATE_IN_MONITOR) {
        return;
    }

    /* Ignore reg_pc+2 reads on branches & JSR
       and return address read on RTS */
    if (type & (MEMMAP_ROM_R | MEMMAP_RAM_R)
      && (((op & 0x1f) == 0x10) || (op == OP_JSR)
      || ((op == OP_RTS) && ((addr > 0x1ff) || (addr < 0x100))))) {
        return;
    }

    mon_memmap[addr & mon_memmap_mask] |= type;
}


void mon_memmap_save(const char *filename, int format)
{
    const char *drvname;
    int i;
    BYTE mon_memmap_palette[256 * 3];
    BYTE *memmap_bitmap = NULL;

    switch (format) {
        case 1:
            drvname = "PCX";
            break;
        case 2:
            drvname = "PNG";
            break;
        case 3:
            drvname = "GIF";
            break;
        case 4:
            drvname = "IFF";
            break;
        default:
            drvname = "BMP";
            break;
    }

    /* build palette */
    for (i = 0; i < 256; ++i) {
        mon_memmap_palette[i * 3 + 0] = (i & (MEMMAP_RAM_W)) ? 0x80 : 0 + (i & (MEMMAP_ROM_W)) ? 0x60 : 0 + (i & (MEMMAP_I_O_W)) ? 0x1f : 0;
        mon_memmap_palette[i * 3 + 1] = (i & (MEMMAP_RAM_X)) ? 0x80 : 0 + (i & (MEMMAP_ROM_X)) ? 0x60 : 0 + (i & (MEMMAP_I_O_W | MEMMAP_I_O_X)) ? 0x1f : 0;
        mon_memmap_palette[i * 3 + 2] = (i & (MEMMAP_RAM_R)) ? 0x80 : 0 + (i & (MEMMAP_ROM_R)) ? 0x60 : 0 + (i & (MEMMAP_I_O_X)) ? 0x1f : 0;
    }

    /* make a 8bpp bitmap of the memmap */
    memmap_bitmap = lib_malloc(mon_memmap_size);

    for (i = 0; i < mon_memmap_size; ++i) {
        /* use same bit for I/O R and X */
        memmap_bitmap[i] = (mon_memmap[i] & 0xffu) | ((mon_memmap[i] & MEMMAP_I_O_R) >> 2);
    }

    if (memmap_screenshot_save(drvname, filename, mon_memmap_picx, mon_memmap_picy, memmap_bitmap, mon_memmap_palette)) {
        mon_out("Failed.\n");
    }

    lib_free(memmap_bitmap);
}

void mon_memmap_init(void)
{
    mon_memmap_picx = MEMMAP_PICX;

    if (machine_class == VICE_MACHINE_C64DTV) {
        mon_memmap_picy = 0x2000;
    } else {
        mon_memmap_picy = 0x100;
    }

    mon_memmap_size = mon_memmap_picx * mon_memmap_picy;
    mon_memmap = lib_malloc(mon_memmap_size * sizeof(MEMMAP_ELEM));
    mon_memmap_mask = mon_memmap_size - 1;

    mon_memmap_zap();
}

void mon_memmap_shutdown(void)
{
    lib_free(mon_memmap);
    mon_memmap = NULL;
}


#else /* !FEATURE_CPUMEMHISTORY */

/* stubs */
static void mon_memmap_stub(void)
{
    mon_out("Disabled. configure with --enable-memmap and recompile.\n");
}

void mon_cpuhistory(int count)
{
    mon_memmap_stub();
}

void mon_memmap_zap(void)
{
    mon_memmap_stub();
}

void mon_memmap_show(int mask, MON_ADDR start_addr, MON_ADDR end_addr)
{
    mon_memmap_stub();
}

void mon_memmap_save(const char* filename, int format)
{
    mon_memmap_stub();
}

void mon_memmap_init(void)
{
}

void mon_memmap_shutdown(void)
{
}

#endif
