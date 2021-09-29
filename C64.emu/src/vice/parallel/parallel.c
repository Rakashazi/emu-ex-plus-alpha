/*
 * parallel.c - IEEE488 emulation.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

/* This file contains the ieee488 emulator.
 * The ieee488 emulator calls (modifed) routines from serial.c
 * to use the standard floppy interface.
 * The current state of the bus and methods to set output lines
 * are exported.
 * This hardware emulation is necessary, as different PET kernels would
 * need different traps. But it's also much faster than the (hardware
 * simulated) serial bus, as it's parallel. So we don't need traps.
 */

/* FIXME: This should have its own log instead of using `LOG_DEFAULT'.  */

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "cmdline.h"
#include "drive.h"
#include "drivetypes.h"
#include "ieee.h"
#include "log.h"
#include "maincpu.h"
#include "parallel-trap.h"
#include "parallel.h"
#include "resources.h"
#include "types.h"


#define PARALLEL_DEBUG_VERBOSE
static int parallel_emu = 1;

void parallel_bus_enable(int enable)
{
    parallel_emu = enable;
}

/***************************************************************************
 * IEEE488 bus lines
 */

/* state of the bus lines -> "if (parallel_eoi) { eoi is active }" */
uint8_t parallel_eoi = 0;
uint8_t parallel_ndac = 0;
uint8_t parallel_nrfd = 0;
uint8_t parallel_dav = 0;
uint8_t parallel_atn = 0;

uint8_t parallel_bus = 0xff;       /* data lines */

static int par_status = 0;      /* lower 8 bits = PET par_status, upper bits own */


/***************************************************************************
 * State engine for the parallel bus
 *
 * Names here are as seen from a device, not from the PET
 *
 * Possible States
 *      WaitATN         Wait for ATN, ignore everything else
 *
 *      In1             Wait for DAV low when reading a byte
 *      In2             Wait for DAV high when reading a byte
 *
 *      OldPet          The PET 3032 doesn't set NRFD and NDAC low
 *                      before releasing ATN after a TALK command,
 *                      wait for a NDAC low first!
 *
 *      Out1            Wait for NRFD high when sending a byte
 *      Out1a           Wait for NRFD low when sending a byte
 *                      Trying to save this didn't work
 *      Out2            Wait for NDAC high when sending a byte
 *
 *
 * Each state reacts on the different line transitions.
 *
 *      ATN_true, ATN_false, NDAC_true, NDAC_false,
 *      NRFD_true, NRFD_false, DAV_true, DAV_false
 *
 *
 * Some common functions are:
 *
 *      ResetBus        Set all lines high
 *      ignore          ignore any transition
 *      unexpected      this transition is unexpected
 *
 *      Go              change the state
 *
 * Globals:
 *
 *      Trans[]         name of the transitions
 *      State[]         jump table
 *      state           actual state
 */

#define NTRANS          8       /* number of possible transitions */
#define NSTATE          7

/* States */

#define WaitATN         0
#define In1             1
#define In2             2
#define OldPet          3
#define Out1            4
#define Out1a           5
#define Out2            6

/* Transitions */

#define ATN_true        0	/* active low: 0 on the physical bus */
#define ATN_false       1
#define DAV_true        2	/* active low: 0 on the physical bus */
#define DAV_false       3
#define NDAC_true       4	/* active low: 0 on the physical bus */
#define NDAC_false      5
#define NRFD_true       6	/* active low: 0 on the physical bus */
#define NRFD_false      7

typedef struct State_t {
    const char *name;
    void (*m[NTRANS])(int);
} State_t;

#ifdef DEBUG
static const char *Trans[NTRANS] = {
    "ATN true", "ATN false", "DAV true", "DAV false",
    "NDAC true", "NDAC false", "NRFD true", "NRFD false"
};
#endif

static State_t State[NSTATE];

static int state = WaitATN;

#define Go(a)           state = (a); return
#define isListening()   ((par_status & 0xf000) == 0x2000)
#define isTalking()     ((par_status & 0xf000) == 0x4000)

#if defined(DEBUG) && defined(PARALLEL_DEBUG_VERBOSE)
static void DoTrans(int tr)
{
    if (debug.ieee) {
        log_debug("DoTrans(%s).%s", State[state].name, Trans[tr]);
    }
    State[state].m[tr](tr);
    if (debug.ieee) {
        log_debug(" -> %s", State[state].name);
    }
}
#else
#define DoTrans(a)      State[state].m[(a)]((a))
#endif

static void ResetBus(void)
{
    parallel_emu_set_dav(0);
    parallel_emu_set_eoi(0);
    parallel_emu_set_nrfd(0);
    parallel_emu_set_ndac(0);
    parallel_emu_set_bus(0xff);
    par_status = 0;
}

/**************************************************************************
 * transition functions for the state engine
 */

/* ignoreown should ignore the transition only when it is initiated
 * by the virtual IEEE488 device itself. This is not yet implemented,
 * so we just ignore all of em */

#define ignoreown       ignore

static void ignore(int i)
{
}

static void unexpected(int trans)
{
#ifdef DEBUG
    if (debug.ieee) {
        log_debug("IEEE488: unexpected line transition in state %s: %s.",
                    State[state].name, Trans[trans]);
    }
#endif
}

static void WATN_ATN_true(int tr)
{
    parallel_emu_set_ndac(1);
    parallel_emu_set_dav(0);
    parallel_emu_set_eoi(0);
    parallel_emu_set_bus(0xff);
    parallel_emu_set_nrfd(0);
    Go(In1);
}

#define In1_ATN_true       WATN_ATN_true

static void In1_ATN_false(int tr)
{
    if (par_status & 0xff) {
        ResetBus();
        Go(WaitATN);
    } else {
        if (isListening()) {
            Go(In1);
        } else {
            if (isTalking()) {
                ResetBus();
                if (!parallel_ndac) {  /* old pet... */
                    Go(OldPet);
                } else {
                    State[OldPet].m[NDAC_true](tr);
                    return;
                }
            } else {
#ifdef DEBUG
                if (debug.ieee) {
                    log_debug("IEEE488: Ouch, something weird happened: %s got %s",
                                State[In1].name, Trans[tr]);
                }
#endif
                ResetBus();
                Go(WaitATN);
            }
        }
    }
}

static void In1_DAV_true(int tr)
{
    static uint8_t b;

    parallel_emu_set_nrfd(1);
    b = parallel_bus;
    parallel_emu_set_ndac(0);

    if (parallel_atn) {
        par_status = parallel_trap_attention(b ^ 0xff);
    } else {
        par_status = parallel_trap_sendbyte((uint8_t)(b ^ 0xff));
    }
#ifdef DEBUG
    if (debug.ieee) {
        log_debug("IEEE488: sendbyte returns %04x",
                (unsigned int)par_status);
    }
#endif

    Go(In2);
}

static void In1_NDAC_true(int tr)
{
    if (!parallel_atn) {
        unexpected(tr);
    }
}

static void In1_NRFD_true(int tr)
{
    if (!parallel_atn) {
        ignoreown(tr);
    }
}


static void In1_NRFD_false(int tr)
{
    if (!parallel_atn) {
        unexpected(tr);
    }
}

#define In2_ATN_true       WATN_ATN_true

static void In2_ATN_false(int a)
{  /* atn data transfer interrupted */
    ResetBus();
    Go(WaitATN);            /* ??? */
}

static void In2_DAV_false(int tr)
{
    parallel_emu_set_ndac(1);
    parallel_emu_set_nrfd(0);

    Go(In1);
}

static void In2_NDAC_false(int tr)
{
    if (!parallel_atn) {
        unexpected(tr);
    }
}

/* OldPET fixed PET2*** and PET3*** IEEE, as well as CBM610 */

#define OPet_ATN_true      WATN_ATN_true

static void OPet_NDAC_true(int tr)
{
    if (!parallel_nrfd) {
        State[Out1].m[NRFD_false](tr);
        return;
    } else {
        Go(Out1);
    }
}

/* this is for CBM 610 only */

static void OPet_NRFD_true(int tr)
{
#ifdef DEBUG
    if (debug.ieee) {
        log_debug("OPet_NRFD_true()");
    }
#endif
    State[Out1].m[NRFD_false](tr);
}

#define Out1_ATN_true      WATN_ATN_true

static void Out1_NRFD_false(int tr)
{
    static uint8_t b;

    par_status = parallel_trap_receivebyte(&b, 1);
#ifdef DEBUG
    if (par_status & PAR_STATUS_DEVICE_NOT_PRESENT) {
        /* If we get to this function, this status should never be possible */
        log_error(LOG_DEFAULT, "Some device is talker but not present as virtual device");
    }
#endif
    parallel_emu_set_bus((uint8_t)(b ^ 0xff));

    if (par_status & PAR_STATUS_EOI) {
        parallel_emu_set_eoi(1);
    } else {
        parallel_emu_set_eoi(0);
    }

    parallel_emu_set_dav(1);

    Go(Out1a);
}

#define Out1a_ATN_true     WATN_ATN_true

static void Out1a_NRFD_true(int tr)
{
    Go(Out2);
}

static void Out1a_NDAC_false(int tr)
{
    ResetBus();
    Go(WaitATN);
}

#define Out2_ATN_true      WATN_ATN_true

static void Out2_NDAC_false(int tr)
{
    static uint8_t b;

    parallel_emu_set_dav(0);
    parallel_emu_set_eoi(0);
    parallel_emu_set_bus(0xff);

    par_status = parallel_trap_receivebyte(&b, 0);

    if (par_status & 0xff) {
        ResetBus();
        Go(WaitATN);
    } else {
        Go(Out1);
    }
}

/**************************************************************************
 * State table
 *
 */

static State_t State[NSTATE] = {
    { "WaitATN", { WATN_ATN_true, ignore, ignore, ignore,
                   ignore, ignore, ignore, ignore } },
    { "In1", { In1_ATN_true, In1_ATN_false, In1_DAV_true, unexpected,
               In1_NDAC_true, ignoreown, In1_NRFD_true, In1_NRFD_false } },
    { "In2", { In2_ATN_true, In2_ATN_false, unexpected, In2_DAV_false,
               ignoreown, In2_NDAC_false, unexpected, ignoreown } },
    { "OldPet", { OPet_ATN_true, unexpected, unexpected, unexpected,
                  OPet_NDAC_true, unexpected, OPet_NRFD_true, unexpected } },
    { "Out1", { Out1_ATN_true, unexpected, ignoreown, unexpected,
                ignore, unexpected, unexpected, Out1_NRFD_false } },
    { "Out1a", { Out1a_ATN_true, unexpected, unexpected, unexpected,
                 unexpected, Out1a_NDAC_false, Out1a_NRFD_true, unexpected } },
    { "Out2", { Out2_ATN_true, unexpected, unexpected, ignoreown,
                unexpected, Out2_NDAC_false, unexpected, unexpected } }
};

/**************************************************************************
 * methods to set handshake lines for the devices
 *
 */

#ifdef DEBUG
#define PARALLEL_LINE_DEBUG_CLR(line, linecap)                          \
    if (debug.ieee) {                                                   \
        if (old && !parallel_ ## line) {                                \
            log_debug("clr_" # line "(%02x) -> " # linecap "_false",    \
                        ~mask & 0xffU); }                               \
        else                                                            \
        if (old & ~mask) {                                              \
            log_debug("clr_" # line "(%02x) -> %02x",                   \
                        ~mask & 0xffU, parallel_ ## line); }            \
    }

#define PARALLEL_LINE_DEBUG_SET(line, linecap)                          \
    if (debug.ieee) {                                                   \
        if (!old) {                                                     \
            log_debug("set_" # line "(%02x) -> " # linecap "_true", mask); } \
        else                                                            \
        if (!(old & mask)) {                                            \
            log_debug("set_" # line "(%02x) -> %02x",                   \
                        mask, parallel_ ## line); }                     \
    }
#else
#define PARALLEL_LINE_DEBUG_CLR(line, linecap)
#define PARALLEL_LINE_DEBUG_SET(line, linecap)
#endif

void parallel_set_eoi(uint8_t mask)
{
#ifdef DEBUG
    uint8_t old = parallel_eoi;
#endif
    parallel_eoi |= mask;

    PARALLEL_LINE_DEBUG_SET(eoi, EOI)
}

void parallel_clr_eoi(uint8_t mask)
{
#ifdef DEBUG
    uint8_t old = parallel_eoi;
#endif
    parallel_eoi &= mask;

    PARALLEL_LINE_DEBUG_CLR(eoi, EOI)
}

static void parallel_atn_signal(int st)
{
    unsigned int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        if (diskunit_context[dnr]->enable) {
            ieee_drive_parallel_set_atn(st, diskunit_context[dnr]);
        }
    }
}

void parallel_set_atn(uint8_t mask)
{
    uint8_t old = parallel_atn;
    parallel_atn |= mask;

    PARALLEL_LINE_DEBUG_SET(atn, ATN)

    /* if ATN went active, signal to attached devices */
    if (!old) {
        if (parallel_emu) {
            DoTrans(ATN_true);
        }
        parallel_atn_signal(1);
    }
}

void parallel_clr_atn(uint8_t mask)
{
    uint8_t old = parallel_atn;
    parallel_atn &= mask;

    PARALLEL_LINE_DEBUG_CLR(atn, ATN)

    /* if ATN went inactive, signal to attached devices */
    if (old && !parallel_atn) {
        if (parallel_emu) {
            DoTrans(ATN_false);
        }
        parallel_atn_signal(0);
    }
}

void parallel_restore_set_atn(uint8_t mask)
{
#ifdef DEBUG
    uint8_t old = parallel_atn;
#endif
    parallel_atn |= mask;

#ifdef DEBUG
    if (debug.ieee && !old) {
        log_debug("set_atn(%02x) -> ATN_true", mask);
    }
#endif

    /* we do not send IRQ signals to chips on restore */
}

void parallel_restore_clr_atn(uint8_t mask)
{
#ifdef DEBUG
    uint8_t old = parallel_atn;
#endif
    parallel_atn &= mask;

#ifdef DEBUG
    if (debug.ieee && old && !parallel_atn) {
        log_debug("clr_atn(%02x) -> ATN_false",
                (unsigned int)(~mask & 0xff));
    }
#endif

    /* we do not send IRQ signals to chips on restore */
}

void parallel_set_dav(uint8_t mask)
{
    uint8_t old = parallel_dav;
    parallel_dav |= mask;

    PARALLEL_LINE_DEBUG_SET(dav, DAV)

    if (parallel_emu && !old) {
        DoTrans(DAV_true);
    }
}

void parallel_clr_dav(uint8_t mask)
{
    uint8_t old = parallel_dav;
    parallel_dav &= mask;

    PARALLEL_LINE_DEBUG_CLR(dav, DAV)

    if (parallel_emu && old && !parallel_dav) {
        DoTrans(DAV_false);
    }
}

void parallel_set_nrfd(uint8_t mask)
{
    uint8_t old = parallel_nrfd;
    parallel_nrfd |= mask;

    PARALLEL_LINE_DEBUG_SET(nrfd, NRFD)

    if (parallel_emu && !old) {
        DoTrans(NRFD_true);
    }
}

void parallel_clr_nrfd(uint8_t mask)
{
    uint8_t old = parallel_nrfd;
    parallel_nrfd &= mask;

    PARALLEL_LINE_DEBUG_CLR(nrfd, NRFD)

    if (parallel_emu && old && !parallel_nrfd) {
        DoTrans(NRFD_false);
    }
}

void parallel_set_ndac(uint8_t mask)
{
    uint8_t old = parallel_ndac;
    parallel_ndac |= mask;

    PARALLEL_LINE_DEBUG_SET(ndac, NDAC)

    if (parallel_emu && !old) {
        DoTrans(NDAC_true);
    }
}

void parallel_clr_ndac(uint8_t mask)
{
    uint8_t old = parallel_ndac;
    parallel_ndac &= mask;

    PARALLEL_LINE_DEBUG_CLR(ndac, NDAC)

    if (parallel_emu && old && !parallel_ndac) {
        DoTrans(NDAC_false);
    }
}

/**************************************************************************
 * methods to set data lines
 */

#ifdef DEBUG
#define PARALLEL_DEBUG_SET_BUS(type)                                    \
    if (debug.ieee) {                                               \
        log_debug(# type "_set_bus(%02x) -> %02x (%02x)", \
                    (unsigned int)b, parallel_bus, ~parallel_bus & 0xffu);             \
    }
#else
#define PARALLEL_DEBUG_SET_BUS(type)
#endif

static uint8_t par_emu_bus = 0xff;
static uint8_t par_cpu_bus = 0xff;
static uint8_t par_drv_bus[NUM_DISK_UNITS] = { 0xff, 0xff, 0xff, 0xff };

void parallel_emu_set_bus(uint8_t b)
{
    par_emu_bus = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(emu)
}

void parallel_cpu_set_bus(uint8_t b)
{
    par_cpu_bus = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(cpu)
}

void parallel_drv0_set_bus(uint8_t b)
{
    par_drv_bus[0] = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(drv0)
}

void parallel_drv1_set_bus(uint8_t b)
{
    par_drv_bus[1] = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(drv1)
}

void parallel_drv2_set_bus(uint8_t b)
{
    par_drv_bus[2] = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(drv2)
}

void parallel_drv3_set_bus(uint8_t b)
{
    par_drv_bus[3] = b;
    parallel_bus = par_emu_bus & par_cpu_bus &
                   par_drv_bus[0] & par_drv_bus[1] &
                   par_drv_bus[2] & par_drv_bus[3];

    PARALLEL_DEBUG_SET_BUS(drv3)
}

drivefunc_context_t drive_funcs[NUM_DISK_UNITS] = {
    { parallel_drv0_set_bus,
      parallel_drv0_set_eoi,
      parallel_drv0_set_dav,
      parallel_drv0_set_ndac,
      parallel_drv0_set_nrfd, },
    { parallel_drv1_set_bus,
      parallel_drv1_set_eoi,
      parallel_drv1_set_dav,
      parallel_drv1_set_ndac,
      parallel_drv1_set_nrfd, },
    { parallel_drv2_set_bus,
      parallel_drv2_set_eoi,
      parallel_drv2_set_dav,
      parallel_drv2_set_ndac,
      parallel_drv2_set_nrfd, },
    { parallel_drv3_set_bus,
      parallel_drv3_set_eoi,
      parallel_drv3_set_dav,
      parallel_drv3_set_ndac,
      parallel_drv3_set_nrfd, },
};

/**************************************************************************
 *
 */

#define PARALLEL_CPU_SET_LINE(line, dev, mask)     \
    void parallel_##dev##_set_##line(char val)     \
    {                                              \
        drive_cpu_execute_all(maincpu_clk);         \
        if (val) {                                 \
            parallel_set_##line(PARALLEL_##mask);  \
        } else {                                   \
            parallel_clr_##line(~PARALLEL_##mask); \
        }                                          \
    }

PARALLEL_CPU_SET_LINE(atn, cpu, CPU)
