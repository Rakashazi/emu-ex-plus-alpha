/*
 * vicii-chip-model.c - Chip model definitions for the VIC-II emulation.
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

#include "log.h"
#include "types.h"
#include "vicii.h"
#include "vicii-chip-model.h"
#include "vicii-resources.h"
#include "viciitypes.h"

/*
 * NOTE:
 *  This file deliberately breaks the naming convention for #defines by
 *  using CamelCase.  The reason for this is to make cycle_tab_* somewhat
 *  compact and readable.
 */

struct ViciiCycle {
    int cycle;
    int xpos;
    int visible;
    int fetch;
    int ba;
    int flags;
};

/* Common */
#define None            0

/* Cycle */
#define Phi1(x)         (x)
#define Phi2(x)         ((x) | 0x80)

/* Visible */
#define Vis(x)          ((x) | 0x80)
#define IsVis(x)        ((x) & 0x80)
#define GetVis(x)       ((x) & 0x7f)

/* Fetch */
#define FetchType_M     0xf00
#define FetchSprNum_M   0x007
#define SprPtr(x)       (0x100 | (x))
#define SprDma0(x)      (0x200 | (x))
#define SprDma1(x)      (0x300 | (x))
#define SprDma2(x)      (0x400 | (x))
#define Refresh         0x500
#define FetchG          0x600
#define FetchC          0x700
#define Idle            0x800


/* BA */
#define BaFetch         0x100
#define BaSpr_M         0xff
#define BaSpr1(x)       (0x000 | (1 << (x)))
#define BaSpr2(x, y)    (0x000 | (1 << (x)) | (1 << (y)))
#define BaSpr3(x, y, z) (0x000 | (1 << (x)) | (1 << (y)) | (1 << (z)))


/* Flags */
#define UpdateMcBase    0x001
#define ChkSprExp       0x002
#define ChkSprDma       0x004
#define ChkSprDisp      0x008
#define ChkSprCrunch    0x010
#define ChkBrdL1        0x020
#define ChkBrdL0        0x040
#define ChkBrdR0        0x080
#define ChkBrdR1        0x100
#define UpdateVc        0x200
#define UpdateRc        0x400

struct ViciiChipModel {
    char *name;
    int cycles_per_line;
    struct ViciiCycle *cycle_tab;
    int num_raster_lines;
    int color_latency;
    int lightpen_old_irq_mode;
    int new_luminances;
};


/* PAL */
static struct ViciiCycle cycle_tab_pal[] = {
    { Phi1(1),  0x194, None,    SprPtr(3),  BaSpr2(3, 4),    None                 },
    { Phi2(1),  0x198, None,    SprDma0(3), BaSpr2(3, 4),    None                 },
    { Phi1(2),  0x19c, None,    SprDma1(3), BaSpr3(3, 4, 5), None                 },
    { Phi2(2),  0x1a0, None,    SprDma2(3), BaSpr3(3, 4, 5), None                 },
    { Phi1(3),  0x1a4, None,    SprPtr(4),  BaSpr2(4, 5),    None                 },
    { Phi2(3),  0x1a8, None,    SprDma0(4), BaSpr2(4, 5),    None                 },
    { Phi1(4),  0x1ac, None,    SprDma1(4), BaSpr3(4, 5, 6), None                 },
    { Phi2(4),  0x1b0, None,    SprDma2(4), BaSpr3(4, 5, 6), None                 },
    { Phi1(5),  0x1b4, None,    SprPtr(5),  BaSpr2(5, 6),    None                 },
    { Phi2(5),  0x1b8, None,    SprDma0(5), BaSpr2(5, 6),    None                 },
    { Phi1(6),  0x1bc, None,    SprDma1(5), BaSpr3(5, 6, 7), None                 },
    { Phi2(6),  0x1c0, None,    SprDma2(5), BaSpr3(5, 6, 7), None                 },
    { Phi1(7),  0x1c4, None,    SprPtr(6),  BaSpr2(6, 7),    None                 },
    { Phi2(7),  0x1c8, None,    SprDma0(6), BaSpr2(6, 7),    None                 },
    { Phi1(8),  0x1cc, None,    SprDma1(6), BaSpr2(6, 7),    None                 },
    { Phi2(8),  0x1d0, None,    SprDma2(6), BaSpr2(6, 7),    None                 },
    { Phi1(9),  0x1d4, None,    SprPtr(7),  BaSpr1(7),       None                 },
    { Phi2(9),  0x1d8, None,    SprDma0(7), BaSpr1(7),       None                 },
    { Phi1(10), 0x1dc, None,    SprDma1(7), BaSpr1(7),       None                 },
    { Phi2(10), 0x1e0, None,    SprDma2(7), BaSpr1(7),       None                 },
    { Phi1(11), 0x1e4, None,    Refresh,    None,            None                 },
    { Phi2(11), 0x1e8, None,    None,       None,            None                 },
    { Phi1(12), 0x1ec, None,    Refresh,    BaFetch,         None                 },
    { Phi2(12), 0x1f0, None,    None,       BaFetch,         None                 },
    { Phi1(13), 0x1f4, None,    Refresh,    BaFetch,         None                 },
    { Phi2(13), 0x000, None,    None,       BaFetch,         None                 },
    { Phi1(14), 0x004, None,    Refresh,    BaFetch,         None                 },
    { Phi2(14), 0x008, None,    None,       BaFetch,         UpdateVc             },
    { Phi1(15), 0x00c, None,    Refresh,    BaFetch,         None                 },
    { Phi2(15), 0x010, None,    FetchC,     BaFetch,         ChkSprCrunch         },
    { Phi1(16), 0x014, None,    FetchG,     BaFetch,         None                 },
    { Phi2(16), 0x018, Vis(0),  FetchC,     BaFetch,         UpdateMcBase         },
    { Phi1(17), 0x01c, Vis(0),  FetchG,     BaFetch,         None                 },
    { Phi2(17), 0x020, Vis(1),  FetchC,     BaFetch,         ChkBrdL1             },
    { Phi1(18), 0x024, Vis(1),  FetchG,     BaFetch,         None                 },
    { Phi2(18), 0x028, Vis(2),  FetchC,     BaFetch,         ChkBrdL0             },
    { Phi1(19), 0x02c, Vis(2),  FetchG,     BaFetch,         None                 },
    { Phi2(19), 0x030, Vis(3),  FetchC,     BaFetch,         None                 },
    { Phi1(20), 0x034, Vis(3),  FetchG,     BaFetch,         None                 },
    { Phi2(20), 0x038, Vis(4),  FetchC,     BaFetch,         None                 },
    { Phi1(21), 0x03c, Vis(4),  FetchG,     BaFetch,         None                 },
    { Phi2(21), 0x040, Vis(5),  FetchC,     BaFetch,         None                 },
    { Phi1(22), 0x044, Vis(5),  FetchG,     BaFetch,         None                 },
    { Phi2(22), 0x048, Vis(6),  FetchC,     BaFetch,         None                 },
    { Phi1(23), 0x04c, Vis(6),  FetchG,     BaFetch,         None                 },
    { Phi2(23), 0x050, Vis(7),  FetchC,     BaFetch,         None                 },
    { Phi1(24), 0x054, Vis(7),  FetchG,     BaFetch,         None                 },
    { Phi2(24), 0x058, Vis(8),  FetchC,     BaFetch,         None                 },
    { Phi1(25), 0x05c, Vis(8),  FetchG,     BaFetch,         None                 },
    { Phi2(25), 0x060, Vis(9),  FetchC,     BaFetch,         None                 },
    { Phi1(26), 0x064, Vis(9),  FetchG,     BaFetch,         None                 },
    { Phi2(26), 0x068, Vis(10), FetchC,     BaFetch,         None                 },
    { Phi1(27), 0x06c, Vis(10), FetchG,     BaFetch,         None                 },
    { Phi2(27), 0x070, Vis(11), FetchC,     BaFetch,         None                 },
    { Phi1(28), 0x074, Vis(11), FetchG,     BaFetch,         None                 },
    { Phi2(28), 0x078, Vis(12), FetchC,     BaFetch,         None                 },
    { Phi1(29), 0x07c, Vis(12), FetchG,     BaFetch,         None                 },
    { Phi2(29), 0x080, Vis(13), FetchC,     BaFetch,         None                 },
    { Phi1(30), 0x084, Vis(13), FetchG,     BaFetch,         None                 },
    { Phi2(30), 0x088, Vis(14), FetchC,     BaFetch,         None                 },
    { Phi1(31), 0x08c, Vis(14), FetchG,     BaFetch,         None                 },
    { Phi2(31), 0x090, Vis(15), FetchC,     BaFetch,         None                 },
    { Phi1(32), 0x094, Vis(15), FetchG,     BaFetch,         None                 },
    { Phi2(32), 0x098, Vis(16), FetchC,     BaFetch,         None                 },
    { Phi1(33), 0x09c, Vis(16), FetchG,     BaFetch,         None                 },
    { Phi2(33), 0x0a0, Vis(17), FetchC,     BaFetch,         None                 },
    { Phi1(34), 0x0a4, Vis(17), FetchG,     BaFetch,         None                 },
    { Phi2(34), 0x0a8, Vis(18), FetchC,     BaFetch,         None                 },
    { Phi1(35), 0x0ac, Vis(18), FetchG,     BaFetch,         None                 },
    { Phi2(35), 0x0b0, Vis(19), FetchC,     BaFetch,         None                 },
    { Phi1(36), 0x0b4, Vis(19), FetchG,     BaFetch,         None                 },
    { Phi2(36), 0x0b8, Vis(20), FetchC,     BaFetch,         None                 },
    { Phi1(37), 0x0bc, Vis(20), FetchG,     BaFetch,         None                 },
    { Phi2(37), 0x0c0, Vis(21), FetchC,     BaFetch,         None                 },
    { Phi1(38), 0x0c4, Vis(21), FetchG,     BaFetch,         None                 },
    { Phi2(38), 0x0c8, Vis(22), FetchC,     BaFetch,         None                 },
    { Phi1(39), 0x0cc, Vis(22), FetchG,     BaFetch,         None                 },
    { Phi2(39), 0x0d0, Vis(23), FetchC,     BaFetch,         None                 },
    { Phi1(40), 0x0d4, Vis(23), FetchG,     BaFetch,         None                 },
    { Phi2(40), 0x0d8, Vis(24), FetchC,     BaFetch,         None                 },
    { Phi1(41), 0x0dc, Vis(24), FetchG,     BaFetch,         None                 },
    { Phi2(41), 0x0e0, Vis(25), FetchC,     BaFetch,         None                 },
    { Phi1(42), 0x0e4, Vis(25), FetchG,     BaFetch,         None                 },
    { Phi2(42), 0x0e8, Vis(26), FetchC,     BaFetch,         None                 },
    { Phi1(43), 0x0ec, Vis(26), FetchG,     BaFetch,         None                 },
    { Phi2(43), 0x0f0, Vis(27), FetchC,     BaFetch,         None                 },
    { Phi1(44), 0x0f4, Vis(27), FetchG,     BaFetch,         None                 },
    { Phi2(44), 0x0f8, Vis(28), FetchC,     BaFetch,         None                 },
    { Phi1(45), 0x0fc, Vis(28), FetchG,     BaFetch,         None                 },
    { Phi2(45), 0x100, Vis(29), FetchC,     BaFetch,         None                 },
    { Phi1(46), 0x104, Vis(29), FetchG,     BaFetch,         None                 },
    { Phi2(46), 0x108, Vis(30), FetchC,     BaFetch,         None                 },
    { Phi1(47), 0x10c, Vis(30), FetchG,     BaFetch,         None                 },
    { Phi2(47), 0x110, Vis(31), FetchC,     BaFetch,         None                 },
    { Phi1(48), 0x114, Vis(31), FetchG,     BaFetch,         None                 },
    { Phi2(48), 0x118, Vis(32), FetchC,     BaFetch,         None                 },
    { Phi1(49), 0x11c, Vis(32), FetchG,     BaFetch,         None                 },
    { Phi2(49), 0x120, Vis(33), FetchC,     BaFetch,         None                 },
    { Phi1(50), 0x124, Vis(33), FetchG,     BaFetch,         None                 },
    { Phi2(50), 0x128, Vis(34), FetchC,     BaFetch,         None                 },
    { Phi1(51), 0x12c, Vis(34), FetchG,     BaFetch,         None                 },
    { Phi2(51), 0x130, Vis(35), FetchC,     BaFetch,         None                 },
    { Phi1(52), 0x134, Vis(35), FetchG,     BaFetch,         None                 },
    { Phi2(52), 0x138, Vis(36), FetchC,     BaFetch,         None                 },
    { Phi1(53), 0x13c, Vis(36), FetchG,     BaFetch,         None                 },
    { Phi2(53), 0x140, Vis(37), FetchC,     BaFetch,         None                 },
    { Phi1(54), 0x144, Vis(37), FetchG,     BaFetch,         None                 },
    { Phi2(54), 0x148, Vis(38), FetchC,     BaFetch,         None                 },
    { Phi1(55), 0x14c, Vis(38), FetchG,     BaSpr1(0),       ChkSprDma            },
    { Phi2(55), 0x150, Vis(39), None,       BaSpr1(0),       None                 },
    { Phi1(56), 0x154, Vis(39), Idle,       BaSpr1(0),       ChkSprDma            },
    { Phi2(56), 0x158, None,    None,       BaSpr1(0),       ChkBrdR0 | ChkSprExp },
    { Phi1(57), 0x15c, None,    Idle,       BaSpr2(0, 1),    None                 },
    { Phi2(57), 0x160, None,    None,       BaSpr2(0, 1),    ChkBrdR1             },
    { Phi1(58), 0x164, None,    SprPtr(0),  BaSpr2(0, 1),    ChkSprDisp           },
    { Phi2(58), 0x168, None,    SprDma0(0), BaSpr2(0, 1),    UpdateRc             },
    { Phi1(59), 0x16c, None,    SprDma1(0), BaSpr3(0, 1, 2), None                 },
    { Phi2(59), 0x170, None,    SprDma2(0), BaSpr3(0, 1, 2), None                 },
    { Phi1(60), 0x174, None,    SprPtr(1),  BaSpr2(1, 2),    None                 },
    { Phi2(60), 0x178, None,    SprDma0(1), BaSpr2(1, 2),    None                 },
    { Phi1(61), 0x17c, None,    SprDma1(1), BaSpr3(1, 2, 3), None                 },
    { Phi2(61), 0x180, None,    SprDma2(1), BaSpr3(1, 2, 3), None                 },
    { Phi1(62), 0x184, None,    SprPtr(2),  BaSpr2(2, 3),    None                 },
    { Phi2(62), 0x188, None,    SprDma0(2), BaSpr2(2, 3),    None                 },
    { Phi1(63), 0x18c, None,    SprDma1(2), BaSpr3(2, 3, 4), None                 },
    { Phi2(63), 0x190, None,    SprDma2(2), BaSpr3(2, 3, 4), None                 }
};

struct ViciiChipModel chip_model_mos6569r1 = {
    "MOS6569R1",     /* name */
    63,              /* cycles per line */
    cycle_tab_pal,   /* cycle table */
    312,             /* number of raster lines */
    1,               /* color latency */
    1,               /* old light pen irq mode */
    0                /* new luminances */
};

struct ViciiChipModel chip_model_mos6569r3 = {
    "MOS6569R3",     /* name */
    63,              /* cycles per line */
    cycle_tab_pal,   /* cycle table */
    312,             /* number of raster lines */
    1,               /* color latency */
    0,               /* old light pen irq mode */
    1                /* new luminances */
};

struct ViciiChipModel chip_model_mos8565 = {
    "MOS8565",       /* name */
    63,              /* cycles per line */
    cycle_tab_pal,   /* cycle table */
    312,             /* number of raster lines */
    0,               /* color latency */
    0,               /* old light pen irq mode */
    1                /* new luminances */
};


/* NTSC (and PAL-N) */
static struct ViciiCycle cycle_tab_ntsc[] = {
    { Phi1(1),  0x19c, None,    SprDma1(3), BaSpr3(3, 4, 5), None                 },
    { Phi2(1),  0x1a0, None,    SprDma2(3), BaSpr3(3, 4, 5), None                 },
    { Phi1(2),  0x1a4, None,    SprPtr(4),  BaSpr2(4, 5),    None                 },
    { Phi2(2),  0x1a8, None,    SprDma0(4), BaSpr2(4, 5),    None                 },
    { Phi1(3),  0x1ac, None,    SprDma1(4), BaSpr3(4, 5, 6), None                 },
    { Phi2(3),  0x1b0, None,    SprDma2(4), BaSpr3(4, 5, 6), None                 },
    { Phi1(4),  0x1b4, None,    SprPtr(5),  BaSpr2(5, 6),    None                 },
    { Phi2(4),  0x1b8, None,    SprDma0(5), BaSpr2(5, 6),    None                 },
    { Phi1(5),  0x1bc, None,    SprDma1(5), BaSpr3(5, 6, 7), None                 },
    { Phi2(5),  0x1c0, None,    SprDma2(5), BaSpr3(5, 6, 7), None                 },
    { Phi1(6),  0x1c4, None,    SprPtr(6),  BaSpr2(6, 7),    None                 },
    { Phi2(6),  0x1c8, None,    SprDma0(6), BaSpr2(6, 7),    None                 },
    { Phi1(7),  0x1cc, None,    SprDma1(6), BaSpr2(6, 7),    None                 },
    { Phi2(7),  0x1d0, None,    SprDma2(6), BaSpr2(6, 7),    None                 },
    { Phi1(8),  0x1d4, None,    SprPtr(7),  BaSpr1(7),       None                 },
    { Phi2(8),  0x1d8, None,    SprDma0(7), BaSpr1(7),       None                 },
    { Phi1(9),  0x1dc, None,    SprDma1(7), BaSpr1(7),       None                 },
    { Phi2(9),  0x1e0, None,    SprDma2(7), BaSpr1(7),       None                 },
    { Phi1(10), 0x1e4, None,    Idle,       None,            None                 },
    { Phi2(10), 0x1e8, None,    None,       None,            None                 },
    { Phi1(11), 0x1ec, None,    Refresh,    None,            None                 },
    { Phi2(11), 0x1f0, None,    None,       None,            None                 },
    { Phi1(12), 0x1f4, None,    Refresh,    BaFetch,         None                 },
    { Phi2(12), 0x1f8, None,    None,       BaFetch,         None                 },
    { Phi1(13), 0x1fc, None,    Refresh,    BaFetch,         None                 },
    { Phi2(13), 0x000, None,    None,       BaFetch,         None                 },
    { Phi1(14), 0x004, None,    Refresh,    BaFetch,         None                 },
    { Phi2(14), 0x008, None,    None,       BaFetch,         UpdateVc             },
    { Phi1(15), 0x00c, None,    Refresh,    BaFetch,         None                 },
    { Phi2(15), 0x010, None,    FetchC,     BaFetch,         ChkSprCrunch         },
    { Phi1(16), 0x014, None,    FetchG,     BaFetch,         None                 },
    { Phi2(16), 0x018, Vis(0),  FetchC,     BaFetch,         UpdateMcBase         },
    { Phi1(17), 0x01c, Vis(0),  FetchG,     BaFetch,         None                 },
    { Phi2(17), 0x020, Vis(1),  FetchC,     BaFetch,         ChkBrdL1             },
    { Phi1(18), 0x024, Vis(1),  FetchG,     BaFetch,         None                 },
    { Phi2(18), 0x028, Vis(2),  FetchC,     BaFetch,         ChkBrdL0             },
    { Phi1(19), 0x02c, Vis(2),  FetchG,     BaFetch,         None                 },
    { Phi2(19), 0x030, Vis(3),  FetchC,     BaFetch,         None                 },
    { Phi1(20), 0x034, Vis(3),  FetchG,     BaFetch,         None                 },
    { Phi2(20), 0x038, Vis(4),  FetchC,     BaFetch,         None                 },
    { Phi1(21), 0x03c, Vis(4),  FetchG,     BaFetch,         None                 },
    { Phi2(21), 0x040, Vis(5),  FetchC,     BaFetch,         None                 },
    { Phi1(22), 0x044, Vis(5),  FetchG,     BaFetch,         None                 },
    { Phi2(22), 0x048, Vis(6),  FetchC,     BaFetch,         None                 },
    { Phi1(23), 0x04c, Vis(6),  FetchG,     BaFetch,         None                 },
    { Phi2(23), 0x050, Vis(7),  FetchC,     BaFetch,         None                 },
    { Phi1(24), 0x054, Vis(7),  FetchG,     BaFetch,         None                 },
    { Phi2(24), 0x058, Vis(8),  FetchC,     BaFetch,         None                 },
    { Phi1(25), 0x05c, Vis(8),  FetchG,     BaFetch,         None                 },
    { Phi2(25), 0x060, Vis(9),  FetchC,     BaFetch,         None                 },
    { Phi1(26), 0x064, Vis(9),  FetchG,     BaFetch,         None                 },
    { Phi2(26), 0x068, Vis(10), FetchC,     BaFetch,         None                 },
    { Phi1(27), 0x06c, Vis(10), FetchG,     BaFetch,         None                 },
    { Phi2(27), 0x070, Vis(11), FetchC,     BaFetch,         None                 },
    { Phi1(28), 0x074, Vis(11), FetchG,     BaFetch,         None                 },
    { Phi2(28), 0x078, Vis(12), FetchC,     BaFetch,         None                 },
    { Phi1(29), 0x07c, Vis(12), FetchG,     BaFetch,         None                 },
    { Phi2(29), 0x080, Vis(13), FetchC,     BaFetch,         None                 },
    { Phi1(30), 0x084, Vis(13), FetchG,     BaFetch,         None                 },
    { Phi2(30), 0x088, Vis(14), FetchC,     BaFetch,         None                 },
    { Phi1(31), 0x08c, Vis(14), FetchG,     BaFetch,         None                 },
    { Phi2(31), 0x090, Vis(15), FetchC,     BaFetch,         None                 },
    { Phi1(32), 0x094, Vis(15), FetchG,     BaFetch,         None                 },
    { Phi2(32), 0x098, Vis(16), FetchC,     BaFetch,         None                 },
    { Phi1(33), 0x09c, Vis(16), FetchG,     BaFetch,         None                 },
    { Phi2(33), 0x0a0, Vis(17), FetchC,     BaFetch,         None                 },
    { Phi1(34), 0x0a4, Vis(17), FetchG,     BaFetch,         None                 },
    { Phi2(34), 0x0a8, Vis(18), FetchC,     BaFetch,         None                 },
    { Phi1(35), 0x0ac, Vis(18), FetchG,     BaFetch,         None                 },
    { Phi2(35), 0x0b0, Vis(19), FetchC,     BaFetch,         None                 },
    { Phi1(36), 0x0b4, Vis(19), FetchG,     BaFetch,         None                 },
    { Phi2(36), 0x0b8, Vis(20), FetchC,     BaFetch,         None                 },
    { Phi1(37), 0x0bc, Vis(20), FetchG,     BaFetch,         None                 },
    { Phi2(37), 0x0c0, Vis(21), FetchC,     BaFetch,         None                 },
    { Phi1(38), 0x0c4, Vis(21), FetchG,     BaFetch,         None                 },
    { Phi2(38), 0x0c8, Vis(22), FetchC,     BaFetch,         None                 },
    { Phi1(39), 0x0cc, Vis(22), FetchG,     BaFetch,         None                 },
    { Phi2(39), 0x0d0, Vis(23), FetchC,     BaFetch,         None                 },
    { Phi1(40), 0x0d4, Vis(23), FetchG,     BaFetch,         None                 },
    { Phi2(40), 0x0d8, Vis(24), FetchC,     BaFetch,         None                 },
    { Phi1(41), 0x0dc, Vis(24), FetchG,     BaFetch,         None                 },
    { Phi2(41), 0x0e0, Vis(25), FetchC,     BaFetch,         None                 },
    { Phi1(42), 0x0e4, Vis(25), FetchG,     BaFetch,         None                 },
    { Phi2(42), 0x0e8, Vis(26), FetchC,     BaFetch,         None                 },
    { Phi1(43), 0x0ec, Vis(26), FetchG,     BaFetch,         None                 },
    { Phi2(43), 0x0f0, Vis(27), FetchC,     BaFetch,         None                 },
    { Phi1(44), 0x0f4, Vis(27), FetchG,     BaFetch,         None                 },
    { Phi2(44), 0x0f8, Vis(28), FetchC,     BaFetch,         None                 },
    { Phi1(45), 0x0fc, Vis(28), FetchG,     BaFetch,         None                 },
    { Phi2(45), 0x100, Vis(29), FetchC,     BaFetch,         None                 },
    { Phi1(46), 0x104, Vis(29), FetchG,     BaFetch,         None                 },
    { Phi2(46), 0x108, Vis(30), FetchC,     BaFetch,         None                 },
    { Phi1(47), 0x10c, Vis(30), FetchG,     BaFetch,         None                 },
    { Phi2(47), 0x110, Vis(31), FetchC,     BaFetch,         None                 },
    { Phi1(48), 0x114, Vis(31), FetchG,     BaFetch,         None                 },
    { Phi2(48), 0x118, Vis(32), FetchC,     BaFetch,         None                 },
    { Phi1(49), 0x11c, Vis(32), FetchG,     BaFetch,         None                 },
    { Phi2(49), 0x120, Vis(33), FetchC,     BaFetch,         None                 },
    { Phi1(50), 0x124, Vis(33), FetchG,     BaFetch,         None                 },
    { Phi2(50), 0x128, Vis(34), FetchC,     BaFetch,         None                 },
    { Phi1(51), 0x12c, Vis(34), FetchG,     BaFetch,         None                 },
    { Phi2(51), 0x130, Vis(35), FetchC,     BaFetch,         None                 },
    { Phi1(52), 0x134, Vis(35), FetchG,     BaFetch,         None                 },
    { Phi2(52), 0x138, Vis(36), FetchC,     BaFetch,         None                 },
    { Phi1(53), 0x13c, Vis(36), FetchG,     BaFetch,         None                 },
    { Phi2(53), 0x140, Vis(37), FetchC,     BaFetch,         None                 },
    { Phi1(54), 0x144, Vis(37), FetchG,     BaFetch,         None                 },
    { Phi2(54), 0x148, Vis(38), FetchC,     BaFetch,         None                 },
    { Phi1(55), 0x14c, Vis(38), FetchG,     None,            None                 },
    { Phi2(55), 0x150, Vis(39), None,       None,            None                 },
    { Phi1(56), 0x154, Vis(39), Idle,       BaSpr1(0),       ChkSprDma            },
    { Phi2(56), 0x158, None,    None,       BaSpr1(0),       ChkBrdR0 | ChkSprExp },
    { Phi1(57), 0x15c, None,    Idle,       BaSpr1(0),       ChkSprDma            },
    { Phi2(57), 0x160, None,    None,       BaSpr1(0),       ChkBrdR1             },
    { Phi1(58), 0x164, None,    Idle,       BaSpr2(0, 1),    None                 },
    { Phi2(58), 0x168, None,    None,       BaSpr2(0, 1),    UpdateRc             },
    { Phi1(59), 0x16c, None,    SprPtr(0),  BaSpr2(0, 1),    ChkSprDisp           },
    { Phi2(59), 0x170, None,    SprDma0(0), BaSpr2(0, 1),    None                 },
    { Phi1(60), 0x174, None,    SprDma1(0), BaSpr3(0, 1, 2), None                 },
    { Phi2(60), 0x178, None,    SprDma2(0), BaSpr3(0, 1, 2), None                 },
    { Phi1(61), 0x17c, None,    SprPtr(1),  BaSpr2(1, 2),    None                 },
    { Phi2(61), 0x180, None,    SprDma0(1), BaSpr2(1, 2),    None                 },
    { Phi1(62), 0x184, None,    SprDma1(1), BaSpr3(1, 2, 3), None                 },
    { Phi2(62), 0x184, None,    SprDma2(1), BaSpr3(1, 2, 3), None                 },
    { Phi1(63), 0x184, None,    SprPtr(2),  BaSpr2(2, 3),    None                 },
    { Phi2(63), 0x188, None,    SprDma0(2), BaSpr2(2, 3),    None                 },
    { Phi1(64), 0x18c, None,    SprDma1(2), BaSpr3(2, 3, 4), None                 },
    { Phi2(64), 0x190, None,    SprDma2(2), BaSpr3(2, 3, 4), None                 },
    { Phi1(65), 0x194, None,    SprPtr(3),  BaSpr2(3, 4),    None                 },
    { Phi2(65), 0x198, None,    SprDma0(3), BaSpr2(3, 4),    None                 }
};

struct ViciiChipModel chip_model_mos6567r8 = {
    "MOS6567R8",     /* name */
    65,              /* cycles per line */
    cycle_tab_ntsc,  /* cycle table */
    263,             /* number of raster lines */
    1,               /* color latency */
    0,               /* old light pen irq mode */
    1                /* new luminances */
};

struct ViciiChipModel chip_model_mos8562 = {
    "MOS8562",       /* name */
    65,              /* cycles per line */
    cycle_tab_ntsc,  /* cycle table */
    263,             /* number of raster lines */
    0,               /* color latency */
    0,               /* old light pen irq mode */
    1                /* new luminances */
};

struct ViciiChipModel chip_model_mos6572 = {
    "MOS6572",       /* name */
    65,              /* cycles per line */
    cycle_tab_ntsc,  /* cycle table */
    312,             /* number of raster lines */
    1,               /* color latency */
    0,               /* old light pen irq mode */
    1                /* new luminances */
};


/* Old NTSC */
static struct ViciiCycle cycle_tab_ntsc_old[] = {
    { Phi1(1),  0x19c, None,    SprPtr(3),  BaSpr2(3, 4),    None                 },
    { Phi2(1),  0x1a0, None,    SprDma0(3), BaSpr2(3, 4),    None                 },
    { Phi1(2),  0x1a4, None,    SprDma1(3), BaSpr3(3, 4, 5), None                 },
    { Phi2(2),  0x1a8, None,    SprDma2(3), BaSpr3(3, 4, 5), None                 },
    { Phi1(3),  0x1ac, None,    SprPtr(4),  BaSpr2(4, 5),    None                 },
    { Phi2(3),  0x1b0, None,    SprDma0(4), BaSpr2(4, 5),    None                 },
    { Phi1(4),  0x1b4, None,    SprDma1(4), BaSpr3(4, 5, 6), None                 },
    { Phi2(4),  0x1b8, None,    SprDma2(4), BaSpr3(4, 5, 6), None                 },
    { Phi1(5),  0x1bc, None,    SprPtr(5),  BaSpr2(5, 6),    None                 },
    { Phi2(5),  0x1c0, None,    SprDma0(5), BaSpr2(5, 6),    None                 },
    { Phi1(6),  0x1c4, None,    SprDma1(5), BaSpr3(5, 6, 7), None                 },
    { Phi2(6),  0x1c8, None,    SprDma2(5), BaSpr3(5, 6, 7), None                 },
    { Phi1(7),  0x1cc, None,    SprPtr(6),  BaSpr2(6, 7),    None                 },
    { Phi2(7),  0x1d0, None,    SprDma0(6), BaSpr2(6, 7),    None                 },
    { Phi1(8),  0x1d4, None,    SprDma1(6), BaSpr2(6, 7),    None                 },
    { Phi2(8),  0x1d8, None,    SprDma2(6), BaSpr2(6, 7),    None                 },
    { Phi1(9),  0x1dc, None,    SprPtr(7),  BaSpr1(7),       None                 },
    { Phi2(9),  0x1e0, None,    SprDma0(7), BaSpr1(7),       None                 },
    { Phi1(10), 0x1e4, None,    SprDma1(7), BaSpr1(7),       None                 },
    { Phi2(10), 0x1e8, None,    SprDma2(7), BaSpr1(7),       None                 },
    { Phi1(11), 0x1ec, None,    Refresh,    None,            None                 },
    { Phi2(11), 0x1f0, None,    None,       None,            None                 },
    { Phi1(12), 0x1f4, None,    Refresh,    BaFetch,         None                 },
    { Phi2(12), 0x1f8, None,    None,       BaFetch,         None                 },
    { Phi1(13), 0x1fc, None,    Refresh,    BaFetch,         None                 },
    { Phi2(13), 0x000, None,    None,       BaFetch,         None                 },
    { Phi1(14), 0x004, None,    Refresh,    BaFetch,         None                 },
    { Phi2(14), 0x008, None,    None,       BaFetch,         UpdateVc             },
    { Phi1(15), 0x00c, None,    Refresh,    BaFetch,         None                 },
    { Phi2(15), 0x010, None,    FetchC,     BaFetch,         ChkSprCrunch         },
    { Phi1(16), 0x014, None,    FetchG,     BaFetch,         None                 },
    { Phi2(16), 0x018, Vis(0),  FetchC,     BaFetch,         UpdateMcBase         },
    { Phi1(17), 0x01c, Vis(0),  FetchG,     BaFetch,         None                 },
    { Phi2(17), 0x020, Vis(1),  FetchC,     BaFetch,         ChkBrdL1             },
    { Phi1(18), 0x024, Vis(1),  FetchG,     BaFetch,         None                 },
    { Phi2(18), 0x028, Vis(2),  FetchC,     BaFetch,         ChkBrdL0             },
    { Phi1(19), 0x02c, Vis(2),  FetchG,     BaFetch,         None                 },
    { Phi2(19), 0x030, Vis(3),  FetchC,     BaFetch,         None                 },
    { Phi1(20), 0x034, Vis(3),  FetchG,     BaFetch,         None                 },
    { Phi2(20), 0x038, Vis(4),  FetchC,     BaFetch,         None                 },
    { Phi1(21), 0x03c, Vis(4),  FetchG,     BaFetch,         None                 },
    { Phi2(21), 0x040, Vis(5),  FetchC,     BaFetch,         None                 },
    { Phi1(22), 0x044, Vis(5),  FetchG,     BaFetch,         None                 },
    { Phi2(22), 0x048, Vis(6),  FetchC,     BaFetch,         None                 },
    { Phi1(23), 0x04c, Vis(6),  FetchG,     BaFetch,         None                 },
    { Phi2(23), 0x050, Vis(7),  FetchC,     BaFetch,         None                 },
    { Phi1(24), 0x054, Vis(7),  FetchG,     BaFetch,         None                 },
    { Phi2(24), 0x058, Vis(8),  FetchC,     BaFetch,         None                 },
    { Phi1(25), 0x05c, Vis(8),  FetchG,     BaFetch,         None                 },
    { Phi2(25), 0x060, Vis(9),  FetchC,     BaFetch,         None                 },
    { Phi1(26), 0x064, Vis(9),  FetchG,     BaFetch,         None                 },
    { Phi2(26), 0x068, Vis(10), FetchC,     BaFetch,         None                 },
    { Phi1(27), 0x06c, Vis(10), FetchG,     BaFetch,         None                 },
    { Phi2(27), 0x070, Vis(11), FetchC,     BaFetch,         None                 },
    { Phi1(28), 0x074, Vis(11), FetchG,     BaFetch,         None                 },
    { Phi2(28), 0x078, Vis(12), FetchC,     BaFetch,         None                 },
    { Phi1(29), 0x07c, Vis(12), FetchG,     BaFetch,         None                 },
    { Phi2(29), 0x080, Vis(13), FetchC,     BaFetch,         None                 },
    { Phi1(30), 0x084, Vis(13), FetchG,     BaFetch,         None                 },
    { Phi2(30), 0x088, Vis(14), FetchC,     BaFetch,         None                 },
    { Phi1(31), 0x08c, Vis(14), FetchG,     BaFetch,         None                 },
    { Phi2(31), 0x090, Vis(15), FetchC,     BaFetch,         None                 },
    { Phi1(32), 0x094, Vis(15), FetchG,     BaFetch,         None                 },
    { Phi2(32), 0x098, Vis(16), FetchC,     BaFetch,         None                 },
    { Phi1(33), 0x09c, Vis(16), FetchG,     BaFetch,         None                 },
    { Phi2(33), 0x0a0, Vis(17), FetchC,     BaFetch,         None                 },
    { Phi1(34), 0x0a4, Vis(17), FetchG,     BaFetch,         None                 },
    { Phi2(34), 0x0a8, Vis(18), FetchC,     BaFetch,         None                 },
    { Phi1(35), 0x0ac, Vis(18), FetchG,     BaFetch,         None                 },
    { Phi2(35), 0x0b0, Vis(19), FetchC,     BaFetch,         None                 },
    { Phi1(36), 0x0b4, Vis(19), FetchG,     BaFetch,         None                 },
    { Phi2(36), 0x0b8, Vis(20), FetchC,     BaFetch,         None                 },
    { Phi1(37), 0x0bc, Vis(20), FetchG,     BaFetch,         None                 },
    { Phi2(37), 0x0c0, Vis(21), FetchC,     BaFetch,         None                 },
    { Phi1(38), 0x0c4, Vis(21), FetchG,     BaFetch,         None                 },
    { Phi2(38), 0x0c8, Vis(22), FetchC,     BaFetch,         None                 },
    { Phi1(39), 0x0cc, Vis(22), FetchG,     BaFetch,         None                 },
    { Phi2(39), 0x0d0, Vis(23), FetchC,     BaFetch,         None                 },
    { Phi1(40), 0x0d4, Vis(23), FetchG,     BaFetch,         None                 },
    { Phi2(40), 0x0d8, Vis(24), FetchC,     BaFetch,         None                 },
    { Phi1(41), 0x0dc, Vis(24), FetchG,     BaFetch,         None                 },
    { Phi2(41), 0x0e0, Vis(25), FetchC,     BaFetch,         None                 },
    { Phi1(42), 0x0e4, Vis(25), FetchG,     BaFetch,         None                 },
    { Phi2(42), 0x0e8, Vis(26), FetchC,     BaFetch,         None                 },
    { Phi1(43), 0x0ec, Vis(26), FetchG,     BaFetch,         None                 },
    { Phi2(43), 0x0f0, Vis(27), FetchC,     BaFetch,         None                 },
    { Phi1(44), 0x0f4, Vis(27), FetchG,     BaFetch,         None                 },
    { Phi2(44), 0x0f8, Vis(28), FetchC,     BaFetch,         None                 },
    { Phi1(45), 0x0fc, Vis(28), FetchG,     BaFetch,         None                 },
    { Phi2(45), 0x100, Vis(29), FetchC,     BaFetch,         None                 },
    { Phi1(46), 0x104, Vis(29), FetchG,     BaFetch,         None                 },
    { Phi2(46), 0x108, Vis(30), FetchC,     BaFetch,         None                 },
    { Phi1(47), 0x10c, Vis(30), FetchG,     BaFetch,         None                 },
    { Phi2(47), 0x110, Vis(31), FetchC,     BaFetch,         None                 },
    { Phi1(48), 0x114, Vis(31), FetchG,     BaFetch,         None                 },
    { Phi2(48), 0x118, Vis(32), FetchC,     BaFetch,         None                 },
    { Phi1(49), 0x11c, Vis(32), FetchG,     BaFetch,         None                 },
    { Phi2(49), 0x120, Vis(33), FetchC,     BaFetch,         None                 },
    { Phi1(50), 0x124, Vis(33), FetchG,     BaFetch,         None                 },
    { Phi2(50), 0x128, Vis(34), FetchC,     BaFetch,         None                 },
    { Phi1(51), 0x12c, Vis(34), FetchG,     BaFetch,         None                 },
    { Phi2(51), 0x130, Vis(35), FetchC,     BaFetch,         None                 },
    { Phi1(52), 0x134, Vis(35), FetchG,     BaFetch,         None                 },
    { Phi2(52), 0x138, Vis(36), FetchC,     BaFetch,         None                 },
    { Phi1(53), 0x13c, Vis(36), FetchG,     BaFetch,         None                 },
    { Phi2(53), 0x140, Vis(37), FetchC,     BaFetch,         None                 },
    { Phi1(54), 0x144, Vis(37), FetchG,     BaFetch,         None                 },
    { Phi2(54), 0x148, Vis(38), FetchC,     BaFetch,         None                 },
    { Phi1(55), 0x14c, Vis(38), FetchG,     None,            None                 },
    { Phi2(55), 0x150, Vis(39), None,       None,            None                 },
    { Phi1(56), 0x154, Vis(39), Idle,       BaSpr1(0),       ChkSprDma            },
    { Phi2(56), 0x158, None,    None,       BaSpr1(0),       ChkBrdR0 | ChkSprExp },
    { Phi1(57), 0x15c, None,    Idle,       BaSpr1(0),       ChkSprDma            },
    { Phi2(57), 0x160, None,    None,       BaSpr1(0),       ChkBrdR1             },
    { Phi1(58), 0x164, None,    Idle,       BaSpr2(0, 1),    ChkSprDisp           },
    { Phi2(58), 0x168, None,    None,       BaSpr2(0, 1),    UpdateRc             },
    { Phi1(59), 0x16c, None,    SprPtr(0),  BaSpr2(0, 1),    None                 },
    { Phi2(59), 0x170, None,    SprDma0(0), BaSpr2(0, 1),    None                 },
    { Phi1(60), 0x174, None,    SprDma1(0), BaSpr3(0, 1, 2), None                 },
    { Phi2(60), 0x178, None,    SprDma2(0), BaSpr3(0, 1, 2), None                 },
    { Phi1(61), 0x17c, None,    SprPtr(1),  BaSpr2(1, 2),    None                 },
    { Phi2(61), 0x180, None,    SprDma0(1), BaSpr2(1, 2),    None                 },
    { Phi1(62), 0x184, None,    SprDma1(1), BaSpr3(1, 2, 3), None                 },
    { Phi2(62), 0x188, None,    SprDma2(1), BaSpr3(1, 2, 3), None                 },
    { Phi1(63), 0x18c, None,    SprPtr(2),  BaSpr2(2, 3),    None                 },
    { Phi2(63), 0x190, None,    SprDma0(2), BaSpr2(2, 3),    None                 },
    { Phi1(64), 0x194, None,    SprDma1(2), BaSpr3(2, 3, 4), None                 },
    { Phi2(64), 0x198, None,    SprDma2(2), BaSpr3(2, 3, 4), None                 }
};

struct ViciiChipModel chip_model_mos6567r56a = {
    "MOS6567R56A",   /* name */
    64,              /* cycles per line */
    cycle_tab_ntsc_old, /* cycle table */
    262,             /* number of raster lines */
    1,               /* color latency */
    1,               /* old light pen irq mode */
    0                /* new luminances */
};


static void vicii_chip_model_set(struct ViciiChipModel *cm)
{
    int i;
    int xpos_phi[2];
    int fetch_phi[2];
    int ba_phi[2];
    int flags_phi[2];

    struct ViciiCycle *ct = cm->cycle_tab;

    vicii.cycles_per_line = cm->cycles_per_line;
    vicii.screen_height = cm->num_raster_lines;
    vicii.color_latency = cm->color_latency;
    vicii.lightpen_old_irq_mode = cm->lightpen_old_irq_mode;
    /* vicii.new_luminances        = cm->new_luminances; */

    log_message(vicii.log,
                "Initializing chip model \"%s\" (%d cycles per line, %d raster lines).",
                cm->name, cm->cycles_per_line, cm->num_raster_lines);

    log_verbose("VIC-II:                    BA");
    log_verbose("VIC-II:  cycle  xpos vi M76543210   fetch    border gfx      sprite");

    for (i = 0; i < (cm->cycles_per_line * 2); i++) {
        int phi = (ct[i].cycle & 0x80) ? 1 : 0;
        int cycle = ct[i].cycle & 0x7f;
        int xpos = ct[i].xpos;
        int visible = IsVis(ct[i].visible) ? GetVis(ct[i].visible) : -1;
        int fetch = ct[i].fetch;
        int ba = ct[i].ba;
        int flags = ct[i].flags;


        {
            /* output parsed cycle information to the log */
            int j;
            char cycle_str[10];
            char visible_str[8];
            char ba_str[10];
            char fetch_str[12];
            char border_str[12];
            char gfx_str[12];
            char sprite_str[16];
            int spr;

            /* cycle */
            if (phi == 0) {
                sprintf(cycle_str, "%2d Phi1", cycle);
            } else {
                sprintf(cycle_str, "-- Phi2");
            }

            /* visible */
            if (visible < 0) {
                sprintf(visible_str, "--");
            } else {
                sprintf(visible_str, "%2d", visible);
            }

            /* BA */
            for (j = 0; j < 9; j++) {
                ba_str[j] = (ba & (1 << (8 - j))) ? '*' : '-';
            }
            ba_str[j] = 0;

            /* fetch */
            spr = fetch & FetchSprNum_M;
            switch (fetch & FetchType_M) {
                case SprPtr(0):
                    sprintf(fetch_str, "SprPtr(%d) ", spr);
                    break;
                case SprDma0(0):
                    sprintf(fetch_str, "SprDma0(%d)", spr);
                    break;
                case SprDma1(0):
                    sprintf(fetch_str, "SprDma1(%d)", spr);
                    break;
                case SprDma2(0):
                    sprintf(fetch_str, "SprDma2(%d)", spr);
                    break;
                case Refresh:
                    sprintf(fetch_str, "Refresh   ");
                    break;
                case FetchC:
                    sprintf(fetch_str, "FetchC    ");
                    break;
                case FetchG:
                    sprintf(fetch_str, "FetchG    ");
                    break;
                case Idle:
                    sprintf(fetch_str, "Idle      ");
                    break;
                default:
                    sprintf(fetch_str, "-         ");
                    break;
            }

            /* border */
            sprintf(border_str, "-     ");
            if (flags & ChkBrdL1) {
                sprintf(border_str, "ChkL1 ");
            }
            if (flags & ChkBrdL0) {
                sprintf(border_str, "ChkL0 ");
            }
            if (flags & ChkBrdR0) {
                sprintf(border_str, "ChkR0 ");
            }
            if (flags & ChkBrdR1) {
                sprintf(border_str, "ChkR1 ");
            }

            /* Graphics */
            sprintf(gfx_str, "-       ");
            if (flags & UpdateVc) {
                sprintf(gfx_str, "UpdateVc");
            }
            if (flags & UpdateRc) {
                sprintf(gfx_str, "UpdateRc");
            }

            /* Sprites */
            sprintf(sprite_str, "-       ");
            if (flags & ChkSprCrunch) {
                sprintf(sprite_str, "ChkSprCrunch");
            }
            if (flags & UpdateMcBase) {
                sprintf(sprite_str, "UpdateMcBase");
            }
            if (flags & ChkSprDma) {
                sprintf(sprite_str, "ChkSprDma   ");
            }
            if (flags & ChkSprExp) {
                sprintf(sprite_str, "ChkSprExp   ");
            }
            if (flags & ChkSprDisp) {
                sprintf(sprite_str, "ChkSprDisp  ");
            }

            /* dump to log */
            log_verbose("VIC-II: %s $%03x %s %s %s %s %s %s", 
                        cycle_str, xpos, visible_str, ba_str, fetch_str, border_str, gfx_str, sprite_str);
        }

        xpos_phi[phi] = xpos;
        fetch_phi[phi] = fetch;
        ba_phi[phi] = ba;
        flags_phi[phi] = flags;

        /* Both Phi1 and Phi2 collected, generate table */
        if (phi == 1) {
            unsigned int flags = flags_phi[0] | flags_phi[1];

            unsigned int entry = 0;

            entry |= (ba_phi[0] & BaSpr_M) << SPRITE_BA_MASK_B;
            entry |= (ba_phi[0] & BaFetch) ? FETCH_BA_M : 0;

            switch (fetch_phi[0] & FetchType_M) {
                case SprPtr(0):
                    /* Sprite Ptr (Phi1) + DMA0 (Phi2) */
                    entry |= PHI1_SPR_PTR;
                    entry |= (fetch_phi[0] & FetchSprNum_M) << PHI1_SPR_NUM_B;
                    break;
                case SprDma1(0):
                    /* Sprite DMA1 (Phi1) + DMA2 (Phi2) */
                    entry |= PHI1_SPR_DMA1;
                    entry |= (fetch_phi[0] & FetchSprNum_M) << PHI1_SPR_NUM_B;
                    break;
                case Refresh:
                    /* Refresh (Phi1) */
                    entry |= PHI1_REFRESH;
                    break;
                case FetchG:
                    /* FetchG (Phi1) */
                    entry |= PHI1_FETCH_G;
                    break;
                default:
                    entry |= PHI1_IDLE;
                    break;
            }
            /* FetchC (Phi2) */
            if ((fetch_phi[1] & FetchType_M) == FetchC) {
                entry |= PHI2_FETCH_C_M;
                entry |= VISIBLE_M;
            }
            /* extract xpos */
            entry |= ((xpos_phi[0] >> 3) << XPOS_B) & XPOS_M;

            /* Update VC/RC (Phi2) */
            if (flags & UpdateVc) {
                entry |= UPDATE_VC_M;
            }
            if (flags & UpdateRc) {
                entry |= UPDATE_RC_M;
            }

            /* Sprites */
            if (flags & ChkSprExp) {
                entry |= CHECK_SPR_EXP_M;
            }
            if (flags & ChkSprDisp) {
                entry |= CHECK_SPR_DISP;
            }
            if (flags & ChkSprDma) {
                entry |= CHECK_SPR_DMA;
            }
            if (flags & UpdateMcBase) {
                entry |= UPDATE_MCBASE;
            }
            if (flags & ChkSprCrunch) {
                entry |= CHECK_SPR_CRUNCH;
            }

            /* Border */
            if (flags & ChkBrdL0) {
                entry |= CHECK_BRD_L;
            }
            if (flags & ChkBrdL1) {
                entry |= CHECK_BRD_L | CHECK_BRD_CSEL;
            }
            if (flags & ChkBrdR0) {
                entry |= CHECK_BRD_R;
            }
            if (flags & ChkBrdR1) {
                entry |= CHECK_BRD_R | CHECK_BRD_CSEL;
            }

            vicii.cycle_table[cycle - 1] = entry;
        }
    }
}

void vicii_chip_model_init(void)
{
    switch (vicii_resources.model) {
        case VICII_MODEL_6569R1:
            vicii_chip_model_set(&chip_model_mos6569r1);
            break;
        case VICII_MODEL_6569:
            vicii_chip_model_set(&chip_model_mos6569r3);
            break;
        case VICII_MODEL_8565:
            vicii_chip_model_set(&chip_model_mos8565);
            break;
        case VICII_MODEL_6567R56A:
            vicii_chip_model_set(&chip_model_mos6567r56a);
            break;
        case VICII_MODEL_6567:
            vicii_chip_model_set(&chip_model_mos6567r8);
            break;
        case VICII_MODEL_8562:
            vicii_chip_model_set(&chip_model_mos8562);
            break;
        case VICII_MODEL_6572:
            vicii_chip_model_set(&chip_model_mos6572);
            break;
        default:
            /* should never happen */
            vicii_chip_model_set(&chip_model_mos6569r3);
            break;
    }
}
