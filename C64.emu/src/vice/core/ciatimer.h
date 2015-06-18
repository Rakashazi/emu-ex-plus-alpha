/*
 * ciatimer.c - MOS6526 (CIA) timer emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * Patches and improvements by
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

/*
 * 07mar1999 a.fachat
 * complete timer rewrite
 *
 * The timer state is now saved in a separate structure and
 * the (inline) functions operate on this structure.
 * Such the timer code need not be duplicated for both timers.
 *
 */

#ifndef VICE_CIATIMER_H
#define VICE_CIATIMER_H

#include "alarm.h"
#include "types.h"


/* #define      CIAT_DEBUG */
/* #undef       NO_INLINE */
/* #define CIAT_NEED_LOG */

/***************************************************************************/
/* constants */

#define CIAT_TABLEN     (2 << 13)

#define CIAT_CR_MASK    0x039
#define CIAT_CR_START   0x001
#define CIAT_CR_ONESHOT 0x008
#define CIAT_CR_FLOAD   0x010
#define CIAT_PHI2IN     0x020
#define CIAT_STEP       0x004

#define CIAT_COUNT2     0x002
#define CIAT_COUNT3     0x040
#define CIAT_COUNT      0x800
#define CIAT_LOAD1      0x080
#define CIAT_ONESHOT0   0x100
#define CIAT_ONESHOT    0x1000
#define CIAT_LOAD       0x200
#define CIAT_OUT        0x400

/***************************************************************************/
/* types */

typedef WORD ciat_tstate_t;     /* 16 bit type */

typedef struct ciat_s {
    const char    *name;        /* name of timer */
    ciat_tstate_t state;        /* timer bits */
    WORD latch;
    WORD cnt;
    CLOCK alarmclk;
    CLOCK clk;
    alarm_t       *alarm;
} ciat_t;

/***************************************************************************/
/* external prototypes */

extern ciat_tstate_t ciat_table[CIAT_TABLEN];

extern void ciat_init_table(void);


/***************************************************************************/
/* Timer debug stuff */

#ifdef CIAT_DEBUG

#  define       CIAT_NEED_LOG
#ifndef NO_INLINE
#  define       NO_INLINE
#endif
#  define       CIAT_LOGIN(a)   do { ciat_login a; } while (0)
#  define       CIAT_LOG(a)     do { ciat_log a; } while (0)
#  define       CIAT_LOGOUT(a)  do { ciat_logout a; } while (0)

#else /* CIAT_DEBUG */

#  define       CIAT_LOGIN(a)
#  define       CIAT_LOG(a)
#  define       CIAT_LOGOUT(a)

#endif  /* CIAT_DEBUG */

#ifdef CIAT_NEED_LOG

extern void ciat_login(const char *format, ...);
extern void ciat_logout(const char *format, ...);
extern void ciat_log(const char *format, ...);

extern void ciat_print_state(const ciat_t *state);

#endif

struct snapshot_module_s;

/***************************************************************************/
/* For maximum performance (these routines are small but used very often), we
   allow use of inlined functions.  This can be overridden by defining
   NO_INLINE (useful for debugging and profiling).  */

#if !defined NO_INLINE
#  ifndef INLINE_CIAT_FUNCS
#    define INLINE_CIAT_FUNCS
#  endif
#else
#  undef INLINE_CIAT_FUNCS
#endif

/***************************************************************************/

/* If we do not want the interrupt functions to be inlined, they are only
   compiled once when included in `ciatimer.c'.  */

#ifdef INLINE_CIAT_FUNCS
#  define _CIAT_FUNC inline static
#else
#  define _CIAT_FUNC
#endif

#if defined INLINE_CIAT_FUNCS || defined _CIATIMER_C

/* check when the next underflow will occur and set the alarm */
/* needs update before */
_CIAT_FUNC void ciat_set_alarm(ciat_t *state, CLOCK cclk)
{
    CLOCK tmp = 0;
    CLOCK aclk = state->clk;
    WORD cnt = state->cnt;
    ciat_tstate_t t = state->state;

    CIAT_LOGIN(("%s set_alarm: cclk=%d, latch=%d",
                state->name, cclk, state->latch));

    while (1) {
        CIAT_LOG(("- state->clk=%d cnt=%d state=%04x", aclk, cnt, t));

        if (((t & (CIAT_CR_START | CIAT_CR_FLOAD | CIAT_LOAD1
                   | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3 | CIAT_COUNT
                   | CIAT_LOAD))
             == (CIAT_CR_START | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3
                 | CIAT_COUNT))
            && (((t & CIAT_CR_ONESHOT) && (t & CIAT_ONESHOT0)
                 && (t & CIAT_ONESHOT))
                || ((!(t & CIAT_CR_ONESHOT)) && (!(t & CIAT_ONESHOT0))
                    && (!(t & CIAT_ONESHOT))))) {
            /* warp counting */
            tmp = aclk + cnt;
            break;
        } else
        if ((!(t & (CIAT_COUNT2 | CIAT_COUNT3 | CIAT_COUNT)))
            && ((!(t & CIAT_CR_START))
                || (!(t & (CIAT_PHI2IN | CIAT_STEP))))
            && (((t & CIAT_CR_ONESHOT) && (t & CIAT_ONESHOT0)
                 && (t & CIAT_ONESHOT))
                || ((!(t & CIAT_CR_ONESHOT)) && (!(t & CIAT_ONESHOT0))
                    && (!(t & CIAT_ONESHOT))))) {
            /* warp stopped */
            tmp = CLOCK_MAX;
            break;
        } else {
            /* inc */
            if (cnt && (t & CIAT_COUNT3)) {
                cnt--;
            }
            t = ciat_table[t];
            aclk++;
        }

        if ((cnt == 0) && (t & CIAT_COUNT3)) {
            t |= CIAT_LOAD | CIAT_OUT;
            tmp = aclk;
            break;
        }
        if (t & CIAT_LOAD) {
            cnt = state->latch;
            t &= ~CIAT_COUNT3;
        }
        if ((t & CIAT_OUT)
            && (t & (CIAT_ONESHOT | CIAT_ONESHOT0))) {
            t &= ~(CIAT_CR_START | CIAT_COUNT2);
        }
    }
/*
    CIAT_LOG(("-> state->clk=%d cnt=%d, latch=%d, state=%04x",
                state->clk, state->cnt, state->latch, t));
*/
    CIAT_LOG((" -> alarmclk=%d", tmp));

    state->alarmclk = tmp;
    if (tmp != CLOCK_MAX) {
        alarm_set(state->alarm, tmp);
    } else {
        alarm_unset(state->alarm);
    }

    CIAT_LOGOUT((""));
}

_CIAT_FUNC CLOCK ciat_alarm_clk(ciat_t *state)
{
    return state->alarmclk;
}


_CIAT_FUNC int ciat_update(ciat_t *state, CLOCK cclk)
{
    int n, m;
    ciat_tstate_t t = state->state;

/* printf("%s update: state->clk=%d cclk=%d, state=%d, cnt=%d, latch=%d\n",
                state->name, state->clk, cclk, t, state->cnt, state->latch); */

#if 0
    if (cclk > clk && cclk - clk > 0x10000) {
        printf("ciat_update(%s: myclk=%d, cclk=%d)\n", state->name, clk, cclk);
    }
    if (cclk < state->clk) {
        /* should never happen! */
        printf("clock handling f*cked up, timer %s, tclk=%d, cpuclk=%d\n",
               state->name, state->clk, cclk);
    }
#endif

    n = 0;

    CIAT_LOGIN(("%s update: cclk=%d, latch=%d",
                state->name, cclk, state->latch));

    while (state->clk < cclk) {
        CIAT_LOG(("- clk=%d cnt=%d state=%04x", state->clk, state->cnt, t));

        if (((t & (CIAT_CR_START | CIAT_CR_FLOAD | CIAT_LOAD1
                   | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3 | CIAT_COUNT
                   | CIAT_LOAD))
             == (CIAT_CR_START | CIAT_PHI2IN | CIAT_COUNT2 | CIAT_COUNT3
                 | CIAT_COUNT))
            && (((t & CIAT_CR_ONESHOT) && (t & CIAT_ONESHOT0)
                 && (t & CIAT_ONESHOT))
                || ((!(t & CIAT_CR_ONESHOT)) && (!(t & CIAT_ONESHOT0))
                    && (!(t & CIAT_ONESHOT))))) {
            /* warp counting */
            if (state->clk + state->cnt > cclk) {
                state->cnt -= ((WORD)(cclk - state->clk));
                state->clk = cclk;
            } else {
                if (t & (CIAT_CR_ONESHOT | CIAT_ONESHOT0)) {
                    state->clk = state->clk + state->cnt;
                    state->cnt = 0;
                } else {
                    /* overflow clk <= cclk */
                    state->clk = state->clk + state->cnt;
                    state->cnt = 0;
                    /* n++; */
                    if (((WORD)(cclk - state->clk)) >= state->latch + 1) {
                        m = (cclk - state->clk) / (state->latch + 1);
                        n += m;
                        state->clk += m * (state->latch + 1);
                    }
                }
                /* here we have cnt=0 and clk <= cclk */
            }
        } else
        if ((!(t & (CIAT_COUNT2 | CIAT_COUNT3 | CIAT_COUNT)))
            && ((!(t & CIAT_CR_START))
                || (!(t & (CIAT_PHI2IN | CIAT_STEP))))
            && (!(t & (CIAT_CR_FLOAD | CIAT_LOAD1 | CIAT_LOAD)))
            && (((t & CIAT_CR_ONESHOT) && (t & CIAT_ONESHOT0)
                 && (t & CIAT_ONESHOT))
                || ((!(t & CIAT_CR_ONESHOT)) && (!(t & CIAT_ONESHOT0))
                    && (!(t & CIAT_ONESHOT))))) {
            /* warp stopped */
            state->clk = cclk;
        } else
        if ((t == (CIAT_COUNT | CIAT_OUT | CIAT_LOAD | CIAT_PHI2IN
                   | CIAT_COUNT2 | CIAT_CR_START))
            && (state->cnt == 1)
            && (state->latch == 1)) {
            /* when latch=1 and cnt=1 this warps up to clk */
            m = (int)((cclk - state->clk) & (CLOCK) ~1);
            if (m) {
                state->clk += m;
                n += (m >> 1);
            } else {
                t = ciat_table[t];
                state->clk++;
            }
        } else {
            /* inc */
            if (state->cnt && (t & CIAT_COUNT3)) {
                state->cnt--;
            }
            t = ciat_table[t];
            state->clk++;
        }

        if ((state->cnt == 0) && (t & CIAT_COUNT3)) {
            t |= CIAT_LOAD | CIAT_OUT;
            n++;
        }
        if (t & CIAT_LOAD) {
            state->cnt = state->latch;
            t &= ~CIAT_COUNT3;
        }
        if ((t & CIAT_OUT)
            && (t & (CIAT_ONESHOT | CIAT_ONESHOT0))) {
            t &= ~(CIAT_CR_START | CIAT_COUNT2);
        }
    }

    state->state = t;

    CIAT_LOG(("-> state->clk=%d cnt=%d, latch=%d, state=%04x",
              state->clk, state->cnt, state->latch, t));

    CIAT_LOGOUT(("-> n=%d", n));

    return n;  /* FIXME FIXME FIXME */
}

/*
 * Timer operations
 */

/* read timer value - ciat_update _must_ have been called before! */
_CIAT_FUNC WORD ciat_read_latch(ciat_t *state, CLOCK cclk)
{
    return state->latch;
}

/* read timer value - ciat_update _must_ have been called before! */
_CIAT_FUNC WORD ciat_read_timer(ciat_t *state, CLOCK cclk)
{
    return state->cnt;
}

/* check whether underflow clk - ciat_update _must_ have been called before!
   Code mostly from ciat_read_timer */
_CIAT_FUNC WORD ciat_is_underflow_clk(ciat_t *state, CLOCK cclk)
{
    return (state->state & CIAT_OUT) ? 1 : 0;
}

/* return 1 when the timer is running - update must have ... */
_CIAT_FUNC WORD ciat_is_running(ciat_t *state, CLOCK cclk)
{
    return (state->state & CIAT_CR_START) ? 1 : 0;
}

/* single-step a timer. update _must_ have been called before */
_CIAT_FUNC int ciat_single_step(ciat_t *state, CLOCK cclk)
{
    if (state->state & CIAT_CR_START) {
        state->state |= CIAT_STEP;
        ciat_set_alarm(state, cclk);
    }

    return 0;
}

_CIAT_FUNC void ciat_set_latchhi(ciat_t *state, CLOCK cclk, BYTE byte)
{
    CIAT_LOGIN(("%s set_latchhi: cclk=%d, byte=%02x", state->name, cclk, byte));
    state->latch = (state->latch & 0xff) | (byte << 8);
    if ((state->state & CIAT_LOAD) || !(state->state & CIAT_CR_START)) {
        state->cnt = state->latch;
    }

    /* just in case we have an underflow or force load and a scheduled alarm */
    ciat_set_alarm(state, cclk);

    CIAT_LOGOUT((""));
}

_CIAT_FUNC void ciat_set_latchlo(ciat_t *state, CLOCK cclk, BYTE byte)
{
    CIAT_LOGIN(("%s set_latchlo(byte=%02x)", state->name, byte));

    state->latch = (state->latch & 0xff00) | byte;
    if (state->state & CIAT_LOAD) {
        state->cnt = (state->cnt & 0xff00) | byte;
    }

    /* just in case we have an underflow or force load and a scheduled alarm */
    ciat_set_alarm(state, cclk);

    CIAT_LOGOUT((""));
}


/* needs update before */
_CIAT_FUNC void ciat_set_ctrl(ciat_t *state, CLOCK cclk, BYTE byte)
{
    CIAT_LOGIN(("%s set_ctrl: cclk=%d, byte=%02x",
                state->name, cclk, byte));

    /* bit 0= start/stop, 3=oneshot 4=force load, 5=0:count phi2 1:singlestep */ state->state &= ~(CIAT_CR_MASK);
    state->state |= (byte & CIAT_CR_MASK) ^ CIAT_PHI2IN;

    ciat_set_alarm(state, cclk);

    CIAT_LOGOUT((""));
}

_CIAT_FUNC void ciat_ack_alarm(ciat_t *state, CLOCK cclk)
{
    CIAT_LOGIN(("%s ack_alarm: cclk=%d, alarmclk=%d",
                state->name, cclk, state->alarmclk));

    alarm_unset(state->alarm);
    state->alarmclk = CLOCK_MAX;

    CIAT_LOGOUT((""));
}

/***************************************************************************/
#else   /* defined INLINE_CIAT_FUNCS || defined _CIATIMER_C */

/* We don't want inline definitions: just provide the prototypes.  */

extern void ciat_ack_alarm(ciat_t *state, CLOCK cclk);
extern void ciat_set_ctrl(ciat_t *state, CLOCK cclk, BYTE byte);
extern void ciat_set_latchlo(ciat_t *state, CLOCK cclk, BYTE byte);
extern void ciat_set_latchhi(ciat_t *state, CLOCK cclk, BYTE byte);
extern int ciat_single_step(ciat_t *state, CLOCK cclk);
extern WORD ciat_read_timer(ciat_t *state, CLOCK cclk);
extern WORD ciat_read_latch(ciat_t *state, CLOCK cclk);
extern int ciat_update(ciat_t *state, CLOCK cclk);
extern CLOCK ciat_alarm_clk(ciat_t *state);
extern void ciat_set_alarm(ciat_t *state, CLOCK clk);

extern WORD ciat_is_underflow_clk(ciat_t *state, CLOCK cclk);
extern WORD ciat_is_running(ciat_t *state, CLOCK cclk);

#endif  /* defined INLINE_CIAT_FUNCS || defined _CIATIMER_C */

extern void ciat_init(ciat_t *state, const char *name, CLOCK cclk,
                      alarm_t *alarm);
extern void ciat_reset(ciat_t *state, CLOCK cclk);
extern void ciat_prevent_clock_overflow(ciat_t *state, CLOCK sub);

extern void ciat_save_snapshot(ciat_t *cia_state, CLOCK cclk,
                               struct snapshot_module_s *m, int ver);
extern void ciat_load_snapshot(ciat_t *state, CLOCK cclk, WORD cnt, WORD latch,
                               BYTE cr, struct snapshot_module_s *m, int ver);

#endif
