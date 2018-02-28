/*
 * Copyright 2001 by Arto Salmi and Joze Fabcic
 * Copyright 2006 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 *
 * GCC6809 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GCC6809 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC6809; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef M6809_H
#define M6809_H

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "alarm.h"
#include "interrupt.h"

/* Uncomment the following if you want emulation of the
   6809 illegal/undocumented opcodes/behaviour. */
#define FULL6809

/* Uncomment the following if you want emulation of the
   6309 CPU. */
/* #define H6309 */

/* Sanity check */
#if defined(FULL6809) && defined(H6309)
#error cannot use FULL6809 and H6309 at the same time.
#endif

typedef unsigned char UINT8;
typedef signed char INT8;

typedef WORD UINT16;
typedef SWORD INT16;

typedef WORD target_addr_t;

typedef unsigned int absolute_address_t;

#define MAX_CPU_ADDR 65536

/* end of #include "machine.h" */

#define E_FLAG 0x80
#define F_FLAG 0x40
#define H_FLAG 0x20
#define I_FLAG 0x10
#define N_FLAG 0x08
#define Z_FLAG 0x04
#define V_FLAG 0x02
#define C_FLAG 0x01

/* Primitive read/write macros */
#define read8(addr)      mem6809_read((addr))
#define write8(addr, val) mem6809_store((addr), (val))


/* 16-bit versions */
#define read16(addr)       mem6809_read16((addr))
#define write16(addr, val)                   \
    do {                                     \
        write8((addr) + 1, (val) & 0xFF);    \
        write8((addr), ((val) >> 8) & 0xFF); \
    } while (0)


/* 32-bit versions */
#ifdef H6309
#define read32(addr)       mem6809_read32((addr))
#define write32(addr, val)                       \
    do {                                         \
        write16((addr) + 2, (val) & 0xFFFF);     \
        write16((addr), ((val) >> 16) & 0xFFFF); \
    } while (0)
#endif

/* Fetch macros */

#define abs_read8(addr)    read8(addr)
#define abs_read16(addr)   ((abs_read8(addr) << 8) | abs_read8(addr + 1))

#define fetch8()           abs_read8 (pc++)
#define fetch16()          (pc += 2, abs_read16(pc - 2))

/* 6809.c */
extern void h6809_mainloop (struct interrupt_cpu_status_s *, struct alarm_context_s *);
extern void cpu6809_reset (void);
struct snapshot_s;
extern int cpu6809_snapshot_write_module(struct snapshot_s *s);
extern int cpu6809_snapshot_read_module(struct snapshot_s *s);

#endif /* M6809_H */
