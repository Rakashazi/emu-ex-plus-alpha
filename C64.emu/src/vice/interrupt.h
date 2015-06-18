/*
 * interrupt.h - Implementation of 6510 interrupts and alarms.
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

#ifndef VICE_INTERRUPT_H
#define VICE_INTERRUPT_H

#include <stdio.h>

#include "debug.h"
#include "log.h"
#include "types.h"

/* Define the number of cycles needed by the CPU to detect the NMI or IRQ.  */
#define INTERRUPT_DELAY 2

#define INTRRUPT_MAX_DMA_PER_OPCODE (7 + 10000)

/* These are the available types of interrupt lines.  */
enum cpu_int {
    IK_NONE    = 0,
    IK_NMI     = 1 << 0,
    IK_IRQ     = 1 << 1,
    IK_RESET   = 1 << 2,
    IK_TRAP    = 1 << 3,
    IK_MONITOR = 1 << 4,
    IK_DMA     = 1 << 5,
    IK_IRQPEND = 1 << 6
};

struct interrupt_cpu_status_s {
    /* Number of interrupt lines.  */
    unsigned int num_ints;

    /* Define, for each interrupt source, whether it has a pending interrupt
       (IK_IRQ, IK_NMI, IK_RESET and IK_TRAP) or not (IK_NONE).  */
    unsigned int *pending_int;

    /* Name for each interrupt source */
    char **int_name;

    /* Number of active IRQ lines.  */
    int nirq;

    /* Tick when the IRQ was triggered.  */
    CLOCK irq_clk;

    /* Number of active NMI lines.  */
    int nnmi;

    /* Tick when the NMI was triggered.  */
    CLOCK nmi_clk;

    /* If an opcode is intercepted by a DMA, save the number of cycles
       left at the start of this particular DMA (needed by *_set_irq() to
       calculate irq_clk).  */
    unsigned int num_dma_per_opcode;
    unsigned int num_cycles_left[INTRRUPT_MAX_DMA_PER_OPCODE];
    CLOCK dma_start_clk[INTRRUPT_MAX_DMA_PER_OPCODE];

    /* counters for delay between interrupt request and handler */
    unsigned int irq_delay_cycles;
    unsigned int nmi_delay_cycles;

    /* If 1, do a RESET.  */
    int reset;

    /* If 1, call the trapping function.  */
    int trap;

    /* Debugging function.  */
    void (*trap_func)(WORD, void *data);

    /* Data to pass to the debugging function when called.  */
    void *trap_data;

    /* Pointer to the last executed opcode information.  */
    unsigned int *last_opcode_info_ptr;

    /* Number of cycles we have stolen to the processor last time.  */
    int num_last_stolen_cycles;

    /* Clock tick at which these cycles have been stolen.  */
    CLOCK last_stolen_cycles_clk;

    /* Clock tick where just ACK'd IRQs may still trigger an interrupt.
       Set to CLOCK_MAX when irrelevant.  */
    CLOCK irq_pending_clk;

    unsigned int global_pending_int;

    void (*nmi_trap_func)(void);

    void (*reset_trap_func)(void);
};
typedef struct interrupt_cpu_status_s interrupt_cpu_status_t;

/* ------------------------------------------------------------------------- */

extern void interrupt_log_wrong_nirq(void);
extern void interrupt_log_wrong_nnmi(void);

extern void interrupt_trigger_dma(interrupt_cpu_status_t *cs, CLOCK cpu_clk);
extern void interrupt_ack_dma(interrupt_cpu_status_t *cs);
extern void interrupt_fixup_int_clk(interrupt_cpu_status_t *cs, CLOCK cpu_clk,
                                    CLOCK *int_clk);

/* Set the IRQ line state.  */
inline static void interrupt_set_irq(interrupt_cpu_status_t *cs,
                                     unsigned int int_num,
                                     int value, CLOCK cpu_clk)
{
    if (cs == NULL || int_num >= cs->num_ints) {
        return;
    }

    if (value) {                /* Trigger the IRQ.  */
        if (!(cs->pending_int[int_num] & IK_IRQ)) {
            cs->nirq++;
            cs->global_pending_int = (cs->global_pending_int | (unsigned int)(IK_IRQ | IK_IRQPEND));
            cs->pending_int[int_num] = (cs->pending_int[int_num] | (unsigned int)IK_IRQ);

            cs->irq_pending_clk = CLOCK_MAX;

            /* This makes sure that IRQ delay is correctly emulated when
               cycles are stolen from the CPU.  */
#ifdef DEBUG
            if (debug.maincpu_traceflg) {
                log_debug("ICLK=%i  last_stolen_cycle=%d", cpu_clk, cs->last_stolen_cycles_clk);
            }
#endif
            cs->irq_delay_cycles = 0;

            if (cs->last_stolen_cycles_clk <= cpu_clk) {
                cs->irq_clk = cpu_clk;
            } else {
                interrupt_fixup_int_clk(cs, cpu_clk, &(cs->irq_clk));
            }
        }
    } else {                    /* Remove the IRQ condition.  */
        if (cs->pending_int[int_num] & IK_IRQ) {
            if (cs->nirq > 0) {
                cs->pending_int[int_num] =
                    (cs->pending_int[int_num] & (unsigned int)~IK_IRQ);
                if (--cs->nirq == 0) {
                    cs->global_pending_int =
                        (cs->global_pending_int & (unsigned int)~IK_IRQ);
                    cs->irq_pending_clk = cpu_clk + 3;
                }
            } else {
                interrupt_log_wrong_nirq();
            }
        }
    }
}

/* Set the NMI line state.  */
inline static void interrupt_set_nmi(interrupt_cpu_status_t *cs,
                                     unsigned int int_num,
                                     int value, CLOCK cpu_clk)
{
    if (cs == NULL || int_num >= cs->num_ints) {
        return;
    }

    if (value) {                /* Trigger the NMI.  */
        if (!(cs->pending_int[int_num] & IK_NMI)) {
            if (cs->nnmi == 0 && !(cs->global_pending_int & IK_NMI)) {
                cs->global_pending_int = (cs->global_pending_int | IK_NMI);

#ifdef DEBUG
                if (debug.maincpu_traceflg) {
                    log_debug("ICLK=%i  last_stolen_cycle=%d", cpu_clk, cs->last_stolen_cycles_clk);
                }
#endif
                /* This makes sure that NMI delay is correctly emulated when
                   cycles are stolen from the CPU.  */
                cs->nmi_delay_cycles = 0;

                if (cs->last_stolen_cycles_clk <= cpu_clk) {
                    cs->nmi_clk = cpu_clk;
                } else {
                    interrupt_fixup_int_clk(cs, cpu_clk, &(cs->nmi_clk));
                }
            }
            cs->nnmi++;
            cs->pending_int[int_num] = (cs->pending_int[int_num] | IK_NMI);
        }
    } else {                    /* Remove the NMI condition.  */
        if (cs->pending_int[int_num] & IK_NMI) {
            if (cs->nnmi > 0) {
                cs->nnmi--;
                cs->pending_int[int_num] =
                    (cs->pending_int[int_num] & ~IK_NMI);
#if 0
                /* It should not be possible to remove the NMI condition,
                   only interrupt_ack_nmi() should clear it.  */
                if (cpu_clk == cs->nmi_clk) {
                    cs->global_pending_int = (enum cpu_int)(cs->global_pending_int & ~IK_NMI);
                }
#endif
            } else {
                interrupt_log_wrong_nnmi();
            }
        }
    }
}

/* Change the interrupt line state: this can be used to change both NMI
   and IRQ lines.  It is slower than `interrupt_set_nmi()' and
   `interrupt_set_irq()', but is left for backward compatibility (it works
   like the old `setirq()').  */
inline static void interrupt_set_int(interrupt_cpu_status_t *cs, int int_num,
                                     enum cpu_int value, CLOCK cpu_clk)
{
    interrupt_set_nmi(cs, int_num, (int)(value & IK_NMI), cpu_clk);
    interrupt_set_irq(cs, int_num, (int)(value & IK_IRQ), cpu_clk);
}

/* ------------------------------------------------------------------------- */

/* This function must be called by the CPU emulator when a pending NMI/IRQ
   request is served.  */
inline static void interrupt_ack_nmi(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int =
        (cs->global_pending_int & ~IK_NMI);

    if (cs->nmi_trap_func) {
        cs->nmi_trap_func();
    }
}

inline static void interrupt_ack_irq(interrupt_cpu_status_t *cs)
{
    cs->global_pending_int =
        (cs->global_pending_int & ~IK_IRQPEND);
    cs->irq_pending_clk = CLOCK_MAX;
}
/* ------------------------------------------------------------------------- */

/* Extern functions.  These are defined in `interrupt.c'.  */

struct snapshot_module_s;

extern interrupt_cpu_status_t *interrupt_cpu_status_new(void);
extern void interrupt_cpu_status_destroy(interrupt_cpu_status_t *cs);
extern void interrupt_cpu_status_init(interrupt_cpu_status_t *cs,
                                      unsigned int *last_opcode_info_ptr);
extern void interrupt_cpu_status_reset(interrupt_cpu_status_t *cs);

extern void interrupt_trigger_reset(interrupt_cpu_status_t *cs, CLOCK cpu_clk);
extern unsigned int interrupt_cpu_status_int_new(interrupt_cpu_status_t *cs,
                                                 const char *name);
extern void interrupt_ack_reset(interrupt_cpu_status_t *cs);
extern void interrupt_set_reset_trap_func(interrupt_cpu_status_t *cs, void (*reset_trap_func)(void));
extern void interrupt_maincpu_trigger_trap(void (*trap_func)(WORD, void *data), void *data);
extern void interrupt_do_trap(interrupt_cpu_status_t *cs, WORD address);

extern void interrupt_monitor_trap_on(interrupt_cpu_status_t *cs);
extern void interrupt_monitor_trap_off(interrupt_cpu_status_t *cs);

extern void interrupt_cpu_status_time_warp(interrupt_cpu_status_t *cs,
                                           CLOCK warp_amount,
                                           int warp_direction);

extern int interrupt_read_snapshot(interrupt_cpu_status_t *cs,
                                   struct snapshot_module_s *m);
extern int interrupt_read_new_snapshot(interrupt_cpu_status_t *cs,
                                       struct snapshot_module_s *m);
extern int interrupt_read_sc_snapshot(interrupt_cpu_status_t *cs,
                                      struct snapshot_module_s *m);
extern int interrupt_write_snapshot(interrupt_cpu_status_t *cs,
                                    struct snapshot_module_s *m);
extern int interrupt_write_new_snapshot(interrupt_cpu_status_t *cs,
                                        struct snapshot_module_s *m);
extern int interrupt_write_sc_snapshot(interrupt_cpu_status_t *cs,
                                       struct snapshot_module_s *m);

extern void interrupt_restore_irq(interrupt_cpu_status_t *cs, int int_num,
                                  int value);
extern void interrupt_restore_nmi(interrupt_cpu_status_t *cs, int int_num,
                                  int value);
extern int interrupt_get_irq(interrupt_cpu_status_t *cs, int int_num);
extern int interrupt_get_nmi(interrupt_cpu_status_t *cs, int int_num);
extern void interrupt_set_nmi_trap_func(interrupt_cpu_status_t *cs,
                                        void (*nmi_trap_func)(void));

/* ------------------------------------------------------------------------- */

extern interrupt_cpu_status_t *maincpu_int_status;
extern CLOCK maincpu_clk;

/* For convenience...  */

#define maincpu_set_irq(int_num, value) \
    interrupt_set_irq(maincpu_int_status, (int_num), (value), maincpu_clk)
#define maincpu_set_irq_clk(int_num, value, clk) \
    interrupt_set_irq(maincpu_int_status, (int_num), (value), (clk))
#define maincpu_set_nmi(int_num, value) \
    interrupt_set_nmi(maincpu_int_status, (int_num), (value), maincpu_clk)
#define maincpu_set_nmi_clk(int_num, value, clk) \
    interrupt_set_nmi(maincpu_int_status, (int_num), (value), (clk))
#define maincpu_set_int(int_num, value) \
    interrupt_set_int(maincpu_int_status, (int_num), (value), maincpu_clk)
#define maincpu_set_int_clk(int_num, value, clk) \
    interrupt_set_int(maincpu_int_status, (int_num), (value), (clk))
#define maincpu_trigger_reset() \
    interrupt_trigger_reset(maincpu_int_status, maincpu_clk)

#endif
