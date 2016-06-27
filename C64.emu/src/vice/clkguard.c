/*
 * clkguard.c - Handle clock counter overflows.
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

#include "clkguard.h"
#include "lib.h"
#include "types.h"


clk_guard_t *clk_guard_new(CLOCK *init_clk_ptr, CLOCK init_clk_max_value)
{
    clk_guard_t *new_guard;

    new_guard = lib_malloc(sizeof(clk_guard_t));

    if (clk_guard_init(new_guard, init_clk_ptr, init_clk_max_value) < 0) {
        return NULL;
    }

    return new_guard;
}

int clk_guard_init(clk_guard_t *guard, CLOCK *init_clk_ptr,
                   CLOCK init_clk_max_value)
{
    if (init_clk_max_value < CLKGUARD_SUB_MIN * 3) {
        return -1;
    }

    guard->clk_ptr = init_clk_ptr;
    guard->clk_base = (CLOCK)0;
    guard->clk_max_value = init_clk_max_value;

    guard->callback_list = NULL;

    return 0;
}

void clk_guard_set_clk_base(clk_guard_t *guard, CLOCK new_clk_base)
{
    guard->clk_base = new_clk_base;
}

CLOCK clk_guard_get_clk_base(clk_guard_t *guard)
{
    return guard->clk_base;
}

void clk_guard_add_callback(clk_guard_t *guard, clk_guard_callback_t function,
                            void *data)
{
    clk_guard_callback_list_t *new_callback;

    new_callback = lib_malloc(sizeof(clk_guard_callback_list_t));
    new_callback->function = function;
    new_callback->data = data;

    /* Add to the head of the list.  Order is supposed not to matter at all
       for this purpose.  */
    new_callback->next = guard->callback_list;
    guard->callback_list = new_callback;
}

void clk_guard_destroy(clk_guard_t *guard)
{
    clk_guard_callback_list_t *lp;

    lp = guard->callback_list;
    while (lp != NULL) {
        clk_guard_callback_list_t *lp_next = lp->next;

        lib_free(lp);
        lp = lp_next;
    }

    lib_free(guard);
}

CLOCK clk_guard_clock_sub(clk_guard_t *guard)
{
    CLOCK sub;

    /* Make sure we have at least CLKGUARD_SUB_MIN cycles for doing our
       jobs.  */
    sub = guard->clk_max_value - CLKGUARD_SUB_MIN;

    /* Make sure we subtract a multiple of the `clk_base'.  */
    if (guard->clk_base) {
        sub = (sub / guard->clk_base) * guard->clk_base;
    }

    return sub;
}

CLOCK clk_guard_prevent_overflow(clk_guard_t *guard)
{
    if (*guard->clk_ptr < guard->clk_max_value) {
        return (CLOCK)0;
    } else {
        clk_guard_callback_list_t *lp;
        CLOCK sub;

        sub = clk_guard_clock_sub(guard);

        /* Update clock counter.  */
        *guard->clk_ptr -= sub;

        /* Execute the callbacks. */
        for (lp = guard->callback_list; lp != NULL; lp = lp->next) {
            lp->function(sub, lp->data);
        }

        return sub;
    }
}
