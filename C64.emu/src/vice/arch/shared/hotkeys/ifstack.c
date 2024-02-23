/** \file   ifstack.c
 * \brief   IF stack implementation
 *
 * \note    Adjusted for VICE from https://github.com/Compyx/if-stack
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "lib.h"

#include "ifstack.h"


/** \brief  Doubly linked list node making up the IF stack
 */
typedef struct ifstack_s {
    bool              state;    /**< branch IF state */
    bool              in_else;  /**< currently in local ELSE branch */
    struct ifstack_s *up;       /**< next node (up in stack) */
    struct ifstack_s *down;     /**< previous node (down in stack) */
} ifstack_t;


/** \brief  Error message strings */
static const char *err_messages[] = {
    "OK",
    "else without if",
    "endif without if"
};

/** \brief  IF stack reference
 *
 * This references the top of the stack.
 */
static ifstack_t *stack;

/** \brief  IF stack bottom reference
 *
 * Used for printing the stack from bottom to top in ifstack_print().
 */
static ifstack_t *stack_bottom;

/** \brief  Global "truth" state
 */
static bool       current_state;


/** \brief  Error code
 */
int ifstack_errno = 0;


/** \brief  Push new condition onto the stack
 *
 * Register an \c IF with condition \a state.
 *
 * \note    Calls \c exit(1) on out-of-memory.
 */
static void ifstack_push(bool state)
{
    ifstack_t *node = lib_malloc(sizeof *node);

    node->state   = state;
    node->in_else = false;
    node->up      = NULL;
    node->down    = stack;
    if (stack != NULL) {
        stack->up = node;
    } else {
        stack_bottom = node;
    }
    stack = node;
}

/** \brief  Pull current condtion off the stack
 */
static void ifstack_pull(void)
{
    if (stack == NULL) {
        fprintf(stderr, "%s(): error: stack empty!\n", __func__);
        exit(1);
    } else {
        ifstack_t *down = stack->down;

        lib_free(stack);
        stack = down;
        if (stack == NULL) {
            /* no previous condition of stack, set current state to true */
            current_state = true;
            /* clear stack bottom pointer */
            stack_bottom = NULL;
        } else {
            stack->up = NULL;
            current_state = stack->state;
        }
    }
}


/** \brief  Initialize stack for use
 */
void ifstack_init(void)
{
    stack         = NULL;
    stack_bottom  = NULL;
    ifstack_errno = 0;
    current_state = true;
}


/** \brief  Reset stack for reuse
 *
 * Frees any old stack remaining and initializes the stack for reuse.
 */
void ifstack_reset(void)
{
    ifstack_free();
    ifstack_init();
}


/** \brief  Free the stack
 */
void ifstack_free(void)
{
    ifstack_t *node = stack;

    while (node != NULL) {
        ifstack_t *down = node->down;

        lib_free(node);
        node = down;
    }
    stack        = NULL;
    stack_bottom = NULL;
}


/** \brief  Determine if stack is empty
 *
 * \return  \c true if stack is empty
 */
bool ifstack_is_empty(void)
{
    return (bool)(stack == NULL);
}


/** \brief  Print stack contents on stdout
 *
 * Print the current stack as an array of 0's and 1's.
 *
 * \note    Doesn't print a newline at the end.
 */
void ifstack_print(void)
{
    putchar('[');
    for (ifstack_t *node = stack_bottom; node != NULL; node = node->up) {
        putchar(node->state ? '1' : '0');
    }
    putchar(']');
}


/** \brief  Get global condition of stack
 *
 * \return  current condition
 */
bool ifstack_true(void)
{
    return current_state;
}


/** \brief  Push new IF condition on stack, update global condition
 *
 * \param[in]   state   condition of IF statement
 */
void ifstack_if(bool state)
{
    ifstack_push(state);
    if (stack->down == NULL || (stack->down != NULL && stack->down->state)) {
        current_state = state;
    }
}


/** \brief  Process ELSE branch
 *
 * Notify the stack an ELSE branch should now be taken.
 *
 * \return  \c false if no preceeding IF or already in ELSE branch
 */
bool ifstack_else(void)
{
    if (stack == NULL || stack->in_else) {
        ifstack_errno = IFSTACK_ERR_ELSE_WITHOUT_IF;
        return false;
    }

    stack->in_else = true;
    stack->state   = !stack->state;

    /* global true?: invert */
    if (current_state) {
        current_state = false;
    } else {
        /* global false */
        /* previous condition? */
        if (stack->down != NULL) {
            if (stack->down->state) {
                /* previous condition is true: set current state true */
                current_state = true;
            }
            /* previous condition is false: ignore */
        } else {
            /* no previous condition, current was false, so invert: */
            current_state = true;
        }
    }

    return true;
}


/** \brief  Process ENDIF
 *
 * Notify the stack an ENDIF statement should be handled.
 *
 * \return  \c false if there's no preceeding IF/ELSE
 */
bool ifstack_endif(void)
{
    if (stack == NULL) {
        ifstack_errno = IFSTACK_ERR_ENDIF_WITHOUT_IF;
        return false;
    }

    /* pull condition off the stack */
    ifstack_pull();
    return true;
}


/** \brief  Get error message for error number
 *
 * \param[in]   errnum  error number
 *
 * \return  error message
 */
const char *ifstack_strerror(int errnum)
{
    if (errnum < 0 || errnum >= (int)(sizeof err_messages / sizeof err_messages[0])) {
        return "invalid error number";
    } else {
        return err_messages[errnum];
    }
}
