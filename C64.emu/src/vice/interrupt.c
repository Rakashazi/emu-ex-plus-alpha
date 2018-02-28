/*
 * interrupt.c - Implementation of CPU interrupts and alarms.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6510core.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "snapshot.h"
#include "types.h"


/* Initialization.  */
void interrupt_cpu_status_init(interrupt_cpu_status_t *cs,
                               unsigned int *last_opcode_info_ptr)
{
    cs->num_ints = 0;
    cs->pending_int = NULL;
    cs->int_name = NULL;
    cs->last_opcode_info_ptr = last_opcode_info_ptr;
}

void interrupt_cpu_status_reset(interrupt_cpu_status_t *cs)
{
    unsigned int num_ints, *pending_int, *last_opcode_info_ptr;
    char **int_name;

    num_ints = cs->num_ints;
    pending_int = cs->pending_int;
    int_name = cs->int_name;
    last_opcode_info_ptr = cs->last_opcode_info_ptr;
    if (num_ints > 0) {
        memset(pending_int, 0, num_ints * sizeof(*(cs->pending_int)));
    }
    memset(cs, 0, sizeof(interrupt_cpu_status_t));
    cs->num_ints = num_ints;
    cs->pending_int = pending_int;
    cs->int_name = int_name;
    cs->last_opcode_info_ptr = last_opcode_info_ptr;

    cs->num_last_stolen_cycles = 0;
    cs->last_stolen_cycles_clk = (CLOCK)0;
    cs->num_dma_per_opcode = 0;
    cs->irq_delay_cycles = 0;
    cs->nmi_delay_cycles = 0;
    cs->global_pending_int = IK_NONE;
    cs->nmi_trap_func = NULL;
    cs->reset_trap_func = NULL;
    cs->irq_pending_clk = CLOCK_MAX;
}

unsigned int interrupt_cpu_status_int_new(interrupt_cpu_status_t *cs,
                                          const char *name)
{
    cs->num_ints += 1;

    cs->pending_int = lib_realloc(cs->pending_int, cs->num_ints * sizeof(*(cs->pending_int)));
    cs->pending_int[cs->num_ints - 1] = 0;

    cs->int_name = lib_realloc(cs->int_name, cs->num_ints * sizeof(char *));
    cs->int_name[cs->num_ints - 1] = lib_stralloc(name);

    return cs->num_ints - 1;
}

interrupt_cpu_status_t *interrupt_cpu_status_new(void)
{
    return (interrupt_cpu_status_t *)lib_calloc(1, sizeof(interrupt_cpu_status_t));
}

void interrupt_cpu_status_destroy(interrupt_cpu_status_t *cs)
{
    if (cs != NULL) {
        unsigned int num;

        for (num = 0; num < cs->num_ints; num++) {
            lib_free(cs->int_name[num]);
        }

        lib_free(cs->int_name);
        lib_free(cs->pending_int);
    }

    lib_free(cs);
}

void interrupt_set_nmi_trap_func(interrupt_cpu_status_t *cs,
                                 void (*nmi_trap_func)(void))
{
    cs->nmi_trap_func = nmi_trap_func;
}

void interrupt_set_reset_trap_func(interrupt_cpu_status_t *cs, void (*reset_trap_func)(void))
{
    cs->reset_trap_func = reset_trap_func;
}

/* Move all the CLOCK time references forward/backward.  */
void interrupt_cpu_status_time_warp(interrupt_cpu_status_t *cs,
                                    CLOCK warp_amount, int warp_direction)
{
    if (warp_direction == 0) {
        return;
    }

    if (warp_direction > 0) {
        cs->irq_clk += warp_amount;
        cs->nmi_clk += warp_amount;
        cs->last_stolen_cycles_clk += warp_amount;
        if (cs->irq_pending_clk < CLOCK_MAX) {
            cs->irq_pending_clk += warp_amount;
        }
    } else {
        if (cs->irq_clk > warp_amount) {
            cs->irq_clk -= warp_amount;
        } else {
            cs->irq_clk = (CLOCK) 0;
        }
        if (cs->nmi_clk > warp_amount) {
            cs->nmi_clk -= warp_amount;
        } else {
            cs->nmi_clk = (CLOCK) 0;
        }
        if (cs->last_stolen_cycles_clk > warp_amount) {
            cs->last_stolen_cycles_clk -= warp_amount;
        } else {
            cs->last_stolen_cycles_clk = (CLOCK) 0;
        }
        if (cs->irq_pending_clk < CLOCK_MAX) {
            if (cs->irq_pending_clk > warp_amount) {
                cs->irq_pending_clk -= warp_amount;
            } else {
                cs->irq_pending_clk = (CLOCK) 0;
            }
        }
    }
}

void interrupt_log_wrong_nirq(void)
{
    log_error(LOG_DEFAULT, "interrupt_set_irq(): wrong nirq!");
}

void interrupt_log_wrong_nnmi(void)
{
    log_error(LOG_DEFAULT, "interrupt_set_nmi(): wrong nnmi!");
}

/* ------------------------------------------------------------------------- */

/* These functions are used by snaphots only: they update the IRQ/NMI line
   value, but do not update the variables used to emulate the timing.  This
   is necessary to allow the chip modules to dump their own IRQ/NMI status
   information; the global timing status is stored in the CPU module (see
   `interrupt_write_snapshot()' and `interrupt_read_snapshot()'). */

void interrupt_restore_irq(interrupt_cpu_status_t *cs, int int_num, int value)
{
    if (value) {
        cs->pending_int[int_num] |= IK_IRQ;
    } else {
        cs->pending_int[int_num] &= ~IK_IRQ;
    }
}

void interrupt_restore_nmi(interrupt_cpu_status_t *cs, int int_num, int value)
{
    if (value) {
        cs->pending_int[int_num] |= IK_NMI;
    } else {
        cs->pending_int[int_num] &= ~IK_NMI;
    }
}

int interrupt_get_irq(interrupt_cpu_status_t *cs, int int_num)
{
    return cs->pending_int[int_num] & IK_IRQ;
}

int interrupt_get_nmi(interrupt_cpu_status_t *cs, int int_num)
{
    return cs->pending_int[int_num] & IK_NMI;
}

void interrupt_fixup_int_clk(interrupt_cpu_status_t *cs, CLOCK cpu_clk,
                             CLOCK *int_clk)
{
    unsigned int num_cycles_left = 0, last_num_cycles_left = 0, num_dma;
    unsigned int cycles_left_to_trigger_irq = (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr) ? 2 : 1);
    CLOCK last_start_clk = CLOCK_MAX;

#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        unsigned int i;
        log_debug("INTREQ %ld NUMWR %i", (long)cpu_clk,
                  maincpu_num_write_cycles());
        for (i = 0; i < cs->num_dma_per_opcode; i++) {
            log_debug("%iCYLEFT %i STCLK %i", i, cs->num_cycles_left[i],
                      cs->dma_start_clk[i]);
        }
    }
#endif

    num_dma = cs->num_dma_per_opcode;
    while (num_dma != 0) {
        num_dma--;
        num_cycles_left = cs->num_cycles_left[num_dma];
        if ((cs->dma_start_clk[num_dma] - 1) <= cpu_clk) {
            break;
        }
        last_num_cycles_left = num_cycles_left;
        last_start_clk = cs->dma_start_clk[num_dma];
    }
    /* if the INTREQ happens between two CPU cycles, we have to interpolate */
    if (num_cycles_left - last_num_cycles_left > last_start_clk - cpu_clk - 1) {
        num_cycles_left = last_num_cycles_left + last_start_clk - cpu_clk - 1;
    }

#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        log_debug("TAKENLEFT %i   LASTSTOLENCYCLECLK %i", num_cycles_left, cs->last_stolen_cycles_clk);
    }
#endif

    *int_clk = cs->last_stolen_cycles_clk;
    if (cs->num_dma_per_opcode > 0 && cs->dma_start_clk[0] > cpu_clk) {
        /* interrupt was triggered before end of last opcode */
        *int_clk -= (cs->dma_start_clk[0] - cpu_clk);
    }
#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        log_debug("INTCLK dma shifted %i   (cs->dma_start_clk[0]=%i", *int_clk, cs->dma_start_clk[0]);
    }
#endif

    if (num_cycles_left >= cycles_left_to_trigger_irq) {
        *int_clk -= (cycles_left_to_trigger_irq + 1);
    }

#ifdef DEBUGIRQDMA
    if (debug.maincpu_traceflg) {
        log_debug("INTCLK fixed %i", *int_clk);
    }
#endif
}

/* ------------------------------------------------------------------------- */

void interrupt_trigger_dma(interrupt_cpu_status_t *cs, CLOCK cpu_clk)
{
    cs->global_pending_int = (enum cpu_int)(cs->global_pending_int | IK_DMA);
}

void interrupt_ack_dma(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int = (enum cpu_int)(cs->global_pending_int & ~IK_DMA);
}

/* ------------------------------------------------------------------------- */

/* Trigger a RESET.  This resets the machine.  */
void interrupt_trigger_reset(interrupt_cpu_status_t *cs, CLOCK cpu_clk)
{
    if (cs == NULL) {
        return;
    }

    cs->global_pending_int |= IK_RESET;
}

/* Acknowledge a RESET condition, by removing it.  */
void interrupt_ack_reset(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int &= ~IK_RESET;

    if (cs->reset_trap_func) {
        cs->reset_trap_func();
    }
}

/* Trigger a TRAP.  This is a special condition that can be used for
   debugging.  `trap_func' will be called with PC as the argument when this
   condition is detected.  */
void interrupt_maincpu_trigger_trap(void (*trap_func)(WORD, void *data),
                                    void *data)
{
    interrupt_cpu_status_t *cs = maincpu_int_status;

    cs->global_pending_int |= IK_TRAP;
    cs->trap_func = trap_func;
    cs->trap_data = data;
}


/* Dispatch the TRAP condition.  */
void interrupt_do_trap(interrupt_cpu_status_t *cs, WORD address)
{
    cs->global_pending_int &= ~IK_TRAP;
    cs->trap_func(address, cs->trap_data);
}

void interrupt_monitor_trap_on(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int |= IK_MONITOR;
}

void interrupt_monitor_trap_off(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int &= ~IK_MONITOR;
}

/* ------------------------------------------------------------------------- */

int interrupt_write_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    /* FIXME: could we avoid some of this info?  */
    if (SMW_DW(m, cs->irq_clk) < 0
        || SMW_DW(m, cs->nmi_clk) < 0
        || SMW_DW(m, cs->irq_pending_clk) < 0
        || SMW_DW(m, (DWORD)cs->num_last_stolen_cycles) < 0
        || SMW_DW(m, cs->last_stolen_cycles_clk) < 0) {
        return -1;
    }

    return 0;
}

int interrupt_write_new_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    if (0
        || SMW_DW(m, cs->nirq) < 0
        || SMW_DW(m, cs->nnmi) < 0
        || SMW_DW(m, cs->global_pending_int) < 0) {
        return -1;
    }

    return 0;
}

int interrupt_write_sc_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    if (0
        || SMW_DW(m, cs->irq_delay_cycles) < 0
        || SMW_DW(m, cs->nmi_delay_cycles) < 0) {
        return -1;
    }

    return 0;
}

int interrupt_read_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    unsigned int i;
    DWORD dw;

    for (i = 0; i < cs->num_ints; i++) {
        cs->pending_int[i] = IK_NONE;
    }

    cs->global_pending_int = IK_NONE;
    cs->nirq = cs->nnmi = cs->reset = cs->trap = 0;

    if (0
        || SMR_DW(m, &cs->irq_clk) < 0
        || SMR_DW(m, &cs->nmi_clk) < 0
        || SMR_DW(m, &cs->irq_pending_clk) < 0) {
        return -1;
    }

    if (SMR_DW(m, &dw) < 0) {
        return -1;
    }
    cs->num_last_stolen_cycles = dw;

    if (SMR_DW(m, &dw) < 0) {
        return -1;
    }
    cs->last_stolen_cycles_clk = dw;

    return 0;
}

int interrupt_read_new_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    if (0
        || SMR_DW_INT(m, &cs->nirq) < 0
        || SMR_DW_INT(m, &cs->nnmi) < 0
        || SMR_DW_UINT(m, &cs->global_pending_int) < 0) {
        return -1;
    }

    return 0;
}

int interrupt_read_sc_snapshot(interrupt_cpu_status_t *cs, snapshot_module_t *m)
{
    if (0
        || SMR_DW_UINT(m, &cs->irq_delay_cycles) < 0
        || SMR_DW_UINT(m, &cs->nmi_delay_cycles) < 0) {
        return -1;
    }

    return 0;
}
