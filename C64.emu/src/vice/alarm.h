/*
 * alarm.h - Alarm handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_ALARM_H
#define VICE_ALARM_H

#include "types.h"

#define ALARM_CONTEXT_MAX_PENDING_ALARMS 0x100

typedef void (*alarm_callback_t)(CLOCK offset, void *data);

/* An alarm.  */
struct alarm_s {
    /* Descriptive name of the alarm.  */
    char *name;

    /* Alarm context this alarm is in.  */
    struct alarm_context_s *context;

    /* Callback to be called when the alarm is dispatched.  */
    alarm_callback_t callback;

    /* Index into the pending alarm list.  If < 0, the alarm is not
       pending.  */
    int pending_idx;

    /* Call data */
    void *data;

    /* Link to the next and previous alarms in the list.  */
    struct alarm_s *next, *prev;
};
typedef struct alarm_s alarm_t;

struct pending_alarms_s {
    /* The alarm.  */
    struct alarm_s *alarm;

    /* Clock tick at which this alarm should be activated.  */
    CLOCK clk;
};
typedef struct pending_alarms_s pending_alarms_t;

/* An alarm context.  */
struct alarm_context_s {
    /* Descriptive name of the alarm context.  */
    char *name;

    /* Alarm list.  */
    struct alarm_s *alarms;

    /* Pending alarm array.  Statically allocated because it's slightly
       faster this way.  */
    pending_alarms_t pending_alarms[ALARM_CONTEXT_MAX_PENDING_ALARMS];
    unsigned int num_pending_alarms;

    /* Clock tick for the next pending alarm.  */
    CLOCK next_pending_alarm_clk;

    /* Pending alarm number.  */
    int next_pending_alarm_idx;
};
typedef struct alarm_context_s alarm_context_t;

/* ------------------------------------------------------------------------ */

alarm_context_t *alarm_context_new(const char *name);
void alarm_context_init(alarm_context_t *context, const char *name);
void alarm_context_destroy(alarm_context_t *context);
void alarm_context_time_warp(alarm_context_t *context, CLOCK warp_amount, int warp_direction);
alarm_t *alarm_new(alarm_context_t *context, const char *name, alarm_callback_t callback, void *data);
void alarm_destroy(alarm_t *alarm);
void alarm_unset(alarm_t *alarm);
void alarm_log_too_many_alarms(void);

/* ------------------------------------------------------------------------- */

/* Inline functions.  */

inline static CLOCK alarm_context_next_pending_clk(alarm_context_t *context)
{
    return context->next_pending_alarm_clk;
}

inline static void alarm_context_update_next_pending(alarm_context_t *context)
{
    CLOCK next_pending_alarm_clk = CLOCK_MAX;
    int next_pending_alarm_idx;
    unsigned int i;

    next_pending_alarm_idx = context->next_pending_alarm_idx;

    for (i = 0; i < context->num_pending_alarms; i++) {
        CLOCK pending_clk = context->pending_alarms[i].clk;

        if (pending_clk <= next_pending_alarm_clk) {
            next_pending_alarm_clk = pending_clk;
            next_pending_alarm_idx = (int)i;
        }
    }

    context->next_pending_alarm_clk = next_pending_alarm_clk;
    context->next_pending_alarm_idx = next_pending_alarm_idx;
}

inline static void alarm_context_dispatch(alarm_context_t *context,
                                          CLOCK cpu_clk)
{
    CLOCK offset;
    int idx;
    alarm_t *alarm;

    offset = cpu_clk - context->next_pending_alarm_clk;

    idx = context->next_pending_alarm_idx;
    alarm = context->pending_alarms[idx].alarm;

    (alarm->callback)(offset, alarm->data);
}

inline static void alarm_set(alarm_t *alarm, CLOCK cpu_clk)
{
    alarm_context_t *context;
    int idx;

    context = alarm->context;
    idx = alarm->pending_idx;

    if (idx < 0) {
        int new_idx;

        /* Not pending yet: add.  */

        new_idx = (int)(context->num_pending_alarms);
        if (new_idx >= (int)ALARM_CONTEXT_MAX_PENDING_ALARMS) {
            alarm_log_too_many_alarms();
            return;
        }

        context->pending_alarms[new_idx].alarm = alarm;
        context->pending_alarms[new_idx].clk = cpu_clk;

        context->num_pending_alarms++;

        if (cpu_clk < context->next_pending_alarm_clk) {
            context->next_pending_alarm_clk = cpu_clk;
            context->next_pending_alarm_idx = new_idx;
        }

        alarm->pending_idx = new_idx;
    } else {
        /* Already pending: modify.  */

        context->pending_alarms[idx].clk = cpu_clk;
        if (context->next_pending_alarm_clk > cpu_clk
            || idx == context->next_pending_alarm_idx) {
            alarm_context_update_next_pending(context);
        }
    }
}

#endif
