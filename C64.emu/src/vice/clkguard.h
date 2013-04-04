/*
 * clkguard.h - Handle clock counter overflows.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_CLKGUARD_H
#define VICE_CLKGUARD_H

#include "types.h"

#define CLKGUARD_SUB_MIN 0xfffff

/* A callback function to call to prevent the overflow.  */
typedef void (*clk_guard_callback_t)(CLOCK amount, void *data);

typedef struct clk_guard_callback_list_s {
    /* Callback function.  */
    clk_guard_callback_t function;

    /* Extra data to pass to the function.  */
    void *data;

    /* Pointer to the next item in the list.  */
    struct clk_guard_callback_list_s *next;
} clk_guard_callback_list_t;

typedef struct clk_guard_s {
    /* Pointer to the clock counter to prevent overflows for.  */
    CLOCK *clk_ptr;

    /* Only subtract multiples of this value from `*clk_ptr'.  */
    CLOCK clk_base;

    /* Maximum value `*clk_ptr' is allowed to reach before we start calling
       the callback functions.  */
    CLOCK clk_max_value;

    /* List of functions to call when the overflow must be prevented.  */
    clk_guard_callback_list_t *callback_list;
} clk_guard_t;

/* ------------------------------------------------------------------------ */

extern clk_guard_t *clk_guard_new(CLOCK *init_clk_ptr,
                                  CLOCK init_clk_max_value);
extern int clk_guard_init(clk_guard_t *guard, CLOCK *init_clk_ptr,
                          CLOCK init_clk_max_value);
extern void clk_guard_set_clk_base(clk_guard_t *guard, CLOCK new_clk_base);
extern CLOCK clk_guard_get_clk_base(clk_guard_t *guard);
extern void clk_guard_add_callback(clk_guard_t *guard,
                                   clk_guard_callback_t function, void *data);
extern void clk_guard_destroy(clk_guard_t *guard);
extern CLOCK clk_guard_clock_sub(clk_guard_t *guard);
extern CLOCK clk_guard_prevent_overflow(clk_guard_t *guard);

#endif
