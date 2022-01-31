/*
 * alarm.c - Alarm handling.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"
#include "lib.h"
#include "log.h"
#include "types.h"


alarm_context_t *alarm_context_new(const char *name)
{
    alarm_context_t *new_alarm_context;

    new_alarm_context = lib_malloc(sizeof(alarm_context_t));
    alarm_context_init(new_alarm_context, name);

    return new_alarm_context;
}

void alarm_context_init(alarm_context_t *context, const char *name)
{
    context->name = lib_strdup(name);

    context->alarms = NULL;

    context->num_pending_alarms = 0;
    context->next_pending_alarm_clk = CLOCK_MAX;
}

void alarm_context_destroy(alarm_context_t *context)
{
    lib_free(context->name);

    /* Destroy all the alarms.  */
    {
        alarm_t *ap;

        ap = context->alarms;
        while (ap != NULL) {
            alarm_t *ap_next = ap->next;

            alarm_destroy(ap);
            ap = ap_next;
        }
    }

    lib_free(context);
}

void alarm_context_time_warp(alarm_context_t *context, CLOCK warp_amount,
                             int warp_direction)
{
    unsigned int i;

    if (warp_direction == 0) {
        return;
    }

    for (i = 0; i < context->num_pending_alarms; i++) {
        if (warp_direction > 0) {
            context->pending_alarms[i].clk += warp_amount;
        } else {
            context->pending_alarms[i].clk -= warp_amount;
        }
    }

    if (warp_direction > 0) {
        context->next_pending_alarm_clk += warp_amount;
    } else {
        context->next_pending_alarm_clk -= warp_amount;
    }
}

/* ------------------------------------------------------------------------ */

static void alarm_init(alarm_t *alarm, alarm_context_t *context,
                       const char *name, alarm_callback_t callback, void *data)
{
    alarm->name = lib_strdup(name);
    alarm->context = context;
    alarm->callback = callback;
    alarm->data = data;

    alarm->pending_idx = -1;      /* Not pending.  */

    /* Add to the head of the alarm list of the alarm context.  */
    if (context->alarms == NULL) {
        context->alarms = alarm;
        alarm->next = NULL;
    } else {
        alarm->next = context->alarms;
        context->alarms->prev = alarm;
        context->alarms = alarm;
    }
    alarm->prev = NULL;
}

alarm_t *alarm_new(alarm_context_t *context, const char *name,
                   alarm_callback_t callback, void *data)
{
    alarm_t *new_alarm;

    new_alarm = lib_malloc(sizeof(alarm_t));

    alarm_init(new_alarm, context, name, callback, data);

    return new_alarm;
}

void alarm_destroy(alarm_t *alarm)
{
    alarm_context_t *context;

    if (alarm == NULL) {
        return;
    }

    alarm_unset(alarm);

    context = alarm->context;

    if (alarm == context->alarms) {
        context->alarms = alarm->next;
    }

    if (alarm->next != NULL) {
        alarm->next->prev = alarm->prev;
    }
    if (alarm->prev != NULL) {
        alarm->prev->next = alarm->next;
    }

    lib_free(alarm->name);

    lib_free(alarm);
}

void alarm_unset(alarm_t *alarm)
{
    alarm_context_t *context;
    int idx;

    idx = alarm->pending_idx;

    if (idx < 0) {
        return;                 /* Not pending.  */
    }
    context = alarm->context;

    if (context->num_pending_alarms > 1) {
        int last;

        last = --context->num_pending_alarms;

        if (last != idx) {
            /* Let's copy the struct by hand to make sure stupid compilers
               don't do stupid things.  */
            context->pending_alarms[idx].alarm
                = context->pending_alarms[last].alarm;
            context->pending_alarms[idx].clk
                = context->pending_alarms[last].clk;

            context->pending_alarms[idx].alarm->pending_idx = idx;
        }

        if (context->next_pending_alarm_idx == idx) {
            alarm_context_update_next_pending(context);
        } else if (context->next_pending_alarm_idx == last) {
            context->next_pending_alarm_idx = idx;
        }
    } else {
        context->num_pending_alarms = 0;
        context->next_pending_alarm_clk = CLOCK_MAX;
        context->next_pending_alarm_idx = -1;
    }

    alarm->pending_idx = -1;
}

void alarm_log_too_many_alarms(void)
{
    log_error(LOG_DEFAULT, "alarm_set(): Too many alarms set!");
}
