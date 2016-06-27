/*
 * ciatimer.c - MOS6526 (CIA) timer emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#define _CIATIMER_C

#include "vice.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "ciatimer.h"
#include "snapshot.h"
#include "types.h"


#ifdef CIAT_NEED_LOG

static int ciat_logfl = 0; /* 0 means logging disabled */
static int logtab = 0;
static const char spaces[] = "                                                  ";
/* extern int traceflg; */

void ciat_login(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (/* traceflg ||*/ ciat_logfl) {
        printf("%s", spaces + strlen(spaces) - logtab);
        vprintf(format, ap);
        printf(" {\n");
    }
    va_end(ap);
    logtab += 2;
}

void ciat_logout(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if ((/* traceflg ||*/ ciat_logfl) && strlen(format)) {
        printf("%s", spaces + strlen(spaces) - logtab);
        vprintf(format, ap);
        printf("\n");
    }
    va_end(ap);
    if (logtab > 1) {
        logtab -= 2;
    }
    if (ciat_logfl) {
        printf("%s}\n", spaces + strlen(spaces) - logtab);
    }
}

void ciat_log(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (/* traceflg ||*/ ciat_logfl) {
        printf("%s", spaces + strlen(spaces) - logtab);
        vprintf(format, ap);
        printf("\n");
    }
    va_end(ap);
}

void ciat_print_state(const ciat_t *state)
{
    printf("%s print: clk=%ld, cnt=%04x (%d), latch=%04x (%d)\n",
           state->name, (long)(state->clk),
           state->cnt, state->cnt,
           state->latch, state->latch);
    printf("          state=%04x = %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           state->state,
           (state->state & CIAT_CR_START) ? "start " : "",
           (state->state & CIAT_CR_ONESHOT) ? "cr_oneshot " : "",
           (state->state & CIAT_CR_FLOAD) ? "cr_fload " : "",
           (state->state & CIAT_PHI2IN) ? "Phi2 " : "",
           (state->state & CIAT_STEP) ? "step " : "",
           (state->state & CIAT_COUNT2) ? "cnt2 " : "",
           (state->state & CIAT_COUNT3) ? "cnt3 " : "",
           (state->state & CIAT_COUNT) ? "cnt " : "",
           (state->state & CIAT_LOAD1) ? "load1 " : "",
           (state->state & CIAT_LOAD) ? "load " : "",
           (state->state & CIAT_ONESHOT0) ? "oneshot0 " : "",
           (state->state & CIAT_ONESHOT) ? "oneshot " : "",
           (state->state & CIAT_OUT) ? "out " : ""
           );
    printf("          alarm at %ld\n", (long)(state->alarmclk));
}
#endif


ciat_tstate_t ciat_table[CIAT_TABLEN];

static unsigned int ciat_init_table_initialized = 0;

void ciat_init_table(void)
{
    int i;
    ciat_tstate_t tmp;

    if (ciat_init_table_initialized) {
        return;
    }

    ciat_init_table_initialized = 1;

    for (i = 0; i < CIAT_TABLEN; i++) {
        tmp = i & (CIAT_CR_START | CIAT_CR_ONESHOT | CIAT_PHI2IN);

        if ((i & CIAT_CR_START) && (i & CIAT_PHI2IN)) {
            tmp |= CIAT_COUNT2;
        }
        if ((i & CIAT_COUNT2) || ((i & CIAT_STEP) && (i & CIAT_CR_START))) {
            tmp |= CIAT_COUNT3;
        }
        if (i & CIAT_COUNT3) {
            tmp |= CIAT_COUNT;
        }

        if (i & CIAT_CR_FLOAD) {
            tmp |= CIAT_LOAD1;
        }
        if (i & CIAT_LOAD1) {
            tmp |= CIAT_LOAD;
        }

        if (i & CIAT_CR_ONESHOT) {
            tmp |= CIAT_ONESHOT0;
        }
        if (i & CIAT_ONESHOT0) {
            tmp |= CIAT_ONESHOT;
        }

        ciat_table[i] = tmp;
    }
}

/*
 * Init the timer
 */
void ciat_init(ciat_t *state, const char *name, CLOCK cclk, alarm_t *alarm)
{
    CIAT_LOGIN(("%s init: cclk=%d", name, cclk));

    state->name = name;
    state->clk = cclk;
    state->alarmclk = CLOCK_MAX;
    state->cnt = 0xffff;
    state->latch = 0xffff;
    state->alarm = alarm;

    CIAT_LOGOUT((""));
}

/* timer reset */
void ciat_reset(ciat_t *state, CLOCK cclk)
{
    CIAT_LOGIN(("%s reset: cclk=%d", state->name, cclk));

    /* FIXME? */
    state->clk = cclk;
    state->alarmclk = CLOCK_MAX;
    state->cnt = 0xffff;
    state->latch = 0xffff;
    state->state = 0;

    alarm_unset(state->alarm);

    CIAT_LOGOUT((""));
}

/* handle clock overflow. needs update */
void ciat_prevent_clock_overflow(ciat_t *state, CLOCK sub)
{
    CIAT_LOGIN(("%s prevent_clock_overflow", state->name));

    state->clk -= sub;
    if (state->alarmclk < CLOCK_MAX) {
        state->alarmclk -= sub;
    }

    CIAT_LOGOUT((""));
}

void ciat_save_snapshot(ciat_t *cia_state, CLOCK cclk, snapshot_module_t *m,
                        int ver)
{
    /* ciat_print_state(state); */

    if (ver >= 0x100) {
        /* major 1, minor >= 1 */
        /* cnt & latch are saved from cia module already */
        SMW_W(m, ((WORD)(cia_state->state)));
    }
}

void ciat_load_snapshot(ciat_t *state, CLOCK cclk, WORD cnt, WORD latch,
                        BYTE cr, snapshot_module_t *m, int ver)
{
    /* cnt & latch are read from cia module already */
    state->clk = cclk;
    state->cnt = cnt;
    state->latch = latch;

    if (ver >= 0x101) {
        /* major 1, minor >= 1 */
        SMR_W(m, &state->state);
    } else {
        state->state = cr;
        if (cr & CIAT_CR_START) {
            state->state |= CIAT_COUNT2 | CIAT_COUNT3 | CIAT_COUNT;
        }
        if (cr & CIAT_CR_ONESHOT) {
            state->state |= CIAT_ONESHOT0 | CIAT_ONESHOT;
        }
    }

    ciat_set_alarm(state, cclk);

    /* ciat_print_state(state); */
}
