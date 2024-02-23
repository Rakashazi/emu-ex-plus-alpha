/*
 * parallel.h
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

/* This file contains the exported interface to the iec488 emulator.
 * The iec488 emulator then calls (modifed) routines from serial.c
 * to use the standard floppy interface.
 * The current state of the bus and methods to set output lines
 * are exported.
 * This hardware emulation is necessary, as different PET kernels would
 * need different traps. But it's also much faster than the (hardware
 * simulated) serial bus, as it's parallel. So we need no traps.
 */

#ifndef VICE_PARALLEL_H
#define VICE_PARALLEL_H

#include "types.h"

/* to switch on/off IEEE488 filesystem engine */
void parallel_bus_enable(unsigned int unit, unsigned int enable);

void parallel_trap_eof_callback_set(void (*func)(void));
void parallel_trap_attention_callback_set(void (*func)(void));

/* state of the bus lines -> "if (parallel_eoi) { eoi is active }" */
extern uint8_t parallel_eoi;
extern uint8_t parallel_ndac;
extern uint8_t parallel_nrfd;
extern uint8_t parallel_dav;
extern uint8_t parallel_atn;

extern uint8_t parallel_bus;       /* data lines */

/* Each device has a mask bit in the parallel_* handshake lines */
#define PARALLEL_EMU    0x01
#define PARALLEL_CPU    0x02
#define PARALLEL_DRV0   0x04
#define PARALLEL_DRV1   0x08
#define PARALLEL_DRV2   0x10
#define PARALLEL_DRV3   0x20

/* methods to set handshake lines active for the devices */
void parallel_set_eoi(uint8_t mask);
void parallel_set_ndac(uint8_t mask);
void parallel_set_nrfd(uint8_t mask);
void parallel_set_dav(uint8_t mask);
void parallel_set_atn(uint8_t mask);
void parallel_restore_set_atn(uint8_t mask);

/* methods to set handshake lines inactive for the devices */
void parallel_clr_eoi(uint8_t mask);
void parallel_clr_ndac(uint8_t mask);
void parallel_clr_nrfd(uint8_t mask);
void parallel_clr_dav(uint8_t mask);
void parallel_clr_atn(uint8_t mask);
void parallel_restore_clr_atn(uint8_t mask);


/* methods to set output lines for the computer */
#define PARALLEL_SET_LINE(line, dev, mask)                   \
    static inline void parallel_##dev##_set_##line(uint8_t val) \
    {                                                        \
        if (val) {                                           \
            parallel_set_##line(PARALLEL_##mask);            \
        } else {                                             \
            parallel_clr_##line(~PARALLEL_##mask);           \
        }                                                    \
    }

#define PARALLEL_RESTORE_LINE(line, dev, mask)                    \
    static inline void parallel_##dev##_restore_##line(uint8_t val)  \
    {                                                             \
        if (val) {                                                \
            parallel_restore_set_##line(PARALLEL_##mask);         \
        } else {                                                  \
            parallel_restore_clr_##line(~PARALLEL_##mask);        \
        }                                                         \
    }

/* Emulator functions */
PARALLEL_SET_LINE(eoi, emu, EMU)
PARALLEL_SET_LINE(dav, emu, EMU)
PARALLEL_SET_LINE(nrfd, emu, EMU)
PARALLEL_SET_LINE(ndac, emu, EMU)

void parallel_emu_set_bus(uint8_t b);

/* CPU functions */
/* The *CPU* macros advance the drive CPU to the current clock. This
   is currently esp. for the VIC20 VIC1112 IEEE488 module. This seems
   to be necessary for ATN only. Too many of them make IEEE488
   slower... (AF, 01AUG1999) */
PARALLEL_SET_LINE(eoi, cpu, CPU)
PARALLEL_SET_LINE(dav, cpu, CPU)
PARALLEL_SET_LINE(nrfd, cpu, CPU)
PARALLEL_SET_LINE(ndac, cpu, CPU)

void parallel_cpu_set_atn(char val);

PARALLEL_RESTORE_LINE(atn, cpu, CPU)

void parallel_cpu_set_bus(uint8_t b);

/* Drive 0 functions */
PARALLEL_SET_LINE(eoi, drv0, DRV0)
PARALLEL_SET_LINE(dav, drv0, DRV0)
PARALLEL_SET_LINE(nrfd, drv0, DRV0)
PARALLEL_SET_LINE(ndac, drv0, DRV0)

void parallel_drv0_set_bus(uint8_t b);

/* Drive 1 functions */
PARALLEL_SET_LINE(eoi, drv1, DRV1)
PARALLEL_SET_LINE(dav, drv1, DRV1)
PARALLEL_SET_LINE(nrfd, drv1, DRV1)
PARALLEL_SET_LINE(ndac, drv1, DRV1)

void parallel_drv1_set_bus(uint8_t b);

/* Drive 2 functions */
PARALLEL_SET_LINE(eoi, drv2, DRV2)
PARALLEL_SET_LINE(dav, drv2, DRV2)
PARALLEL_SET_LINE(nrfd, drv2, DRV2)
PARALLEL_SET_LINE(ndac, drv2, DRV2)

void parallel_drv2_set_bus(uint8_t b);

/* Drive 3 functions */
PARALLEL_SET_LINE(eoi, drv3, DRV3)
PARALLEL_SET_LINE(dav, drv3, DRV3)
PARALLEL_SET_LINE(nrfd, drv3, DRV3)
PARALLEL_SET_LINE(ndac, drv3, DRV3)

void parallel_drv3_set_bus(uint8_t b);

#endif
