/*
 * ciacore.c - Template file for MOS6526 (CIA) emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * Patches and improvements by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de>
 *  Olaf Seibert <rhialto@falu.nl>
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

/* #define DEBUG_CIA */

#ifdef DEBUG_CIA
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cia.h"
#include "ciatimer.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"

#define STORE_OFFSET 1
#define READ_OFFSET 0

/*
 * Values for field sdr_delay.
 *
 * Each group of bits is either some event that needs to happen in the near
 * future, where ...0 is the time when it happens.
 *
 * Or it is something that happened in the near past, where the number
 * increases every clock tick.
 *
 * As long as the sdr_alarm is active, they will be shifted LEFT one position
 * each time it is called (and the bits moving into the next group are
 * cleared), which is once per clock until no longer needed.
 * Flags are added as needed to schedule tasks in the future.
 *
 * It's like a software version of a mercury delay line (or several in
 * parallel). It avoids needing several alarm callbacks, where you're
 * not quite sure of the ordering if they are for the same cycle.
 *
 * Not all bits are actually used, but using groups of 4 makes it
 * easier to understand the value when printed.
 */
#define CIA_SDR_TOGGLE_CNT2     0x0001u /* countdown to toggling CNT */
#define CIA_SDR_TOGGLE_CNT1     0x0002u
#define CIA_SDR_TOGGLE_CNT0     0x0004u
#define CIA_SDR_TOGGLE_CNT_1    0x0008u /* _1 = -1 */

#define CIA_SDR_NOGGLE_CNT2     0x0010u /* countdown to NOT toggling CNT */
#define CIA_SDR_NOGGLE_CNT1     0x0020u
#define CIA_SDR_NOGGLE_CNT0     0x0040u
#define CIA_SDR_NOGGLE_CNT_1    0x0080u

#define CIA_SDR_SET_SDR_IRQ3    0x0100u /* countdown to setting the SDR IRQ */
#define CIA_SDR_SET_SDR_IRQ2    0x0200u
#define CIA_SDR_SET_SDR_IRQ1    0x0400u
#define CIA_SDR_SET_SDR_IRQ0    0x0800u

#define CIA_SDR_CNT0            0x1000u /* history of the CNT output value */
#define CIA_SDR_CNT1            0x2000u /* After all these bits are set or */
#define CIA_SDR_CNT2            0x4000u /* reset, we can stop updating them */
#define CIA_SDR_CNT3            0x8000u /* until CNT is toggled. */

#define CIA_SDR_SET3        0x00010000u /* countdown to setting shifter */
#define CIA_SDR_SET2        0x00020000u /* from SDR */
#define CIA_SDR_SET1        0x00040000u
#define CIA_SDR_SET0        0x00080000u

#define CIA_SDR_LEFTMOST    0x00100000u

#define CIA_SDR_CLEAR   (CIA_SDR_NOGGLE_CNT2 | CIA_SDR_SET_SDR_IRQ3 | \
                         CIA_SDR_CNT0 | CIA_SDR_SET3 | CIA_SDR_LEFTMOST)

#define CIA_SDR_ACTIVE  (CIA_SDR_TOGGLE_CNT2 | CIA_SDR_TOGGLE_CNT1 |   \
                         CIA_SDR_TOGGLE_CNT0 | CIA_SDR_TOGGLE_CNT_1 |  \
                         CIA_SDR_NOGGLE_CNT2 | CIA_SDR_NOGGLE_CNT1 |   \
                         CIA_SDR_NOGGLE_CNT0 | CIA_SDR_NOGGLE_CNT_1 |  \
                         CIA_SDR_SET_SDR_IRQ3 | CIA_SDR_SET_SDR_IRQ2 | \
                         CIA_SDR_SET_SDR_IRQ1 | CIA_SDR_SET_SDR_IRQ0 | \
                         CIA_SDR_SET3 | CIA_SDR_SET2 |                 \
                         CIA_SDR_SET1 | CIA_SDR_SET0)

#define ALL_SDR_CNT     (CIA_SDR_CNT0|CIA_SDR_CNT1|CIA_SDR_CNT2|CIA_SDR_CNT3)

#define ALL_SDR_TOGGLE_CNT  (CIA_SDR_TOGGLE_CNT2 | CIA_SDR_TOGGLE_CNT1 | \
                             CIA_SDR_TOGGLE_CNT0 | CIA_SDR_TOGGLE_CNT_1)
#define ALL_SDR_NOGGLE_CNT  (CIA_SDR_NOGGLE_CNT2 | CIA_SDR_NOGGLE_CNT1 | \
                             CIA_SDR_NOGGLE_CNT0 | CIA_SDR_NOGGLE_CNT_1)

/*
 * Here are the flag bits for ifr_delay.
 */

#define CIA_IRQ_ACK1    0x0001  /* ClearIcr0 Countdown to ack-ing (resetting) irq source bits */
#define CIA_IRQ_ACK0    0x0002  /* ClearIcr1 */
#define CIA_IRQ_ACK_1   0x0004  /* ClearIcr2 */
#define CIA_IRQ_ACK_2   0x0008

#define CIA_IRQ_D7SET1  0x0010  /* SetIcr0 Countdown to setting D7 in IFR */
#define CIA_IRQ_D7SET0  0x0020  /* SetIcr1 */
#define CIA_IRQ_D7SET_1 0x0040

#define CIA_IRQ_RAISE1  0x0100  /* Interrupt0 Countdown to raising interrupt */
#define CIA_IRQ_RAISE0  0x0200  /* Interrupt1 */
#define CIA_IRQ_RAISE_1 0x0400

#define CIA_IRQ_READ0   0x1000  /* ReadIcr0 How long since ICR was read */
#define CIA_IRQ_READ1   0x2000  /* ReadIcr1 */
#define CIA_IRQ_READ2   0x4000  /* ReadIcr2 */

#define CIA_IRQ_CLEAR   (CIA_IRQ_ACK_2    | \
                         CIA_IRQ_D7SET_1  | \
                         CIA_IRQ_RAISE_1  | \
                         CIA_IRQ_READ2)

#define USE_IRQ_RAISE0_SHORTCUT 1  /* Documented at location of use */

static void ciacore_intta(CLOCK offset, void *data);
static void ciacore_inttb(CLOCK offset, void *data);
static void ciacore_intsdr(CLOCK offset, void *data);
static void schedule_sdr_alarm(cia_context_t *cia_context, CLOCK rclk, uint32_t feed);
static void cia_set_irq_flag(cia_context_t *cia_context, CLOCK rclk, unsigned int bits);


/* The following is an attempt in rewriting the interrupt defines into
   static inline functions. This should not hurt, but I still kept the
   define below, to be able to compare speeds.
   The semantics of the call has changed, the interrupt number is
   not needed anymore (because it's known to my_set_int(). Actually
   one could also remove MYCIA_INT as it is also known... */

/* new semantics and as inline function, value can be replaced by 0/1 */
static inline void my_set_int(cia_context_t *cia_context, bool value,
                              CLOCK rclk)
{
#ifdef CIA_TIMER_DEBUG
    if (cia_context->debugFlag) {
        log_message(cia_context->log, "set_int(rclk=%d, d=%d pc=).",
                    rclk, (value));
    }
#endif
    (cia_context->cia_set_int_clk)(cia_context, value, rclk);
    cia_context->irq_enabled = value;
}

/* ------------------------------------------------------------------------- */

/*
 * Return the clock when this alarm is due.
 * Alarms for clock N run after CPU accesses for that clock.
 * If the alarm is not set, returns 0.
 */
inline static CLOCK alarm_clk(alarm_t *alarm)
{
    alarm_context_t *context;
    int idx;

    context = alarm->context;
    idx = alarm->pending_idx;

    if (idx >= 0) {
        return context->pending_alarms[idx].clk;
    } else {
        return 0;
    }
}

/*
 * To be called only during CPU access to the registers.
 *
 * Run any pending alarms that should have run before the current clock cycle.
 * Alarms scheduled for cycle N run AFTER CPU accesses of cycle N.
 * Therefore we only run alarms < clk, i.e. alarms that should have run
 * up to and including the previous cycle.
 *
 * The normal execution run uses "clk >= alarm...", but since we're running
 * this during CPU access, here we use "clk > ...".
 *
 * In some cases, clk here differs from the global clock by some offset
 * (due to ->write_offset). The dispatch function needs the proper clock
 * value to calculate the offset parameter to the alarm callbacks.
 * So we need an offset here to re-calculate the correct clk.
 *
 * NOTE: cia_update_ta() and _tb() actually run their own alarms including
 * those for the end of the current cycle.
 * Unlike those ad-hoc functions, this one runs the alarms in their correct
 * relative time order.
 */
inline static void run_pending_alarms(CLOCK clk, int offset, alarm_context_t *alarm_context)
{
    while (clk > alarm_context_next_pending_clk(alarm_context)) {
        alarm_context_dispatch(alarm_context, clk + offset);
    }
}


/* ------------------------------------------------------------------------- */
/* cia */


inline static void check_ciatodalarm(cia_context_t *cia_context, CLOCK rclk)
{
    if (!memcmp(cia_context->todalarm, cia_context->c_cia + CIA_TOD_TEN,
                   sizeof(cia_context->todalarm))) {
        cia_set_irq_flag(cia_context, rclk, CIA_IM_TOD);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * ciat_update return the number of underflows
 * FIXME: SDR count, etc
 */

static void cia_do_update_ta(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_update(cia_context->ta, rclk))) {
        cia_set_irq_flag(cia_context, rclk, CIA_IM_TA);
        cia_context->tat = (cia_context->tat + n) & 1;
    }
}

static void cia_do_update_tb(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_update(cia_context->tb, rclk))) {
        cia_set_irq_flag(cia_context, rclk, CIA_IM_TB);
        if (cia_context->model == CIA_MODEL_6526
            && cia_context->rdi == rclk - 1) {
            /* flag the timer B bug */
            cia_context->irqflags |= CIA_IM_TBB;
        } else {
            cia_context->irqflags &= ~CIA_IM_TBB;
        }
        cia_context->tbt = (cia_context->tbt + n) & 1;
    }
}

static void cia_do_step_tb(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_single_step(cia_context->tb, rclk))) {
        cia_set_irq_flag(cia_context, rclk, CIA_IM_TB);
        cia_context->tbt = (cia_context->tbt + n) & 1;
    }
}

/*
 * Those functions are called everywhere but in the alarm functions.
 */

static void cia_update_ta(cia_context_t *cia_context, CLOCK rclk)
{
    CLOCK tmp, last_tmp;

    /*
     * This is essentially the same idea as run_pending_alarms()
     * except it runs the alarms a cycle further (due to <=) to the current
     * cycle.
     * Fortunately ciacore_intta() either unsets or reschedules the
     * alarm so it won't run twice.
     */
    last_tmp = 0;
    tmp = ciat_alarm_clk(cia_context->ta);

    while (tmp <= rclk) {
        ciacore_intta(*(cia_context->clk_ptr) - tmp, (void *)cia_context);
        last_tmp = tmp;
        tmp = ciat_alarm_clk(cia_context->ta);
    }

    if (last_tmp != rclk) {
        cia_do_update_ta(cia_context, rclk);
    }

}

static void cia_update_tb(cia_context_t *cia_context, CLOCK rclk)
{
    CLOCK tmp, last_tmp;

    if ((cia_context->c_cia[CIA_CRB] & (CIA_CRB_INMODE_TA|CIA_CR_START))
            == (CIA_CRB_INMODE_TA|CIA_CR_START)) {
        cia_update_ta(cia_context, rclk);
    }

    /*
     * This is essentially the same idea as run_pending_alarms()
     * except it runs the alarms a cycle further (due to <=) to the current
     * cycle.
     * Fortunately ciacore_inttb() either unsets or reschedules the
     * alarm so it won't run twice.
     */
    last_tmp = 0;
    tmp = ciat_alarm_clk(cia_context->tb);
    while (tmp <= rclk) {
        ciacore_inttb(*(cia_context->clk_ptr) - tmp, (void *)cia_context);
        last_tmp = tmp;
        tmp = ciat_alarm_clk(cia_context->tb);
    }

    if (last_tmp != rclk) {
        cia_do_update_tb(cia_context, rclk);
    }
}

#if defined(IFR_DEBUG)
static void dump_ifr_delay(const char *name, uint32_t delay)
{
#define DO_BIT(name) if (delay & name) fprintf(stderr, #name " ")
    fprintf(stderr, "%s: ", name);
    DO_BIT(CIA_IRQ_ACK1);
    DO_BIT(CIA_IRQ_ACK0);
    DO_BIT(CIA_IRQ_ACK_1);
    DO_BIT(CIA_IRQ_ACK_2);
    DO_BIT(CIA_IRQ_D7SET1);
    DO_BIT(CIA_IRQ_D7SET0);
    DO_BIT(CIA_IRQ_D7SET_1);
    DO_BIT(CIA_IRQ_RAISE1);
    DO_BIT(CIA_IRQ_RAISE0);
    DO_BIT(CIA_IRQ_RAISE_1);
    DO_BIT(CIA_IRQ_READ0);
    DO_BIT(CIA_IRQ_READ1);
    DO_BIT(CIA_IRQ_READ2);
    fprintf(stderr, "\n");
#undef DO_BIT
}
#endif /* IFR_DEBUG */

/*
 * This function is called to catch up on changes to the Interrupt Flag
 * Register; one cycle at a time. It is for things we didn't do immediately
 * because they are delayed wrt the action that triggers them.
 */
static void cia_run_ifr_cycle(cia_context_t *cia_context)
{
    uint32_t delay = cia_context->ifr_delay;
    CLOCK rclk = cia_context->ifr_clock;

    /* Not done here but in cia_set_irq_flag()
    cia_context->ack_irqflags &= ~cia_context->new_irqflags;*/

    if (cia_context->model != CIA_MODEL_6526) { /* new fast CIA */
        if ((delay & CIA_IRQ_ACK0) != 0) {
            cia_context->irqflags &= ~cia_context->ack_irqflags;
            cia_context->ack_irqflags = 0;
        }
    } else { /* old slow CIA */
        if ((delay & CIA_IRQ_ACK0) != 0) {
            cia_context->irqflags &= ~cia_context->ack_irqflags;
            cia_context->irqflags &= ~CIA_IM_SET;
            cia_context->ack_irqflags = 0;
        }
    }

    /* Not done here but in cia_do_update_tb()
    if (bTimerBbug && (cia_context->new_irqflags & 2) !=0 && ClockReadICR == CurrentClock && (delay & (ReadIcr1)) == 0) {
        cia_context->ack_irqflags |= CIA_IM_TB;
    } */
    /* Not done here but in cia_set_irq_flag()
    cia_context->irqflags |= cia_context->new_irqflags; */

    if (cia_context->new_irqflags & cia_context->c_cia[CIA_ICR] & 0x1F) {
        if (cia_context->model != CIA_MODEL_6526) { /* new CIA */
            if (cia_context->rdi + 1 == rclk) {
                /* TEST TLR's CIA read DC0D (icr) when Timer counts to zero;
                 * Used by testprogs/interrupts/irqnmi/cia-int-irq-new.prg
                 * Used by testprogs/interrupts/irqnmi/cia-int-nmi-new.prg */
                delay |= CIA_IRQ_RAISE1;
                delay |= CIA_IRQ_D7SET1;
            } else {
                delay |= CIA_IRQ_RAISE0;
                delay |= CIA_IRQ_D7SET0;
            }
        } else { /* Old CIA */
            delay |= CIA_IRQ_RAISE1;
            delay |= CIA_IRQ_D7SET1;
        }
    }

    /* Check for interrupt condition */
    if ((delay & CIA_IRQ_D7SET0) != 0) {
        cia_context->irqflags |= CIA_IM_SET;
    }

    if (delay & CIA_IRQ_RAISE0) {
        my_set_int(cia_context, true, rclk);
    }

    cia_context->new_irqflags = 0;

    delay <<= 1;
    delay &= ~CIA_IRQ_CLEAR;
    cia_context->ifr_delay = delay;
    cia_context->ifr_clock++;
}

/*
 * Only run the IFR delay pipeline once per cycle.
 * Timer A/B need to be updated first.
 * SDR, TOD and idle alarms can run after, and check before calling here.
 *
 * If we want to look at the updated ICR after this (such as when the cpu
 * accesses it), you must make sure that all relevant alarms have alreay run
 * before this.
 *
 * If we're not sure we are going to be called in the next cycle,
 * work ahead; only do the things that cannot be done later, just
 * before looking at it again:
 * i.e. do trigger irq, don't change registers.
 * cia_ifr_catchup() will be called later which will do the rest
 * (and possibly duplicate some).
 */
#define CIA_IFR_CURRENT  0x01
#define CIA_IFR_NEXT     0x02
#define CIA_IFR_CUR_NXT  0x03
static void cia_ifr_current(cia_context_t *cia_context, CLOCK rclk, int what)
{
    /*
     * Check if Timer A/B alarms for this cycle are still scheduled.
     * This check should be needed if all callers are careful,
     * but is left as a consistency check.
     * Another consistency check could be that ->ifr_clock == rclk.
     */
    if (ciat_alarm_clk(cia_context->ta) != rclk &&
        ciat_alarm_clk(cia_context->tb) != rclk) {
        if (what & CIA_IFR_CURRENT) {
            cia_run_ifr_cycle(cia_context);
        }

        if (what & CIA_IFR_NEXT) {
            uint32_t delay = cia_context->ifr_delay;
            /* Look 1 tick into the future */
            if (delay & CIA_IRQ_RAISE0) {
#if USE_IRQ_RAISE0_SHORTCUT
                /*
                 * We can safely schedule the IRQ/NMI 1 cycle into the future.
                 * There are cases where CIA_IRQ_RAISE0 is cleared (can happen
                 * when writing CIA_ICR or reading it), but in those cases
                 * another CIA_IFR_CURRENT has been done before, which does a
                 * shift of the delay word.  So the RAISE0 there would be a
                 * RAISE1 (or higher) here.
                 */
                my_set_int(cia_context, true, rclk + 1);
#else
                /*
                 * Instead of scheduling the interrupt "ahead of time", we
                 * can also cause the delay line to run in the next cycle,
                 * using the idle alarm.
                 * This variant must always work too (if it doesn't,
                 * there is something subtly wrong which could fail too in
                 * other cases).
                 */
                alarm_set(cia_context->idle_alarm, rclk + 1);
#endif
            }
            /* Look 2 ticks into the future */
            else if (delay & CIA_IRQ_RAISE1) {
                /*
                 * This case is pretty rare, but the Lorenz imr.prg "set imr
                 * clock 3" test (for old CIAs) triggers it.
                 */
                alarm_set(cia_context->idle_alarm, rclk + 1);
            }
        }
    } else {
        /*
         * This case should not happen; if it does, some logic has to be
         * adjusted so that this function is called only after updating the
         * timers (which reschedules their alarms to some later time).
         */
    }
}

/*
 * Catch up on the IFR delay pipeline, if it is behind.
 * This function is safe to call more than once.
 * It must be called before cia_ifr_current().
 *
 * It knows how to shortcut doing all clock iterations, if no more
 * changes can happen.
 */
static void cia_ifr_catchup(cia_context_t *cia_context, CLOCK rclk)
{
    if (cia_context->ifr_clock < rclk) {
        while ((cia_context->ifr_delay ||
                cia_context->new_irqflags ||
                cia_context->ack_irqflags) &&
               cia_context->ifr_clock < rclk) {
            cia_run_ifr_cycle(cia_context);
        }

        cia_context->ifr_clock = rclk;
    }
}

/*
 * Set a flag in the Interrupt Flag Register.
 *
 * Unlike earlier versions of this code, it doesn't immediately signal an
 * interrupt to the CPU. This is to be done later (as late as possible in
 * the same cycle, and precisely one time) in cia_run_ifr_cycle().
 *
 * This is normally done by calling, after any function that may get here:
 *
 *     cia_ifr_catchup();
 *     cia_ifr_current(CIA_IFR_CUR_NXT); (possibly for CUR and NEXT separately)
 *
 * cia_ifr_catchup() is safe to call multiple times (but must be called once
 * before cia_ifr_current()),
 * but cia_ifr_current() must NOT be called more than once for a cycle!
 *
 * Timer A and B alarm code wants to run before cpu access in the same cycle,
 * but alarms actually run after cpu access. So various places call code
 * to update the timers before taking other action, such as calling
 * cia_ifr_current().
 * (So strictly speaking, cia_ifr_current() ought not need to check for TA or TB
 * alarms being pending?)
 * SDR, TOD and idle alarms are happy to run after the cpu access,
 * so cia_ifr_current() doesn't check for them.
 *
 * Functions that call here are:
 *
 * - check_ciatodalarm()
 *   - ciacore_inttod()
 *     - ciacore_inttod_entry() ok, from alarm
 * - cia_do_update_ta()
 *   - cia_update_ta()
 *     - cia_update_tb()
 *       - ciacore_update_pb67()
 *         - ciacore_store_internal() ok, CPU access
 *       - ciacore_store_internal() see ^
 *       - ciacore_read() ok, CPU access
 *     - ciacore_update_pb67() see ^
 *     - ciacore_read() see ^
 *     - ciacore_store_internal() see ^
 *   - ciacore_intta()
 *     - cia_update_ta() see ^
 *     - ciacore_intta_entry() ok, from alarm
 * - cia_do_update_tb()
 *   - cia_update_tb() see ^
 *   - ciacore_intta() see ^
 *   - ciacore_inttb()
 *     - cia_update_tb() see ^
 *     - ciacore_inttb_entry() ok, from alarm
 * - cia_do_step_tb(),
 *   - ciacore_intta() see ^
 * - ciacore_set_flag() ok, from extern
 * - ciacore_set_sdr() ok, from extern
 * - ciacore_intsdr(),
 *   - ciacore_intsdr_entry() ok, from alarm
 */
static void cia_set_irq_flag(cia_context_t *cia_context, CLOCK rclk, unsigned int bits)
{
    cia_ifr_catchup(cia_context, rclk);

    cia_context->irqflags |= bits;
    cia_context->new_irqflags |= bits;
    /* Done here instead of doing all new bits at once in cia_run_ifr_cycle(): */
    cia_context->ack_irqflags &= ~bits;
}


/* -------------------------------------------------------------------------- */
void ciacore_disable(cia_context_t *cia_context)
{
    alarm_unset(cia_context->idle_alarm);
    alarm_unset(cia_context->ta_alarm);
    alarm_unset(cia_context->tb_alarm);
    alarm_unset(cia_context->tod_alarm);
    alarm_unset(cia_context->sdr_alarm);
    cia_context->enabled = false;
}

/*
    we must take care to choose a value which is small enough so the counters do
    not fall behind too much to cause a significant peak in cpu usage, and one
    that is big enough so the overall performance impact is not too big.
    it seems reasonable to also consider how peaks in cpu usage interact with
    automatic framerate adjustment, so choosing a value that makes sure a more
    or less constant amount of cpu time per frame is consumed is a good idea.
    (about 20000 cycles are a full PAL frame on the C64, making sure that we do
    not fall behind one frame at all seems a good idea.)
 */
#define CIA_MAX_IDLE_CYCLES     5000

void ciacore_reset(cia_context_t *cia_context)
{
    int i;

    for (i = 0; i < 16; i++) {
        cia_context->c_cia[i] = 0;
    }

    cia_context->rdi = 0;
    cia_context->sr_bits = 0;
    cia_context->read_clk = 0;

    ciat_reset(cia_context->ta, *(cia_context->clk_ptr));
    ciat_reset(cia_context->tb, *(cia_context->clk_ptr));

    cia_context->sdr_valid = false;
    cia_context->sdr_force_finish = false;
    cia_context->sdr_delay = CIA_SDR_CNT0|CIA_SDR_CNT1|CIA_SDR_CNT2|CIA_SDR_CNT3;

    cia_context->sp_in_state = true;
    cia_context->cnt_in_state = true;
    cia_context->cnt_out_state = true;

    memset(cia_context->todalarm, 0, sizeof(cia_context->todalarm));
    cia_context->todlatched = 0;
    cia_context->todstopped = 1;
    cia_context->c_cia[CIA_TOD_HR] = 1;          /* the most common value */
    memcpy(cia_context->todlatch, cia_context->c_cia + CIA_TOD_TEN, sizeof(cia_context->todlatch));
    cia_context->todclk = *(cia_context->clk_ptr) + cia_context->todticks;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);
    cia_context->todtickcounter = 0;

    cia_context->irqflags = 0;
    cia_context->ack_irqflags = 0;
    cia_context->new_irqflags = 0;
    cia_context->irq_enabled = 0;

    cia_context->ifr_clock = 0;
    cia_context->ifr_delay = 0;

    my_set_int(cia_context, false, *(cia_context->clk_ptr));

    /* these must be 0xff, or programs relying on the initial value may not
       work correctly, see bug #1143 */
    cia_context->old_pa = 0xff;
    cia_context->old_pb = 0xff;

    (cia_context->do_reset_cia)(cia_context);
    cia_context->enabled = true;

    /*
     * A reset also resets the system clock, so any existing alarms
     * (except the one set above) at this time would now be invalid.
     */
    alarm_set(cia_context->idle_alarm,
              *(cia_context->clk_ptr) + CIA_MAX_IDLE_CYCLES);
    alarm_unset(cia_context->ta_alarm);
    alarm_unset(cia_context->tb_alarm);
    alarm_unset(cia_context->sdr_alarm);
}

/*
 * https://sourceforge.net/p/vice-emu/bugs/1219
 */
static inline void strange_extra_sdr_flags(cia_context_t *cia_context, CLOCK rclk, uint8_t byte)
{
    if ((byte & CIA_CRA_SPMODE_OUT) == CIA_CRA_SPMODE_IN) {
        /* Switching from output to input */
        bool forceFinish = false;
        uint32_t cnt_wanted;
        uint32_t sdr_delay = cia_context->sdr_delay;
        uint32_t cnt_delay = sdr_delay;

        if (cia_context->model == CIA_MODEL_6526) {
            cnt_wanted = (CIA_SDR_CNT0 | CIA_SDR_CNT1 | CIA_SDR_CNT2);
        } else {
            cnt_wanted = (               CIA_SDR_CNT1 | CIA_SDR_CNT2);
        }
        forceFinish = (cnt_delay & cnt_wanted) != cnt_wanted;

        if (!forceFinish) {
            if (cia_context->sr_bits != 2 &&
                    !(sdr_delay & CIA_SDR_TOGGLE_CNT2) &&
                    !(sdr_delay & CIA_SDR_TOGGLE_CNT1) &&
                     (sdr_delay & CIA_SDR_TOGGLE_CNT0)) {
                forceFinish = true;
            }
        }

        cia_context->sdr_force_finish = forceFinish;
    } else {
        /* Switching from input to output */

        /*
         * Act on positive incoming CNT flank.
         * This doesn't really belong here. And does it even happen?
         */
        if (!cia_context->cnt_out_state && cia_context->sr_bits != 0) {
            cia_context->shifter <<= 1;
        }

        if (cia_context->sdr_force_finish) {
            schedule_sdr_alarm(cia_context, rclk, CIA_SDR_SET_SDR_IRQ2);
            cia_context->sdr_force_finish = false;
        }
    }
}

/*
 * Update bits 6 and 7 of Port B, depending on if the timer(s) are set
 * to output to them.
 *
 * Mostly identical to reading CIA_PRB.
 */
static inline bool ciacore_update_pb67(cia_context_t *cia_context, CLOCK rclk)
{
    uint8_t byte;
    bool current_called = false;

    byte = cia_context->c_cia[CIA_PRB] | ~(cia_context->c_cia[CIA_DDRB]);

    if ((cia_context->c_cia[CIA_CRA]
            | cia_context->c_cia[CIA_CRB]) & CIA_CR_PBON) {
        if (cia_context->c_cia[CIA_CRA] & CIA_CR_PBON) {
            cia_update_ta(cia_context, rclk);
            byte &= 0xbf;
            if ((cia_context->c_cia[CIA_CRA] & CIA_CR_OUTMODE_TOGGLE)
                        ? cia_context->tat
                        : ciat_is_underflow_clk(cia_context->ta, rclk)) {
                byte |= 0x40;   /* PB6 for timer A */
            }
        }
        if (cia_context->c_cia[CIA_CRB] & CIA_CR_PBON) {
            cia_update_tb(cia_context, rclk);
            byte &= 0x7f;
            if (((cia_context->c_cia[CIA_CRB] & CIA_CR_OUTMODE_TOGGLE)
                        ? cia_context->tbt
                        : ciat_is_underflow_clk(cia_context->tb, rclk))) {
                byte |= 0x80;   /* PB7 for timer B */
            }
        }
        cia_ifr_catchup(cia_context, rclk);
        cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
        /* We called these functions already: caller should not! */
        current_called = true;
    }

    if (byte != cia_context->old_pb) {
        (cia_context->store_ciapb)(cia_context,
                                    *(cia_context->clk_ptr),
                                    byte);
        cia_context->old_pb = byte;
    }

    return current_called;
}

static void ciacore_store_internal(cia_context_t *cia_context, uint16_t addr, uint8_t byte)
{
    CLOCK rclk;

    addr &= 0xf;

    /* stores have a one-cycle offset if CLK++ happens before store */
    rclk = *(cia_context->clk_ptr) - cia_context->write_offset;

    run_pending_alarms(rclk, cia_context->write_offset, cia_context->tb_alarm->context);

#ifdef CIA_TIMER_DEBUG
    if (cia_context->debugFlag) {
        log_message(cia_context->log, "store cia[%02x] %02x @ clk=%d",
                    (int) addr, (int) byte, rclk);
    }
#endif

    switch (addr) {
        case CIA_PRA:           /* port A */
        case CIA_DDRA:
            cia_context->c_cia[addr] = byte;
            byte = cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]);
            if (byte != cia_context->old_pa) {
                (cia_context->store_ciapa)(cia_context, *(cia_context->clk_ptr),
                                           byte);
                cia_context->old_pa = byte;
            }
            break;

        case CIA_PRB:           /* port B */
        case CIA_DDRB:
            cia_context->c_cia[addr] = byte;
            ciacore_update_pb67(cia_context, rclk);

            if (addr == CIA_PRB) {
                (cia_context->pulse_ciapc)(cia_context, rclk);
            }
            break;

        case CIA_TAL:
            cia_update_ta(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            ciat_set_latchlo(cia_context->ta, rclk, (uint8_t)byte);
            break;
        case CIA_TBL:
            cia_update_tb(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            ciat_set_latchlo(cia_context->tb, rclk, (uint8_t)byte);
            break;
        case CIA_TAH:
            cia_update_ta(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            ciat_set_latchhi(cia_context->ta, rclk, (uint8_t)byte);
            break;
        case CIA_TBH:
            cia_update_tb(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            ciat_set_latchhi(cia_context->tb, rclk, (uint8_t)byte);
            break;

        /*
         * TOD clock is stopped by writing Hours, and restarted
         * upon writing Tenths of Seconds.
         *
         * REAL:  TOD register + (wallclock - ciatodrel)
         * VIRT:  TOD register + (cycles - begin)/cycles_per_sec
         */
        case CIA_TOD_TEN: /* Time Of Day clock 1/10 s */
        case CIA_TOD_HR:  /* Time Of Day clock hour */
        case CIA_TOD_SEC: /* Time Of Day clock sec */
        case CIA_TOD_MIN: /* Time Of Day clock min */
            if (addr == CIA_TOD_HR) {
                /* force bits 6-5 = 0 */
                byte &= 0x9f;
            } else if (addr == CIA_TOD_MIN) {
                byte &= 0x7f;
            } else if (addr == CIA_TOD_SEC) {
                byte &= 0x7f;
            } else if (addr == CIA_TOD_TEN) {
                byte &= 0x0f;
            }

            {
                char changed;
                if (cia_context->c_cia[CIA_CRB] & CIA_CRB_ALARM_ALARM) {
                    /* set alarm */
                    changed = cia_context->todalarm[addr - CIA_TOD_TEN] != byte;
                    cia_context->todalarm[addr - CIA_TOD_TEN] = byte;
                } else {
                    /* set time */
                    if (addr == CIA_TOD_TEN) {
                        /* the tickcounter is kept clear while the clock
                           is not running and then restarted by writing to the 10th
                           seconds register */
                        if (cia_context->todstopped) {
                            cia_context->todtickcounter = 0;
                            cia_context->todstopped = 0;
                        }
                    }
                    if (addr == CIA_TOD_HR) {
                        cia_context->todstopped = 1;
                    }
                    changed = cia_context->c_cia[addr] != byte;
                    if (changed) {
                        /* Flip AM/PM on hour 12 on the rising edge of the comparator */
                        if ((addr == CIA_TOD_HR) && ((byte & 0x1f) == 0x12))
                            byte ^= 0x80;
                        cia_context->c_cia[addr] = byte;
                    }
                }
                if (changed) {
                    check_ciatodalarm(cia_context, rclk);
                    cia_ifr_catchup(cia_context, rclk);
                    cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
                }
            }
            break;

        case CIA_SDR:           /* Serial Port output buffer */
            DBG(("write SDR(1) %s: %02x, sr_bits %2u, shifter %04x sdr_valid %d rclk %lu\n",
                        cia_context->myname, byte, cia_context->sr_bits,
                        cia_context->shifter, cia_context->sdr_valid, rclk));
            if ((cia_context->c_cia[CIA_CRA] & (CIA_CRA_SPMODE)) ==
                                           (CIA_CRA_SPMODE_OUT)) {
                /*
                 * There should be a 2 clock delay before this is effective.
                 * That makes a difference in case there is a Timer A underflow
                 * in between.
                 * That also means that if the TA underflow isn't in the near
                 * future, we could do a shortcut and set the shifter
                 * directly. This is for later.
                 */
                schedule_sdr_alarm(cia_context, rclk, CIA_SDR_SET1);
            }
            DBG(("write SDR(2) %s: %02x, sr_bits %2u, shifter %04x sdr_valid %d\n",
                        cia_context->myname, byte, cia_context->sr_bits,
                        cia_context->shifter, cia_context->sdr_valid));
            cia_context->c_cia[addr] = byte;
            break;

        /* Interrupts */

        case CIA_ICR:           /* Interrupt Control Register */

            CIAT_LOGIN(("store_icr: rclk=%lu, byte=%02x", rclk, byte));

            cia_update_ta(cia_context, rclk);
            cia_update_tb(cia_context, rclk);

            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CURRENT);


#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log, "cia set CIA_ICR: 0x%x.", byte);
            }
#endif

            if (byte & CIA_IM_SET) {
                cia_context->c_cia[CIA_ICR] |= (byte & 0x7f);
            } else {
                cia_context->c_cia[CIA_ICR] &= ~(byte & 0x7f);
            }

            if ((cia_context->irqflags & cia_context->c_cia[CIA_ICR] & 0x7F) != 0) {
                /* Both pending interrupts and currently active interrupts
                 * are never cancelled or cleared. */
                if (cia_context->irq_enabled == 0) {
                    if (cia_context->model != CIA_MODEL_6526) { /* new CIA */
                        if ((cia_context->ifr_delay & CIA_IRQ_READ1) == 0) {
                            cia_context->ifr_delay |= CIA_IRQ_RAISE0;
                            cia_context->ifr_delay |= CIA_IRQ_D7SET0;
                        }
                    } else { /* old CIA */
                        cia_context->ifr_delay |= CIA_IRQ_RAISE1;
                        cia_context->ifr_delay |= CIA_IRQ_D7SET1;
                    }
                }
            } else {
                if (cia_context->model == CIA_MODEL_6526) {
                    /* This relates to "old" CIA 6526.
                     * Currently active interrupts are never cleared.
                     * Setting RAISE1 (as above) in one cycle and clearing it
                     * here (as RAISE0) in the next cycle can potentially happen;
                     * and it also needs a read of the ICR 2 cycles ago.
                     * hmmmm... NOTE_1.
                     * However I can't think of any RMW instruction to do this.
                     */
                    if ((cia_context->ifr_delay & CIA_IRQ_ACK_1) != 0) {
                        cia_context->ifr_delay &= ~(CIA_IRQ_RAISE0);
                        cia_context->ifr_delay &= ~(CIA_IRQ_D7SET0);
                    }
                }
            }

#if defined(CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log,
                            "    set icr: ifr & ier & 0x7f -> %02x, ier=%02x, ifr=%02x.",
                            cia_context->c_cia[CIA_ICR] & cia_context->irqflags
                            & 0x7f, cia_context->c_cia[CIA_ICR], cia_context->irqflags);
            }
#endif
            if (cia_context->c_cia[CIA_ICR] & CIA_IM_TA) {
                ciat_set_alarm(cia_context->ta, rclk);
            }
            if (cia_context->c_cia[CIA_ICR] & CIA_IM_TB) {
                ciat_set_alarm(cia_context->tb, rclk);
            }

            cia_ifr_current(cia_context, rclk, CIA_IFR_NEXT);
            CIAT_LOGOUT((""));
            break;

        case CIA_CRA:           /* control register A */
            cia_update_ta(cia_context, rclk);

            if ((byte & CIA_CR_START) &&
                    !(cia_context->c_cia[CIA_CRA] & CIA_CR_START)) {
                /* Starting timer A */
                cia_context->tat = 1;
            }

            /* Is the serial I/O direction changing? */
            if ((byte ^ cia_context->c_cia[CIA_CRA]) & CIA_CRA_SPMODE) {
                strange_extra_sdr_flags(cia_context, rclk, byte);
                cia_context->sr_bits = 0;
                cia_context->sdr_valid = false;
                cia_context->sdr_delay &= ~(ALL_SDR_TOGGLE_CNT|ALL_SDR_NOGGLE_CNT);

                if (!cia_context->cnt_out_state) {
                    cia_context->cnt_out_state = true;
                    if (cia_context->set_cnt) {
                        (cia_context->set_cnt)(cia_context, rclk, true);
                    }
                    /* TODO: timers driven by positive CNT edge */
                }
            }

            ciat_set_ctrl(cia_context->ta, rclk, byte);

#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log,
                            "cia set CIA_CRA: 0x%x (clk=%d, pc=, tal=%u, tac=%u).",
                            byte, rclk, /*program_counter,*/ cia_tal, cia_tac);
            }
#endif

            /* bit 7 tod frequency */
            /* bit 6 serial port mode */

            /* bit 3 timer run mode */
            /* bit 2 & 1 timer output to PB6 */

            /* bit 0 start/stop timer */
            /* bit 5 timer count mode */

#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log, "    -> tas=%d, tau=%d.", cia_tas,
                            cia_tau);
            }
#endif
            cia_context->c_cia[addr] = byte & 0xef;    /* remove strobe */

            if (!ciacore_update_pb67(cia_context, rclk)) {
                cia_ifr_catchup(cia_context, rclk);
                cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            }
            break;

        case CIA_CRB:           /* control register B */
            if ((byte & 1) && !(cia_context->c_cia[CIA_CRB] & CIA_CR_START)) {
                cia_context->tbt = 1;
            }

            cia_update_ta(cia_context, rclk);
            cia_update_tb(cia_context, rclk);

            /* bit 5 is set when single-stepping is set */
            if (byte & CIA_CRB_INMODE_TA) {
                /* we count ta - so we enable that */
                ciat_set_alarm(cia_context->ta, rclk);
                ciat_set_ctrl(cia_context->tb, rclk, (uint8_t)(byte | 0x20));   /* 0x20 = CIA_CRB_INMODE_CNT or CIAT_PHI2IN */
            } else {
                ciat_set_ctrl(cia_context->tb, rclk, byte);
            }

#if 0 /* defined (CIA_TIMER_DEBUG)
        if (cia_context->debugFlag) */
            log_message(cia_context->log, "cia set CIA_CRB: 0x%x (clk=%d).",
                        byte, rclk);
#endif


            /* bit 7 set alarm/tod clock */
            /* bit 4 force load */

            /* bit 3 timer run mode */
            /* bit 2 & 1 timer output to PB6 */

            /* bit 0 stbrt/stop timer */
            /* bit 5 & 6 timer count mode */

            cia_context->c_cia[addr] = byte & 0xef;    /* remove strobe */

            if (!ciacore_update_pb67(cia_context, rclk)) {
                cia_ifr_catchup(cia_context, rclk);
                cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            }
            break;

        default:
            cia_context->c_cia[addr] = byte;
    }                           /* switch */
}

void ciacore_store(cia_context_t *cia_context, uint16_t addr, uint8_t byte)
{
    if (cia_context->pre_store != NULL) {
        (cia_context->pre_store)();
    }

    if (*(cia_context->rmw_flag)) {
        (*(cia_context->clk_ptr))--;
        ciacore_store_internal(cia_context, addr, cia_context->last_read);
        (*(cia_context->clk_ptr))++;
    }

    ciacore_store_internal(cia_context, addr, byte);
}

/* ------------------------------------------------------------------------- */


uint8_t ciacore_read(cia_context_t *cia_context, uint16_t addr)
{
#if defined(CIA_TIMER_DEBUG)

    uint8_t cia_read_(cia_context_t *, uint16_t addr);
    uint8_t tmp = cia_read_(cia_context, addr);

    if (cia_context->debugFlag) {
        log_message(cia_context->log, "read cia[%x] returns %02x @ clk=%d.",
                    addr, tmp, *(cia_context->clk_ptr) - READ_OFFSET);
    }
    return tmp;
}

uint8_t cia_read_(cia_context_t *cia_context, uint16_t addr)
{
#endif

    uint8_t byte = 0xff;
    CLOCK rclk;

    addr &= 0xf;

    if (cia_context->pre_read != NULL) {
        (cia_context->pre_read)();
    }

    cia_context->read_clk = *(cia_context->clk_ptr);
    cia_context->read_offset = 0;
    rclk = *(cia_context->clk_ptr) - READ_OFFSET;

    if (cia_context->pre_read != NULL) {
        (cia_context->pre_read)();
    }

    run_pending_alarms(rclk, READ_OFFSET, cia_context->tb_alarm->context);

    switch (addr) {
        case CIA_PRA:           /* port A */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value. Value read might be different from what is
               expected due to excessive load. */
            cia_context->last_read = (cia_context->read_ciapa)(cia_context);
            return cia_context->last_read;
            break;

        case CIA_PRB:           /* port B */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value. Value read might be different from what is
               expected due to excessive load. */
            byte = (cia_context->read_ciapb)(cia_context);
            (cia_context->pulse_ciapc)(cia_context, rclk);

            /* Mostly identical to ciacore_update_pb67() */
            if ((cia_context->c_cia[CIA_CRA]
                 | cia_context->c_cia[CIA_CRB]) & CIA_CR_PBON) {

                if (cia_context->c_cia[CIA_CRA] & CIA_CR_PBON) {
                    cia_update_ta(cia_context, rclk);
                    byte &= 0xbf;
                    if (((cia_context->c_cia[CIA_CRA] & CIA_CR_OUTMODE_TOGGLE)
                            ? cia_context->tat
                            : ciat_is_underflow_clk(cia_context->ta, rclk))) {
                        byte |= 0x40;   /* PB6 for timer A */
                    }
                }

                if (cia_context->c_cia[CIA_CRB] & CIA_CR_PBON) {
                    cia_update_tb(cia_context, rclk);
                    byte &= 0x7f;
                    if (((cia_context->c_cia[CIA_CRB] & CIA_CR_OUTMODE_TOGGLE)
                            ? cia_context->tbt
                            : ciat_is_underflow_clk(cia_context->tb, rclk))) {
                        byte |= 0x80;   /* PB7 for timer B */
                    }
                }

                cia_ifr_catchup(cia_context, rclk);
                cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            }
            cia_context->last_read = byte;
            return byte;
            break;

        /* Timers */
        case CIA_TAL:           /* timer A low */
            cia_update_ta(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = ciat_read_timer(cia_context->ta, rclk)
                                     & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TAH:           /* timer A high */
            cia_update_ta(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = (ciat_read_timer(cia_context->ta, rclk)
                                      >> 8) & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TBL:           /* timer B low */
            cia_update_tb(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = ciat_read_timer(cia_context->tb, rclk)
                                     & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TBH:           /* timer B high */
            cia_update_tb(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = (ciat_read_timer(cia_context->tb, rclk)
                                      >> 8) & 0xff;
            return cia_context->last_read;
            break;

        /*
         * TOD clock is latched by reading Hours, and released
         * upon reading Tenths of Seconds. The counter itself
         * keeps ticking all the time.
         * Also note that this latching is different from the input one.
         */
        case CIA_TOD_TEN: /* Time Of Day clock 1/10 s */
        case CIA_TOD_SEC: /* Time Of Day clock sec */
        case CIA_TOD_MIN: /* Time Of Day clock min */
        case CIA_TOD_HR:  /* Time Of Day clock hour */
            if (!(cia_context->todlatched)) {
                memcpy(cia_context->todlatch, cia_context->c_cia + CIA_TOD_TEN,
                       sizeof(cia_context->todlatch));
            }
            if (addr == CIA_TOD_TEN) {
                cia_context->todlatched = 0;
            }
            if (addr == CIA_TOD_HR) {
                cia_context->todlatched = 1;
            }
            cia_context->last_read = cia_context->todlatch[addr - CIA_TOD_TEN];
            return cia_context->last_read;
            break;

        case CIA_SDR:           /* Serial Port Shift Register */
            if (cia_context->read_sdr) {
                (cia_context->read_sdr)(cia_context);
            }
            cia_context->last_read = cia_context->c_cia[CIA_SDR];
            return cia_context->last_read;
            break;

        /* Interrupts */

        case CIA_ICR:           /* Interrupt Control/Mask/Flag Register */
            {
                uint8_t result = 0;

                CIAT_LOGIN(("read_icr: A rclk=%lu, rdi=%lu", rclk, cia_context->rdi));

                cia_update_ta(cia_context, rclk);
                cia_update_tb(cia_context, rclk);

                cia_ifr_catchup(cia_context, rclk);
                cia_ifr_current(cia_context, rclk, CIA_IFR_CURRENT);

                cia_context->rdi = rclk;

                (cia_context->read_ciaicr)(cia_context);

#ifdef CIA_TIMER_DEBUG
                if (cia_context->debugFlag) {
                    log_message(cia_context->log,
                                "read_icr: rclk=%lu, alarm_ta=%lu, alarm_tb=%lu, ciaint=%02x",
                                rclk,
                                ciat_alarm_clk(cia_context->ta),
                                ciat_alarm_clk(cia_context->tb),
                                cia_context->irqflags);
                }
#endif

                ciat_set_alarm(cia_context->ta, rclk);
                ciat_set_alarm(cia_context->tb, rclk);

                CIAT_LOG(("read_icr -> ta alarm at %lu, tb at %lu",
                          ciat_alarm_clk(cia_context->ta),
                          ciat_alarm_clk(cia_context->tb)));

                if (cia_context->irqflags & CIA_IM_TBB) {
                    /* timer b bug */
                    cia_context->irqflags &= ~(CIA_IM_TBB | CIA_IM_TB);
                }

                if (cia_context->model != CIA_MODEL_6526) /* new fast CIA */
                {
                    if ((cia_context->ifr_delay & CIA_IRQ_RAISE0) != 0) {
                        if ((cia_context->irqflags & 0x1f) != 0) {
                            cia_context->irqflags |= CIA_IM_SET;
                        }
                    }

                    if ((cia_context->irqflags & 0x9F) != 0) {
                        cia_context->ack_irqflags |= ((cia_context->irqflags & 0x9F) | 0x80);
                    }
                    cia_context->ifr_delay |= CIA_IRQ_ACK1;
                    cia_context->ifr_delay &= ~(CIA_IRQ_RAISE0);
                    cia_context->ifr_delay &= ~(CIA_IRQ_D7SET0);
                    result = cia_context->irqflags;
                } else {
                    cia_context->ifr_delay |= CIA_IRQ_ACK1;
                    cia_context->ifr_delay &= ~(CIA_IRQ_RAISE0);
                    result = cia_context->irqflags;
                    cia_context->irqflags &= CIA_IM_SET;
                    cia_context->new_irqflags = 0;
                    /* ack_irqflags effectively isn't used for old CIAs */
                }

                cia_context->ifr_delay |= CIA_IRQ_READ0;

                my_set_int(cia_context, false, rclk);

                CIAT_LOG(("read_icr: Z gives ciaint=%02x -> %02x "
                          "sr_bits=%u, rclk=%lu",
                          (uint8_t)cia_context->irqflags, result, cia_context->sr_bits, rclk));
                cia_ifr_current(cia_context, rclk, CIA_IFR_NEXT);

                CIAT_LOGOUT((""));

                cia_context->last_read = result;

                return result;
            }
            break;

        case CIA_CRA:           /* Control Register A */
            cia_update_ta(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = (cia_context->c_cia[CIA_CRA] & ~CIA_CR_START)
                                     | ciat_is_running(cia_context->ta, rclk);
            return cia_context->last_read;
            break;

        case CIA_CRB:           /* Control Register B */
            cia_update_tb(cia_context, rclk);
            cia_ifr_catchup(cia_context, rclk);
            cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
            cia_context->last_read = (cia_context->c_cia[CIA_CRB] & ~CIA_CR_START)
                                     | ciat_is_running(cia_context->tb, rclk);
            return cia_context->last_read;
            break;
    }                           /* switch */

    cia_context->last_read = cia_context->c_cia[addr];
    return (cia_context->c_cia[addr]);
}

/* FIXME: this function should return the current state of the registers
          without affecting the state of the emulation. */
uint8_t ciacore_peek(cia_context_t *cia_context, uint16_t addr)
{
    uint8_t ret;

    addr &= 0xf;

    switch (addr) {
        /* reading the ports should have no side effects, we do however have
           to use the read function to update the port lines.
        */
        case CIA_PRA:
        case CIA_PRB:
        /* reading the DDR should have no side effects */
        case CIA_DDRA:
        case CIA_DDRB:
        /* reading the timer values should have no side effects, we do however
           have to use the read function to update the timers
        */
        case CIA_TAL:
        case CIA_TAH:
        case CIA_TBL:
        case CIA_TBH:
            ret = ciacore_read(cia_context, addr);
            break;
        /* reading the hours and tenth secs latches/unlatches the TOD, so we
           directly return the counter values here */
        case CIA_TOD_TEN:
        case CIA_TOD_SEC:
        case CIA_TOD_MIN:
        case CIA_TOD_HR:
            ret = cia_context->c_cia[addr];
            break;
        /* Serial Port Shift Register
         * FIXME: does reading SDR have side effects? do we need to update it?
         */
        case CIA_SDR:
            ret = cia_context->c_cia[CIA_SDR];
            break;
        /* reading ICR will clear it
         * FIXME: this is likely broken
         */
        case CIA_ICR:
            ret = cia_context->irqflags;
            break;
        /* reading the control registers should have no side effects, we do however
           have to use the read function to update the timers for bit 0
        */
        case CIA_CRA:
        case CIA_CRB:
            ret = ciacore_read(cia_context, addr);
            break;
    }

    /* FIXME: perhaps we need to restore some of the state from before reading.
     *        needs testing.
     * NOTE:  cia_context->last_read is only used to handle RMW instructions,
     *        since we only ever call this function in between instructions,
     *        we dont have to restore it's value.
     */
    return ret;
}

/* ------------------------------------------------------------------------- */


static void ciacore_intta(CLOCK offset, void *data)
{
    CLOCK rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    CIAT_LOGIN(("ciaTimerA ciacore_intta: myclk=%lu rclk=%lu",
                *(cia_context->clk_ptr), rclk));

    cia_do_update_ta(cia_context, rclk); /* may set CIA_IM_TA */

    ciat_ack_alarm(cia_context->ta, rclk);

    CIAT_LOG((
                 "ciacore_intta(rclk = %lu, tal = %lu, cra=%02x, int=%02x, ier=%02x.",
                 rclk, ciat_read_latch(cia_context->ta, rclk), cia_context->c_cia[CIA_CRA],
                 cia_context->irqflags, cia_context->c_cia[CIA_ICR]));

    /* cia_context->tat = (cia_context->tat + 1) & 1; */

    if ((cia_context->c_cia[CIA_CRA] & (CIA_CRA_INMODE|CIA_CR_RUNMODE|CIA_CR_START))
            == (CIA_CRA_INMODE_PHI2|CIA_CR_RUNMODE_CONTINUOUS|CIA_CR_START)) {
        /* if we do not need alarm, no PB6, no shift register, and not timer B
           counting timer A, then we can safely skip alarms... */
        if (((cia_context->c_cia[CIA_ICR] & CIA_IM_TA) &&
             (!(cia_context->irqflags & CIA_IM_SET)))
            || (cia_context->c_cia[CIA_CRA] & (CIA_CRA_SPMODE|CIA_CRA_INMODE)) /* != CIA_CRA_SPMODE_IN|CIA_CRA_INMODE_PHI2 */
            || (cia_context->c_cia[CIA_CRB] & CIA_CRB_INMODE_TA)) {
            ciat_set_alarm(cia_context->ta, rclk);
        }
    }

    if ((cia_context->c_cia[CIA_CRA] & CIA_CRA_SPMODE_OUT)) {
        /*
         * ~1.5 cycle delay until CNT is toggled.
         */
        if (cia_context->sr_bits != 0 || cia_context->sdr_valid) {
            uint32_t event = CIA_SDR_TOGGLE_CNT1;
            /*
             * If the clock pulses come too close together, we can't detect
             * the transition of the timer A output any more...
             * (This is generally only relevant if the timer starts at 0000)
             */
            if (cia_context->sdr_delay & (CIA_SDR_TOGGLE_CNT0|CIA_SDR_NOGGLE_CNT0)) {
                event = CIA_SDR_NOGGLE_CNT1;
            }
            schedule_sdr_alarm(cia_context, rclk, event);
        }
    }

    if ((cia_context->c_cia[CIA_CRB] & (CIA_CRB_INMODE_TA|CIA_CR_START)) == (CIA_CRB_INMODE_TA|CIA_CR_START)) {
        cia_update_tb(cia_context, rclk);
        cia_do_step_tb(cia_context, rclk);
    }

    CIAT_LOGOUT((""));
}

/*
 * Entry point for alarm callbacks.
 */
static void ciacore_intta_entry(CLOCK offset, void *data)
{
    cia_context_t *cia_context = (cia_context_t *)data;
    CLOCK rclk = *(cia_context->clk_ptr) - offset;

    ciacore_intta(offset, data);

    cia_ifr_catchup(cia_context, rclk);
    cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
}

/*
 * Timer B can run in 2 (4) modes
 * cia[f] & 0x60 == 0x00   count system 02 pulses
 * cia[f] & 0x60 == 0x40   count timer A underflows
 * cia[f] & 0x60 == 0x20 | 0x60 count CNT pulses => counter stops
 */


static void ciacore_inttb(CLOCK offset, void *data)
{
    CLOCK rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    CIAT_LOGIN(("ciaTimerB int_myciatb: myclk=%lu, rclk=%lu",
                *(cia_context->clk_ptr), rclk));

    cia_do_update_tb(cia_context, rclk);

    ciat_ack_alarm(cia_context->tb, rclk);

    CIAT_LOG((
                 "timer B ciacore_inttb(rclk=%lu, crb=%d, int=%02x, ier=%02x).",
                 rclk, cia_context->c_cia[CIA_CRB], cia_context->irqflags,
                 cia_context->c_cia[CIA_ICR]));

    /* cia_context->tbt = (cia_context->tbt + 1) & 1; */

    /* running and continous, then next alarm */
    if ((cia_context->c_cia[CIA_CRB] & (CIA_CRB_INMODE|CIA_CR_RUNMODE|CIA_CR_START)) ==
            (CIA_CRB_INMODE_PHI2|CIA_CR_RUNMODE_CONTINUOUS|CIA_CR_START)) {
        /* if no interrupt flag we can safely skip alarms */
        if (cia_context->c_cia[CIA_ICR] & CIA_IM_TB) {
            ciat_set_alarm(cia_context->tb, rclk);
        }
    }

    CIAT_LOGOUT((""));
}

/*
 * Entry point for alarm callbacks.
 */
static void ciacore_inttb_entry(CLOCK offset, void *data)
{
    cia_context_t *cia_context = (cia_context_t *)data;
    CLOCK rclk = *(cia_context->clk_ptr) - offset;

    ciacore_inttb(offset, data);

    cia_ifr_catchup(cia_context, rclk);
    cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
}

/* ------------------------------------------------------------------------- */

/*
 * The next several functions can be called from outside the CIA
 * to set the FLAG, CNT or SP input lines, or the whole SDR at once
 * (which is cheating).
 */

static void ciacore_async_interrupt(cia_context_t *cia_context, int flag)
{
    /*
     * This is for when an external signal has come in,
     * which doesn't synchronize with our internal activities.
     */
    CLOCK rclk = *(cia_context->clk_ptr);

    cia_set_irq_flag(cia_context, rclk, flag);

    /*
     * Leave calling cia_ifr_current() to the idle (or another) alarm,
     * since we're totally async here.
     */
    CLOCK idleclk = alarm_clk(cia_context->idle_alarm);
    if (idleclk > rclk + 1) {
        /*
         * If not already scheduled for the near future, re-schedule
         * it sooner. We could schedule it for rclk+0 if we could be
         * sure it won't be scheduled twice then (but we can't).
         */
        alarm_set(cia_context->idle_alarm, rclk + 1);
    } else {
        DBG(("ciacore_async_interrupt: idleclk == rclk + %ld\n", (long)(idleclk - rclk)));
    }
}

void ciacore_set_flag(cia_context_t *cia_context)
{
    ciacore_async_interrupt(cia_context, CIA_IM_FLG);
}

/*
 * Shortcut to shift a whole byte into the shift register all at once
 * instead of bit-by-bit.
 */
void ciacore_set_sdr(cia_context_t *cia_context, uint8_t data)
{
    if ((cia_context->c_cia[CIA_CRA] & CIA_CRA_SPMODE) == CIA_CRA_SPMODE_IN) {

        cia_context->c_cia[CIA_SDR] = data;
        DBG(("ciacore_set_sdr %s: %02x\n", cia_context->myname, data));

        /*
         * For more cycle-exactness, something like
         * cia_context->sdr_delay |= CIA_SDR_SET_SDR_IRQ2;
         * and enable the sdr_alarm ?
         */
        ciacore_async_interrupt(cia_context, CIA_IM_SDR);

        alarm_unset(cia_context->sdr_alarm);
    }
}

void ciacore_set_cnt(cia_context_t *cia_context, bool data)
{
    /* Is the CNT input changing? */
    if (data != cia_context->cnt_in_state) {
        /* Is the shift register set to input? */
        if ((cia_context->c_cia[CIA_CRA] & CIA_CRA_SPMODE) == CIA_CRA_SPMODE_IN) {
            /* Is this the right way to start shifting a byte? */
            if (!data && cia_context->sr_bits == 0) {
                cia_context->sr_bits = 16;
            }

            cia_context->sr_bits--;

            /* Is it rising? */
            if (data) {
                /* Shift register */
                cia_context->shifter <<= 1;
                /* https://www.forum64.de/index.php?thread/109935-mos6526-cia-detailed-test-vectors-and-models/&pageNo=2
                 * "My impression was that after the positive edge of CNT the
                 * CIA samples SP on the second rising edge of PHI2 following
                 * the next falling edge of PHI2. That's true even if CNT goes
                 * low again before this time (but a second rising edge of CNT
                 * before the data is sampled seems to kill the cycle). This
                 * would suggest that it takes 1,5 (best case) to 3,5 (worst
                 * case) PHI2 cycles to sample the data, depending on the phase
                 * difference between PHI2 and CNT. And I could in fact
                 * "reliably" transfer data into the SR with about double the
                 * nominal rate (that it, 2 x PHI2 instead of 4 x) when I
                 * aligned CNT to just before the negative PHI2 edges."
                 *
                 * We ignore that for now and sample and shift immediately.
                 */
                cia_context->shifter |= cia_context->sp_in_state;

                if (cia_context->sr_bits == 0) {
                    ciacore_set_sdr(cia_context, cia_context->shifter & 0xFF);
                    /* Most likely not needed:
                     * cia_context->sr_bits = 16;
                     * because it's set to 16 anyway on the next falling edge.
                     */
                }
            }
        }
        /* TODO: Handle Timer B if it is in CNT pulse counting mode */
        cia_context->cnt_in_state = data;
    }
}

void ciacore_set_sp(cia_context_t *cia_context, bool data)
{
    cia_context->sp_in_state = data & 1;
}

/* ------------------------------------------------------------------------- */

/*
 * Schedule a new alarm, for the current cycle.
 * That means it should normally be invoked only from a CPU register access,
 * not from an alarm.
 */
static void schedule_sdr_alarm(cia_context_t *cia_context, CLOCK rclk, uint32_t feed)
{
    cia_context->sdr_delay |= feed;
    alarm_set(cia_context->sdr_alarm, rclk);
}

/*
 * Alarm to handle the shift register for some cycles after TA reaches 0000.
 *
 * We need to make sure that if this alarm and also ciacore_intta
 * is scheduled for the same clock, that TA is handled first.
 * If this one runs first, ciacore_intta will schedule another run of intsdr
 * for the same cycle...
 */
static void ciacore_intsdr(CLOCK offset, void *data)
{
    cia_context_t *cia_context = (cia_context_t *)data;
    CLOCK rclk = *(cia_context->clk_ptr) - offset;
    uint32_t feed = 0;

    DBG(("ciacore_intsdr: sr_bits %u, shifter %04x sdr_valid %d sdr_delay %05x\n", cia_context->sr_bits, cia_context->shifter, cia_context->sdr_valid, cia_context->sdr_delay));

    if (alarm_clk(cia_context->ta_alarm) == rclk) {
        /* INT TA scheduled at the same clock cycle - do that one first */
        ciacore_intta(offset, data);
    }

    if (cia_context->sdr_delay & CIA_SDR_SET0) {
        if (cia_context->sr_bits == 0) {
            cia_context->sr_bits = 16;
            cia_context->shifter = cia_context->c_cia[CIA_SDR] << 1;
        } else if (cia_context->sr_bits == 1) {
            /* If this case is even possible, because when sr_bits
             * is decremented to 1 at TA underflow,
             * the value is loaded already and sr_bits is set to 17.
             */
            cia_context->shifter |= cia_context->c_cia[CIA_SDR];
            cia_context->sr_bits = 17;
        } else {
            cia_context->sdr_valid = true;
        }
    }

    if (cia_context->sdr_delay & CIA_SDR_TOGGLE_CNT0) {
        /*
         * TODO: should this condition be re-checked here, or should the
         * event bit be cleared when it becomes false elsewhere?
         *if (cia_context->sr_bits || cia_context->sdr_valid)
         */
        {
            CIAT_LOG(("rclk=%lu SDR: timer A underflow, bits=%u",
                        rclk, cia_context->sr_bits));

            if (cia_context->sr_bits && (--cia_context->sr_bits & 1)) {

                /*printf("%s: rclk=%d, store_sdr(%02x, '%c'\n",
                  cia_context->myname,
                  rclk, cia_context->shifter);*/

                if (cia_context->set_sp) {
                    /* Note: the bit that's just left of the byte */
                    bool bit = (cia_context->shifter >> 8) & 1;
                    (cia_context->set_sp)(cia_context, rclk, bit);
                }
                cia_context->cnt_out_state = false;
                if (cia_context->set_cnt) {
                    (cia_context->set_cnt)(cia_context, rclk, false);
                }

                if (cia_context->sr_bits == 1) {
                    DBG(("(store_sdr) %s: %04x %02x rclk %lu\n", cia_context->myname, cia_context->shifter, cia_context->shifter>>8, rclk));
                    (cia_context->store_sdr)(cia_context, (cia_context->shifter >> 8) & 0xFF);
                    /* IFR/IRQ requires 2 cycle delay */
                    feed |= CIA_SDR_SET_SDR_IRQ2;
                    /* TODO: maybe unset the other SET_SDR_IRQx bits? */

                    if (cia_context->sdr_valid) {
                        cia_context->shifter |= cia_context->c_cia[CIA_SDR];
                        cia_context->sdr_valid = false;
                        cia_context->sr_bits = 17;
                    }
                }
            } else {
                /* So either sr_bits was 0, or after decrementing it is even. */
                cia_context->shifter <<= 1;

                cia_context->cnt_out_state = true;
                if (cia_context->set_cnt) {
                    (cia_context->set_cnt)(cia_context, rclk, true);
                }
            }
        }
        DBG(("ciacore_intta(2): sr_bits %u, shifter %04x sdr_valid %d\n", cia_context->sr_bits, cia_context->shifter, cia_context->sdr_valid));
    }

    if (cia_context->sdr_delay & CIA_SDR_SET_SDR_IRQ0) {
        DBG(("ciacore_intsdr: set CIA_IM_SDR  %lu\n", rclk));
        cia_set_irq_flag(cia_context, rclk, CIA_IM_SDR);
    }

    cia_context->sdr_delay |= feed;
    cia_context->sdr_delay <<= 1;
    cia_context->sdr_delay &= ~CIA_SDR_CLEAR;

    if (cia_context->cnt_out_state) {
        cia_context->sdr_delay |= CIA_SDR_CNT0;
    }

    bool active = (cia_context->sdr_delay & CIA_SDR_ACTIVE) != 0;
    if (!active) {
        uint32_t all_cnt = cia_context->sdr_delay & ALL_SDR_CNT;

        if (all_cnt != 0 && all_cnt != ALL_SDR_CNT) {
            active = true;
        }
    }
    if (active) {
        alarm_set(cia_context->sdr_alarm, rclk + 1);
    } else {
        alarm_unset(cia_context->sdr_alarm);
    }
}

/*
 * Entry point for alarm callbacks.
 */
static void ciacore_intsdr_entry(CLOCK offset, void *data)
{
    cia_context_t *cia_context = (cia_context_t *)data;
    CLOCK rclk = *(cia_context->clk_ptr) - offset;

    ciacore_intsdr(offset, data);

    cia_ifr_catchup(cia_context, rclk);
    if (cia_context->ifr_clock == rclk) { /* don't call it twice */
        cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
    }
}

/* ------------------------------------------------------------------------- */

/* when defined, randomize the power frequency a bit instead of linearly
   meandering around the correct value */
#define TODRANDOM

static void ciacore_inttod(CLOCK offset, void *data)
{
    int ts, sl, sh, ml, mh, hl, hh, pm, update = 0;
    CLOCK rclk, tclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    if (cia_context->power_freq == 0) {
        /* if power frequency is not initialized, or not present, return immediatly.
           check again in about 1/10th second */
        cia_context->todclk = *(cia_context->clk_ptr) + 100000;
        alarm_set(cia_context->tod_alarm, cia_context->todclk);
        return;
    }

    /* set up new int
       the time between power ticks should be ticks_per_sec / power_freq
       in reality the deviation can be quite large in small time frames, but is
       very accurate in longer time frames. we try to maintain a stable tick
       frequency that is as close as possible to the wanted 50/60 Hz. we should
       also introduce some kind of randomness to mimic realistic behaviour.

       for some details (german) http://www.netzfrequenzmessung.de/
     */
    cia_context->todticks = cia_context->ticks_per_sec / cia_context->power_freq;
    tclk = ((cia_context->power_tickcounter * cia_context->ticks_per_sec) / cia_context->power_freq);
    if (cia_context->power_ticks < tclk) {
#ifdef TODRANDOM
          cia_context->todticks += lib_unsigned_rand(0, 3);
#else
          /* cia_context->todticks += (((tclk - cia_context->power_ticks) * 3) / 2); */
          cia_context->todticks++;
#endif
    } else if (cia_context->power_ticks > tclk) {
#ifdef TODRANDOM
          cia_context->todticks -= lib_unsigned_rand(0, 3);
#else
          /* cia_context->todticks -= (((cia_context->power_ticks - tclk) * 3) / 2); */
          cia_context->todticks--;
#endif
    }
    cia_context->power_tickcounter++;
    if (cia_context->power_tickcounter >= cia_context->power_freq) {
        cia_context->todticks = cia_context->ticks_per_sec - cia_context->power_ticks;
        cia_context->power_tickcounter = 0;
        cia_context->power_ticks = 0;
    } else {
        cia_context->power_ticks += cia_context->todticks;
    }

    cia_context->todclk = *(cia_context->clk_ptr) + cia_context->todticks;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);

    if (!(cia_context->todstopped)) {
        /*
         * The divider which divides the 50 or 60 Hz power supply ticks into
         * 10 Hz uses a 3-bit ring counter, which goes 000, 001 011, 111, 110,
         * 100.
         * For 50 Hz: matches at 110 (like "4")
         * For 60 Hz: matches at 100 (like "5")
         * (the middle bit of the match value is CRA7)
         * After a match there is a 1 tick delay (until the next power supply
         * tick) and then the 1/10 seconds counter increases, and the ring
         * resets to 000.
         */
        update = (cia_context->todtickcounter ==
            ((cia_context->c_cia[CIA_CRA] & CIA_CRA_TODIN_50HZ) ? 4 : 5));

        if (update) {
            /* Reset the counter */
            cia_context->todtickcounter = 0;
        } else {
            /* Count 50/60 Hz power supply ticks */
            cia_context->todtickcounter++;
            /* Ring counter of 3 bits has 6 states */
            if (cia_context->todtickcounter > 5)
                cia_context->todtickcounter = 0;
        }
    }

    if (update) {
        /* advance the counters.
         * - individual counters are 4 bit
         *   except for sh and mh which are 3 bits
         */
        ts = cia_context->c_cia[CIA_TOD_TEN] & 0x0f;
        sl = cia_context->c_cia[CIA_TOD_SEC] & 0x0f;
        sh = (cia_context->c_cia[CIA_TOD_SEC] >> 4) & 0x07;
        ml = cia_context->c_cia[CIA_TOD_MIN] & 0x0f;
        mh = (cia_context->c_cia[CIA_TOD_MIN] >> 4) & 0x07;
        hl = cia_context->c_cia[CIA_TOD_HR] & 0x0f;
        hh = (cia_context->c_cia[CIA_TOD_HR] >> 4) & 0x01;
        pm = cia_context->c_cia[CIA_TOD_HR] & 0x80;

        /* tenth seconds (0-9) */
        ts = (ts + 1) & 0x0f;
        if (ts == 10) {
            ts = 0;
            /* seconds (0-59) */
            sl = (sl + 1) & 0x0f; /* x0...x9 */
            if (sl == 10) {
                sl = 0;
                sh = (sh + 1) & 0x07; /* 0x...5x */
                if (sh == 6) {
                    sh = 0;
                    /* minutes (0-59) */
                    ml = (ml + 1) & 0x0f; /* x0...x9 */
                    if (ml == 10) {
                        ml = 0;
                        mh = (mh + 1) & 0x07; /* 0x...5x */
                        if (mh == 6) {
                            mh = 0;
                            /* hours (1-12) */
                            /* flip from 09:59:59 to 10:00:00 */
                            /* or from 12:59:59 to 01:00:00 */
                            if (((hh == 1) && (hl == 2))
                                || ((hh == 0) && (hl == 9))) {
                                hl = hh;
                                hh ^= 1;
                            } else {
                                hl = (hl + 1) & 0x0f;
                                /* toggle the am/pm flag when reaching 12 */
                                if ((hh == 1) && (hl == 2)) {
                                    pm ^= 0x80;
                                }
                            }
                        }
                    }
                }
            }
        }

        DBG(("ciacore_inttod [%s %02x:%02x:%02x.%x0]->[%s %x%x:%x%x:%x%x.%x0]\n",
             cia_context->c_cia[CIA_TOD_HR] & 0x80 ? "pm" : "am",
             cia_context->c_cia[CIA_TOD_HR] & 0x7f,
             cia_context->c_cia[CIA_TOD_MIN],
             cia_context->c_cia[CIA_TOD_SEC],
             cia_context->c_cia[CIA_TOD_TEN],
             pm ? "pm" : "am", hh, hl, mh, ml, sh, sl, ts));

        cia_context->c_cia[CIA_TOD_TEN] = ts;
        cia_context->c_cia[CIA_TOD_SEC] = sl | (sh << 4);
        cia_context->c_cia[CIA_TOD_MIN] = ml | (mh << 4);
        cia_context->c_cia[CIA_TOD_HR] = hl | (hh << 4) | pm;

        /* check alarm */
        check_ciatodalarm(cia_context, rclk);
    }
}
#undef TODRANDOM

/*
 * Entry point for alarm callbacks.
 */
static void ciacore_inttod_entry(CLOCK offset, void *data)
{
    cia_context_t *cia_context = (cia_context_t *)data;
    CLOCK rclk = *(cia_context->clk_ptr) - offset;

    ciacore_inttod(offset, data);

    cia_ifr_catchup(cia_context, rclk);
    if (cia_context->ifr_clock == rclk) { /* don't call it twice */
        cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
    }
}

void ciacore_setup_context(cia_context_t *cia_context)
{
    cia_context->log = LOG_ERR;
    cia_context->read_clk = 0;
    cia_context->read_offset = 0;
    cia_context->last_read = 0;
    cia_context->write_offset = 1;
    cia_context->model = 0;
}

/*
    this callback takes care of the problem that when ciat_update has to catch
    up with an excessive amount of clock cycles it will consume a lot of cpu
    time, in the worst case leading to a noticeable stall of the entire emulation
    (see bug #3424428)

    FIXME: maybe other stuff must be handled here
 */
static void ciacore_idle(CLOCK offset, void *data)
{
    CLOCK clk, rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    clk = *(cia_context->clk_ptr);
    rclk = clk - offset;

/* printf("ciacore_idle: clk=%lu rclk=%lu\n", clk, rclk); */

    cia_update_ta(cia_context, rclk);
    cia_update_tb(cia_context, rclk);

    /*
     * Schedule another idle alarm far in the future.
     * Do it before running the ifr delay line, in case that it wants
     * to schedule it for sooner (we don't want to override that here).
     */
    alarm_set(cia_context->idle_alarm, rclk + CIA_MAX_IDLE_CYCLES);

    cia_ifr_catchup(cia_context, rclk);
    if (cia_context->ifr_clock == rclk) { /* don't call it twice */
        cia_ifr_current(cia_context, rclk, CIA_IFR_CUR_NXT);
    }
}

void ciacore_init(cia_context_t *cia_context, alarm_context_t *alarm_context,
                  interrupt_cpu_status_t *int_status)
{
    char *buffer;

    cia_context->ta = lib_calloc(1, sizeof(ciat_t));
    cia_context->tb = lib_calloc(1, sizeof(ciat_t));

    ciat_init_table();

    cia_context->log = log_open(cia_context->myname);

    buffer = lib_msprintf("%s_IDLE", cia_context->myname);
    cia_context->idle_alarm = alarm_new(alarm_context, buffer,
                                        ciacore_idle,
                                        (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TA", cia_context->myname);
    cia_context->ta_alarm = alarm_new(alarm_context, buffer,
                                      ciacore_intta_entry,
                                      (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TB", cia_context->myname);
    cia_context->tb_alarm = alarm_new(alarm_context, buffer,
                                      ciacore_inttb_entry,
                                      (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TOD", cia_context->myname);
    cia_context->tod_alarm = alarm_new(alarm_context, buffer,
                                       ciacore_inttod_entry,
                                       (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_SDR", cia_context->myname);
    cia_context->sdr_alarm = alarm_new(alarm_context, buffer,
                                       ciacore_intsdr_entry,
                                       (void *)cia_context);
    lib_free(buffer);

    cia_context->int_num
        = interrupt_cpu_status_int_new(int_status, cia_context->myname);

    buffer = lib_msprintf("%s_TA", cia_context->myname);
    ciat_init(cia_context->ta, buffer, *(cia_context->clk_ptr),
              cia_context->ta_alarm);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TB", cia_context->myname);
    ciat_init(cia_context->tb, buffer, *(cia_context->clk_ptr),
              cia_context->tb_alarm);
    lib_free(buffer);

    /* Clear the optional callbacks */
    cia_context->set_sp = NULL;
    cia_context->set_cnt = NULL;

    /* This is not internal state, so does not get reset on RESET */
    cia_context->sp_in_state = true;
    cia_context->cnt_in_state = true;
}

void ciacore_shutdown(cia_context_t *cia_context)
{
    lib_free(cia_context->prv);
    lib_free(cia_context->ta);
    lib_free(cia_context->tb);
    lib_free(cia_context->myname);
    lib_free(cia_context);
}

/* -------------------------------------------------------------------------- */
/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 2.5
 */

#define CIA_DUMP_VER_MAJOR      2
#define CIA_DUMP_VER_MINOR      5

/*
 * The dump data:
 *
 * UBYTE        ORA
 * UBYTE        ORB
 * UBYTE        DDRA
 * UBYTE        DDRB
 * UWORD        TA
 * UWORD        TB
 * UBYTE        TOD_TEN         current value
 * UBYTE        TOD_SEC
 * UBYTE        TOD_MIN
 * UBYTE        TOD_HR
 * UBYTE        SDR
 * UBYTE        IER             enabled interrupts mask
 * UBYTE        CRA
 * UBYTE        CRB
 *
 * UWORD        TAL             latch value
 * UWORD        TBL             latch value
 * UBYTE        IFR             interrupts currently active
 * UBYTE        PBSTATE         bit6/7 reflect PB6/7 toggle state
 *                              bit 2/3 reflect port bit state
 * UBYTE        SRHBITS         number of half-bits to still shift in/out SDR
 * UBYTE        ALARM_TEN
 * UBYTE        ALARM_SEC
 * UBYTE        ALARM_MIN
 * UBYTE        ALARM_HR
 *
 * UBYTE        READICR         clk - when ICR was read last + 128
 * UBYTE        TODLATCHED      0=running, 1=latched, 2=stopped (writing)
 * UBYTE        TODL_TEN                latch value
 * UBYTE        TODL_SEC
 * UBYTE        TODL_MIN
 * UBYTE        TODL_HR
 * DWORD        TOD_TICKS       clk ticks till next tenth of second
 *
 * UBYTE        IRQ             0=IRQ line inactive, 1=IRQ line active
 *
 *                              These bits have been added in V1.1
 *
 * WORD         TA              Timer A state bits (see ciatimer.h)
 * WORD         TB              Timer B state bits (see ciatimer.h)
 *
 *                              These bits have been added in V1.2
 *
 * BYTE         SHIFTER         actual shift register (original data)
 * BYTE         SDR_VALID       data in SDR valid to be shifted
 *
 * BYTE         irq_enabled     IRQ enabled
 *
 * BYTE         todtickcounter  TOD tick counter
 *
 *                              These bits have been added in V2.3
 *
 * BYTE         SHIFTER_HI      high byte of SHIFTER
 * BYTE         SDR_ALARM       if an sdr_alarm is pending and when
 * BYTE         SP_CNT_IN       input state of SP and CNT
 *
 *                              These bits have been added in V2.4
 *
 * DWORD        SDR_DELAY       event/history bits for the serial data register
 * BYTE         SP_CNT_OUT      output state of SP and CNT
 *
 *                              These bits have been added in V2.5
 *
 * DWORD        IFR_DELAY       event/history bits for the interrupt flag reg.
 * BYTE         ACK_IRQFLAGS    flags in IFR to acknowledge, i.e. clear.
 * BYTE         NEW_IRQFLAGS    new flags in IFR that were set recently.
 */

/* FIXME!!!  Error check.  */
int ciacore_snapshot_write_module(cia_context_t *cia_context, snapshot_t *s)
{
    CLOCK rclk = *(cia_context->clk_ptr);
    snapshot_module_t *m;
    uint8_t byte;

    cia_update_ta(cia_context, rclk);
    cia_update_tb(cia_context, rclk);
    cia_ifr_catchup(cia_context, rclk);
    cia_ifr_current(cia_context, rclk, CIA_IFR_CURRENT);

    m = snapshot_module_create(s, cia_context->myname,
                               (uint8_t)CIA_DUMP_VER_MAJOR,
                               (uint8_t)CIA_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }


#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                rclk, cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
    log_message(cia_context->log, "write ciaint=%02x, ciaier=%02x",
                cia_context->irqflags,
                cia_context->c_cia[CIA_ICR]);
#endif

    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_PRA]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_PRB]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_DDRA]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_DDRB]));
    SMW_W(m, ciat_read_timer(cia_context->ta, rclk));
    SMW_W(m, ciat_read_timer(cia_context->tb, rclk));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_TOD_TEN]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_TOD_SEC]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_TOD_MIN]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_TOD_HR]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_SDR]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_ICR]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_CRA]));
    SMW_B(m, (uint8_t)(cia_context->c_cia[CIA_CRB]));

    SMW_W(m, ciat_read_latch(cia_context->ta, rclk));
    SMW_W(m, ciat_read_latch(cia_context->tb, rclk));
    SMW_B(m, ciacore_peek(cia_context, CIA_ICR));

    /* Bits 2 & 3 are compatibility to snapshot format v1.0 */
    SMW_B(m, (uint8_t)((cia_context->tat ? 0x40 : 0)
                    | (cia_context->tbt ? 0x80 : 0)
                    | (ciat_is_underflow_clk(cia_context->ta,
                                             rclk) ? 0x04 : 0)
                    | (ciat_is_underflow_clk(cia_context->tb, rclk)
                       ? 0x08 : 0)));
    SMW_B(m, (uint8_t)cia_context->sr_bits);
    SMW_B(m, cia_context->todalarm[0]);
    SMW_B(m, cia_context->todalarm[1]);
    SMW_B(m, cia_context->todalarm[2]);
    SMW_B(m, cia_context->todalarm[3]);

    if (cia_context->rdi) {
        if ((rclk - cia_context->rdi) > 120) {
            byte = 0;
        } else {
            byte = rclk + 128 - cia_context->rdi;
        }
    } else {
        byte = 0;
    }
    SMW_B(m, byte);

    SMW_B(m, (uint8_t)((cia_context->todlatched ? 1 : 0)
                    | (cia_context->todstopped ? 2 : 0)));
    SMW_B(m, cia_context->todlatch[0]);
    SMW_B(m, cia_context->todlatch[1]);
    SMW_B(m, cia_context->todlatch[2]);
    SMW_B(m, cia_context->todlatch[3]);

    SMW_CLOCK(m, (cia_context->todclk - rclk));

    ciat_save_snapshot(cia_context->ta, rclk, m,
                       (CIA_DUMP_VER_MAJOR << 8) | CIA_DUMP_VER_MINOR);
    ciat_save_snapshot(cia_context->tb, rclk, m,
                       (CIA_DUMP_VER_MAJOR << 8) | CIA_DUMP_VER_MINOR);

    SMW_B(m, cia_context->shifter & 0xFF);
    SMW_B(m, (uint8_t)(cia_context->sdr_valid));
    /* This has to be tested */
    SMW_B(m, cia_context->irq_enabled);

    SMW_B(m, cia_context->todtickcounter);

    SMW_B(m, (cia_context->shifter >> 8) & 0xFF);        /* SHIFTER_HI */

    CLOCK sdr_alarm_pending = alarm_clk(cia_context->sdr_alarm);
    if (sdr_alarm_pending > 0) {
        byte = 1 + sdr_alarm_pending - rclk;
    } else {
        byte = 0;
    }
    SMW_B(m, byte);     /* SDR_ALARM */

    byte = (cia_context->sp_in_state      ? 0x80 : 0)
         | (cia_context->cnt_in_state     ? 0x40 : 0)
         | (cia_context->sdr_force_finish ? 0x20 : 0);

    SMW_B(m, byte);     /* SP_CNT_IN */

    /* 2.4 */

    SMW_DW(m, cia_context->sdr_delay);    /* SDR_DELAY */

    byte = /* (cia_context->sp_out_state      ? 0x80 : 0)
         | */ (cia_context->cnt_out_state     ? 0x40 : 0);

    SMW_B(m, byte);     /* SP_CNT_OUT */

    /* 2.5 */

    SMW_DW(m, cia_context->ifr_delay);    /* IFR_DELAY */
    SMW_DB(m, cia_context->ack_irqflags); /* ACK_IRQFLAGS */
    SMW_DB(m, cia_context->new_irqflags); /* NEW_IRQFLAGS */
    /*
     * We don't need to save ifr_clock, since we made sure it
     * is up to date, i.e. equal to rclk.
     */

    snapshot_module_close(m);

    return 0;
}

int ciacore_snapshot_read_module(cia_context_t *cia_context, snapshot_t *s)
{
    uint8_t vmajor, vminor;
    uint8_t byte;
    CLOCK qword;
    CLOCK rclk = *(cia_context->clk_ptr);
    snapshot_module_t *m;
    uint16_t cia_tal, cia_tbl, cia_tac, cia_tbc;

    m = snapshot_module_open(s, cia_context->myname, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    if (vmajor != CIA_DUMP_VER_MAJOR) {
        log_error(cia_context->log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  vmajor, vminor, CIA_DUMP_VER_MAJOR, CIA_DUMP_VER_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    ciacore_reset(cia_context);

    /* stop timers, just in case */
    ciat_set_ctrl(cia_context->ta, *(cia_context->clk_ptr), 0);
    ciat_set_ctrl(cia_context->tb, *(cia_context->clk_ptr), 0);
    alarm_unset(cia_context->tod_alarm);
    alarm_unset(cia_context->sdr_alarm);

    {
        SMR_B(m, &(cia_context->c_cia[CIA_PRA]));
        SMR_B(m, &(cia_context->c_cia[CIA_PRB]));
        SMR_B(m, &(cia_context->c_cia[CIA_DDRA]));
        SMR_B(m, &(cia_context->c_cia[CIA_DDRB]));

        byte = cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]);
        cia_context->old_pa = byte ^ 0xff;    /* all bits change? */
        (cia_context->undump_ciapa)(cia_context, rclk, byte);
        cia_context->old_pa = byte;

        byte = cia_context->c_cia[CIA_PRB] | ~(cia_context->c_cia[CIA_DDRB]);
        cia_context->old_pb = byte ^ 0xff;    /* all bits change? */
        (cia_context->undump_ciapb)(cia_context, rclk, byte);
        cia_context->old_pb = byte;
    }

    SMR_W(m, &cia_tac);
    SMR_W(m, &cia_tbc);
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_TEN]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_SEC]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_MIN]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_HR]));
    SMR_B(m, &(cia_context->c_cia[CIA_SDR]));

    SMR_B(m, &(cia_context->c_cia[CIA_ICR]));
    SMR_B(m, &(cia_context->c_cia[CIA_CRA]));
    SMR_B(m, &(cia_context->c_cia[CIA_CRB]));

    SMR_W(m, &cia_tal);
    SMR_W(m, &cia_tbl);

    SMR_B(m, &byte);
    cia_context->irqflags = byte;

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log,
                "read ciaint=%02x, ciaier=%02x.", cia_context->irqflags,
                cia_context->c_cia[CIA_ICR]);
#endif

    SMR_B(m, &byte);
    cia_context->tat = (byte & 0x40) ? 1 : 0;
    cia_context->tbt = (byte & 0x80) ? 1 : 0;

    SMR_B(m, &byte);
    cia_context->sr_bits = byte;

    SMR_B(m, &(cia_context->todalarm[0]));
    SMR_B(m, &(cia_context->todalarm[1]));
    SMR_B(m, &(cia_context->todalarm[2]));
    SMR_B(m, &(cia_context->todalarm[3]));

    SMR_B(m, &byte);
    if (byte) {
        cia_context->rdi = *(cia_context->clk_ptr) + 128 - byte;
    } else {
        cia_context->rdi = 0;
    }
#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "snap read rdi=%02x", byte);
    log_message(cia_context->log, "snap setting rdi to %d (rclk=%d)",
                cia_context->rdi,
                *(cia_context->clk_ptr));
#endif

    SMR_B(m, &byte);
    cia_context->todlatched = byte & 1;
    cia_context->todstopped = byte & 2;
    SMR_B(m, &(cia_context->todlatch[0]));
    SMR_B(m, &(cia_context->todlatch[1]));
    SMR_B(m, &(cia_context->todlatch[2]));
    SMR_B(m, &(cia_context->todlatch[3]));

    SMR_CLOCK(m, &qword);       /* TOD_TICKS */
    cia_context->todclk = *(cia_context->clk_ptr) + qword;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);

    /* timer switch-on code from store_cia[CIA_CRA/CRB] */

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                *(cia_context->clk_ptr), cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
#endif

    ciat_load_snapshot(cia_context->ta, rclk, cia_tac, cia_tal,
                       cia_context->c_cia[CIA_CRA], m, (vmajor << 8) | vminor);
    ciat_load_snapshot(cia_context->tb, rclk, cia_tbc, cia_tbl,
                       cia_context->c_cia[CIA_CRB], m, (vmajor << 8) | vminor);

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                *(cia_context->clk_ptr), cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
#endif

    if (vminor > 1) {
        SMR_B(m, &byte);
        cia_context->shifter = byte;
        SMR_B(m, &byte);
        cia_context->sdr_valid = byte;

        if (SMR_B(m, &(cia_context->irq_enabled)) < 0) {
            /* old (buggy) way to restore interrupt */
            /* still enabled; will be fixed in 1.13.x */
            cia_context->irq_enabled = (cia_context->c_cia[CIA_ICR] & CIA_IM_SET) ? 1 : 0;
        }

        if (cia_context->irq_enabled) {
            (cia_context->cia_restore_int)(cia_context, 1);
        } else {
            (cia_context->cia_restore_int)(cia_context, 0);
        }

        SMR_B(m, &(cia_context->todtickcounter));
    }

    if (vminor > 2) {
        SMR_B(m, &byte);        /* SHIFTER_HI */
        cia_context->shifter |= byte << 8;

        SMR_B(m, &byte);        /* SDR_ALARM */
        if (byte) {
            alarm_set(cia_context->sdr_alarm, rclk + byte - 1);
        }

        SMR_B(m, &byte);        /* SP_CNT_IN */
        cia_context->sp_in_state      = (byte & 0x80) != 0;
        cia_context->cnt_in_state     = (byte & 0x40) != 0;
        cia_context->sdr_force_finish = (byte & 0x20) != 0;
    }

    if (vminor > 3) {
        uint32_t dword;
        SMR_DW(m, &dword);      /* SDR_DELAY */
        cia_context->sdr_delay = dword;

        SMR_B(m, &byte);        /* SP_CNT_OUT */
        /* cia_context->sp_out_state   = (byte & 0x80) != 0; */
        cia_context->cnt_out_state     = (byte & 0x40) != 0;
    }

    if (vminor > 4) {
        uint32_t dword;

        SMR_DW(m, &dword);      /* IFR_DELAY */
        cia_context->ifr_delay = dword;
        cia_context->ifr_clock = rclk + 1;

        SMR_B(m, &byte);        /* ACK_IRQFLAGS */
        cia_context->ack_irqflags = byte;
        SMR_B(m, &byte);        /* NEW_IRQFLAGS */
        cia_context->new_irqflags = byte;

        cia_ifr_current(cia_context, rclk, CIA_IFR_NEXT);
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}

int ciacore_dump(cia_context_t *cia_context)
{
    char *s;
    CLOCK clk = *(cia_context->clk_ptr);

    mon_out("ICR: %02x (mask: %02x)  CTRLA: %02x  CTRLB: %02x, delay: %04x rclk-%d to_ack: %02x\n",
            ciacore_peek(cia_context, 0x0d),
            cia_context->c_cia[CIA_ICR],
            ciacore_peek(cia_context, 0x0e),
            ciacore_peek(cia_context, 0x0f),
            cia_context->ifr_delay,
            (int)(clk - cia_context->ifr_clock),
            cia_context->ack_irqflags);

    mon_out("\nPort A: %02x  DDR: %02x\n",
            ciacore_peek(cia_context, 0x00),
            ciacore_peek(cia_context, 0x02));
    mon_out("Port B: %02x  DDR: %02x\n",
            ciacore_peek(cia_context, 0x01),
            ciacore_peek(cia_context, 0x03));

    mon_out("\nTimer A IRQ: %s  running: %s  mode: %s\n",
            (cia_context->c_cia[CIA_ICR] & 1) ? "on" : "off",
            ciacore_peek(cia_context, 0x0e) & 1 ? "yes" : "no",
            ciacore_peek(cia_context, 0x0e) & (1 << 3) ? "one-shot" : "continues");
    mon_out("Timer A counts: %s  PB6 output: %s (%s)\n",
            ciacore_peek(cia_context, 0x0e) & (1 << 5) ? "CNT transitions" : "System clock",
            ciacore_peek(cia_context, 0x0e) & (1 << 1) ? "yes" : "no",
            ciacore_peek(cia_context, 0x0e) & (1 << 2) ? "Toggle" : "Pulse");
    mon_out("Timer A: %04x (latched %04x)\n",
            (unsigned int)(ciacore_peek(cia_context, 0x04) + (ciacore_peek(cia_context, 0x05) << 8)),
            cia_context->ta->latch);

    mon_out("Timer B IRQ: %s  running: %s  mode: %s\n",
            (cia_context->c_cia[CIA_ICR] & (1 << 1)) ? "on" : "off",
            ciacore_peek(cia_context, 0x0f) & 1 ? "yes" : "no",
            ciacore_peek(cia_context, 0x0f) & (1 << 3) ? "one-shot" : "continues");
    switch (ciacore_peek(cia_context, 0x0f) & (3 << 5)) {
        default:
        case (0 << 5): s = "System clock"; break;
        case (1 << 5): s = "CNT transitions"; break;
        case (2 << 5): s = "Timer A undeflows"; break;
        case (3 << 5): s = "Timer A undeflows with CNT"; break;
    }
    mon_out("Timer B counts: %s  PB7 output: %s (%s)\n",
            s,
            ciacore_peek(cia_context, 0x0f) & (1 << 1) ? "yes" : "no",
            ciacore_peek(cia_context, 0x0f) & (1 << 2) ? "Toggle" : "Pulse");
    mon_out("Timer B: %04x (latched %04x)\n",
            (unsigned int)(ciacore_peek(cia_context, 0x06) + (ciacore_peek(cia_context, 0x07) << 8)),
            cia_context->tb->latch);

    mon_out("\nTOD IRQ: %s  latched: %s  running: %s  mode: %sHz\n",
            (cia_context->c_cia[CIA_ICR] & (1<<2)) ? "on" : "off",
            cia_context->todlatched ? "yes" : "no",
            cia_context->todstopped ? "no" : "yes",
            ciacore_peek(cia_context, 0x0e) & (1 << 7) ? "50" : "60");
    mon_out("TOD Time:  %02x:%02x:%02x.%x (%s)\n",
            ciacore_peek(cia_context, 0x0b) & 0x7fU,
            ciacore_peek(cia_context, 0x0a),
            ciacore_peek(cia_context, 0x09),
            ciacore_peek(cia_context, 0x08),
            ciacore_peek(cia_context, 0x0b) & 0x80 ? "pm" : "am");
    mon_out("TOD Alarm: %02x:%02x:%02x.%x (%s)\n",
            cia_context->todalarm[0x0b - CIA_TOD_TEN] & 0x7fU,
            cia_context->todalarm[0x0a - CIA_TOD_TEN],
            cia_context->todalarm[0x09 - CIA_TOD_TEN],
            cia_context->todalarm[0x08 - CIA_TOD_TEN],
            cia_context->todalarm[0x0b - CIA_TOD_TEN] & 0x80 ? "pm" : "am");

    mon_out("\nShift Register IRQ: %s  mode: %s\n",
            (cia_context->c_cia[CIA_ICR] & (1<<3)) ? "on" : "off",
            ciacore_peek(cia_context, 0x0e) & (1 << 6) ? "output" : "input");
    mon_out("Shift Register Data Buffer: %02x\n",
            ciacore_peek(cia_context, 0x0c));
    mon_out("Timer A SDR: delay %05x\n", cia_context->sdr_delay);

    mon_out("\nFLAG1 IRQ: %s\n",
            (cia_context->c_cia[CIA_ICR] & (1<<4)) ? "on" : "off");

    return 0;
}
