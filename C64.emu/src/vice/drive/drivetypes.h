/*
 * drivetypes.h - Drive-specific types like the drive context structure.
 *
 * Written by
 *  Andreas Dehmel <dehmel@forwiss.tu-muenchen.de>
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

#ifndef VICE_DRIVETYPES_H
#define VICE_DRIVETYPES_H

#include "drive.h"
#include "mos6510.h"
#include "r65c02.h"
#include "types.h"

/*
 *  The philosophy behind this approach is that only the drive module knows
 *  the exact layout of the drive_context_t structure. Therefore only include
 *  drivetypes.h from source files within the drive module. All other modules
 *  only need to use pointers transparently, which only requires a forward
 *  declaration of struct drive_context_s (see below).
 */

struct drive_context_s;         /* forward declaration */
struct monitor_interface_s;

/* This defines the memory access for the drive CPU.  */
typedef BYTE drive_read_func_t (struct drive_context_s *, WORD);
typedef drive_read_func_t *drive_read_func_ptr_t;
typedef void drive_store_func_t (struct drive_context_s *, WORD, BYTE);
typedef drive_store_func_t *drive_store_func_ptr_t;

/*
 *  The private CPU data.
 */

typedef struct drivecpu_context_s {
    int traceflg;
    /* This is non-zero each time a Read-Modify-Write instructions that accesses
       memory is executed.  We can emulate the RMW bug of the 6502 this way.  */
    int rmw_flag; /* init to 0 */

    /* Interrupt/alarm status.  */
    struct interrupt_cpu_status_s *int_status;

    struct alarm_context_s *alarm_context;

    /* Clk guard.  */
    struct clk_guard_s *clk_guard;

    struct monitor_interface_s *monitor_interface;

    /* Value of clk for the last time mydrive_cpu_execute() was called.  */
    CLOCK last_clk;

    /* Number of cycles in excess we executed last time mydrive_cpu_execute()
       was called.  */
    CLOCK last_exc_cycles;

    CLOCK stop_clk;

    CLOCK cycle_accum;
    BYTE *d_bank_base;
    unsigned int d_bank_start;
    unsigned int d_bank_limit;

    /* Information about the last executed opcode.  */
    unsigned int last_opcode_info;

    /* Address of the last executed opcode. This is used by watchpoints. */
    unsigned int last_opcode_addr;

    /* Public copy of the registers.  */
    mos6510_regs_t cpu_regs;
    R65C02_regs_t cpu_R65C02_regs;

    BYTE *pageone;        /* init to NULL */

    int monspace;         /* init to e_disk[89]_space */

    char *snap_module_name;

    char *identification_string;
} drivecpu_context_t;


/*
 *  Large data used in the CPU emulation. Often more efficient to move
 *  to the end of the drive context structure to minimize the average
 *  offset of members within the context structure.
 */

typedef struct drivecpud_context_s {
    /* Pointers to the currently used memory read and write tables. */
    drive_read_func_ptr_t *read_func_ptr;
    drive_store_func_ptr_t *store_func_ptr;
    BYTE **read_base_tab_ptr;
    DWORD *read_limit_tab_ptr;

    /* Memory read and write tables.  */
    drive_read_func_t *read_tab[1][0x101];
    drive_store_func_t *store_tab[1][0x101];
    BYTE *read_base_tab[1][0x101];
    DWORD read_limit_tab[1][0x101];

    int sync_factor;
} drivecpud_context_t;


/*
 *  Some function pointers shared by several components (VIA1, CIA1581, RIOT2)
 */

typedef struct drivefunc_context_s {
    void (*parallel_set_bus)(BYTE);
    void (*parallel_set_eoi)(BYTE); /* we may be able to eleminate these... */
    void (*parallel_set_dav)(BYTE);
    void (*parallel_set_ndac)(BYTE);
    void (*parallel_set_nrfd)(BYTE);
} drivefunc_context_t;

extern drivefunc_context_t drive_funcs[DRIVE_NUM];

/*
 * Helper macros for dual disk drives.
 */
#define is_drive0(d)    (!is_drive1(d))
#define is_drive1(d)    ((d) & 1)
#define mk_drive0(d)    ((d) & ~1)
#define mk_drive1(d)    ((d) | 1)

/*
 * The context for an entire drive.
 */

struct cia_context_s;
struct riot_context_s;
struct tpi_context_s;
struct via_context_s;
struct pc8477_s;
struct wd1770_s;

typedef struct drive_context_s {
    int mynumber;         /* init to [0123] */
    CLOCK *clk_ptr;       /* shortcut to drive_clk[mynumber] */
    struct drive_s *drive;

    struct drivecpu_context_s *cpu;
    struct drivecpud_context_s *cpud;
    struct drivefunc_context_s *func;
    struct via_context_s *via1d1541;
    struct via_context_s *via1d2031;
    struct via_context_s *via2;
    struct cia_context_s *cia1571;
    struct cia_context_s *cia1581;
    struct via_context_s *via4000;
    struct riot_context_s *riot1;
    struct riot_context_s *riot2;
    struct tpi_context_s *tpid;
    struct pc8477_s *pc8477;
    struct wd1770_s *wd1770;
} drive_context_t;

#endif
