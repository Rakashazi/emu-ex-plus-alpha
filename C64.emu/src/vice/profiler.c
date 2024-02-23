/*
 * profiler.c -- CPU Profiler
 *
 * Written by
 *  Oskar Linde <oskar.linde@gmail.com>
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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "lib.h"
#include "mem.h"
#include "profiler.h"
#include "profiler_data.h"


#define MAX_CALLSTACK_SIZE 129

/* Store the PC address for JSR calls and the SP where PC is stored
 * this allows us to differentiate between fake RTS/RTI-calls used as indirect
 * JMPs
 *
 * for interrupts, we use "magic" PC_SRC values corresponding to the 6502
 * interrupt vectors */

uint16_t callstack_pc_dst[MAX_CALLSTACK_SIZE];
uint16_t callstack_pc_src[MAX_CALLSTACK_SIZE];
uint8_t  callstack_sp[MAX_CALLSTACK_SIZE];
uint16_t callstack_memory_bank_config[MAX_CALLSTACK_SIZE];
unsigned callstack_size = 0;
bool     context_dirty = true;
bool     maincpu_profiling = false;

/* (fragile) flags if the current command is a JSR/INT or RTS/RTI */
bool     entered_context = false;
bool     exited_context = false;

profiling_context_t  *root_context = NULL;
profiling_context_t  *current_context = NULL;
uint16_t              current_pc;
int                   num_context_ids = 0;
int                   context_id_capacity = 0;
profiling_context_t **id_to_context = NULL;

profiling_context_t  *profile_context_by_id(int id) {
    if (id > 0 && id <= num_context_ids) {
        return id_to_context[id-1];
    }
    return NULL;
}

int get_context_id(profiling_context_t *context) {
    if (context->id == 0) {
        context->id = num_context_ids + 1;
        num_context_ids++;

        if (context->id > context_id_capacity) {
            context_id_capacity *= 2;
            if (context_id_capacity < 100) context_id_capacity = 100;
            id_to_context = lib_realloc(id_to_context,
                                        context_id_capacity
                                            * sizeof(*id_to_context));
        }
        id_to_context[context->id - 1] = context;
    }
    return context->id;
}

#if 0 /* unused */
static void callstack_reset(void) {
    callstack_size = 0;
    context_dirty = true;
}
#endif

/* push pc to callstack (triggered by interrupt or JSR) */
static void callstack_push(uint16_t pc_dst, uint16_t pc_src, uint8_t sp) {
    if (callstack_size >= MAX_CALLSTACK_SIZE) {
        /* stack overflow; do nothing */
        return;
    }
    callstack_pc_dst[callstack_size] = pc_dst;
    callstack_pc_src[callstack_size] = pc_src;
    callstack_sp[callstack_size] = sp;
    callstack_memory_bank_config[callstack_size] = mem_get_current_bank_config(); /* TODO: move out to interface? */
    callstack_size++;
    context_dirty = true;
}

/* check if we should pop the callstack */
static void callstack_pop_check(uint8_t sp) {
    while (callstack_size != 0 && sp >= callstack_sp[callstack_size-1]
           && callstack_sp[callstack_size-1] > 0x01) {
        callstack_size--;
    }

    context_dirty = true;
}

static profiling_page_t *alloc_profiling_page(void) {
    return lib_calloc(1, sizeof(profiling_page_t));
}

profiling_context_t *alloc_profiling_context(void) {
    return lib_calloc(1, sizeof(profiling_context_t));
}

void free_profiling_context(profiling_context_t *data) {
    int i;
    if (!data) { return; }
    if (data->child) {
        profiling_context_t *c = data->child;
        do {
            profiling_context_t *next = c->next;
            free_profiling_context(c);
            c = next;
        } while(c != data->child);
    }

    for (i = 0; i < 256; i++) {
        lib_free(data->page[i]);
    }

    if (data->next_mem_config) {
        free_profiling_context(data->next_mem_config);
    }
    lib_free(data);
}

profiling_page_t *profiling_get_page(profiling_context_t *ctx, uint8_t page) {
    if (!ctx->page[page]) {
        ctx->page[page] = alloc_profiling_page();
    }
    return ctx->page[page];
}

profiling_context_t *get_mem_config_context(profiling_context_t *main_context,
                                            uint16_t mem_config) {
    /* if mem_config matches main context's mem config, simply return the same */
    if (main_context->memory_bank_config == mem_config) {
        return main_context;
    }

    profiling_context_t *c = main_context;
    while (c->next_mem_config) {
        if (c->next_mem_config->memory_bank_config == mem_config) {
            return c->next_mem_config;
        }
        c = c->next_mem_config;
    }

    /* create new memory bank sibling context */
    c->next_mem_config = alloc_profiling_context();
    c->next_mem_config->memory_bank_config = mem_config;
    /* calloc() ensures all other members are NULL */

    return c->next_mem_config;
}

static profiling_context_t *get_child_context(profiling_context_t *parent,
                                              uint16_t pc_dst,
                                              uint16_t pc_src,
                                              uint16_t mem_config) {
    profiling_context_t *new_context;

    if (parent->child) {
        profiling_context_t *c = parent->child;
        do {
            if (c->pc_dst == pc_dst && c->pc_src == pc_src && c->memory_bank_config == mem_config) {
                return c;
            }
            c = c->next;
        } while(c != parent->child);
    }

    new_context = alloc_profiling_context();
    new_context->pc_dst = pc_dst;
    new_context->pc_src = pc_src;
    new_context->memory_bank_config = mem_config;
    new_context->next_mem_config = NULL;

    /* link */
    new_context->parent = parent;
    if (!parent->child) {
        parent->child = new_context;
        new_context->next = new_context;
    } else {
        new_context->next = parent->child->next;
        parent->child->next = new_context;
    }
    return new_context;
}

/* store profiling samples */
static void initialize_context(void) {
    int callstack_head;

    /* find head of context (>0 if interrupt) */
    for (callstack_head = callstack_size-1;
         callstack_head > 0;
         callstack_head--) {
        if (callstack_pc_src[callstack_head] >= 0xfffa) break;
    }

    current_context = root_context;
    while (callstack_head < callstack_size) {
        current_context = get_child_context(current_context,
                                            callstack_pc_dst[callstack_head],
                                            callstack_pc_src[callstack_head],
                                            callstack_memory_bank_config[callstack_head]);
        callstack_head++;
    }

    current_context = get_mem_config_context(current_context, mem_get_current_bank_config());
}

void profile_sample_start(uint16_t pc)
{
    if (exited_context) {
        current_context->num_exits++;
        exited_context = false;
    }

    if (context_dirty || mem_get_current_bank_config() != current_context->memory_bank_config) {
        initialize_context();
        context_dirty = false;
    }
    current_pc = pc;

    if (entered_context) {
        current_context->num_enters++;
        entered_context = false;
    }
}

void profile_sample_finish(uint16_t cycle_time, uint16_t stolen_cycles)
{
    profiling_data_t * data = &profiling_get_page(current_context,
                                                 current_pc >> 8)
                                  ->data[current_pc & 0xff];
    data->num_cycles += cycle_time;
    data->num_samples++;
    current_context->total_stolen_cycles_self   += stolen_cycles;
}

void profile_jsr(uint16_t pc_dst, uint16_t pc_src, uint8_t sp)
{
    callstack_push(pc_dst, pc_src, sp);
    entered_context = true;
}

void profile_int(uint16_t pc_dst,
                 uint16_t handler,
                 uint8_t sp,
                 uint16_t cycle_time)
{
    callstack_push(pc_dst, handler, sp);
    entered_context = true;
    if (maincpu_profiling) {
        profile_sample_start(handler);
        profile_sample_finish(cycle_time, 0 /* stolen_cycles */);
    }
}

/* note: RTS/RTI can be (ab)used to implement an indirect jump by pushing a
 * return addr to the stack, we need to differentiate between that (jump) and
 * actually returning and clearing the return address on the stack.
 * For the purpose of call stack tracking, we simply ignore rts/rti if sp is
 * not at (or below) the stored earlier call stack value */
void profile_rtx(uint8_t sp)
{
    callstack_pop_check(sp);
    exited_context = true;
}

void profile_start(void)
{
    if (root_context) free_profiling_context(root_context);
    root_context    = alloc_profiling_context();
    num_context_ids = 0;
    current_context = root_context;
    maincpu_profiling = true;
    entered_context = false;
    exited_context  = false;
    context_dirty   = true;
}

void compute_aggregate_stats(profiling_context_t *context) {
    profiling_context_t *c;
    profiling_counter_t total_child_cycles        = 0;
    profiling_counter_t total_self_cycles         = 0;
    profiling_counter_t total_stolen_child_cycles = 0;
    int i,j;

    if (context->child) {
        c = context->child;
        do {
            uint16_t src = c->pc_src;
            /* ensure we touch each child source instruction */
            if (src < 0xfffa) {
                src -= 2;
                profiling_context_t *context_mem_config = context;
                while (context_mem_config) {
                    if (context_mem_config->memory_bank_config == c->memory_bank_config) {
                        profiling_get_page(context_mem_config, src >> 8)
                            ->data[src & 0xff].touched = 1;
                    }
                    context_mem_config = context_mem_config->next_mem_config;
                }
            }
            compute_aggregate_stats(c);
            total_child_cycles        += c->total_cycles;
            total_stolen_child_cycles += c->total_stolen_cycles;
            c = c->next;
        } while(c != context->child);
    }

    c = context;
    while (c) {
        for (i = 0; i < 256; i++) {
            if (c->page[i]) {
                for (j = 0; j < 256; j++) {
                    total_self_cycles += c->page[i]->data[j].num_cycles;
                }
            }
        }
        c = c->next_mem_config;
    }

    context->total_cycles_self   = total_self_cycles;
    context->total_cycles        = total_self_cycles + total_child_cycles;
    context->total_stolen_cycles = context->total_stolen_cycles_self + total_stolen_child_cycles;
}


void profile_stop(void)
{
    maincpu_profiling = false;
}

static void profile_reset(void) {
    free_profiling_context(root_context);
    root_context = NULL;
    current_context = NULL;
    lib_free(id_to_context);
    id_to_context = NULL;
    num_context_ids = 0;
    context_id_capacity = 0;
}


void profile_shutdown(void)
{
    profile_reset();
}
